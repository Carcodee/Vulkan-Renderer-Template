//







// Created by carlo on 2024-12-02.
//










#ifndef FLATRENDERER_HPP
#define FLATRENDERER_HPP

namespace Rendering
{
    using namespace ENGINE;

    class FlatRenderer : public BaseRenderer
    {
    public:
        FlatRenderer(Core* core, WindowProvider* windowProvider,
                     DescriptorAllocator* descriptorAllocator)
        {
            this->core = core;
            this->renderGraph = core->renderGraphRef;
            this->windowProvider = windowProvider;
            this->descriptorAllocator = descriptorAllocator;
            outputCache = std::make_unique<DescriptorCache>(core);
            probesGenCache = std::make_unique<DescriptorCache>(core);
            paintingCache = std::make_unique<DescriptorCache>(core);
            mergeCascadesCache = std::make_unique<DescriptorCache>(core);
            cascadesResultCache = std::make_unique<DescriptorCache>(core);
            CreateResources();
            CreateBuffers();
            CreatePipelines();
        }

        void CreateResources()
        {
            cascadesInfo.cascadeCount = 5;
            cascadesInfo.probeSizePx = 8;
            cascadesInfo.intervalCount = 2;
            cascadesInfo.baseIntervalLength = 12;
            auto imageInfo = Image::CreateInfo2d(windowProvider->GetWindowSize(), 1, 1,
                                                 ENGINE::g_32bFormat,
                                                 vk::ImageUsageFlagBits::eColorAttachment |
                                                 vk::ImageUsageFlagBits::eSampled);
            cascadesAttachmentsImagesViews.reserve(cascadesInfo.cascadeCount);
            for (int i = 0; i < cascadesInfo.cascadeCount; ++i)
            {
                std::string name = "CascadeAttachment_" + std::to_string(i);
                ImageView* imageView = ResourcesManager::GetInstance()->GetImage(name, imageInfo, 0, 0);
                cascadesAttachmentsImagesViews.emplace_back(imageView);
            }

            
            probesGenPc.cascadeIndex = 0;
            probesGenPc.intervalSize = 2;
            probesGenPc.probeSizePx = cascadesInfo.probeSizePx;

            rcPc.probeSizePx = cascadesInfo.probeSizePx;
            rcPc.intervalCount = cascadesInfo.intervalCount;
            rcPc.baseIntervalLength = cascadesInfo.baseIntervalLength;
            
            paintingPc.radius = 5;


            auto storageImageInfo = ENGINE::Image::CreateInfo2d(windowProvider->GetWindowSize(), 1, 1,
                                                         ENGINE::g_32bFormat,
                                                         vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst);
            ImageView* lightLayer = ResourcesManager::GetInstance()->GetImage("PaintingLayer", storageImageInfo, 0, 0);
            ImageView* occluderLayer = ResourcesManager::GetInstance()->GetImage("OccluderLayer", storageImageInfo, 0, 0);
            ImageView* debugLayer = ResourcesManager::GetInstance()->GetImage("DebugRaysLayer", storageImageInfo, 0, 0);
            paintingLayers.push_back(lightLayer);
            paintingLayers.push_back(occluderLayer);
            paintingLayers.push_back(debugLayer);
            
            for (int i = 0; i < cascadesInfo.cascadeCount; ++i)
            {
                std::string name = "radianceStorage_" + std::to_string(i);
                ImageView* imageView = ResourcesManager::GetInstance()->GetImage(name, storageImageInfo, 0, 0);
                radiancesImages.emplace_back(imageView);
            }

            
            std::string resourcesPath = SYSTEMS::OS::GetInstance()->GetEngineResourcesPath();
            std::string assetPath = SYSTEMS::OS::GetInstance()->GetAssetsPath();

            testImage = ResourcesManager::GetInstance()->GetShipper("TestImage", resourcesPath + "\\Images\\VulkanLogo.png", 1, 1,
                                                     ENGINE::g_ShipperFormat, LayoutPatterns::GRAPHICS_READ);

            // testSpriteAnim.LoadAtlas(
            // assetPath + "\\Animations\\SmokeFreePack_v2\\Compressed\\512\\Smoke_4_512-sheet.png",
            // glm::uvec2(512, 512), 7, 7, 10);

            AnimatorInfo animatorInfo = {glm::uvec2(512, 512), 4, 4, -1, -1, -1, true};
            testSpriteAnim = RenderingResManager::GetInstance()->GetAnimator("TestAnim",
                                                                            assetPath +
                                                                            "\\Animations\\FireFreePack\\Compressed\\512\\Fire_1_512-comp_sheet.png",
                                                                            30, animatorInfo);
            
        }

