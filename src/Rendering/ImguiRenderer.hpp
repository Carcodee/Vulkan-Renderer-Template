//




// Created by carlo on 2024-10-10.
//






#ifndef IMGUIRENDERER_HPP
#define IMGUIRENDERER_HPP

namespace Rendering
{
    class ImguiRenderer
    {
    public:
        ImguiRenderer(ENGINE::Core* core, WindowProvider* windowProvider, ClusterRenderer* clusterRenderer)
        {
            this->clusterRenderer = clusterRenderer;
            this->core =core;
            this->windowProvider= windowProvider;

            std::vector<ENGINE::DescriptorAllocator::PoolSizeRatio> poolSizeRatios = {
                {vk::DescriptorType::eSampler, 1},
                {vk::DescriptorType::eCombinedImageSampler, 1},
                {vk::DescriptorType::eSampledImage, 1},
                {vk::DescriptorType::eStorageImage, 1},
                {vk::DescriptorType::eUniformTexelBuffer, 1},
                {vk::DescriptorType::eStorageTexelBuffer, 1},
                {vk::DescriptorType::eUniformBuffer, 1},
                {vk::DescriptorType::eStorageBuffer, 1},
                {vk::DescriptorType::eUniformBufferDynamic, 1},
                {vk::DescriptorType::eStorageBufferDynamic, 1},
                {vk::DescriptorType::eInputAttachment, 1}
            };
            descriptorAllocator.BeginPool(core->logicalDevice.get(), 1000, poolSizeRatios);
            
            ImGui::CreateContext();

            ImGui_ImplGlfw_InitForVulkan(windowProvider->window, true);

            ImGui_ImplVulkan_InitInfo initInfo = {};
            initInfo.Instance = core->instance.get();
            initInfo.PhysicalDevice = core->physicalDevice;
            initInfo.Device = core->logicalDevice.get();
            initInfo.Queue = core->graphicsQueue;
            initInfo.DescriptorPool = descriptorAllocator.pool.get();
            initInfo.MinImageCount = 3;
            initInfo.ImageCount = 3;
            initInfo.UseDynamicRendering = true;

            initInfo.PipelineRenderingCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
            initInfo.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
            VkFormat swapchainFormat =static_cast<VkFormat>(core->swapchainRef->GetFormat());
            initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchainFormat;

            initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

            SetStyle();
        	
            ImGui_ImplVulkan_Init(&initInfo);

            ImGui_ImplVulkan_CreateFontsTexture();


        }
        void RenderFrame(vk::CommandBuffer commandBuffer, vk::ImageView& imageView)
        {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::ShowDemoWindow();
            ClusterRendererInfo();
            RenderGraphProfiler();
            
            ImGui::Render();
            ENGINE::AttachmentInfo attachmentInfo = ENGINE::GetColorAttachmentInfo(glm::vec4(0.0f),core->swapchainRef->GetFormat(), vk::AttachmentLoadOp::eLoad);
            attachmentInfo.attachmentInfo.setImageView(imageView);
            
            std::vector<vk::RenderingAttachmentInfo> attachmentInfos = {attachmentInfo.attachmentInfo};
            vk::RenderingAttachmentInfo depthAttachment;
            
            dynamicRenderPass.SetRenderInfoUnsafe(attachmentInfos, windowProvider->GetWindowSize(), &depthAttachment);

            commandBuffer.beginRendering(dynamicRenderPass.renderInfo);
            
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

            
            commandBuffer.endRendering();
        }
        void RenderGraphProfiler()
        {
	        profilersWindow.cpuGraph.LoadFrameData(Profiler::GetInstance()->cpuTasks.data(),
	                                               Profiler::GetInstance()->cpuTasks.size());
	        profilersWindow.gpuGraph.LoadFrameData(Profiler::GetInstance()->gpuTasks.data(),
	                                               Profiler::GetInstance()->gpuTasks.size());
	        profilersWindow.Render();
        }

        void SetStyle()
        {

        	std::string fontPath = SYSTEMS::OS::GetInstance()->GetEngineResourcesPath() + "\\Fonts";

        	std::string lightOpenSans = SYSTEMS::OS::GetInstance()->GetEngineResourcesPath() + "\\Fonts\\Open_Sans\\static\\OpenSans-Light.ttf";
        	std::string regularOpenSans = SYSTEMS::OS::GetInstance()->GetEngineResourcesPath() + "\\Fonts\\Open_Sans\\static\\OpenSans-Regular.ttf";
        	std::string boldOpenSans = SYSTEMS::OS::GetInstance()->GetEngineResourcesPath() + "\\Fonts\\Open_Sans\\static\\OpenSans-Bold.ttf";
        	ImGuiIO& io = ImGui::GetIO();

        	io.Fonts->Clear();
        	io.Fonts->AddFontFromFileTTF(lightOpenSans.c_str(),  16);
        	io.Fonts->AddFontFromFileTTF(regularOpenSans.c_str(), 16);
        	io.Fonts->AddFontFromFileTTF(lightOpenSans.c_str(), 32);
        	io.Fonts->AddFontFromFileTTF(regularOpenSans.c_str(), 11);
        	io.Fonts->AddFontFromFileTTF(boldOpenSans.c_str(), 11);
        	io.Fonts->Build();
        	
	        ImGuiStyle* style = &ImGui::GetStyle();

	        style->WindowPadding = ImVec2(15, 15);
	        style->WindowRounding = 5.0f;
	        style->FramePadding = ImVec2(5, 5);
	        style->FrameRounding = 4.0f;
	        style->ItemSpacing = ImVec2(12, 8);
	        style->ItemInnerSpacing = ImVec2(8, 6);
	        style->IndentSpacing = 25.0f;
	        style->ScrollbarSize = 15.0f;
	        style->ScrollbarRounding = 9.0f;
	        style->GrabMinSize = 5.0f;
	        style->GrabRounding = 3.0f;

	        style->Colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.83f, 1.00f);
	        style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
	        style->Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	        style->Colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
	        style->Colors[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.83f, 0.88f);
	        style->Colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
	        style->Colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	        style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
	        style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	        style->Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	        style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.98f, 0.95f, 0.75f);
	        style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
	        style->Colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	        style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	        style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
	        style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	        style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	        style->Colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
	        style->Colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
	        style->Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	        style->Colors[ImGuiCol_Button] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	        style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
	        style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	        style->Colors[ImGuiCol_Header] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	        style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	        style->Colors[ImGuiCol_HeaderActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	        style->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	        style->Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	        style->Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	        style->Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
	        style->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
	        style->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
	        style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
	        style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
        }
        
