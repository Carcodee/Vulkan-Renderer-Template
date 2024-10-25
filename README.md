# CodeVkRenderer: A Vulkan-Based RenderGraph Renderer

CodeVk_Renderer is a lightweight Vulkan-based rendering engine utilizing a RenderGraph architecture. It aims to simplify the development of graphics applications by providing an easy-to-use interface for rapid prototyping while maintaining the flexibility and control of Vulkan's explicit API.

## Features

- **RenderGraph-Based Architecture**: Flexible setup of rendering pipelines designed to facilitate rapid prototyping. This system is functional but still undergoing optimization for improved performance.

- **Modular Vulkan Interface**: Built with modularity in mind, allowing rapid iteration and customization.

- **GLTF Loader**: Import and render GLTF models with ease, streamlining 3D asset integration.

- **Simple First-Person Camera**: Includes a basic first-person camera controller.

- **Examples**:
  - [**Forward Renderer**](https://github.com/Carcodee/CodeVk_Renderer/blob/main/src/Rendering/Examples/ForwardRenderer.hpp): A straightforward implementation of a forward rendering pipeline.
  - [**Compute Renderer**](https://github.com/Carcodee/CodeVk_Renderer/blob/main/src/Rendering/Examples/ComputeRenderer.hpp): Demonstrates compute shader usage with the RenderGraph.

- **Imgui Integration**.

## Getting Started

### Project Structure

The project is organized into two primary branches: **Main** and **Personal**.

- **Main**: This branch contains a streamlined version of the renderer, stripped of any personal additions. It serves as a lightweight, clean base version of the project.
  
- **Personal**: This branch builds upon the template found in the Main branch, incorporating all of my personal features and customizations.

### Prerequisites

- **C++20 Compiler**: A modern C++ compiler supporting C++20 features is required.
- **CMake**: Version 3.26 or later.
- **Vulkan SDK**: Ensure you have the latest version of the Vulkan SDK installed.

### Building the Project

1. Clone the repository.
   ```sh
   git clone https://github.com/Carcodee/CodeVk_Renderer.git
   ```
2. Create a build directory and run CMake.
   ```sh
   mkdir build && cd build
   cmake ..
   ```
3. Compile the project.
   ```sh
   make
   ```

## Example Usage: Setting Up a Forward Renderer

Below is an example of setting up a forward rendering pipeline using CodeVk_Renderer.

### Shader Setup

To begin, create shader modules for the vertex and fragment shaders:

```cpp
std::vector<uint32_t> vertCode = ENGINE::GetByteCode("path/to/vertex_shader.vert.spv");
std::vector<uint32_t> fragCode = ENGINE::GetByteCode("path/to/fragment_shader.frag.spv");

ENGINE::ShaderParser vertParser(vertCode);
ENGINE::ShaderParser fragParser(fragCode);

ENGINE::ShaderModule vertShaderModule(logicalDevice, vertCode);
ENGINE::ShaderModule fragShaderModule(logicalDevice, fragCode);
```

### Descriptor Setup

Automatically generate descriptor set layouts based on the parsed shaders:

```cpp
ENGINE::DescriptorLayoutBuilder builder;
ENGINE::ShaderParser::GetLayout(vertParser, builder);
ENGINE::ShaderParser::GetLayout(fragParser, builder);

vk::DescriptorSetLayout dstLayout = builder.BuildBindings(
    core->logicalDevice.get(),
    vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment
);
```

### RenderGraph Node Setup

Next, create a render pass node for the forward rendering pass:

```cpp
forwardPassName = "ForwardPass";
auto renderNode = renderGraphRef->AddPass(forwardPassName);

renderNode->SetVertModule(&vertShaderModule);
renderNode->SetFragModule(&fragShaderModule);
renderNode->SetFramebufferSize(windowProvider->GetWindowSize());
renderNode->SetPipelineLayoutCI(layoutCreateInfo);
renderNode->SetVertexInput(ENGINE::VertexInput::GetVertexInput());
renderNode->AddColorAttachmentOutput("color", ENGINE::GetColorAttachmentInfo());
renderNode->SetDepthAttachmentOutput("depth", ENGINE::GetDepthAttachmentInfo());
renderNode->AddColorBlendConfig(ENGINE::BlendConfigs::B_OPAQUE);
renderNode->SetDepthConfig(ENGINE::DepthConfigs::D_ENABLE);
renderNode->BuildRenderGraphNode();
```

### Frame Tasks and Render Operation

Define the tasks that must be executed before the render operation. The RenderGraph manages image transitions and pipeline barriers automatically.

```cpp
void SetRenderOperation(ENGINE::InFlightQueue* inflightQueue) override {
    auto setViewTask = new std::function<void()>([this, inflightQueue]() {
        auto* currImage = inflightQueue->currentSwapchainImageView;
        auto* currDepthImage = core->swapchainRef->depthImagesFull.at(inflightQueue->frameIndex).imageView.get();
        renderGraphRef->AddColorImageResource("ForwardPass", "color", currImage);
        renderGraphRef->AddDepthImageResource("ForwardPass", "depth", currDepthImage);
        renderGraphRef->GetNode("ForwardPass")->SetFramebufferSize(windowProvider->GetWindowSize());
    });

    auto renderOp = new std::function<void(vk::CommandBuffer&)>([this](vk::CommandBuffer& commandBuffer) {
        vk::DeviceSize offset = 0;
        commandBuffer.bindDescriptorSets(
            renderGraphRef->GetNode(forwardPassName)->pipelineType,
            renderGraphRef->GetNode(forwardPassName)->pipelineLayout.get(),
            0, 1, &dstSet.get(), 0, nullptr
        );

        commandBuffer.bindVertexBuffers(0, 1, &vertexBuffer->bufferHandle.get(), &offset);
        commandBuffer.bindIndexBuffer(indexBuffer->bufferHandle.get(), 0, vk::IndexType::eUint32);

        for (int i = 0; i < model.meshCount; ++i) {
            camera.SetPerspective(45.0f, (float)windowProvider->GetWindowSize().x / (float)windowProvider->GetWindowSize().y, 0.1f, 512.0f);
            pc.projView = camera.matrices.perspective * camera.matrices.view;
            pc.model = model.modelsMat[i];

            commandBuffer.pushConstants(
                renderGraphRef->GetNode(forwardPassName)->pipelineLayout.get(),
                vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                0, sizeof(ForwardPc), &pc
            );

            commandBuffer.drawIndexed(
                model.indicesCount[i], 1, model.firstIndices[i],
                static_cast<int32_t>(model.firstVertices[i]), 0
            );
        }
    });

    renderGraphRef->GetNode(forwardPassName)->AddTask(setViewTask);
    renderGraphRef->GetNode(forwardPassName)->SetRenderOperation(renderOp);
}
```

## Current Status

CodeVk_Renderer is actively under development. More features, optimizations, and examples will be added in future updates.

## Contributions

Contributions are welcome! If you'd like to contribute, please feel free to open an issue or submit a pull request. Suggestions for features, optimizations, and code improvements are highly appreciated.

## Future Improvements

- **Extended Documentation**: More detailed usage guides and API documentation are in the pipeline.
- **Stability Improvements**: Ongoing efforts to refactor and optimize for robust performance.
- **Feature Expansion**: Addition of new rendering paths, advanced shading techniques, and improved debugging tools to enhance development capabilities.