        void CreateBuffers()
        {
            quadVertBufferRef = ResourcesManager::GetInstance()->GetStageBuffer(
                "QuadRcVertices", vk::BufferUsageFlagBits::eVertexBuffer,
                sizeof(Vertex2D) * Vertex2D::GetQuadVertices().size(),
                Vertex2D::GetQuadVertices().data())->deviceBuffer.get();
            quadIndexBufferRef = ResourcesManager::GetInstance()->GetStageBuffer(
                "QuadRcIndices", vk::BufferUsageFlagBits::eIndexBuffer,
                sizeof(uint32_t) * Vertex2D::GetQuadIndices().size(),
                Vertex2D::GetQuadIndices().data())->deviceBuffer.get();
        }

        void CreatePipelines()
        {
            auto logicalDevice = core->logicalDevice.get();
            std::string shaderPath = SYSTEMS::OS::GetInstance()->GetShadersPath();


            paintCompShader = std::make_unique<Shader>(logicalDevice,
                                                       shaderPath + "\\spirv\\Compute\\paintingGen.comp.spv");

            paintingCache->AddShaderInfo(paintCompShader.get()->sParser.get());
            paintingCache->BuildDescriptorsCache(descriptorAllocator,
                                                 vk::ShaderStageFlagBits::eCompute |
                                                 vk::ShaderStageFlagBits::eFragment);

            auto paintingPushConstantRanges = vk::PushConstantRange()
                                              .setOffset(0)
                                              .setStageFlags(
                                                  vk::ShaderStageFlagBits::eCompute)
                                              .setSize(sizeof(PaintingPc));
            auto paintingLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
                                            .setSetLayoutCount(1)
                                            .setPushConstantRanges(paintingPushConstantRanges)
                                            .setPSetLayouts(&paintingCache->dstLayout.get());

            auto* paintingNode = renderGraph->AddPass(paintingPassName);
            paintingNode->SetCompShader(paintCompShader.get());
            paintingNode->SetPipelineLayoutCI(paintingLayoutCreateInfo);
            paintingNode->AddStorageResource("PaintingStorage", paintingLayers[0]);
            paintingNode->AddStorageResource("OcluddersStorage", paintingLayers[1]);
            paintingNode->AddStorageResource("DebugLayer", paintingLayers[2]);
            paintingNode->BuildRenderGraphNode();

            VertexInput vertexInput = Vertex2D::GetVertexInput();
            AttachmentInfo colInfo = GetColorAttachmentInfo(
                glm::vec4(0.0f), vk::Format::eR32G32B32A32Sfloat);


            probesVertShader = std::make_unique<Shader>(logicalDevice,
                                                        shaderPath + "\\spirv\\Common\\Quad.vert.spv");
            probesFragShader = std::make_unique<Shader>(logicalDevice,
                                                        shaderPath + "\\spirv\\FlatRendering\\cascadeGen.frag.spv");
            probesGenCache->AddShaderInfo(probesVertShader->sParser.get());
            probesGenCache->AddShaderInfo(probesFragShader->sParser.get());
            probesGenCache->BuildDescriptorsCache(descriptorAllocator,
                                                  vk::ShaderStageFlagBits::eVertex |
                                                  vk::ShaderStageFlagBits::eFragment);


            auto genPushConstantRange = vk::PushConstantRange()
                                        .setOffset(0)
                                        .setStageFlags(
                                            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
                                        .setSize(sizeof(ProbesGenPc));

            auto genLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
                                       .setSetLayoutCount(1)
                                       .setPushConstantRanges(genPushConstantRange)
                                       .setPSetLayouts(&probesGenCache->dstLayout.get());

            for (int i = 0; i < cascadesInfo.cascadeCount; ++i)
            {
                std::string name = "ProbesGen_" + std::to_string(i);
                probesGenPassNames.push_back(name);
                auto renderNode = renderGraph->AddPass(name);
                renderNode->SetVertShader(probesVertShader.get());
                renderNode->SetFragShader(probesFragShader.get());
                renderNode->SetFramebufferSize(windowProvider->GetWindowSize());
                renderNode->SetPipelineLayoutCI(genLayoutCreateInfo);
                renderNode->SetVertexInput(vertexInput);
                renderNode->AddColorAttachmentOutput("CascadeAttachment_" + std::to_string(i), colInfo);
                renderNode->AddColorBlendConfig(BlendConfigs::B_OPAQUE);
                renderNode->SetRasterizationConfigs(RasterizationConfigs::R_FILL);
                renderNode->AddColorImageResource("CascadeAttachment_" + std::to_string(i),
                                                  cascadesAttachmentsImagesViews[i]);
                renderNode->BuildRenderGraphNode();
            }


            vertShader = std::make_unique<Shader>(logicalDevice,
                                                  shaderPath + "\\spirv\\Common\\Quad.vert.spv");
            fragShader = std::make_unique<Shader>(logicalDevice,
                                                  shaderPath + "\\spirv\\FlatRendering\\rCascadesOutput.frag.spv");
            outputCache->AddShaderInfo(vertShader->sParser.get());
            outputCache->AddShaderInfo(fragShader->sParser.get());
            outputCache->BuildDescriptorsCache(descriptorAllocator,
                                               vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);

            AttachmentInfo outputColInfo = GetColorAttachmentInfo(
                glm::vec4(0.0f), core->swapchainRef->GetFormat());

            auto pushConstantRange = vk::PushConstantRange()
                                     .setOffset(0)
                                     .setStageFlags(
                                         vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
                                     .setSize(sizeof(RcPc));

            auto layoutCreateInfo = vk::PipelineLayoutCreateInfo()
                                    .setSetLayoutCount(1)
                                    .setPushConstantRanges(pushConstantRange)
                                    .setPSetLayouts(&outputCache->dstLayout.get());

            auto renderNode = renderGraph->AddPass(rCascadesPassName);

            renderNode->SetVertShader(vertShader.get());
            renderNode->SetFragShader(fragShader.get());
            renderNode->SetFramebufferSize(windowProvider->GetWindowSize());
            renderNode->SetPipelineLayoutCI(layoutCreateInfo);
            renderNode->SetVertexInput(vertexInput);
            renderNode->AddColorAttachmentOutput("rColor", outputColInfo);
            renderNode->AddColorBlendConfig(BlendConfigs::B_ALPHA_BLEND);
            renderNode->SetRasterizationConfigs(RasterizationConfigs::R_FILL);
            renderNode->BuildRenderGraphNode();
            renderNode->DependsOn(paintingPassName);
            for (int i = 0; i < cascadesInfo.cascadeCount; ++i)
            {
                renderNode->DependsOn(probesGenPassNames[i]);
            }

            AttachmentInfo mergeColInfo = GetColorAttachmentInfo(
                glm::vec4(0.0f), core->swapchainRef->GetFormat(), vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore);

            mergeVertShader = std::make_unique<Shader>(logicalDevice,
                                                  shaderPath + "\\spirv\\Common\\Quad.vert.spv");
            mergeFragShader = std::make_unique<Shader>(logicalDevice,
                                                  shaderPath + "\\spirv\\FlatRendering\\cascadesMerge.frag.spv");
            mergeCascadesCache->AddShaderInfo(mergeVertShader->sParser.get());
            mergeCascadesCache->AddShaderInfo(mergeFragShader->sParser.get());
            mergeCascadesCache->BuildDescriptorsCache(descriptorAllocator,
                                               vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);

            auto mergeLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
                                    .setSetLayoutCount(1)
                                    .setPushConstantRanges(pushConstantRange)
                                    .setPSetLayouts(&mergeCascadesCache->dstLayout.get());

            for (int i = cascadesInfo.cascadeCount - 2; i >= 0 ; i--)
            {
                std::string name =rMergePassName + "_" + std::to_string(i);
                auto mergeRenderNode = renderGraph->AddPass(name);
                            mergeRenderNode->SetVertShader(mergeVertShader.get());
                mergeRenderNode->SetFragShader(mergeFragShader.get());
                mergeRenderNode->SetFramebufferSize(windowProvider->GetWindowSize());
                mergeRenderNode->SetPipelineLayoutCI(mergeLayoutCreateInfo);
                mergeRenderNode->SetVertexInput(vertexInput);
                mergeRenderNode->AddColorAttachmentOutput("mergeColor_"+std::to_string(i), mergeColInfo);
                mergeRenderNode->AddColorBlendConfig(BlendConfigs::B_OPAQUE);
                mergeRenderNode->SetRasterizationConfigs(RasterizationConfigs::R_FILL);
                std::string name1 = "radianceStorage_" + std::to_string(i);
                mergeRenderNode->AddStorageResource(name1, radiancesImages[i]);
                std::string name2 = "radianceStorage_" + std::to_string(i + 1);
                mergeRenderNode->AddStorageResource(name2, radiancesImages[i + 1]);
                
                mergeRenderNode->BuildRenderGraphNode();
                mergeRenderNode->DependsOn(rCascadesPassName);
                if (i < cascadesInfo.cascadeCount - 2)
                {
                    std::string dependancyName = rMergePassName + "_" + std::to_string(i + 1);
                    mergeRenderNode->DependsOn(rCascadesPassName);
                }

            }

            
            resultVertShader = std::make_unique<Shader>(logicalDevice,
                                                  shaderPath + "\\spirv\\Common\\Quad.vert.spv");
            resultFragShader = std::make_unique<Shader>(logicalDevice,
                                                  shaderPath + "\\spirv\\FlatRendering\\cascadesResult.frag.spv");
            cascadesResultCache->AddShaderInfo(resultVertShader->sParser.get());
            cascadesResultCache->AddShaderInfo(resultFragShader->sParser.get());
            cascadesResultCache->BuildDescriptorsCache(descriptorAllocator,
                                               vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);

            auto resultLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
                                    .setSetLayoutCount(1)
                                    .setPushConstantRanges(pushConstantRange)
                                    .setPSetLayouts(&cascadesResultCache->dstLayout.get());

            auto resultNode = renderGraph->AddPass(resultPassName);

            resultNode->SetVertShader(resultVertShader.get());
            resultNode->SetFragShader(resultFragShader.get());
            resultNode->SetFramebufferSize(windowProvider->GetWindowSize());
            resultNode->SetPipelineLayoutCI(resultLayoutCreateInfo);
            resultNode->SetVertexInput(vertexInput);
            resultNode->AddColorAttachmentOutput("resultColor", outputColInfo);
            resultNode->AddColorBlendConfig(BlendConfigs::B_OPAQUE);
            resultNode->SetRasterizationConfigs(RasterizationConfigs::R_FILL);
            resultNode->BuildRenderGraphNode();
            resultNode->DependsOn(rMergePassName);

        }

        void RecreateSwapChainResources() override
        {
        }

        void SetRenderOperation(ENGINE::InFlightQueue* inflightQueue) override
        {
            auto paintingRenderOP = new std::function<void(vk::CommandBuffer& command_buffer)>(
                [this](vk::CommandBuffer& commandBuffer)
                {
                    glm::vec2 mouseInput = glm::vec2(ImGui::GetMousePos().x, ImGui::GetMousePos().y);
                    paintingPc.xMousePos = mouseInput.x;
                    paintingPc.yMousePos = mouseInput.y;
                    if (glfwGetMouseButton(windowProvider->window, GLFW_MOUSE_BUTTON_2))
                    {
                        paintingPc.painting = 1;
                    }else
                    {
                        paintingPc.painting = 0;
                    }
                    
                    paintingCache->SetStorageImageArray("PaintingLayers",paintingLayers);

                    auto& renderNode = renderGraph->renderNodes.at(paintingPassName);
                    commandBuffer.pushConstants(renderNode->pipelineLayout.get(),
                                                vk::ShaderStageFlagBits::eCompute,
                                                0, sizeof(PaintingPc), &paintingPc);
                    commandBuffer.bindDescriptorSets(renderNode->pipelineType,
                                                     renderNode->pipelineLayout.get(), 0,
                                                     1,
                                                     &paintingCache->dstSet.get(), 0, nullptr);
                    commandBuffer.bindPipeline(renderNode->pipelineType, renderNode->pipeline.get());
                    commandBuffer.dispatch(paintingPc.radius, paintingPc.radius, 1);
                    
                });
            renderGraph->GetNode(paintingPassName)->SetRenderOperation(paintingRenderOP);
            
            for (int i = 0; i < cascadesInfo.cascadeCount; ++i)
            {
                auto probesGenOp = new std::function<void(vk::CommandBuffer& command_buffer)>(
                    [this, i](vk::CommandBuffer& commandBuffer)
                    {
                        int idx = i;
                        int intervalSizePc = cascadesInfo.intervalCount;
                        int gridSizePc = cascadesInfo.probeSizePx;
                        for (int j = 0; j < idx; ++j)
                        {
                            intervalSizePc *= 2;
                            gridSizePc *= 2;
                        }
                        probesGenPc.cascadeIndex = idx;
                        probesGenPc.intervalSize = intervalSizePc;
                        probesGenPc.probeSizePx = gridSizePc;
                        auto& renderNode = renderGraph->renderNodes.at(probesGenPassNames[idx]);
                        commandBuffer.bindDescriptorSets(renderNode->pipelineType,
                                                         renderNode->pipelineLayout.get(), 0,
                                                         1,
                                                         &probesGenCache->dstSet.get(), 0, nullptr);

                        commandBuffer.pushConstants(renderNode->pipelineLayout.get(),
                                                    vk::ShaderStageFlagBits::eVertex |
                                                    vk::ShaderStageFlagBits::eFragment,
                                                    0, sizeof(ProbesGenPc), &probesGenPc);
                        commandBuffer.bindPipeline(renderNode->pipelineType, renderNode->pipeline.get());
                        vk::DeviceSize offset = 0;
                        commandBuffer.bindVertexBuffers(0, 1, &quadVertBufferRef->bufferHandle.get(), &offset);
                        commandBuffer.bindIndexBuffer(quadIndexBufferRef->bufferHandle.get(), 0,
                                                      vk::IndexType::eUint32);
                        commandBuffer.drawIndexed(Vertex2D::GetQuadIndices().size(), 1, 0, 0, 0);
                    });
                renderGraph->GetNode(probesGenPassNames[i])->SetRenderOperation(probesGenOp);
            }

            auto radianceOutputTask = new std::function<void()>([this, inflightQueue]()
            {
                rcPc.cascadesCount = cascadesInfo.cascadeCount;
                rcPc.probeSizePx = cascadesInfo.probeSizePx;
                rcPc.intervalCount = cascadesInfo.intervalCount;
                rcPc.baseIntervalLength = cascadesInfo.baseIntervalLength;
                rcPc.fWidth = windowProvider->GetWindowSize().x;
                rcPc.fHeight = windowProvider->GetWindowSize().y;

                auto* currImage = inflightQueue->currentSwapchainImageView;
                renderGraph->AddColorImageResource(rCascadesPassName, "rColor", currImage);
                renderGraph->GetNode(rCascadesPassName)->SetFramebufferSize(windowProvider->GetWindowSize());
            });
            auto radianceOutputOp = new std::function<void(vk::CommandBuffer& command_buffer)>(
                [this](vk::CommandBuffer& commandBuffer)
                {
                    outputCache->SetSamplerArray("Cascades", cascadesAttachmentsImagesViews);
                    outputCache->SetStorageImageArray("PaintingLayers", paintingLayers);
                    outputCache->SetStorageImageArray("Radiances", radiancesImages);
                    outputCache->SetSampler("TestImage", testImage->imageView.get());
                    outputCache->SetSamplerArray("SpriteAnims", testSpriteAnim->imagesFrames);
                    outputCache->SetBuffer("SpriteInfo", testSpriteAnim->animatorInfo);
                    
                    auto& renderNode = renderGraph->renderNodes.at(rCascadesPassName);
                    commandBuffer.bindDescriptorSets(renderNode->pipelineType,
                                                     renderNode->pipelineLayout.get(), 0,
                                                     1,
                                                     &outputCache->dstSet.get(), 0, nullptr);
                    vk::DeviceSize offset = 0;
                    commandBuffer.bindVertexBuffers(0, 1, &quadVertBufferRef->bufferHandle.get(), &offset);
                    commandBuffer.bindIndexBuffer(quadIndexBufferRef->bufferHandle.get(), 0, vk::IndexType::eUint32);

                    commandBuffer.pushConstants(renderNode->pipelineLayout.get(),
                                                vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                                                0, sizeof(RcPc), &rcPc);
                    commandBuffer.bindPipeline(renderNode->pipelineType, renderNode->pipeline.get());

                    commandBuffer.drawIndexed(Vertex2D::GetQuadIndices().size(), 1, 0,
                                              0, 0);
                });
            renderGraph->GetNode(rCascadesPassName)->SetRenderOperation(radianceOutputOp);
            renderGraph->GetNode(rCascadesPassName)->AddTask(radianceOutputTask);

            for (int i = cascadesInfo.cascadeCount - 2; i >= 0 ; i--)
            {
                std::string mergeNameCascades = rMergePassName + "_" + std::to_string(i);
                auto mergeTask = new std::function<void()>([this, inflightQueue, i]()
                {
                    int idx = i;
                    std::string mergeName =rMergePassName + "_" + std::to_string(idx);
                    auto* currImage = inflightQueue->currentSwapchainImageView;
                    renderGraph->GetNode(mergeName)->AddColorImageResource("mergeColor_"+std::to_string(idx), currImage);
                    renderGraph->GetNode(mergeName)->SetFramebufferSize(windowProvider->GetWindowSize());
                });
                auto mergeRenderOp = new std::function<void(vk::CommandBuffer& command_buffer)>(
                    [this, i](vk::CommandBuffer& commandBuffer)
                    {
                        int idx = i;
                        std::string mergeName = rMergePassName + "_" + std::to_string(idx);
                        mergeCascadesCache->SetSamplerArray("Cascades", cascadesAttachmentsImagesViews);
                        mergeCascadesCache->SetStorageImageArray("Radiances", radiancesImages);

                        rcPc.cascadeIndex = idx;

                        auto& renderNode = renderGraph->renderNodes.at(mergeName);
                        commandBuffer.bindDescriptorSets(renderNode->pipelineType,
                                                         renderNode->pipelineLayout.get(), 0,
                                                         1,
                                                         &mergeCascadesCache->dstSet.get(), 0, nullptr);
                        vk::DeviceSize offset = 0;
                        commandBuffer.bindVertexBuffers(0, 1, &quadVertBufferRef->bufferHandle.get(), &offset);
                        commandBuffer.bindIndexBuffer(quadIndexBufferRef->bufferHandle.get(), 0,
                                                      vk::IndexType::eUint32);

                        commandBuffer.pushConstants(renderNode->pipelineLayout.get(),
                                                    vk::ShaderStageFlagBits::eVertex |
                                                    vk::ShaderStageFlagBits::eFragment,
                                                    0, sizeof(RcPc), &rcPc);
                        commandBuffer.bindPipeline(renderNode->pipelineType, renderNode->pipeline.get());

                        commandBuffer.drawIndexed(Vertex2D::GetQuadIndices().size(), 1, 0,
                                                  0, 0);
                    });
                renderGraph->GetNode(mergeNameCascades)->SetRenderOperation(mergeRenderOp);
                renderGraph->GetNode(mergeNameCascades)->AddTask(mergeTask);
            }

            auto resultTask = new std::function<void()>([this, inflightQueue]()
            {
                auto* currImage = inflightQueue->currentSwapchainImageView;
                renderGraph->GetNode(resultPassName)->AddColorImageResource("resultColor",currImage);
                renderGraph->GetNode(resultPassName)->SetFramebufferSize(windowProvider->GetWindowSize());
            });
            auto resultRenderOp = new std::function<void(vk::CommandBuffer& command_buffer)>(
                [this](vk::CommandBuffer& commandBuffer)
                {
                    cascadesResultCache->SetStorageImageArray("PaintingLayers", paintingLayers);
                    cascadesResultCache->SetStorageImageArray("Radiances", radiancesImages);
                    cascadesResultCache->SetSampler("TestImage", testImage->imageView.get());
                    cascadesResultCache->SetSamplerArray("SpriteAnims", testSpriteAnim->imagesFrames);
                    cascadesResultCache->SetBuffer("SpriteInfo", testSpriteAnim->animatorInfo);
                        
                    
                    auto& renderNode = renderGraph->renderNodes.at(resultPassName);
                    commandBuffer.bindDescriptorSets(renderNode->pipelineType,
                                                     renderNode->pipelineLayout.get(), 0,
                                                     1,
                                                     &cascadesResultCache->dstSet.get(), 0, nullptr);
                    vk::DeviceSize offset = 0;
                    commandBuffer.bindVertexBuffers(0, 1, &quadVertBufferRef->bufferHandle.get(), &offset);
                    commandBuffer.bindIndexBuffer(quadIndexBufferRef->bufferHandle.get(), 0, vk::IndexType::eUint32);
                    
                    commandBuffer.pushConstants(renderNode->pipelineLayout.get(),
                                                vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                                                0, sizeof(RcPc), &rcPc);
                    commandBuffer.bindPipeline(renderNode->pipelineType, renderNode->pipeline.get());

                    commandBuffer.drawIndexed(Vertex2D::GetQuadIndices().size(), 1, 0,
                                              0, 0);
                    testSpriteAnim->UseFrame();
                });
            renderGraph->GetNode(resultPassName)->SetRenderOperation(resultRenderOp);
            renderGraph->GetNode(resultPassName)->AddTask(resultTask);
        }

        void ReloadShaders() override
        {
            auto* paintingNode = renderGraph->GetNode(paintingPassName);
            paintingNode->RecreateResources();
            for (int i = 0; i < cascadesInfo.cascadeCount; ++i)
            {
                auto* genNode = renderGraph->GetNode(probesGenPassNames[i]);
                genNode->RecreateResources();
            }
            auto* outputNode = renderGraph->GetNode(rCascadesPassName);
            outputNode->RecreateResources();
            for (int i = cascadesInfo.cascadeCount - 2; i >= 0; i--)
            {
                std::string name = rMergePassName+"_"+ std::to_string(i);
                auto* mergeNode = renderGraph->GetNode(name);
                mergeNode->RecreateResources();
            }
            auto* resultNode = renderGraph->GetNode(resultPassName);
            resultNode->RecreateResources();

        }

        Core* core;
        RenderGraph* renderGraph;
        WindowProvider* windowProvider;
        DescriptorAllocator* descriptorAllocator;

        std::string paintingPassName = "PaintingPass";
        std::string rCascadesPassName = "rCascadesPass";
        std::string rMergePassName = "rMergePass";
        std::string resultPassName = "resultPass";

         std::unique_ptr<DescriptorCache> cascadesResultCache;
        std::unique_ptr<Shader> resultVertShader;
        std::unique_ptr<Shader> resultFragShader;
        
        std::unique_ptr<DescriptorCache> mergeCascadesCache;
        std::unique_ptr<Shader> mergeVertShader;
        std::unique_ptr<Shader> mergeFragShader;
        std::vector<ImageView*> radiancesImages;
        
        std::unique_ptr<DescriptorCache> outputCache;
        std::unique_ptr<Shader> vertShader;
        std::unique_ptr<Shader> fragShader;

        std::vector<std::string> probesGenPassNames;
        std::unique_ptr<DescriptorCache> probesGenCache;
        std::unique_ptr<Shader> probesVertShader;
        std::unique_ptr<Shader> probesFragShader;
        std::vector<ImageView*> cascadesAttachmentsImagesViews;


        std::unique_ptr<DescriptorCache> paintingCache;
        std::unique_ptr<Shader> paintCompShader;
        std::vector<ImageView*> paintingLayers;
        ImageShipper* testImage;


        Buffer* quadVertBufferRef;
        Buffer* quadIndexBufferRef;

        Animator2D* testSpriteAnim;
        PaintingPc paintingPc;
        RcPc rcPc;
        ProbesGenPc probesGenPc;
        CascadesInfo cascadesInfo;
    };
}
#endif //FLATRENDERER_HPP
