//
// Created by carlo on 2024-10-25.
//

#ifndef CLUSTERRENDERER_HPP
#define CLUSTERRENDERER_HPP

namespace Rendering
{
    class ClusterRenderer : BaseRenderer
    {
    public:
        ClusterRenderer(ENGINE::Core* core, WindowProvider* windowProvider,
                        ENGINE::DescriptorAllocator* descriptorAllocator)
        {
            
            this->core = core;
            this->renderGraphRef = core->renderGraphRef;
            this->windowProvider = windowProvider;
            this->descriptorAllocatorRef = descriptorAllocator;
            auto logicalDevice = core->logicalDevice.get();
            auto physicalDevice = core->physicalDevice;
            
            auto imageInfo = ENGINE::Image::CreateInfo2d(windowProvider->GetWindowSize(), 1, 1,
                                                         renderGraphRef->storageImageFormat,
                                                         vk::ImageUsageFlagBits::eColorAttachment);
 
            colAttachment = std::make_unique<ENGINE::Image>(physicalDevice, logicalDevice, imageInfo);
            colAttachmentData = std::make_unique<ENGINE::ImageData>(colAttachment->imageHandle.get(), vk::ImageType::e2D,
                                                                   glm::vec3(windowProvider->GetWindowSize().x,
                                                                             windowProvider->GetWindowSize().y, 1), 1,
                                                                   1,
                                                                   renderGraphRef->storageImageFormat,
                                                                   vk::ImageLayout::eUndefined);
            colAttachmentView = std::make_unique<ENGINE::ImageView>(logicalDevice, colAttachmentData.get(), 0, 1, 0, 1);
            
            normAttachment = std::make_unique<ENGINE::Image>(physicalDevice, logicalDevice, imageInfo);
            normAttachmentData = std::make_unique<ENGINE::ImageData>(normAttachment->imageHandle.get(),
                                                                    vk::ImageType::e2D,
                                                                    glm::vec3(windowProvider->GetWindowSize().x,
                                                                              windowProvider->GetWindowSize().y, 1), 1,
                                                                    1,
                                                                    renderGraphRef->storageImageFormat,
                                                                    vk::ImageLayout::eUndefined);
            normAttachmentView = std::make_unique<ENGINE::ImageView>(logicalDevice, normAttachmentData.get(), 0, 1, 0, 1);
            
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
                "C:\\Users\\carlo\\CLionProjects\\Vulkan_Engine_Template\\src\\Shaders\\spirv\\ClusterRendering\\gBuffer.vert.spv");
            std::vector<uint32_t> fragCode = ENGINE::GetByteCode(
                "C:\\Users\\carlo\\CLionProjects\\Vulkan_Engine_Template\\src\\Shaders\\spirv\\ClusterRendering\\gBuffer.frag.spv");

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
                                     .setStageFlags(
                                         vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
                                     .setSize(sizeof(ForwardPc));
            
            auto layoutCreateInfo = vk::PipelineLayoutCreateInfo()
                                    .setSetLayoutCount(1)
                                    .setPushConstantRanges(pushConstantRange)
                                    .setPSetLayouts(&dstLayout.get());
            
            imageShipperCol.SetDataFromPath(
                "C:\\Users\\carlo\\OneDrive\\Pictures\\Screenshots\\Screenshot 2024-09-19 172847.png");
            imageShipperCol.BuildImage(core, 1, 1, renderGraphRef->core->swapchainRef->GetFormat(), ENGINE::GRAPHICS_READ);
            imageShipperNorm.SetDataFromPath(
                "C:\\Users\\carlo\\OneDrive\\Pictures\\Screenshots\\Screenshot 2024-09-19 172847.png");
            imageShipperNorm.BuildImage(core, 1, 1, renderGraphRef->core->swapchainRef->GetFormat(),
                                       ENGINE::GRAPHICS_READ);
          
            dstSet = descriptorAllocatorRef->Allocate(core->logicalDevice.get(), dstLayout.get());
            
            writerBuilder.AddWriteImage(0, imageShipperCol.imageView.get(),
                                        imageShipperCol.sampler->samplerHandle.get(),
                                        vk::ImageLayout::eShaderReadOnlyOptimal,
                                        vk::DescriptorType::eCombinedImageSampler);
            writerBuilder.AddWriteImage(1, imageShipperCol.imageView.get(),
                                        imageShipperCol.sampler->samplerHandle.get(),
                                        vk::ImageLayout::eShaderReadOnlyOptimal,
                                        vk::DescriptorType::eCombinedImageSampler);

            writerBuilder.UpdateSet(core->logicalDevice.get(), dstSet.get());
            ENGINE::VertexInput vertexInput= M_Vertex3D::GetVertexInput();
            ENGINE::AttachmentInfo colInfo = ENGINE::GetColorAttachmentInfo();
            ENGINE::AttachmentInfo depthInfo = ENGINE::GetDepthAttachmentInfo();
            auto renderNode = renderGraphRef->AddPass(gBufferPassName);
            
            renderNode->SetVertModule(&vertShaderModule);
            renderNode->SetFragModule(&fragShaderModule);
            renderNode->SetFramebufferSize(windowProvider->GetWindowSize());
            renderNode->SetPipelineLayoutCI(layoutCreateInfo);
            renderNode->SetVertexInput(vertexInput);
            renderNode->AddColorAttachmentOutput("gColor", colInfo);
            renderNode->AddColorAttachmentOutput("gNorm", colInfo);
            renderNode->SetDepthAttachmentOutput("gDepth", depthInfo);
            renderNode->AddColorBlendConfig(ENGINE::BlendConfigs::B_OPAQUE);
            renderNode->SetDepthConfig(ENGINE::DepthConfigs::D_ENABLE);
            renderNode->AddNodeSampler("colGSampler", imageShipperCol.imageView.get());
            renderNode->AddNodeSampler("normGSampler", imageShipperNorm.imageView.get());
            renderNode->BuildRenderGraphNode();
 
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
                renderGraphRef->AddColorImageResource(gBufferPassName, "gColor", currImage);
                renderGraphRef->AddColorImageResource(gBufferPassName,"gDepth", colAttachmentView.get());
                renderGraphRef->AddDepthImageResource(gBufferPassName, "gDepth", currDepthImage);
                renderGraphRef->GetNode(gBufferPassName)->SetFramebufferSize(windowProvider->GetWindowSize());
            });
            auto renderOp = new std::function<void(vk::CommandBuffer& command_buffer)>(
                [this](vk::CommandBuffer& commandBuffer)
                {
                    vk::DeviceSize offset = 0;
                    commandBuffer.bindDescriptorSets(renderGraphRef->GetNode(gBufferPassName)->pipelineType,
                                                     renderGraphRef->GetNode(gBufferPassName)->pipelineLayout.get(), 0, 1,
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
                    
                        commandBuffer.pushConstants(renderGraphRef->GetNode(gBufferPassName)->pipelineLayout.get(),
                                                    vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                                                    0, sizeof(ForwardPc), &pc);
                    
                        commandBuffer.drawIndexed(model.indicesCount[i], 1, model.firstIndices[i],
                                                  static_cast<int32_t>(model.firstVertices[i]), 0);
                    }
           
                });
            
