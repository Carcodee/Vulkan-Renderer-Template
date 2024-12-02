﻿//


// Created by carlo on 2024-09-21.
//






double deltaTime;
double previousTime;



#include "WindowAPI/WindowInclude.hpp"
#include "Systems/SystemsInclude.hpp"
#include "Engine/EngineInclude.hpp"


#include "Rendering/RenderingInclude.hpp"

CONST int WINDOWS_WIDTH = 1920;
CONST int WINDOWS_HEIGHT = 1080;

#define ENGINE_ENABLE_DEBUGGING


void run(WindowProvider* windowProvider)
{
    int imageCount = 3;
    
    ENGINE::WindowDesc windowDesc = {};
    windowDesc.hInstance = GetModuleHandle(NULL);
    windowDesc.hWnd = glfwGetWin32Window(windowProvider->window);

    bool enableDebugging = false;
#if defined ENGINE_ENABLE_DEBUGGING
    enableDebugging = true;
#endif

    const char* glfwExtensions[] = {"VK_KHR_surface", "VK_KHR_win32_surface"};
    uint32_t glfwExtensionCount = sizeof(glfwExtensions) / sizeof(glfwExtensions[0]);

    std::unique_ptr<ENGINE::Core> core = std::make_unique<ENGINE::Core>(
        glfwExtensions, glfwExtensionCount, &windowDesc, enableDebugging);
 
    
    std::unique_ptr<ENGINE::RenderGraph> renderGraph = core->CreateRenderGraph();
    renderGraph->samplerPool.AddSampler(core->logicalDevice.get(), vk::SamplerAddressMode::eRepeat,
                                                vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear);
    
    std::unique_ptr<ENGINE::InFlightQueue> inFlightQueue = std::make_unique<ENGINE::InFlightQueue>(
        core.get(),renderGraph.get(), windowDesc, imageCount, vk::PresentModeKHR::eMailbox,
        windowProvider->GetWindowSize());
    
    std::unique_ptr<ENGINE::DescriptorAllocator> descriptorAllocator = std::make_unique<ENGINE::DescriptorAllocator>();


    ENGINE::ResourcesManager* resourcesManager = ENGINE::ResourcesManager::GetInstance(core.get());

    Rendering::RenderingResManager* renderingResManager = Rendering::RenderingResManager::GetInstance();
    // Rendering::ModelLoader::GetInstance(core.get());
    

    std::vector<ENGINE::DescriptorAllocator::PoolSizeRatio> poolSizeRatios ={
        {vk::DescriptorType::eSampler, 1.5f},
        {vk::DescriptorType::eStorageBuffer, 1.5f},
        {vk::DescriptorType::eUniformBuffer, 1.5f},
        {vk::DescriptorType::eStorageImage, 1.5f},
    };
    
    descriptorAllocator->BeginPool(core->logicalDevice.get(), 10, poolSizeRatios);

    std::unique_ptr<Rendering::ClusterRenderer> clusterRenderer = std::make_unique<Rendering::ClusterRenderer>(
        core.get(), windowProvider, descriptorAllocator.get());
    clusterRenderer->SetRenderOperation(inFlightQueue.get());
    
    std::unique_ptr<Rendering::ImguiRenderer> imguiRenderer = std::make_unique<Rendering::ImguiRenderer>(
        core.get(), windowProvider, clusterRenderer.get());


    while (!windowProvider->WindowShouldClose())
    {
        //handle time and frames better
        float time = windowProvider->GetTime();
        deltaTime = time - previousTime;
        previousTime = time;
        auto profiler =ENGINE::Profiler::GetInstance();
        profiler->StartProfiler();
        
        windowProvider->PollEvents();
        {
            glm::uvec2 windowSize = windowProvider->GetWindowSize();
            if (core->resizeRequested || windowProvider->framebufferResized)
            {
                std::cout << "recreated swapchain\n";
                core->WaitIdle();
                inFlightQueue.reset();
                inFlightQueue = std::make_unique<ENGINE::InFlightQueue>(
                    core.get(), renderGraph.get(),windowDesc, imageCount, vk::PresentModeKHR::eMailbox,
                    windowSize);
                windowProvider->framebufferResized = false;
                core->resizeRequested = false;
                renderGraph->RecreateFrameResources();
                clusterRenderer->SetRenderOperation(inFlightQueue.get());
            }
            try
            {
                profiler->AddProfilerCpuSpot(legit::Colors::sunFlower,"Cpu");
                if (glfwGetKey(windowProvider->window, GLFW_KEY_R))
                {
                    renderGraph->RecompileShaders();
                    clusterRenderer->ReloadShaders(); 
                }

                renderingResManager->UpdateResources();
                resourcesManager->UpdateBuffers();
                resourcesManager->UpdateImages();
                
                inFlightQueue->BeginFrame();

                auto& currFrame = inFlightQueue->frameResources[inFlightQueue->frameIndex];

                profiler->AddProfilerGpuSpot(legit::Colors::emerald,"RenderGraph");
                core->renderGraphRef->ExecuteAll(&currFrame);
                profiler->EndProfilerGpuSpot("RenderGraph");
              
                profiler->EndProfilerCpuSpot("Cpu");
                glm::vec2 input = glm::vec2(0.0f);
                if (glfwGetKey(windowProvider->window, GLFW_KEY_W)) { input += glm::vec2(0.0f, 1.0f); }
                if (glfwGetKey(windowProvider->window, GLFW_KEY_S)) { input += glm::vec2(0.0f, -1.0f); }
                if (glfwGetKey(windowProvider->window, GLFW_KEY_D)) { input += glm::vec2(1.0f, 0.0f); }
                if (glfwGetKey(windowProvider->window, GLFW_KEY_A)) { input += glm::vec2(-1.0f, 0.0f); }
                input =glm::clamp(input, glm::vec2(-1.0, -1.0), glm::vec2(1.0, 1.0));
                glm::vec2 mouseInput = glm::vec2(-ImGui::GetMousePos().x, ImGui::GetMousePos().y);
                clusterRenderer->camera.mouseInput = mouseInput;
                if (glfwGetMouseButton(windowProvider->window, GLFW_MOUSE_BUTTON_2))
                {
                    clusterRenderer->camera.RotateCamera();
                    clusterRenderer->camera.Move(deltaTime, input);
                }else
                {
                    clusterRenderer->camera.firstMouse = true;
                }

                profiler->AddProfilerCpuSpot(legit::Colors::alizarin,"Imgui");
                imguiRenderer->RenderFrame(currFrame.commandBuffer.get(),
                                           inFlightQueue->currentSwapchainImageView->imageView.get());

                profiler->EndProfilerCpuSpot("Imgui");
                profiler->AddProfilerGpuSpot(legit::Colors::carrot,"Gpu");
                inFlightQueue->EndFrame();
                profiler->EndProfilerGpuSpot("Gpu");

            }
            catch (vk::OutOfDateKHRError err)
            {
                core->resizeRequested = true;
            }
        }

        profiler->AddProfilerGpuSpot(legit::Colors::amethyst, "WaitIdle");
        core->WaitIdle();
        profiler->EndProfilerGpuSpot("WaitIdle");
        profiler->UpdateProfiler();
        
    }
    ENGINE::ResourcesManager::GetInstance()->DestroyResources();
    imguiRenderer->Destroy();
    windowProvider->DestroyWindow();
}

int main()
{
    std::unique_ptr<WindowProvider> windowProvider = std::make_unique<WindowProvider>(WINDOWS_WIDTH, WINDOWS_HEIGHT, "Vulkan Engine Template");
    windowProvider->InitGlfw();
    
    run(windowProvider.get());

    windowProvider->Terminate();

    return 0;
}
