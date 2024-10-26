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
                                                         core->swapchainRef->GetFormat(),
                                                         vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
            auto depthImageInfo = ENGINE::Image::CreateInfo2d(windowProvider->GetWindowSize(), 1, 1,
                                                         core->swapchainRef->depthFormat,
                                                         vk::ImageUsageFlagBits::eDepthStencilAttachment| vk::ImageUsageFlagBits::eSampled);
            
            
            colAttachment = std::make_unique<ENGINE::Image>(physicalDevice, logicalDevice, imageInfo);
            colAttachmentData = std::make_unique<ENGINE::ImageData>(colAttachment->imageHandle.get(),
                                                                    vk::ImageType::e2D,
                                                                    glm::vec3(windowProvider->GetWindowSize().x,
                                                                              windowProvider->GetWindowSize().y, 1), 1,
                                                                    1,
                                                                    core->swapchainRef->GetFormat(),
                                                                    vk::ImageLayout::eUndefined);
            colAttachmentView = std::make_unique<ENGINE::ImageView>(logicalDevice, colAttachmentData.get(), 0, 1, 0, 1);
            
            normAttachment = std::make_unique<ENGINE::Image>(physicalDevice, logicalDevice, imageInfo);
            normAttachmentData = std::make_unique<ENGINE::ImageData>(normAttachment->imageHandle.get(),
                                                                    vk::ImageType::e2D,
                                                                    glm::vec3(windowProvider->GetWindowSize().x,
                                                                              windowProvider->GetWindowSize().y, 1), 1,
                                                                    1,
                                                                    core->swapchainRef->GetFormat(),
                                                                    vk::ImageLayout::eUndefined);
            normAttachmentView = std::make_unique<ENGINE::ImageView>(logicalDevice, normAttachmentData.get(), 0, 1, 0, 1);
            
            depthAttachment = std::make_unique<ENGINE::Image>(physicalDevice, logicalDevice, depthImageInfo);
            depthAttachmentData = std::make_unique<ENGINE::ImageData>(depthAttachment->imageHandle.get(),
                                                                    vk::ImageType::e2D,
                                                                    glm::vec3(windowProvider->GetWindowSize().x,
                                                                              windowProvider->GetWindowSize().y, 1), 1,
                                                                    1,
                                                                    core->swapchainRef->depthFormat,
                                                                    vk::ImageLayout::eUndefined);
            depthAttachmentView = std::make_unique<ENGINE::ImageView>(logicalDevice, depthAttachmentData.get(), 0, 1, 0, 1);



            imageShipperCol.SetDataFromPath(
                "C:\\Users\\carlo\\OneDrive\\Pictures\\Screenshots\\Screenshot 2024-09-19 172847.png");
            imageShipperCol.BuildImage(core, 1, 1, renderGraphRef->core->swapchainRef->GetFormat(),
                                       ENGINE::GRAPHICS_READ);
            imageShipperNorm.SetDataFromPath(
                "C:\\Users\\carlo\\OneDrive\\Pictures\\Screenshots\\Screenshot 2024-09-19 172847.png");
            imageShipperNorm.BuildImage(core, 1, 1, renderGraphRef->core->swapchainRef->GetFormat(),
                                        ENGINE::GRAPHICS_READ);
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


            gDstLayout = builder.BuildBindings(
                core->logicalDevice.get(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
            

            auto pushConstantRange = vk::PushConstantRange()
                                     .setOffset(0)
                                     .setStageFlags(
                                         vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
                                     .setSize(sizeof(ForwardPc));
            
            auto layoutCreateInfo = vk::PipelineLayoutCreateInfo()
                                    .setSetLayoutCount(1)
                                    .setPushConstantRanges(pushConstantRange)
                                    .setPSetLayouts(&gDstLayout.get());
          
            gDstSet = descriptorAllocatorRef->Allocate(core->logicalDevice.get(), gDstLayout.get());
            
            writerBuilder.AddWriteImage(0, imageShipperCol.imageView.get(),
                                        imageShipperCol.sampler->samplerHandle.get(),
                                        vk::ImageLayout::eShaderReadOnlyOptimal,
                                        vk::DescriptorType::eCombinedImageSampler);
            writerBuilder.AddWriteImage(1, imageShipperNorm.imageView.get(),
                                        imageShipperNorm.sampler->samplerHandle.get(),
                                        vk::ImageLayout::eShaderReadOnlyOptimal,
                                        vk::DescriptorType::eCombinedImageSampler);

            writerBuilder.UpdateSet(core->logicalDevice.get(), gDstSet.get());
            
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
            renderNode->AddColorBlendConfig(ENGINE::BlendConfigs::B_OPAQUE);
            renderNode->SetDepthConfig(ENGINE::DepthConfigs::D_ENABLE);
            renderNode->AddNodeSampler("colGSampler", imageShipperCol.imageView.get());
            renderNode->AddNodeSampler("normGSampler", imageShipperNorm.imageView.get());
            renderNode->AddNodeColAttachmentImg("gColor", colAttachmentView.get());
            renderNode->AddNodeColAttachmentImg("gNorm", normAttachmentView.get());
            renderNode->SetNodeDepthAttachmentImg("gDepth", depthAttachmentView.get());
            renderNode->BuildRenderGraphNode();

            //light pass//

            quadVert = Vertex2D::GetQuadVertices();
            quadIndices = Vertex2D::GetQuadIndices();
            propsUbo.invProj = glm::inverse(camera.matrices.perspective);
            propsUbo.invView = glm::inverse(camera.matrices.view);
            
            lVertexBuffer = std::make_unique<ENGINE::Buffer>(
                physicalDevice, logicalDevice, vk::BufferUsageFlagBits::eVertexBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                sizeof(Vertex2D) * quadVert.size(), quadVert.data());
            lIndexBuffer = std::make_unique<ENGINE::Buffer>(
                physicalDevice, logicalDevice, vk::BufferUsageFlagBits::eIndexBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                sizeof(uint32_t) * quadIndices.size(), quadIndices.data());
            cPropsBuffer = std::make_unique<ENGINE::Buffer>(
                physicalDevice, logicalDevice, vk::BufferUsageFlagBits::eUniformBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                sizeof(cPropsBuffer), &propsUbo);
            cPropsBuffer->Map();
            cPropsBuffer->SetupDescriptor();
            
            std::vector<uint32_t> lightVertCode = ENGINE::GetByteCode(
                "C:\\Users\\carlo\\CLionProjects\\Vulkan_Engine_Template\\src\\Shaders\\spirv\\Common\\Quad.vert.spv");
            std::vector<uint32_t> lightFragCode = ENGINE::GetByteCode(
                "C:\\Users\\carlo\\CLionProjects\\Vulkan_Engine_Template\\src\\Shaders\\spirv\\ClusterRendering\\light.frag.spv");

            ENGINE::ShaderParser lVertParser(lightVertCode);
            ENGINE::ShaderParser lFragParser(lightFragCode);

            ENGINE::ShaderModule lVertShaderModule(logicalDevice, lightVertCode);
            ENGINE::ShaderModule lFragShaderModule(logicalDevice, lightFragCode);

            ENGINE::ShaderParser::GetLayout(lVertParser, builder);
            ENGINE::ShaderParser::GetLayout(lFragParser, builder);
            
            lDstLayout = builder.BuildBindings(
                core->logicalDevice.get(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);

            auto lLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
                                    .setSetLayoutCount(1)
                                    .setPSetLayouts(&lDstLayout.get());
          
            lDstSet = descriptorAllocatorRef->Allocate(core->logicalDevice.get(), lDstLayout.get());

            ENGINE::Sampler* sampler = renderGraphRef->samplerPool.GetSampler(vk::SamplerAddressMode::eRepeat, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear);
            
            writerBuilder.AddWriteImage(0, colAttachmentView.get(),
                                        sampler->samplerHandle.get(),
                                        vk::ImageLayout::eShaderReadOnlyOptimal,
                                        vk::DescriptorType::eCombinedImageSampler);
            writerBuilder.AddWriteImage(1, normAttachmentView.get(),
                                        sampler->samplerHandle.get(),
                                        vk::ImageLayout::eShaderReadOnlyOptimal,
                                        vk::DescriptorType::eCombinedImageSampler);
            writerBuilder.AddWriteImage(2, depthAttachmentView.get(),
                                        sampler->samplerHandle.get(),
                                        vk::ImageLayout::eShaderReadOnlyOptimal,
                                        vk::DescriptorType::eCombinedImageSampler);
            writerBuilder.AddWriteBuffer(3,
                                         cPropsBuffer.get(),
                                         vk::DescriptorType::eUniformBuffer);


            writerBuilder.UpdateSet(logicalDevice, lDstSet.get());
            
            ENGINE::VertexInput lVertexInput= Vertex2D::GetVertexInput();
            
            auto lRenderNode = renderGraphRef->AddPass(lightPassName);
            lRenderNode->SetVertModule(&lVertShaderModule);
            lRenderNode->SetFragModule(&lFragShaderModule);
            lRenderNode->SetFramebufferSize(windowProvider->GetWindowSize());
            lRenderNode->SetPipelineLayoutCI(lLayoutCreateInfo);
            lRenderNode->SetVertexInput(lVertexInput);
            lRenderNode->AddColorAttachmentOutput("lColor", colInfo);
            lRenderNode->SetDepthAttachmentOutput("lDepth", depthInfo);
            lRenderNode->AddColorBlendConfig(ENGINE::BlendConfigs::B_OPAQUE);
            lRenderNode->SetDepthConfig(ENGINE::DepthConfigs::D_ENABLE);
            lRenderNode->AddNodeSampler("colGSampler", colAttachmentView.get());
            lRenderNode->AddNodeSampler("normGSampler", normAttachmentView.get());
            lRenderNode->AddNodeSampler("depthGSampler", depthAttachmentView.get());
            lRenderNode->BuildRenderGraphNode();

            
        }
        void RecreateSwapChainResources() override
        {
            
        }
        void SetRenderOperation(ENGINE::InFlightQueue* inflightQueue) override
        {
            auto setViewTask = new std::function<void()>([this, inflightQueue]()
            {
                // renderGraphRef->GetNode(gBufferPassName)->SetFramebufferSize(windowProvider->GetWindowSize());
            });
            auto renderOp = new std::function<void(vk::CommandBuffer& command_buffer)>(
                [this](vk::CommandBuffer& commandBuffer)
                {
                    vk::DeviceSize offset = 0;
                    commandBuffer.bindDescriptorSets(renderGraphRef->GetNode(gBufferPassName)->pipelineType,
                                                     renderGraphRef->GetNode(gBufferPassName)->pipelineLayout.get(), 0, 1,
                                                     &gDstSet.get(), 0 , nullptr);

                    commandBuffer.bindVertexBuffers(0, 1, &vertexBuffer->bufferHandle.get(), &offset);
                    commandBuffer.bindIndexBuffer(indexBuffer->bufferHandle.get(), 0, vk::IndexType::eUint32);
                    
                    for (int i = 0; i < model.meshCount; ++i)
                    {
                        camera.SetPerspective(
                            45.0f, (float)windowProvider->GetWindowSize().x / (float)windowProvider->GetWindowSize().y,
                            0.1f, 512.0f);
                        pc.projView = camera.matrices.perspective * camera.matrices.view;
                        pc.model = model.modelsMat[i];
                        commandBuffer.pushConstants(renderGraphRef->GetNode(gBufferPassName)->pipelineLayout.get(),
                                                    vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                                                    0, sizeof(ForwardPc), &pc);
                    
                        commandBuffer.drawIndexed(model.indicesCount[i], 1, model.firstIndices[i],
                                                  static_cast<int32_t>(model.firstVertices[i]), 0);
                    }
           
                });

                renderGraphRef->GetNode(gBufferPassName)->AddTask(setViewTask);
                renderGraphRef->GetNode(gBufferPassName)->SetRenderOperation(renderOp);

                auto lSetViewTask = new std::function<void()>([this, inflightQueue]()
                {
                    auto* currImage = inflightQueue->currentSwapchainImageView;
                    auto& currDepthImage = core->swapchainRef->depthImagesFull.at(inflightQueue->frameIndex);
                    renderGraphRef->AddColorImageResource(lightPassName, "lColor", currImage);
                    renderGraphRef->AddDepthImageResource(lightPassName, "lDepth", currDepthImage.imageView.get());
                    renderGraphRef->GetNode(lightPassName)->SetFramebufferSize(windowProvider->GetWindowSize());
                    propsUbo.invProj = glm::inverse(camera.matrices.perspective);
                    propsUbo.invView = glm::inverse(camera.matrices.view);
                    memcpy(cPropsBuffer->mappedMem, &propsUbo, sizeof(CPropsUbo)); 
                    
                });
                auto lRenderOp = new std::function<void(vk::CommandBuffer& command_buffer)>(
                    [this](vk::CommandBuffer& commandBuffer)
                    {
                        vk::DeviceSize offset = 0;
                        commandBuffer.bindDescriptorSets(renderGraphRef->GetNode(lightPassName)->pipelineType,
                                                         renderGraphRef->GetNode(lightPassName)->pipelineLayout.get(),
                                                         0, 1,
                                                         &lDstSet.get(), 0, nullptr);
                        commandBuffer.bindVertexBuffers(0, 1, &lVertexBuffer->bufferHandle.get(), &offset);
                        commandBuffer.bindIndexBuffer(lIndexBuffer->bufferHandle.get(), 0, vk::IndexType::eUint32);

                        commandBuffer.drawIndexed(quadIndices.size(), 1, 0,
                                                  0, 0);
                    });
            
                renderGraphRef->GetNode(lightPassName)->AddTask(lSetViewTask);
                renderGraphRef->GetNode(lightPassName)->SetRenderOperation(lRenderOp);
            
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
            std::vector<uint32_t> gVertCode = ENGINE::GetByteCode(
                "C:\\Users\\carlo\\CLionProjects\\Vulkan_Engine_Template\\src\\Shaders\\spirv\\ClusterRendering\\gBuffer.vert.spv");
            std::vector<uint32_t> gFragCode = ENGINE::GetByteCode(
                "C:\\Users\\carlo\\CLionProjects\\Vulkan_Engine_Template\\src\\Shaders\\spirv\\ClusterRendering\\gBuffer.frag.spv");
            std::vector<uint32_t> vertCode = ENGINE::GetByteCode(
                "C:\\Users\\carlo\\CLionProjects\\Vulkan_Engine_Template\\src\\Shaders\\spirv\\Common\\Quad.vert.spv");
            std::vector<uint32_t> fragCode = ENGINE::GetByteCode(
                "C:\\Users\\carlo\\CLionProjects\\Vulkan_Engine_Template\\src\\Shaders\\spirv\\ClusterRendering\\light.frag.spv");

            
            ENGINE::ShaderModule gVertShaderModule(core->logicalDevice.get(), gVertCode);
            ENGINE::ShaderModule gFragShaderModule(core->logicalDevice.get(), gFragCode);

            ENGINE::ShaderModule vertShaderModule(core->logicalDevice.get(), vertCode);
            ENGINE::ShaderModule fragShaderModule(core->logicalDevice.get(), fragCode);
            
           auto pushConstantRange = vk::PushConstantRange()
                                     .setOffset(0)
                                     .setStageFlags(
                                         vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
                                     .setSize(sizeof(ForwardPc));
  
            auto gLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
                                     .setPushConstantRanges(pushConstantRange)
                                     .setSetLayoutCount(1)
                                     .setPSetLayouts(&gDstLayout.get());
           
            auto layoutCreateInfo = vk::PipelineLayoutCreateInfo()
                                    .setSetLayoutCount(1)
                                    .setPSetLayouts(&lDstLayout.get());
            
            
            auto* gRenderNode = renderGraphRef->GetNode(gBufferPassName);
            auto* renderNode = renderGraphRef->GetNode(lightPassName);
            
            gRenderNode->SetPipelineLayoutCI(gLayoutCreateInfo);
            gRenderNode->SetVertModule(&gVertShaderModule);
            gRenderNode->SetFragModule(&gFragShaderModule);
            gRenderNode->RecreateResources();
            
            renderNode->SetPipelineLayoutCI(layoutCreateInfo);
            renderNode->SetVertModule(&vertShaderModule);
            renderNode->SetFragModule(&fragShaderModule);
            renderNode->RecreateResources();
            
            std::cout<< "Shaders reloaded\n";
        }


        ENGINE::DescriptorAllocator* descriptorAllocatorRef;
        WindowProvider* windowProvider;
        ENGINE::Core* core;
        ENGINE::RenderGraph* renderGraphRef;

        ENGINE::DescriptorWriterBuilder writerBuilder;
        
        vk::UniqueDescriptorSetLayout gDstLayout;
        vk::UniqueDescriptorSet gDstSet;
        
        vk::UniqueDescriptorSetLayout lDstLayout;
        vk::UniqueDescriptorSet lDstSet;
        
        ENGINE::ImageShipper imageShipperCol;
        ENGINE::ImageShipper imageShipperNorm;
        
        std::unique_ptr<ENGINE::Image> colAttachment;
        std::unique_ptr<ENGINE::ImageData> colAttachmentData;
        std::unique_ptr<ENGINE::ImageView> colAttachmentView;
        
        std::unique_ptr<ENGINE::Image> normAttachment;
        std::unique_ptr<ENGINE::ImageData> normAttachmentData;
        std::unique_ptr<ENGINE::ImageView> normAttachmentView;

        std::unique_ptr<ENGINE::Image> depthAttachment;
        std::unique_ptr<ENGINE::ImageData> depthAttachmentData;
        std::unique_ptr<ENGINE::ImageView> depthAttachmentView;
        
        std::unique_ptr<ENGINE::Buffer> vertexBuffer;
        std::unique_ptr<ENGINE::Buffer> indexBuffer;
        
        std::unique_ptr<ENGINE::Buffer> lVertexBuffer;
        std::unique_ptr<ENGINE::Buffer> lIndexBuffer;
        
        std::unique_ptr<ENGINE::Buffer> cPropsBuffer;
        
        std::string gBufferPassName = "gBuffer";
        std::string lightPassName = "Light";

        Camera camera = {glm::vec3(3.0f), Camera::CameraMode::E_FIXED};
        CPropsUbo propsUbo;
        Model model{};
        
        
        std::vector<Vertex2D> quadVert;
        std::vector<uint32_t> quadIndices;
        ForwardPc pc{};
    };   
}

#endif //CLUSTERRENDERER_HPP