            renderGraphRef->GetNode(gBufferPassName)->AddTask(setViewTask);
            renderGraphRef->GetNode(gBufferPassName)->SetRenderOperation(renderOp);

            
        }
        void ReloadShaders() override
        {
            
        }

        ENGINE::DescriptorAllocator* descriptorAllocatorRef;
        WindowProvider* windowProvider;
        ENGINE::Core* core;
        ENGINE::RenderGraph* renderGraphRef;

        ENGINE::DescriptorWriterBuilder writerBuilder;
        vk::UniqueDescriptorSetLayout dstLayout;
        vk::UniqueDescriptorSet dstSet;
        
        ENGINE::ImageShipper imageShipperCol;
        ENGINE::ImageShipper imageShipperNorm;
        
        std::unique_ptr<ENGINE::Image> colAttachment;
        std::unique_ptr<ENGINE::Image> normAttachment;
        
        std::unique_ptr<ENGINE::ImageData> colAttachmentData;
        std::unique_ptr<ENGINE::ImageData> normAttachmentData;

        std::unique_ptr<ENGINE::ImageView> colAttachmentView;
        std::unique_ptr<ENGINE::ImageView> normAttachmentView;
        
        
        std::unique_ptr<ENGINE::Buffer> vertexBuffer;
        std::unique_ptr<ENGINE::Buffer> indexBuffer;
        
        std::string gBufferPassName = "gBuffer";
        std::string lightPassName = "Light";

        Camera camera = {glm::vec3(3.0f), Camera::CameraMode::E_FIXED};
        Model model{};
        ForwardPc pc{};
    };   
}

#endif //CLUSTERRENDERER_HPP
