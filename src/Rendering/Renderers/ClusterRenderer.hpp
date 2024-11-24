//





// Created by carlo on 2024-10-25.
//

#ifndef CLUSTERRENDERER_HPP
#define CLUSTERRENDERER_HPP

namespace Rendering
{
    using namespace ENGINE;
    class ClusterRenderer : BaseRenderer
    {
    public:
        ClusterRenderer(Core* core, WindowProvider* windowProvider,
                        DescriptorAllocator* descriptorAllocator)
        {
            this->core = core;
            this->renderGraphRef = core->renderGraphRef;
            this->windowProvider = windowProvider;
            this->descriptorAllocatorRef = descriptorAllocator;
            computeDescCache = std::make_unique<DescriptorCache>(this->core);
            gBufferDescCache = std::make_unique<DescriptorCache>(this->core);
            lightDecCache = std::make_unique<DescriptorCache>(this->core);

            CreateResources();
            CreateBuffers();
            CreatePipelines();
        }

        void RecreateSwapChainResources() override
        {
        }

        void SetRenderOperation(InFlightQueue* inflightQueue) override
        {
            auto cullTask = new std::function<void()>([this, inflightQueue]()
            {
                Debug();
            });

            auto cullRenderOp = new std::function<void(vk::CommandBuffer& command_buffer)>(
                [this](vk::CommandBuffer& commandBuffer)
                {
                    auto& renderNode = renderGraphRef->renderNodes.at(computePassName);
                    commandBuffer.bindDescriptorSets(renderNode->pipelineType,
                                                     renderNode->pipelineLayout.get(), 0,
                                                     1,
                                                     &cullDstSet.get(), 0, nullptr);
                    commandBuffer.pushConstants(renderGraphRef->GetNode(computePassName)->pipelineLayout.get(),
                                                vk::ShaderStageFlagBits::eCompute,
                                                0, sizeof(ScreenDataPc), &cullDataPc);
                    commandBuffer.bindPipeline(renderNode->pipelineType, renderNode->pipeline.get());
                    commandBuffer.dispatch(cullDataPc.xTileCount / localSize, cullDataPc.yTileCount / localSize,
                                           zSlicesSize);
                });


            renderGraphRef->GetNode(computePassName)->AddTask(cullTask);
            renderGraphRef->GetNode(computePassName)->SetRenderOperation(cullRenderOp);

            auto renderOp = new std::function<void(vk::CommandBuffer& command_buffer)>(
                [this](vk::CommandBuffer& commandBuffer)
                {
                    vk::DeviceSize offset = 0;
                    commandBuffer.bindDescriptorSets(renderGraphRef->GetNode(gBufferPassName)->pipelineType,
                                                     renderGraphRef->GetNode(gBufferPassName)->pipelineLayout.get(), 0,
                                                     1,
                                                     &gDstSet.get(), 0, nullptr);

                    commandBuffer.bindVertexBuffers(0, 1, &vertexBuffer->bufferHandle.get(), &offset);
                    commandBuffer.bindIndexBuffer(indexBuffer->bufferHandle.get(), 0, vk::IndexType::eUint32);

                    for (int i = 0; i < model.meshCount; ++i)
                    {
                        pc.projView = camera.matrices.perspective * camera.matrices.view;
                        pc.model = model.modelsMat[i];
                        commandBuffer.pushConstants(renderGraphRef->GetNode(gBufferPassName)->pipelineLayout.get(),
                                                    vk::ShaderStageFlagBits::eVertex |
                                                    vk::ShaderStageFlagBits::eFragment,
                                                    0, sizeof(ForwardPc), &pc);

                        commandBuffer.drawIndexed(model.indicesCount[i], 1, model.firstIndices[i],
                                                  static_cast<int32_t>(model.firstVertices[i]), 0);
                    }
                });

            renderGraphRef->GetNode(gBufferPassName)->SetRenderOperation(renderOp);


            auto lSetViewTask = new std::function<void()>([this, inflightQueue]()
            {
                lightPc.xTileCount = cullDataPc.xTileCount;
                lightPc.yTileCount = cullDataPc.yTileCount;
                lightPc.xTileSizePx = xTileSizePx;
                lightPc.yTileSizePx = yTileSizePx;
                lightPc.zSlices = zSlicesSize;
                
                auto* currImage = inflightQueue->currentSwapchainImageView;
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

                    commandBuffer.pushConstants(renderGraphRef->GetNode(lightPassName)->pipelineLayout.get(),
                                                vk::ShaderStageFlagBits::eFragment,
                                                0, sizeof(LightPc), &lightPc);
                    commandBuffer.drawIndexed(quadIndices.size(), 1, 0,
                                              0, 0);
                });

            renderGraphRef->GetNode(lightPassName)->AddTask(lSetViewTask);
            renderGraphRef->GetNode(lightPassName)->SetRenderOperation(lRenderOp);
        }

