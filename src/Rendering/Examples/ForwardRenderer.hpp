//




// Created by carlo on 2024-10-07.
//


#ifndef FORWARDRENDERER_HPP
#define FORWARDRENDERER_HPP

namespace Rendering
{
    class ForwardRenderer : BaseRenderer
    {
    public:
        ForwardRenderer(ENGINE::Core* core, WindowProvider* windowProvider,
                        ENGINE::DescriptorAllocator* descriptorAllocator)
        {
            this->core = core;
            this->renderGraphRef = core->renderGraphRef;
            this->windowProvider = windowProvider;
            this->descriptorAllocatorRef = descriptorAllocator;
            auto logicalDevice = core->logicalDevice.get();
            auto physicalDevice = core->physicalDevice;

            camera.SetLookAt(glm::vec3(0.0f));
            
            ModelLoader::GetInstance()->LoadGLTF(
                "C:\\Users\\carlo\\CLionProjects\\Vulkan_Engine_Template\\Resources\\Assets\\Models\\3d_pbr_curved_sofa\\scene.gltf",
                model);

            vertexBuffer = std::make_unique<ENGINE::Buffer>(
                physicalDevice, logicalDevice, vk::BufferUsageFlagBits::eVertexBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                sizeof(M_Vertex3D) * model.vertices.size(), model.vertices.data());
            indexBuffer = std::make_unique<ENGINE::Buffer>(
                physicalDevice, logicalDevice, vk::BufferUsageFlagBits::eIndexBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                sizeof(uint32_t) * model.indices.size(), model.indices.data());
            
            
            std::vector<uint32_t> vertCode = ENGINE::GetByteCode(
                    "C:\\Users\\carlo\\CLionProjects\\Vulkan_Engine_Template\\src\\Shaders\\spirv\\Examples\\fSample.vert.spv");
            std::vector<uint32_t> fragCode = ENGINE::GetByteCode(
                    "C:\\Users\\carlo\\CLionProjects\\Vulkan_Engine_Template\\src\\Shaders\\spirv\\Examples\\fSample.frag.spv");
            
            
            ENGINE::ShaderParser vertParser(vertCode);
            ENGINE::ShaderParser fragParser(fragCode);
            
            ENGINE::ShaderModule vertShaderModule(logicalDevice, vertCode);
            ENGINE::ShaderModule fragShaderModule(logicalDevice, fragCode);

            ENGINE::DescriptorLayoutBuilder builder;

            ENGINE::ShaderParser::GetLayout(vertParser, builder);
            ENGINE::ShaderParser::GetLayout(fragParser, builder);
            
            
             dstLayout = builder.BuildBindings(
                core->logicalDevice.get(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);

            auto pushConstantRange = vk::PushConstantRange()
            .setOffset(0)
            .setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
            .setSize(sizeof(ForwardPc));
            
            auto layoutCreateInfo = vk::PipelineLayoutCreateInfo()
                                    .setSetLayoutCount(1)
                                    .setPushConstantRanges(pushConstantRange)    
                                    .setPSetLayouts(&dstLayout.get());
            
            imageShipper.SetDataFromPath("C:\\Users\\carlo\\OneDrive\\Pictures\\Screenshots\\Screenshot 2024-09-19 172847.png");
            imageShipper.BuildImage(core, 1, 1, renderGraphRef->core->swapchainRef->GetFormat(), ENGINE::GRAPHICS_READ);

            dstSet = descriptorAllocatorRef->Allocate(core->logicalDevice.get(), dstLayout.get());

            writerBuilder.AddWriteImage(0, imageShipper.imageView.get(), imageShipper.sampler->samplerHandle.get(),
                                        vk::ImageLayout::eShaderReadOnlyOptimal, vk::DescriptorType::eCombinedImageSampler);
            
            writerBuilder.UpdateSet(core->logicalDevice.get(), dstSet.get());
 
            
            ENGINE::VertexInput vertexInput= M_Vertex3D::GetVertexInput();

            ENGINE::AttachmentInfo colInfo = ENGINE::GetColorAttachmentInfo();
            ENGINE::AttachmentInfo depthInfo = ENGINE::GetDepthAttachmentInfo();
            forwardPassName = "ForwardPass";
            auto renderNode = renderGraphRef->AddPass(forwardPassName);
            
            renderNode->SetVertModule(&vertShaderModule);
            renderNode->SetFragModule(&fragShaderModule);
            renderNode->SetFramebufferSize(windowProvider->GetWindowSize());
            renderNode->SetPipelineLayoutCI(layoutCreateInfo);
            renderNode->SetVertexInput(vertexInput);
            renderNode->AddColorAttachmentOutput("color", colInfo);
            renderNode->SetDepthAttachmentOutput("depth", depthInfo);
            renderNode->AddColorBlendConfig(ENGINE::BlendConfigs::B_OPAQUE);
            renderNode->SetDepthConfig(ENGINE::DepthConfigs::D_ENABLE);
            renderNode->AddNodeSampler("sampler", imageShipper.imageView.get());
            renderNode->BuildRenderGraphNode();
            
        }
        ~ForwardRenderer() override
        {
        }

        void RecreateSwapChainResources() override
        {
        }

        void SetRenderOperation(ENGINE::InFlightQueue* inflightQueue) override
        {
            auto setViewTask = new std::function<void()>([this, inflightQueue]()
            {
                auto* currImage = inflightQueue->currentSwapchainImageView;
                auto* currDepthImage = core->swapchainRef->depthImagesFull.at(inflightQueue->frameIndex).imageView.get();
                renderGraphRef->AddColorImageResource("ForwardPass", "color", currImage);
                renderGraphRef->AddDepthImageResource("ForwardPass", "depth", currDepthImage);
                renderGraphRef->GetNode("ForwardPass")->SetFramebufferSize(windowProvider->GetWindowSize());
            });

            auto renderOp = new std::function<void(vk::CommandBuffer& command_buffer)>(
                [this](vk::CommandBuffer& commandBuffer)
                {
                    vk::DeviceSize offset = 0;
                    commandBuffer.bindDescriptorSets(renderGraphRef->GetNode(forwardPassName)->pipelineType,
                                                     renderGraphRef->GetNode(forwardPassName)->pipelineLayout.get(), 0, 1,
                                                     &dstSet.get(), 0 , nullptr);

                    commandBuffer.bindVertexBuffers(0, 1, &vertexBuffer->bufferHandle.get(), &offset);
                    commandBuffer.bindIndexBuffer(indexBuffer->bufferHandle.get(), 0, vk::IndexType::eUint32);
                    
                    for (int i = 0; i < model.meshCount; ++i)
                    {
                        camera.SetPerspective(
                            45.0f, (float)windowProvider->GetWindowSize().x / (float)windowProvider->GetWindowSize().y,
                            0.1f, 512.0f);
                        pc.projView = camera.matrices.perspective * camera.matrices.view;
                        pc.model = model.modelsMat[i];
                        // pc.model = glm::scale(pc.model, glm::vec3(0.01f));
                    
                        commandBuffer.pushConstants(renderGraphRef->GetNode(forwardPassName)->pipelineLayout.get(),
                                                    vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                                                    0, sizeof(ForwardPc), &pc);
                    
                        commandBuffer.drawIndexed(model.indicesCount[i], 1, model.firstIndices[i],
                                                  static_cast<int32_t>(model.firstVertices[i]), 0);
                    }
           
                });
            
            renderGraphRef->GetNode(forwardPassName)->AddTask(setViewTask);
            renderGraphRef->GetNode(forwardPassName)->SetRenderOperation(renderOp);
        }


        void ReloadShaders() override
        {
            int result = std::system("C:\\Users\\carlo\\CLionProjects\\Vulkan_Engine_Template\\src\\shaders\\compile.bat");
            if (result == 0)
            {
            }else
            {
                assert(false &&"reload shaders failed");
            }
            auto renderNode = renderGraphRef->GetNode(forwardPassName);
            auto pushConstantRange = vk::PushConstantRange()
                                     .setOffset(0)
                                     .setStageFlags(
                                         vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
                                     .setSize(sizeof(ForwardPc));

            auto layoutCreateInfo = vk::PipelineLayoutCreateInfo()
                                    .setSetLayoutCount(1)
                                    .setPushConstantRanges(pushConstantRange)
                                    .setPSetLayouts(&dstLayout.get());
            
            std::cout<< "Shaders reloaded\n";
            std::vector<uint32_t> vertCode = ENGINE::GetByteCode(
                "C:\\Users\\carlo\\CLionProjects\\Vulkan_Engine_Template\\src\\Shaders\\spirv\\Examples\\fSample.vert.spv");
            std::vector<uint32_t> fragCode = ENGINE::GetByteCode(
                "C:\\Users\\carlo\\CLionProjects\\Vulkan_Engine_Template\\src\\Shaders\\spirv\\Examples\\fSample.frag.spv");

            ENGINE::ShaderModule vertShaderModule(core->logicalDevice.get(), vertCode);
            ENGINE::ShaderModule fragShaderModule(core->logicalDevice.get(), fragCode);
            renderNode->SetPipelineLayoutCI(layoutCreateInfo);
            renderNode->SetVertModule(&vertShaderModule);
            renderNode->SetFragModule(&fragShaderModule);
            renderNode->RecreateResources();
        }


        ENGINE::DescriptorAllocator* descriptorAllocatorRef;
        WindowProvider* windowProvider;
        ENGINE::Core* core;
        ENGINE::RenderGraph* renderGraphRef;
        
        ENGINE::DescriptorWriterBuilder writerBuilder;
        vk::UniqueDescriptorSetLayout dstLayout;
        vk::UniqueDescriptorSet dstSet;

        std::string forwardPassName;
        ENGINE::ImageShipper imageShipper;
        std::unique_ptr<ENGINE::Buffer> vertexBuffer;
        std::unique_ptr<ENGINE::Buffer> indexBuffer;
        
       
        Camera camera = {glm::vec3(3.0f), Camera::CameraMode::E_FIXED};
        Model model{};
        ForwardPc pc{};
        std::vector<Vertex2D> vertices;
        std::vector<uint32_t> indices;

    };
}

#endif //FORWARDRENDERER_HPP