        void ClusterRendererInfo()
        {
            ImGui::Begin("Debug Info");

            // ImGui::Text("Gpu frame ms: %f.3f ms", gpuMs);
            // ImGui::Text("Cpu Frame ms: %f.3f ms", cpuMs);

            
            ImGui::SeparatorText("Light Info");
            
            float speed = 0.1f;
            for (auto& pointLight : clusterRenderer->pointLights)
            {
                if (pointLight.pos.y >= 20.0f)
                {
                    pointLight.pos.y = 0;
                }
                pointLight.pos+= glm::vec3(0.0f, 1.0f, 0.0f) * speed;
                pointLight.CalculateQAttenuationFromRadius();
            }
            static float pointLightRadiuses = 1.0f;
            if(ImGui::SliderFloat("Point lights Radiuses", &pointLightRadiuses, 1.0f, 20.0f))
            {
                for (auto& pointLight : clusterRenderer->pointLights)
                {
                    pointLight.radius = pointLightRadiuses;
                    pointLight.CalculateQAttenuationFromRadius();
                }
            }
            static float pointQuadraticAttenuation = 1.0f;
            if (ImGui::SliderFloat("Point lights Quadratic Attenuation", &pointQuadraticAttenuation, 1.0f, 30.0f))
            {
                for (auto& pointLight : clusterRenderer->pointLights)
                {
                    pointLight.qAttenuation = pointQuadraticAttenuation;
                    pointLight.CalculateRadiusFromParams();
                    pointLightRadiuses = pointLight.radius;
                }
            }
            static float pointLightLinearAttenuation = 1.0f;
            if (ImGui::SliderFloat("Point lights Linear Attenuation", &pointLightLinearAttenuation, 1.0f, 30.0f))
            {
                for (auto& pointLight : clusterRenderer->pointLights)
                {
                    pointLight.lAttenuation = pointLightLinearAttenuation;
                    pointLight.CalculateRadiusFromParams();
                    pointLightRadiuses = pointLight.radius;
                }
            }
            static float pointLightIntensity = 1.0f;
            if (ImGui::SliderFloat("Point lights Intensity", &pointLightIntensity, 0.0f, 30.0f))
            {
                for (auto& pointLight : clusterRenderer->pointLights)
                {
                    pointLight.intensity = pointLightIntensity;
                    pointLight.CalculateRadiusFromParams();
                    pointLightRadiuses = pointLight.radius;
                }
            }

            ImGui::SeparatorText("Tile/Cluster renderer");

            static int xTileSizePx = 256;
            static int yTileSizePx = 256;
            static int zSlicesSize = 24;

            if (ImGui::SliderInt("x tile size (px): ", &xTileSizePx, 32, 512))
            {
                clusterRenderer->xTileSizePx = xTileSizePx;
            }
            if (ImGui::SliderInt("y tile size (px): ", &yTileSizePx, 32, 512))
            {
                clusterRenderer->yTileSizePx = yTileSizePx;
            }
            if (ImGui::SliderInt("number of z slices: ", &zSlicesSize, 1, 28))
            {
                clusterRenderer->zSlicesSize = zSlicesSize;
            }


            ImGui::SeparatorText("Camera Info");
            
            ImGui::LabelText(":X", "%f.3f", clusterRenderer->camera.position.x);
            ImGui::LabelText(":Y", "%f.3f", clusterRenderer->camera.position.y);
            ImGui::LabelText(":Z", "%f.3f", clusterRenderer->camera.position.z);
            
            ImGui::LabelText("Forward :X", "%f.3f", clusterRenderer->camera.forward.x);
            ImGui::LabelText("Forward :Y", "%f.3f", clusterRenderer->camera.forward.y);
            ImGui::LabelText("Forward :Z", "%f.3f", clusterRenderer->camera.forward.z);
            
            ImGui::End();
            
        }

        void Destroy()
        {
            ImGui_ImplVulkan_Shutdown();
        }
        DynamicRenderPass dynamicRenderPass;
        WindowProvider* windowProvider;
        DescriptorAllocator descriptorAllocator;
        Core* core;
        ClusterRenderer* clusterRenderer;
        ImGuiUtils::ProfilersWindow profilersWindow{};
    };
}

#endif //IMGUIRENDERER_HPP
