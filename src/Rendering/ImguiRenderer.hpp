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

            ImGui_ImplVulkan_Init(&initInfo);

            ImGui_ImplVulkan_CreateFontsTexture();


        }
        void RenderFrame(vk::CommandBuffer commandBuffer, vk::ImageView& imageView)
        {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::ShowDemoWindow();
            DisplayLightOptions();
            
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
        void Destroy()
        {
            ImGui_ImplVulkan_Shutdown();
        }
        void DisplayLightOptions()
        {
            ImGui::Begin("Light Options");
            static float pointLightRadiuses = 1.0f;
            if(ImGui::SliderFloat("Point lights Radiuses", &pointLightRadiuses, 1.0f, 10.0f))
            {
                for (auto& pointLight : clusterRenderer->pointLights)
                {
                    pointLight.radius = pointLightRadiuses;
                    pointLight.CalculateQAttenuationFromRadius();
                }
            }
            static float pointQuadraticAttenuation = 1.0f;
            if (ImGui::SliderFloat("Point lights Attenuation", &pointQuadraticAttenuation, 1.0f, 10.0f))
            {
                for (auto& pointLight : clusterRenderer->pointLights)
                {
                    pointLight.qAttenuation = pointQuadraticAttenuation;
                    pointLight.CalculateRadiusFromAttenuation();
                }
            }
            static float pointLightIntensity = 1.0f;
            if (ImGui::SliderFloat("Point lights Intensity", &pointLightIntensity, 1.0f, 10.0f))
            {
                for (auto& pointLight : clusterRenderer->pointLights)
                {
                    pointLight.intensity = pointLightIntensity;
                }
            }
            ImGui::End();
        }

        ENGINE::DynamicRenderPass dynamicRenderPass;
        WindowProvider* windowProvider;
        ENGINE::DescriptorAllocator descriptorAllocator;
        ENGINE::Core* core;
        ClusterRenderer* clusterRenderer;
    };
}

#endif //IMGUIRENDERER_HPP
