//
// Created by carlo on 2024-10-09.
//

#ifndef SHADERPARSER_HPP
#define SHADERPARSER_HPP

namespace ENGINE
{
    class ShaderParser
    {

    public:
        enum ShaderStage
        {
            S_VERT,
            S_FRAG,
            S_COMP,
            S_UNKNOWN
        };
        struct ShaderResource
        {
            std::string name;
            uint32_t binding;
            uint32_t set;
            bool array = false;
            vk::DescriptorType type;
        };

        ShaderParser(std::vector<uint32_t>& byteCode)
        {
            spirv_cross::CompilerGLSL glsl((byteCode));

            spirv_cross::ShaderResources resources = glsl.get_shader_resources();

            auto entryPoint = glsl.get_entry_points_and_stages();
            for (auto& entry : entryPoint)
            {
                spv::ExecutionModel model = entry.execution_model;

                switch (model)
                {
                case spv::ExecutionModelVertex:
                    stage = S_VERT;
                    break;
                case spv::ExecutionModelFragment:
                    stage = S_FRAG;
                    break;
                case spv::ExecutionModelGLCompute:
                    stage = S_COMP;
                    break;
                default:
                    std::cout << "Unknown shader stage\n";
                    stage = S_UNKNOWN;
                    break;
                }
            }


            for (auto& resource : resources.uniform_buffers)
            {
                std::string name = resource.name;
                uint32_t binding = glsl.get_decoration(resource.id, spv::DecorationBinding);
                uint32_t set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
                bool array = false;
                spirv_cross::SPIRType type = glsl.get_type(resource.type_id);
                if (!type.array.empty())
                {
                    array = true;
                }
                uniformBuffers.emplace_back(ShaderResource{name, binding, set, array, vk::DescriptorType::eUniformBuffer});


            }
             for (auto& resource : resources.storage_buffers)
            {
                
                std::string name = resource.name;
                uint32_t binding = glsl.get_decoration(resource.id, spv::DecorationBinding);
                uint32_t set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
                bool array = false;
                spirv_cross::SPIRType type = glsl.get_type(resource.type_id);
                if (!type.array.empty())
                {
                    array = true;
                }
                storageBuffers.emplace_back(ShaderResource{name, binding, set, array, vk::DescriptorType::eStorageBuffer});
            }
            for (auto& resource : resources.sampled_images)
            {

                std::string name = resource.name;
                uint32_t binding = glsl.get_decoration(resource.id, spv::DecorationBinding);
                uint32_t set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
                bool array = false;
                spirv_cross::SPIRType type = glsl.get_type(resource.type_id);
                if (!type.array.empty())
                {
                    array = true;
                }
                
                sampledImages.emplace_back(ShaderResource{name, binding, set, array, vk::DescriptorType::eCombinedImageSampler});
            }
            
            for (auto& resource : resources.storage_images)
            {
                std::string name = resource.name;
                uint32_t binding = glsl.get_decoration(resource.id, spv::DecorationBinding);
                uint32_t set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
                bool array = false;
                spirv_cross::SPIRType type = glsl.get_type(resource.type_id);
                if (!type.array.empty())
                {
                    array = true;
                }
                
                storageImages.emplace_back(ShaderResource{name, binding, set, array, vk::DescriptorType::eStorageImage});
            }
            
        }

        static void GetBinding(std::vector<ShaderResource>& resources, DescriptorLayoutBuilder& builder, std::set <uint32_t>& bindings)
        {
            for (auto& resource : resources)
            {
                if (bindings.contains(resource.binding))
                {
                    continue;
                }
                builder.AddBinding(resource.binding, resource.type);
                bindings.insert(resource.binding);
            }
            
        }
        static void GetLayout(ShaderParser& parser, DescriptorLayoutBuilder& builder)
        {
            std::set<uint32_t> repeatedBindings;
            GetBinding(parser.sampledImages, builder, repeatedBindings);
            GetBinding(parser.storageImages, builder, repeatedBindings);
            GetBinding(parser.uniformBuffers, builder, repeatedBindings);
            GetBinding(parser.storageBuffers, builder, repeatedBindings);
        }
        std::vector<ShaderResource> uniformBuffers;
        std::vector<ShaderResource> storageBuffers;
        std::vector<ShaderResource> sampledImages;
        std::vector<ShaderResource> storageImages;
        ShaderStage stage;
    };

    class Shader 
    {
    public:
        Shader(vk::Device logicalDevice, std::string path)
        {
            this->path = path;
            this->logicalDevice = logicalDevice;
            std::vector<uint32_t> byteCode = GetByteCode(path);
            sParser = std::make_unique<ShaderParser>(byteCode);
            sModule = std::make_unique<ShaderModule>(logicalDevice, byteCode);
        }
        void Reload()
        {
            std::vector<uint32_t> byteCode = GetByteCode(path);
            sParser.reset();
            sModule.reset();
            sParser = std::make_unique<ShaderParser>(byteCode);
            sModule = std::make_unique<ShaderModule>(logicalDevice, byteCode);
            
        }
        std::unique_ptr<ShaderParser> sParser;
        std::unique_ptr<ShaderModule> sModule;
        vk::Device logicalDevice;
        std::string path;
        
        
    };
}

#endif //SHADERPARSER_HPP
