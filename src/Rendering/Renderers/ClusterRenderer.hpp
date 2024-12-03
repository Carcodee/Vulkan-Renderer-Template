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
            lightDecCache = std::make_unique<DescriptorCache>(this->core);
            gBuffDescCache = std::make_unique<DescriptorCache>(this->core);
            cullMeshesCache = std::make_unique<DescriptorCache>(this->core);

            CreateResources();
            CreateBuffers();
            CreatePipelines();
        }

        void RecreateSwapChainResources() override
        {
        }

        void SetRenderOperation(InFlightQueue* inflightQueue) override
        {
            auto meshCullRenderOp = new std::function<void(vk::CommandBuffer& command_buffer)>(
                [this](vk::CommandBuffer& commandBuffer)
                {
                    
                    cullMeshesCache->SetBuffer("IndirectCmds",
                                               RenderingResManager::GetInstance()->indirectDrawBuffer);
                    
                    auto& renderNode = renderGraphRef->renderNodes.at(meshCullPassName);
                    commandBuffer.bindDescriptorSets(renderNode->pipelineType,
                                                     renderNode->pipelineLayout.get(), 0,
                                                     1,
                                                     &cullMeshesCache->dstSet.get(), 0, nullptr);
                    commandBuffer.bindPipeline(renderNode->pipelineType, renderNode->pipeline.get());
                    commandBuffer.dispatch(RenderingResManager::GetInstance()->indirectDrawsCmdInfos.size(), 1, 1);

                    // BufferAccessPattern srcPattern = GetSrcBufferAccessPattern(B_DRAW_INDIRECT);
                    // BufferAccessPattern dstPattern = GetDstBufferAccessPattern(B_DRAW_INDIRECT);
                    // CreateBufferBarrier(srcPattern, dstPattern, commandBuffer,
                                        // RenderingResManager::GetInstance()->indirectDrawBuffer);
                });
            
            renderGraphRef->GetNode(meshCullPassName)->SetRenderOperation(meshCullRenderOp);

            auto cullTask = new std::function<void()>([this, inflightQueue]()
            {
                Debug();
            });

            auto cullRenderOp = new std::function<void(vk::CommandBuffer& command_buffer)>(
                [this](vk::CommandBuffer& commandBuffer)
                {
                    computeDescCache->SetBuffer("PointLights", pointLights);
                    computeDescCache->SetBuffer("LightMap", lightsMap);
                    computeDescCache->SetBuffer("LightIndices", lightsIndices);
                    computeDescCache->SetBuffer("CameraProperties", cPropsUbo);
                    
                    auto& renderNode = renderGraphRef->renderNodes.at(computePassName);
                    commandBuffer.bindDescriptorSets(renderNode->pipelineType,
                                                     renderNode->pipelineLayout.get(), 0,
                                                     1,
                                                     &computeDescCache->dstSet.get(), 0, nullptr);
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
                    std::vector<ImageView*> textures;
                    for (auto& image : ResourcesManager::GetInstance()->imageShippers)
                    {
                        textures.emplace_back(image->imageView.get());
                    }
                    std::vector<MaterialPackedData> materials;
                    for (auto& mat : RenderingResManager::GetInstance()->materialPackedData)
                    {
                        materials.emplace_back(*mat);
                    }
                   
                    gBuffDescCache->SetSamplerArray("textures", textures);
                    gBuffDescCache->SetBuffer("MaterialsPacked", materials);
                    gBuffDescCache->SetBuffer("MeshMaterialsIds", model->materials);
                    gBuffDescCache->SetBuffer("MeshesModelMatrices", model->modelsMat);
                    
                    commandBuffer.bindDescriptorSets(renderGraphRef->GetNode(gBufferPassName)->pipelineType,
                                                     renderGraphRef->GetNode(gBufferPassName)->pipelineLayout.get(), 0,
                                                     1,
                                                     &gBuffDescCache->dstSet.get(), 0, nullptr);

                    commandBuffer.bindVertexBuffers(0, 1, &vertexBuffer->deviceBuffer.get()->bufferHandle.get(), &offset);
                    commandBuffer.bindIndexBuffer(indexBuffer->deviceBuffer.get()->bufferHandle.get(), 0, vk::IndexType::eUint32);

                    pc.projView = camera.matrices.perspective * camera.matrices.view;
                    commandBuffer.pushConstants(renderGraphRef->GetNode(gBufferPassName)->pipelineLayout.get(),
                                                vk::ShaderStageFlagBits::eVertex |
                                                vk::ShaderStageFlagBits::eFragment,
                                                0, sizeof(ForwardPc), &pc);
 
                    int meshOffset = 0;
                    for (int i = 0; i < RenderingResManager::GetInstance()->models.size(); ++i)
                    {
                        Model* modelRef = RenderingResManager::GetInstance()->models[i].get();
                        vk::DeviceSize sizeOffset = (meshOffset) * sizeof(DrawIndirectIndexedCmd);
                        uint32_t stride = sizeof(DrawIndirectIndexedCmd);
                        commandBuffer.drawIndexedIndirect(
                            RenderingResManager::GetInstance()->indirectDrawBuffer->bufferHandle.get(),
                            sizeOffset,
                            modelRef->meshCount,
                            stride);
                        meshOffset += modelRef->meshCount;
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

                    
                    lightDecCache->SetSampler("gCol", colAttachmentView);
                    lightDecCache->SetSampler("gNormals", normAttachmentView);
                    lightDecCache->SetSampler("gDepth", depthAttachmentView);
                    lightDecCache->SetBuffer("CameraProperties", cPropsUbo);
                    lightDecCache->SetBuffer("PointLights", pointLights);
                    lightDecCache->SetBuffer("LightMap", lightsMap);
                    lightDecCache->SetBuffer("LightIndices", lightsIndices);
                    vk::DeviceSize offset = 0;
                    commandBuffer.bindDescriptorSets(renderGraphRef->GetNode(lightPassName)->pipelineType,
                                                     renderGraphRef->GetNode(lightPassName)->pipelineLayout.get(),
                                                     0, 1,
                                                     &lightDecCache->dstSet.get(), 0, nullptr);
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
            lightsIndices.clear();
            lightsIndices.reserve(lightsMap.size() * pointLights.size());
            for (int i = 0; i < lightsMap.size() * pointLights.size(); ++i)
            {
                lightsIndices.emplace_back(-1);
            }

            cPropsUbo.invProj = glm::inverse(camera.matrices.perspective);
            cPropsUbo.invView = glm::inverse(camera.matrices.view);
            cPropsUbo.pos = camera.position;
            cPropsUbo.zNear = camera.cameraProperties.zNear;
            cPropsUbo.zFar = camera.cameraProperties.zFar;

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

            std::string path = SYSTEMS::OS::GetInstance()->GetAssetsPath();
            model = RenderingResManager::GetInstance()->GetModel(path + "\\Models\\tomb\\scene.gltf");

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

            RenderingResManager::GetInstance()->BuildIndirectBuffers();
            
            vertexBuffer = ResourcesManager::GetInstance()->GetStageBuffer("vertexBuffer",vk::BufferUsageFlagBits::eVertexBuffer,
                sizeof(M_Vertex3D) * model->vertices.size(), model->vertices.data());
            
            indexBuffer = ResourcesManager::GetInstance()->GetStageBuffer("indexBuffer", vk::BufferUsageFlagBits::eIndexBuffer,
                sizeof(uint32_t) * model->indices.size(), model->indices.data());
            
            lVertexBuffer =  ResourcesManager::GetInstance()->GetBuffer("lVertexBuffer",vk::BufferUsageFlagBits::eVertexBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                sizeof(Vertex2D) * quadVert.size(), quadVert.data());
            
            lIndexBuffer =  ResourcesManager::GetInstance()->GetBuffer("lIndexBuffer",vk::BufferUsageFlagBits::eIndexBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                sizeof(uint32_t) * quadIndices.size(), quadIndices.data());

        }

        void CreatePipelines()
        {
            auto& logicalDevice = core->logicalDevice.get();

            //Cull meshes
            std::string shaderPath = SYSTEMS::OS::GetInstance()->GetShadersPath();

            cullMeshesCompShader = std::make_unique<Shader>(logicalDevice, shaderPath + "\\spirv\\Common\\meshCull.comp.spv");

            cullMeshesCache->AddShaderInfo(cullMeshesCompShader.get()->sParser.get());
            cullMeshesCache->BuildDescriptorsCache(descriptorAllocatorRef, vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eFragment);

            auto meshCullLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
                                        .setSetLayoutCount(1)
                                        .setPSetLayouts(&cullMeshesCache->dstLayout.get());

            auto* meshCullRenderNode = renderGraphRef->AddPass(meshCullPassName);
            meshCullRenderNode->SetCompShader(cullMeshesCompShader.get());
            meshCullRenderNode->SetPipelineLayoutCI(meshCullLayoutCreateInfo);
            meshCullRenderNode->BuildRenderGraphNode();
            
            //Cull pass//

            cullCompShader = std::make_unique<Shader>(logicalDevice, shaderPath+ "\\spirv\\ClusterRendering\\lightCulling.comp.spv");

            computeDescCache->AddShaderInfo(cullCompShader.get()->sParser.get());
            computeDescCache->BuildDescriptorsCache(descriptorAllocatorRef, vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eFragment);

            auto cullPushConstantRange = vk::PushConstantRange()
                                         .setOffset(0)
                                         .setStageFlags(vk::ShaderStageFlagBits::eCompute)
                                         .setSize(sizeof(ScreenDataPc));

            auto cullLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
                                        .setSetLayoutCount(1)
                                        .setPushConstantRanges(cullPushConstantRange)
                                        .setPSetLayouts(&computeDescCache->dstLayout.get());

            auto* cullRenderNode = renderGraphRef->AddPass(computePassName);
            cullRenderNode->SetCompShader(cullCompShader.get());
            cullRenderNode->SetPipelineLayoutCI(cullLayoutCreateInfo);
            cullRenderNode->BuildRenderGraphNode();


            //gbuffer

            gVertShader = std::make_unique<Shader>(logicalDevice,
                                                           "C:\\Users\\carlo\\CLionProjects\\Vulkan_Engine_Template\\src\\Shaders\\spirv\\ClusterRendering\\gBuffer.vert.spv");
            gFragShader = std::make_unique<Shader>(logicalDevice,
                                                           "C:\\Users\\carlo\\CLionProjects\\Vulkan_Engine_Template\\src\\Shaders\\spirv\\ClusterRendering\\gBuffer.frag.spv");
            gBuffDescCache->AddShaderInfo(gVertShader->sParser.get());
            gBuffDescCache->AddShaderInfo(gFragShader->sParser.get());
            gBuffDescCache->BuildDescriptorsCache(descriptorAllocatorRef, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);

            auto pushConstantRange = vk::PushConstantRange()
                                     .setOffset(0)
                                     .setStageFlags(
                                         vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
                                     .setSize(sizeof(ForwardPc));

            auto layoutCreateInfo = vk::PipelineLayoutCreateInfo()
                                    .setSetLayoutCount(1)
                                    .setPushConstantRanges(pushConstantRange)
                                    .setPSetLayouts(&gBuffDescCache->dstLayout.get());

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
            renderNode->AddColorImageResource("gColor", colAttachmentView);
            renderNode->AddColorImageResource("gNorm", normAttachmentView);
            renderNode->SetDepthImageResource("gDepth", depthAttachmentView);
            renderNode->AddBufferSync("indirectBuffer", {B_COMPUTE_WRITE, B_DRAW_INDIRECT});
            renderNode->DependsOn(meshCullPassName);
            renderNode->BuildRenderGraphNode();

            //light pass//

            lVertShader = std::make_unique<Shader>(logicalDevice, "C:\\Users\\carlo\\CLionProjects\\Vulkan_Engine_Template\\src\\Shaders\\spirv\\Common\\Quad.vert.spv");
            lFragShader = std::make_unique<Shader>(logicalDevice,"C:\\Users\\carlo\\CLionProjects\\Vulkan_Engine_Template\\src\\Shaders\\spirv\\ClusterRendering\\light.frag.spv");

            lightDecCache->AddShaderInfo(lVertShader->sParser.get());
            lightDecCache->AddShaderInfo(lFragShader->sParser.get());
            lightDecCache->BuildDescriptorsCache(descriptorAllocatorRef, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);

            auto lPushConstantRange = vk::PushConstantRange()
                                      .setOffset(0)
                                      .setStageFlags(vk::ShaderStageFlagBits::eFragment)
                                      .setSize(sizeof(LightPc));

            auto lLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
                                     .setPushConstantRanges(lPushConstantRange)
                                     .setSetLayoutCount(1)
                                     .setPSetLayouts(&lightDecCache->dstLayout.get());


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

        std::unique_ptr<Shader> gVertShader;
        std::unique_ptr<Shader> gFragShader;

        std::unique_ptr<Shader> lVertShader;
        std::unique_ptr<Shader> lFragShader;

        std::unique_ptr<Shader> cullCompShader;
        std::unique_ptr<Shader> cullMeshesCompShader;
        
        ImageView* colAttachmentView;
        ImageView* normAttachmentView;
        ImageView* depthAttachmentView;

        StagedBuffer* vertexBuffer;
        StagedBuffer* indexBuffer;

        Buffer* lVertexBuffer;
        Buffer* lIndexBuffer;

        std::unique_ptr<DescriptorCache> computeDescCache;
        std::unique_ptr<DescriptorCache> lightDecCache;
        std::unique_ptr<DescriptorCache> gBuffDescCache;
        std::unique_ptr<DescriptorCache> cullMeshesCache;
        
        std::string gBufferPassName = "gBuffer";
        std::string computePassName = "cullLight";
        std::string lightPassName = "light";
        std::string meshCullPassName = "cullMesh";

        //gbuff
        Camera camera = {glm::vec3(5.0f), Camera::CameraMode::E_FREE};
        Model* model{};
        ForwardPc pc{};

        //culling
        std::vector<PointLight> pointLights;
        std::vector<ArrayIndexer> lightsMap;
        std::vector<int32_t> lightsIndices;
        ScreenDataPc cullDataPc{};
        uint32_t xTileSizePx = 512;
        uint32_t yTileSizePx = 512;
        uint32_t zSlicesSize = 1;
        uint32_t localSize = 1;


        //light
        std::vector<Vertex2D> quadVert;
        std::vector<uint32_t> quadIndices;
        CPropsUbo cPropsUbo;
        LightPc lightPc{};
    };
}

#endif //CLUSTERRENDERER_HPP
