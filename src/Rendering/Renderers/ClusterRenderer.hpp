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
            camera.SetPerspective( 45.0f, (float)windowProvider->GetWindowSize().x / (float)windowProvider->GetWindowSize().y,
                0.1f, 512.0f);

            screenDataPc.sWidth = windowProvider->GetWindowSize().x;
            screenDataPc.sHeight = windowProvider->GetWindowSize().y;
            
            camera.SetLookAt(glm::vec3(0.0f));
            auto imageInfo = ENGINE::Image::CreateInfo2d(windowProvider->GetWindowSize(), 1, 1,
                                                         vk::Format::eR32G32B32A32Sfloat,
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
                                                                    vk::Format::eR32G32B32A32Sfloat,
                                                                    vk::ImageLayout::eUndefined);
            colAttachmentView = std::make_unique<ENGINE::ImageView>(logicalDevice, colAttachmentData.get(), 0, 1, 0, 1);
            
            normAttachment = std::make_unique<ENGINE::Image>(physicalDevice, logicalDevice, imageInfo);
            normAttachmentData = std::make_unique<ENGINE::ImageData>(normAttachment->imageHandle.get(),
                                                                    vk::ImageType::e2D,
                                                                    glm::vec3(windowProvider->GetWindowSize().x,
                                                                              windowProvider->GetWindowSize().y, 1), 1,
                                                                    1,
                                                                    vk::Format::eR32G32B32A32Sfloat,
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
            imageShipperCol.BuildImage(core, 1, 1, core->swapchainRef->GetFormat(),
                                       ENGINE::GRAPHICS_READ);
            imageShipperNorm.SetDataFromPath(
                "C:\\Users\\carlo\\OneDrive\\Pictures\\Screenshots\\Screenshot 2024-09-19 172847.png");
            imageShipperNorm.BuildImage(core, 1, 1, core->swapchainRef->GetFormat(),
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

            gVertShader =std::make_unique<ENGINE::Shader>(logicalDevice, "C:\\Users\\carlo\\CLionProjects\\Vulkan_Engine_Template\\src\\Shaders\\spirv\\ClusterRendering\\gBuffer.vert.spv");
            gFragShader =std::make_unique<ENGINE::Shader>(logicalDevice, "C:\\Users\\carlo\\CLionProjects\\Vulkan_Engine_Template\\src\\Shaders\\spirv\\ClusterRendering\\gBuffer.frag.spv");

            ENGINE::DescriptorLayoutBuilder builder;

            ENGINE::ShaderParser::GetLayout(*gVertShader.get()->sParser, builder);
            ENGINE::ShaderParser::GetLayout(*gFragShader.get()->sParser, builder);

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
            ENGINE::AttachmentInfo colInfo = ENGINE::GetColorAttachmentInfo(glm::vec4(0.0f), vk::Format::eR32G32B32A32Sfloat);
            ENGINE::AttachmentInfo depthInfo = ENGINE::GetDepthAttachmentInfo();
            auto renderNode = renderGraphRef->AddPass(gBufferPassName);
            
            renderNode->SetVertShader(gVertShader.get());
            renderNode->SetFragShader(gFragShader.get());
            renderNode->SetFramebufferSize(windowProvider->GetWindowSize());
            renderNode->SetPipelineLayoutCI(layoutCreateInfo);
            renderNode->SetVertexInput(vertexInput);
            renderNode->AddColorAttachmentOutput("gColor", colInfo);
            renderNode->AddColorAttachmentOutput("gNorm", colInfo);
            renderNode->SetDepthAttachmentOutput("gDepth", depthInfo);
            renderNode->AddColorBlendConfig(ENGINE::BlendConfigs::B_OPAQUE);
            renderNode->AddColorBlendConfig(ENGINE::BlendConfigs::B_OPAQUE);
            renderNode->SetDepthConfig(ENGINE::DepthConfigs::D_ENABLE);
            renderNode->AddSamplerResource("colGSampler", imageShipperCol.imageView.get());
            renderNode->AddSamplerResource("normGSampler", imageShipperNorm.imageView.get());
            renderNode->AddColorImageResource("gColor", colAttachmentView.get());
            renderNode->AddColorImageResource("gNorm", normAttachmentView.get());
            renderNode->SetDepthImageResource("gDepth", depthAttachmentView.get());
            renderNode->BuildRenderGraphNode();
            
            //Cull pass//

            std::random_device rd;
            std::mt19937 gen(rd());

            pointLights.reserve(1024);
            for (int i = 0; i < 1024; ++i)
            {
                std::uniform_real_distribution<> distribution(0.0f, 2.0f);
                float radius = distribution(gen);

                std::uniform_real_distribution<> distributionPos(-3.0f, 3.0f);
                glm::vec3 pos = glm::vec3(distributionPos(gen), 0.2f, distributionPos(gen));
                
                std::uniform_real_distribution<> distributionCol(0.0f, 1.0f);
                glm::vec3 col = glm::vec3(distributionCol(gen), distributionCol(gen), distributionCol(gen));
                
                std::uniform_real_distribution<> distributionIntensity(3.0f, 9.0f);
                float intensity = distributionIntensity(gen);
                
                std::uniform_real_distribution<> distributionAttenuation(0.0f, 5.0f);
                float lAttenuation = distributionAttenuation(gen);
                float qAttenuation = distributionAttenuation(gen);
                
                pointLights.emplace_back(PointLight{pos, col, radius, intensity, lAttenuation, qAttenuation});
            }
            lightsMap.reserve(tileSize * tileSize);
            for (int i = 0; i < tileSize * tileSize; ++i)
            {
                lightsMap.emplace_back(ArrayIndexer{});
            }
            

            pointLightsBuff = std::make_unique<ENGINE::Buffer>(
                physicalDevice, logicalDevice, vk::BufferUsageFlagBits::eStorageBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                sizeof(PointLight) * pointLights.size(), pointLights.data());
            

            lightsMapBuff = std::make_unique<ENGINE::Buffer>(
                physicalDevice, logicalDevice, vk::BufferUsageFlagBits::eStorageBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                sizeof(ArrayIndexer) * lightsMap.size(), lightsMap.data());
            lightsMapBuff->Map();
            
            camPropsBuff = std::make_unique<ENGINE::Buffer>(
                physicalDevice, logicalDevice, vk::BufferUsageFlagBits::eUniformBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                sizeof(CPropsUbo), &cPropsUbo);
            camPropsBuff->Map();
            
            
            
            cullCompShader = std::make_unique<ENGINE::Shader>(logicalDevice,
                                                              "C:\\Users\\carlo\\CLionProjects\\Vulkan_Engine_Template\\src\\Shaders\\spirv\\ClusterRendering\\lightCulling.comp.spv");


            ENGINE::ShaderParser::GetLayout(*cullCompShader->sParser, builder);
            
            auto cullPushConstantRange = vk::PushConstantRange()
                                     .setOffset(0)
                                     .setStageFlags(vk::ShaderStageFlagBits::eCompute)
                                     .setSize(sizeof(ScreenDataPc));
            
            cullDstLayout = builder.BuildBindings(logicalDevice,
                                                  vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eFragment
                                                  | vk::ShaderStageFlagBits::eFragment);

            auto cullLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
            .setSetLayoutCount(1)
            .setPushConstantRanges(cullPushConstantRange)
            .setPSetLayouts(&cullDstLayout.get());
            
            cullDstSet = descriptorAllocatorRef->Allocate(core->logicalDevice.get(), cullDstLayout.get());

            writerBuilder.AddWriteBuffer(0, pointLightsBuff->descriptor, vk::DescriptorType::eStorageBuffer);
            writerBuilder.AddWriteBuffer(1, lightsMapBuff->descriptor, vk::DescriptorType::eStorageBuffer);
            writerBuilder.AddWriteBuffer(2, camPropsBuff->descriptor,vk::DescriptorType::eUniformBuffer);
            
            writerBuilder.UpdateSet(core->logicalDevice.get(), cullDstSet.get());

            auto* cullRenderNode = renderGraphRef->AddPass(computePassName);
            cullRenderNode->SetCompShader(cullCompShader.get());
            cullRenderNode->SetPipelineLayoutCI(cullLayoutCreateInfo);
            cullRenderNode->BuildRenderGraphNode();
            

            //light pass//

            quadVert = Vertex2D::GetQuadVertices();
            quadIndices = Vertex2D::GetQuadIndices();
            cPropsUbo.invProj = glm::inverse(camera.matrices.perspective);
            cPropsUbo.invView = glm::inverse(camera.matrices.view);
            
            lVertexBuffer = std::make_unique<ENGINE::Buffer>(
                physicalDevice, logicalDevice, vk::BufferUsageFlagBits::eVertexBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                sizeof(Vertex2D) * quadVert.size(), quadVert.data());
            lIndexBuffer = std::make_unique<ENGINE::Buffer>(
                physicalDevice, logicalDevice, vk::BufferUsageFlagBits::eIndexBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                sizeof(uint32_t) * quadIndices.size(), quadIndices.data());
           
            lVertShader =std::make_unique<ENGINE::Shader>(logicalDevice, "C:\\Users\\carlo\\CLionProjects\\Vulkan_Engine_Template\\src\\Shaders\\spirv\\Common\\Quad.vert.spv");
            lFragShader =std::make_unique<ENGINE::Shader>(logicalDevice, "C:\\Users\\carlo\\CLionProjects\\Vulkan_Engine_Template\\src\\Shaders\\spirv\\ClusterRendering\\light.frag.spv");
            
            ENGINE::ShaderParser::GetLayout(*lVertShader->sParser, builder);
            ENGINE::ShaderParser::GetLayout(*lFragShader->sParser, builder);
            
            lDstLayout = builder.BuildBindings(
                core->logicalDevice.get(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);

            auto lLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
                                    .setSetLayoutCount(1)
                                    .setPSetLayouts(&lDstLayout.get());
          
            lDstSet = descriptorAllocatorRef->Allocate(core->logicalDevice.get(), lDstLayout.get());

            ENGINE::Sampler* sampler = renderGraphRef->samplerPool.AddSampler(logicalDevice, vk::SamplerAddressMode::eRepeat, vk::Filter::eNearest, vk::SamplerMipmapMode::eNearest);
            ENGINE::Sampler* depthSampler = renderGraphRef->samplerPool.AddSampler(
                logicalDevice, vk::SamplerAddressMode::eClampToEdge, vk::Filter::eNearest,
                vk::SamplerMipmapMode::eNearest);
            
            writerBuilder.AddWriteImage(0, colAttachmentView.get(),
                                        sampler->samplerHandle.get(),
                                        vk::ImageLayout::eShaderReadOnlyOptimal,
                                        vk::DescriptorType::eCombinedImageSampler);
            writerBuilder.AddWriteImage(1, normAttachmentView.get(),
                                        sampler->samplerHandle.get(),
                                        vk::ImageLayout::eShaderReadOnlyOptimal,
                                        vk::DescriptorType::eCombinedImageSampler);
            writerBuilder.AddWriteImage(2, depthAttachmentView.get(),
                                        depthSampler->samplerHandle.get(),
                                        vk::ImageLayout::eShaderReadOnlyOptimal,
                                        vk::DescriptorType::eCombinedImageSampler);
            writerBuilder.AddWriteBuffer(3, camPropsBuff->descriptor,vk::DescriptorType::eUniformBuffer);
            writerBuilder.AddWriteBuffer(4, pointLightsBuff->descriptor, vk::DescriptorType::eStorageBuffer);
            writerBuilder.AddWriteBuffer(5, lightsMapBuff->descriptor, vk::DescriptorType::eStorageBuffer);
            
            writerBuilder.UpdateSet(logicalDevice, lDstSet.get());
            
            ENGINE::AttachmentInfo lColInfo = ENGINE::GetColorAttachmentInfo(glm::vec4(0.0f), core->swapchainRef->GetFormat());
            
            ENGINE::VertexInput lVertexInput= Vertex2D::GetVertexInput();
            
            auto lRenderNode = renderGraphRef->AddPass(lightPassName);
            lRenderNode->SetVertShader(lVertShader.get());
            lRenderNode->SetFragShader(lFragShader.get());
            lRenderNode->SetFramebufferSize(windowProvider->GetWindowSize());
            lRenderNode->SetPipelineLayoutCI(lLayoutCreateInfo);
            lRenderNode->SetVertexInput(lVertexInput);
            lRenderNode->AddColorAttachmentOutput("lColor", lColInfo);
            lRenderNode->AddColorBlendConfig(ENGINE::BlendConfigs::B_OPAQUE);
            lRenderNode->AddSamplerResource("colGSampler", colAttachmentView.get());
            lRenderNode->AddSamplerResource("normGSampler", normAttachmentView.get());
            lRenderNode->AddSamplerResource("depthGSampler", depthAttachmentView.get());
            lRenderNode->DependsOn(computePassName);
            lRenderNode->BuildRenderGraphNode();

            
        }

        void RecreateSwapChainResources() override
        {
            
        }
        void SetRenderOperation(ENGINE::InFlightQueue* inflightQueue) override
        {
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

                        pc.projView = camera.matrices.perspective * camera.matrices.view;
                        pc.model = model.modelsMat[i];
                        commandBuffer.pushConstants(renderGraphRef->GetNode(gBufferPassName)->pipelineLayout.get(),
                                                    vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                                                    0, sizeof(ForwardPc), &pc);
                    
                        commandBuffer.drawIndexed(model.indicesCount[i], 1, model.firstIndices[i],
                                                  static_cast<int32_t>(model.firstVertices[i]), 0);
                    }
           
                });

                renderGraphRef->GetNode(gBufferPassName)->SetRenderOperation(renderOp);

                auto cullTask = new std::function<void()>([this, inflightQueue]()
                {
                    lightsMap.clear();
                    for (int i = 0; i < tileSize * tileSize; ++i)
                    {
                        lightsMap.emplace_back(ArrayIndexer{});
                    }
                    memcpy(lightsMapBuff->mappedMem, lightsMap.data(), sizeof(ArrayIndexer) * lightsMap.size());
                    
                    cPropsUbo.invProj = glm::inverse(camera.matrices.perspective);
                    cPropsUbo.invView = glm::inverse(camera.matrices.view);
                    memcpy(camPropsBuff->mappedMem, &cPropsUbo, sizeof(CPropsUbo));
                    
                    screenDataPc.sWidth = (int)windowProvider->GetWindowSize().x;
                    screenDataPc.sHeight = (int)windowProvider->GetWindowSize().y;
                });
            
                auto cullRenderOp =  new std::function<void(vk::CommandBuffer& command_buffer)>(
                    [this](vk::CommandBuffer& commandBuffer)
                    {
                        auto& renderNode = renderGraphRef->renderNodes.at(computePassName);
                        commandBuffer.bindDescriptorSets(renderNode->pipelineType,
                                                         renderNode->pipelineLayout.get(), 0,
                                                         1,
                                                         &cullDstSet.get(), 0, nullptr);
                        commandBuffer.pushConstants(renderGraphRef->GetNode(computePassName)->pipelineLayout.get(),
                                                    vk::ShaderStageFlagBits::eCompute,
                                                    0, sizeof(ScreenDataPc), &screenDataPc);
                        commandBuffer.bindPipeline(renderNode->pipelineType, renderNode->pipeline.get());
                        int xGpCount = tileSize / localSize;
                        int yGpCount = tileSize / localSize;
                        commandBuffer.dispatch(xGpCount, yGpCount, 1);
                    });

                renderGraphRef->GetNode(computePassName)->AddTask(cullTask);
                renderGraphRef->GetNode(computePassName)->SetRenderOperation(cullRenderOp);
                auto lSetViewTask = new std::function<void()>([this, inflightQueue]()
                {
                    auto* currImage = inflightQueue->currentSwapchainImageView;
                    auto& currDepthImage = core->swapchainRef->depthImagesFull.at(inflightQueue->frameIndex);
                    renderGraphRef->AddColorImageResource(lightPassName, "lColor", currImage);
                    renderGraphRef->GetNode(lightPassName)->SetFramebufferSize(windowProvider->GetWindowSize());
                    
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
            
            auto* gRenderNode = renderGraphRef->GetNode(gBufferPassName);
            auto* renderNode = renderGraphRef->GetNode(lightPassName);
            auto* cRenderNode = renderGraphRef->GetNode(computePassName);
            
            gRenderNode->RecreateResources();
            renderNode->RecreateResources();
            cRenderNode->RecreateResources();
            
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

        vk::UniqueDescriptorSetLayout cullDstLayout;
        vk::UniqueDescriptorSet cullDstSet;

        std::unique_ptr<ENGINE::Shader> gVertShader;
        std::unique_ptr<ENGINE::Shader> gFragShader;
        
        std::unique_ptr<ENGINE::Shader> lVertShader;
        std::unique_ptr<ENGINE::Shader> lFragShader;
        
        std::unique_ptr<ENGINE::Shader> cullCompShader;
        
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
        
        std::unique_ptr<ENGINE::Buffer> camPropsBuff;

        //std::unique_ptr<ENGINE::Buffer> dirLightsBuff;
        std::unique_ptr<ENGINE::Buffer> pointLightsBuff;
        std::unique_ptr<ENGINE::Buffer> lightsMapBuff;
        
        
        std::string gBufferPassName = "gBuffer";
        std::string computePassName= "cullLight";
        std::string lightPassName = "light";

        //gbuff
        Camera camera = {glm::vec3(5.0f), Camera::CameraMode::E_FREE};
        Model model{};
        ForwardPc pc{};

        //culling
        std::vector<PointLight> pointLights;
        std::vector<ArrayIndexer> lightsMap;
        ScreenDataPc screenDataPc;
        //std::vector<DirectionalLight> directionalLights;
        int tileSize = 32;
        int localSize = 1;
        

        //light
        std::vector<Vertex2D> quadVert;
        std::vector<uint32_t> quadIndices;
        CPropsUbo cPropsUbo;
    };   
}

#endif //CLUSTERRENDERER_HPP