        void Debug()
        {
            cullDataPc.sWidth = (int)windowProvider->GetWindowSize().x;
            cullDataPc.sHeight = (int)windowProvider->GetWindowSize().y;
            cullDataPc.pointLightsCount = pointLights.size();
            cullDataPc.xTileCount = static_cast<uint32_t>((core->swapchainRef->extent.width - 1) / xTileSizePx +
                1);
            cullDataPc.yTileCount = static_cast<uint32_t>((core->swapchainRef->extent.height - 1) / yTileSizePx +
                1);

            lightsMap.clear();
            for (int i = 0; i < cullDataPc.xTileCount * cullDataPc.yTileCount * zSlicesSize; ++i)
            {
                lightsMap.emplace_back(ArrayIndexer{});
            }
            memcpy(lightsMapBuff->mappedMem, lightsMap.data(), sizeof(ArrayIndexer) * lightsMap.size());

            lightsIndices.clear();
            for (int i = 0; i < lightsIndices.capacity(); ++i)
            {
                lightsIndices.emplace_back(-1);
            }
            memcpy(lightsIndicesBuff->mappedMem, lightsIndices.data(), sizeof(int32_t) * lightsIndices.size());

            cPropsUbo.invProj = glm::inverse(camera.matrices.perspective);
            cPropsUbo.invView = glm::inverse(camera.matrices.view);
            cPropsUbo.pos = camera.position;
            cPropsUbo.zNear = camera.cameraProperties.zNear;
            cPropsUbo.zFar = camera.cameraProperties.zFar;

            memcpy(camPropsBuff->mappedMem, &cPropsUbo, sizeof(CPropsUbo));

            memcpy(pointLightsBuff->mappedMem, pointLights.data(), sizeof(PointLight) * pointLights.size());

            // computeDescCache->SetBuffer("PointLights", pointLights);
            // computeDescCache->SetBuffer("LightMap", lightsMap);
            // computeDescCache->SetBuffer("LightIndices", lightsIndices);
            // computeDescCache->SetBuffer("CameraProperties", cPropsUbo);
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

        void CreateResources()
        {
            auto& physicalDevice = core->physicalDevice;
            auto& logicalDevice = core->logicalDevice.get();
            auto imageInfo = Image::CreateInfo2d(windowProvider->GetWindowSize(), 1, 1,
                                                         vk::Format::eR32G32B32A32Sfloat,
                                                         vk::ImageUsageFlagBits::eColorAttachment |
                                                         vk::ImageUsageFlagBits::eSampled);
            auto depthImageInfo = Image::CreateInfo2d(windowProvider->GetWindowSize(), 1, 1,
                                                              core->swapchainRef->depthFormat,
                                                              vk::ImageUsageFlagBits::eDepthStencilAttachment |
                                                              vk::ImageUsageFlagBits::eSampled);


            colAttachmentView = ResourcesManager::GetInstance()->GetImage("colAttachment", imageInfo, 0, 1, 0, 1);
            normAttachmentView = ResourcesManager::GetInstance()->GetImage("normAttachment", imageInfo, 0, 1, 0, 1); 
            depthAttachmentView = ResourcesManager::GetInstance()->GetImage("depthAttachment", depthImageInfo, 0, 1, 0, 1); 

            //gbuff
            camera.SetPerspective(
                45.0f, (float)windowProvider->GetWindowSize().x / (float)windowProvider->GetWindowSize().y,
                0.1f, 512.0f);


            camera.SetLookAt(glm::vec3(0.0f, 0.0f, 1.0f));
            camera.position = glm::vec3(0.0f);

            imageShipperCol = ResourcesManager::GetInstance()->SetShipperPath( "Color", "C:\\Users\\carlo\\OneDrive\\Pictures\\Screenshots\\Screenshot 2024-09-19 172847.png");
            imageShipperCol = ResourcesManager::GetInstance()->GetShipper("Color", 1, 1, core->swapchainRef->GetFormat(),
                                       GRAPHICS_READ);
            imageShipperNorm=ResourcesManager::GetInstance()->SetShipperPath("Norm","C:\\Users\\carlo\\OneDrive\\Pictures\\Screenshots\\Screenshot 2024-09-19 172847.png");
            imageShipperNorm=ResourcesManager::GetInstance()->GetShipper("Norm", 1, 1, core->swapchainRef->GetFormat(),
                                        GRAPHICS_READ);
            ModelLoader::GetInstance()->LoadGLTF(
                "C:\\Users\\carlo\\CLionProjects\\Vulkan_Engine_Template\\Resources\\Assets\\Models\\sponza\\scene.gltf",
                model);

            //compute
            std::random_device rd;
            std::mt19937 gen(rd());

            pointLights.reserve(400);
            for (int i = 0; i < 400; ++i)
            {
                std::uniform_real_distribution<> distributionPos(-10.0f, 10.0f);
                std::uniform_real_distribution<> distributionCol(0.0f, 1.0f);
                glm::vec3 pos = glm::vec3(distributionPos(gen), distributionPos(gen), distributionPos(gen));

                glm::vec3 col = glm::vec3(distributionCol(gen), distributionCol(gen), distributionCol(gen));

                std::uniform_real_distribution<> distributionIntensity(0.5f, 2.0f);
                float intensity = static_cast<float>(distributionIntensity(gen));

                std::uniform_real_distribution<> distributionRadius(0.5f, 10.0f);
                float radius = 2.0f;

                std::uniform_real_distribution<> distributionAttenuation(0.3f, 10.0f);
                float lAttenuation = 0.01f;
                float qAttenuation = static_cast<float>(distributionAttenuation(gen));

                pointLights.emplace_back(PointLight{pos, col, radius, intensity, lAttenuation, 0.0f});
            }

            cullDataPc.sWidth = windowProvider->GetWindowSize().x;
            cullDataPc.sHeight = windowProvider->GetWindowSize().y;
            cullDataPc.pointLightsCount = 0;
            cullDataPc.xTileCount = static_cast<uint32_t>((core->swapchainRef->extent.width - 1) / xTileSizePx + 1);
            cullDataPc.yTileCount = static_cast<uint32_t>((core->swapchainRef->extent.height - 1) / yTileSizePx + 1);
            cullDataPc.xTileSizePx = xTileSizePx;
            cullDataPc.yTileSizePx = yTileSizePx;
            cullDataPc.pointLightsCount = (int)pointLights.size();

            lightsMap.reserve(cullDataPc.xTileCount * cullDataPc.yTileCount * zSlicesSize);
            for (int i = 0; i < lightsMap.capacity(); ++i)
            {
                lightsMap.emplace_back(ArrayIndexer{});
            }

            lightsIndices.reserve(lightsMap.size() * pointLights.size());
            for (int i = 0; i < lightsMap.size() * pointLights.size(); ++i)
            {
                lightsIndices.emplace_back(-1);
            }

            //light
            quadVert = Vertex2D::GetQuadVertices();
            quadIndices = Vertex2D::GetQuadIndices();
            cPropsUbo.invProj = glm::inverse(camera.matrices.perspective);
            cPropsUbo.invView = glm::inverse(camera.matrices.view);
            cPropsUbo.pos = camera.position;
            cPropsUbo.zNear = camera.cameraProperties.zNear;
            cPropsUbo.zFar = camera.cameraProperties.zFar;
            
            lightPc.xTileCount = cullDataPc.xTileCount;
            lightPc.yTileCount = cullDataPc.yTileCount;
            lightPc.xTileSizePx = xTileSizePx;
            lightPc.yTileSizePx = yTileSizePx;
             lightPc.zSlices = zSlicesSize;
        }

        void CreateBuffers()
        {
            pointLightsBuff = ResourcesManager::GetInstance()->GetBuffer("pointLightsBuff", vk::BufferUsageFlagBits::eStorageBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                sizeof(PointLight) * pointLights.size(), pointLights.data());
            
            vertexBuffer = ResourcesManager::GetInstance()->GetBuffer("vertexBuffer",vk::BufferUsageFlagBits::eVertexBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                sizeof(M_Vertex3D) * model.vertices.size(), model.vertices.data());
            
            indexBuffer = ResourcesManager::GetInstance()->GetBuffer("indexBuffer",vk::BufferUsageFlagBits::eIndexBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                sizeof(uint32_t) * model.indices.size(), model.indices.data());
            
            lightsMapBuff = ResourcesManager::GetInstance()->GetBuffer("lightsMapBuff",vk::BufferUsageFlagBits::eStorageBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                sizeof(ArrayIndexer) * lightsMap.size(), lightsMap.data());
            
            lightsIndicesBuff =  ResourcesManager::GetInstance()->GetBuffer("lightsIndicesBuff",vk::BufferUsageFlagBits::eStorageBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                sizeof(int32_t) * lightsIndices.size(), lightsIndices.data());
            
            //light
            camPropsBuff =  ResourcesManager::GetInstance()->GetBuffer("camPropsBuff",vk::BufferUsageFlagBits::eUniformBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                sizeof(CPropsUbo), &cPropsUbo);
            
            lVertexBuffer =  ResourcesManager::GetInstance()->GetBuffer("lVertexBuffer",vk::BufferUsageFlagBits::eVertexBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                sizeof(Vertex2D) * quadVert.size(), quadVert.data());
            
            lIndexBuffer =  ResourcesManager::GetInstance()->GetBuffer("lIndexBuffer",vk::BufferUsageFlagBits::eIndexBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                sizeof(uint32_t) * quadIndices.size(), quadIndices.data());

            pointLightsBuff->Map();
            lightsMapBuff->Map();
            lightsIndicesBuff->Map();
            camPropsBuff->Map();
        }

        void CreatePipelines()
        {
            auto& logicalDevice = core->logicalDevice.get();
            gVertShader = std::make_unique<Shader>(logicalDevice,
                                                           "C:\\Users\\carlo\\CLionProjects\\Vulkan_Engine_Template\\src\\Shaders\\spirv\\ClusterRendering\\gBuffer.vert.spv");
            gFragShader = std::make_unique<Shader>(logicalDevice,
                                                           "C:\\Users\\carlo\\CLionProjects\\Vulkan_Engine_Template\\src\\Shaders\\spirv\\ClusterRendering\\gBuffer.frag.spv");

            DescriptorLayoutBuilder builder;

            //Cull pass//

            cullCompShader = std::make_unique<Shader>(logicalDevice,
                                                              "C:\\Users\\carlo\\CLionProjects\\Vulkan_Engine_Template\\src\\Shaders\\spirv\\ClusterRendering\\lightCulling.comp.spv");

            cullCompShader.get()->sParser->GetLayout(builder);
            // computeDescCache->AddShaderInfo(cullCompShader.get()->sParser.get());
            // computeDescCache->BuildDescriptorsCache(descriptorAllocatorRef, vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eFragment);

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

            

            auto* cullRenderNode = renderGraphRef->AddPass(computePassName);
            cullRenderNode->SetCompShader(cullCompShader.get());
            cullRenderNode->SetPipelineLayoutCI(cullLayoutCreateInfo);
            cullRenderNode->BuildRenderGraphNode();

            cullDstSet = descriptorAllocatorRef->Allocate(core->logicalDevice.get(), cullDstLayout.get());
            writerBuilder.AddWriteBuffer(0, pointLightsBuff->descriptor, vk::DescriptorType::eStorageBuffer);
            writerBuilder.AddWriteBuffer(1, lightsMapBuff->descriptor, vk::DescriptorType::eStorageBuffer);
            writerBuilder.AddWriteBuffer(2, lightsIndicesBuff->descriptor, vk::DescriptorType::eStorageBuffer);
            writerBuilder.AddWriteBuffer(3, camPropsBuff->descriptor, vk::DescriptorType::eUniformBuffer);
            writerBuilder.UpdateSet(core->logicalDevice.get(), cullDstSet.get());
            
            gVertShader.get()->sParser->GetLayout(builder);
            gFragShader.get()->sParser->GetLayout(builder);

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

            writerBuilder.AddWriteImage(0, imageShipperCol->imageView.get(),
                                        imageShipperCol->sampler->samplerHandle.get(),
                                        vk::ImageLayout::eShaderReadOnlyOptimal,
                                        vk::DescriptorType::eCombinedImageSampler);
            writerBuilder.AddWriteImage(1, imageShipperNorm->imageView.get(),
                                        imageShipperNorm->sampler->samplerHandle.get(),
                                        vk::ImageLayout::eShaderReadOnlyOptimal,
                                        vk::DescriptorType::eCombinedImageSampler);

            writerBuilder.UpdateSet(core->logicalDevice.get(), gDstSet.get());

            VertexInput vertexInput = M_Vertex3D::GetVertexInput();
            AttachmentInfo colInfo = GetColorAttachmentInfo(
                glm::vec4(0.0f), vk::Format::eR32G32B32A32Sfloat);
            AttachmentInfo depthInfo = GetDepthAttachmentInfo();
            auto renderNode = renderGraphRef->AddPass(gBufferPassName);

            renderNode->SetVertShader(gVertShader.get());
            renderNode->SetFragShader(gFragShader.get());
            renderNode->SetFramebufferSize(windowProvider->GetWindowSize());
            renderNode->SetPipelineLayoutCI(layoutCreateInfo);
            renderNode->SetVertexInput(vertexInput);
            renderNode->AddColorAttachmentOutput("gColor", colInfo);
            renderNode->AddColorAttachmentOutput("gNorm", colInfo);
            renderNode->SetDepthAttachmentOutput("gDepth", depthInfo);
            renderNode->AddColorBlendConfig(BlendConfigs::B_OPAQUE);
            renderNode->AddColorBlendConfig(BlendConfigs::B_OPAQUE);
            renderNode->SetDepthConfig(DepthConfigs::D_ENABLE);
            renderNode->AddSamplerResource("colGSampler", imageShipperCol->imageView.get());
            renderNode->AddSamplerResource("normGSampler", imageShipperNorm->imageView.get());
            renderNode->AddColorImageResource("gColor", colAttachmentView);
            renderNode->AddColorImageResource("gNorm", normAttachmentView);
            renderNode->SetDepthImageResource("gDepth", depthAttachmentView);
            renderNode->BuildRenderGraphNode();

            //light pass//

            lVertShader = std::make_unique<Shader>(logicalDevice,
                                                           "C:\\Users\\carlo\\CLionProjects\\Vulkan_Engine_Template\\src\\Shaders\\spirv\\Common\\Quad.vert.spv");
            lFragShader = std::make_unique<Shader>(logicalDevice,
                                                           "C:\\Users\\carlo\\CLionProjects\\Vulkan_Engine_Template\\src\\Shaders\\spirv\\ClusterRendering\\light.frag.spv");

            lVertShader.get()->sParser->GetLayout(builder);
            lFragShader.get()->sParser->GetLayout(builder);

            lDstLayout = builder.BuildBindings(
                core->logicalDevice.get(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);

            auto lPushConstantRange = vk::PushConstantRange()
                                      .setOffset(0)
                                      .setStageFlags(vk::ShaderStageFlagBits::eFragment)
                                      .setSize(sizeof(LightPc));

            auto lLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
                                     .setPushConstantRanges(lPushConstantRange)
                                     .setSetLayoutCount(1)
                                     .setPSetLayouts(&lDstLayout.get());

            lDstSet = descriptorAllocatorRef->Allocate(core->logicalDevice.get(), lDstLayout.get());

            Sampler* sampler = renderGraphRef->samplerPool.AddSampler(
                logicalDevice, vk::SamplerAddressMode::eRepeat, vk::Filter::eNearest, vk::SamplerMipmapMode::eNearest);
            Sampler* depthSampler = renderGraphRef->samplerPool.AddSampler(
                logicalDevice, vk::SamplerAddressMode::eClampToEdge, vk::Filter::eNearest,
                vk::SamplerMipmapMode::eNearest);

            writerBuilder.AddWriteImage(0, colAttachmentView,
                                        sampler->samplerHandle.get(),
                                        vk::ImageLayout::eShaderReadOnlyOptimal,
                                        vk::DescriptorType::eCombinedImageSampler);
            writerBuilder.AddWriteImage(1, normAttachmentView,
                                        sampler->samplerHandle.get(),
                                        vk::ImageLayout::eShaderReadOnlyOptimal,
                                        vk::DescriptorType::eCombinedImageSampler);
            writerBuilder.AddWriteImage(2, depthAttachmentView,
                                        depthSampler->samplerHandle.get(),
                                        vk::ImageLayout::eShaderReadOnlyOptimal,
                                        vk::DescriptorType::eCombinedImageSampler);
            writerBuilder.AddWriteBuffer(3, camPropsBuff->descriptor, vk::DescriptorType::eUniformBuffer);
            writerBuilder.AddWriteBuffer(4, pointLightsBuff->descriptor, vk::DescriptorType::eStorageBuffer);
            writerBuilder.AddWriteBuffer(5, lightsMapBuff->descriptor, vk::DescriptorType::eStorageBuffer);
            writerBuilder.AddWriteBuffer(6, lightsIndicesBuff->descriptor, vk::DescriptorType::eStorageBuffer);

            writerBuilder.UpdateSet(logicalDevice, lDstSet.get());
            
            AttachmentInfo lColInfo = GetColorAttachmentInfo(
                glm::vec4(0.0f), core->swapchainRef->GetFormat());

            VertexInput lVertexInput = Vertex2D::GetVertexInput();

            auto lRenderNode = renderGraphRef->AddPass(lightPassName);
            lRenderNode->SetVertShader(lVertShader.get());
            lRenderNode->SetFragShader(lFragShader.get());
            lRenderNode->SetFramebufferSize(windowProvider->GetWindowSize());
            lRenderNode->SetPipelineLayoutCI(lLayoutCreateInfo);
            lRenderNode->SetVertexInput(lVertexInput);
            lRenderNode->AddColorAttachmentOutput("lColor", lColInfo);
            lRenderNode->AddColorBlendConfig(BlendConfigs::B_OPAQUE);
            lRenderNode->AddSamplerResource("colGSampler", colAttachmentView);
            lRenderNode->AddSamplerResource("normGSampler", normAttachmentView);
            lRenderNode->AddSamplerResource("depthGSampler", depthAttachmentView);
            lRenderNode->DependsOn(computePassName);
            lRenderNode->BuildRenderGraphNode();
        }

        DescriptorAllocator* descriptorAllocatorRef;
        WindowProvider* windowProvider;
        Core* core;
        RenderGraph* renderGraphRef;

        DescriptorWriterBuilder writerBuilder;

        vk::UniqueDescriptorSetLayout gDstLayout;
        vk::UniqueDescriptorSet gDstSet;

        vk::UniqueDescriptorSetLayout lDstLayout;
        vk::UniqueDescriptorSet lDstSet;

        vk::UniqueDescriptorSetLayout cullDstLayout;
        vk::UniqueDescriptorSet cullDstSet;

        std::unique_ptr<Shader> gVertShader;
        std::unique_ptr<Shader> gFragShader;

        std::unique_ptr<Shader> lVertShader;
        std::unique_ptr<Shader> lFragShader;

        std::unique_ptr<Shader> cullCompShader;

        ImageShipper* imageShipperCol;
        ImageShipper* imageShipperNorm;

        ImageView* colAttachmentView;
        ImageView* normAttachmentView;
        ImageView* depthAttachmentView;

        Buffer* vertexBuffer;
        Buffer* indexBuffer;

        Buffer* lVertexBuffer;
        Buffer* lIndexBuffer;

        Buffer* camPropsBuff;
        Buffer* pointLightsBuff;
        Buffer* lightsMapBuff;
        Buffer* lightsIndicesBuff;


        std::unique_ptr<DescriptorCache> computeDescCache;
        std::unique_ptr<DescriptorCache> gBufferDescCache;
        std::unique_ptr<DescriptorCache> lightDecCache;
        std::string gBufferPassName = "gBuffer";
        std::string computePassName = "cullLight";
        std::string lightPassName = "light";

        //gbuff
        Camera camera = {glm::vec3(5.0f), Camera::CameraMode::E_FREE};
        Model model{};
        ForwardPc pc{};

        //culling
        std::vector<PointLight> pointLights;
        std::vector<ArrayIndexer> lightsMap;
        std::vector<int32_t> lightsIndices;
        ScreenDataPc cullDataPc{};
        uint32_t xTileSizePx = 256;
        uint32_t yTileSizePx = 256;
        uint32_t zSlicesSize = 12;
        uint32_t localSize = 1;


        //light
        std::vector<Vertex2D> quadVert;
        std::vector<uint32_t> quadIndices;
        CPropsUbo cPropsUbo;
        LightPc lightPc{};
    };
}

#endif //CLUSTERRENDERER_HPP
