//




// Created by carlo on 2024-11-19.
//


#ifndef DESCRIPTORCACHE_HPP
#define DESCRIPTORCACHE_HPP
namespace ENGINE
{
    class DescriptorCache
    {

    public:
        DescriptorCache(Core* core)
        {
            this->core = core;
        }

        void AddDefaultSampler(Sampler* defaultSampler)
        {
            this->defaultSampler = defaultSampler;
        }
        void AddDefaultImageView(ImageView* defaultImageView)
        {
            this->defaultImageView = defaultImageView;
        }
        void AddDefaultStorageSampler(Sampler* defaultStorgeSampler)
        {
            this->defaultStorgeSampler = defaultStorgeSampler;
        }
        void AddDefaultStorageImageView(ImageView* defaultStorageImageView)
        {
            this->defaultStorageImageView = defaultStorageImageView;
        }
        
        void AddShaderInfo(ShaderParser& parser, std::string name)
        {
            std::vector<ShaderResource> uniqueResources;
            parser.GetLayout(uniqueResources);
            for (auto& resource : uniqueResources)
            {
                if (dstSetBuilder.uniqueBindings.contains(resource.binding))
                {
                    continue;
                }
                std::unique_ptr<Buffer> ubo;
                std::unique_ptr<Buffer> ssbo;
                std::unique_ptr<Image> storageImage;
                std::unique_ptr<ImageData> storageImageData;
                std::unique_ptr<ImageView> storageImageView;
                vk::ImageCreateInfo imageInfo;
                switch (resource.type)
                {
                case vk::DescriptorType::eCombinedImageSampler:
                    assert(defaultSampler != nullptr && "Default sampler is needed");
                    assert(defaultImageView != nullptr && "Default Image view is needed");
                    imageBindings.try_emplace(resource.name, resource);
                    imageBindingsData.try_emplace(resource.binding, ImageBinding(defaultImageView, defaultSampler));
                    break;
                case vk::DescriptorType::eStorageImage:
                    assert(defaultStorgeSampler != nullptr && "Default sampler is needed");
                    assert(defaultStorageImageView != nullptr && "Default Image view is needed");
                    imageBindings.try_emplace(resource.name, resource);
                    imageBindingsData.try_emplace(resource.binding, ImageBinding(defaultStorageImageView, defaultStorgeSampler));

                    break;
                case vk::DescriptorType::eUniformBuffer:
                    bufferBindings.try_emplace(resource.name, resource);
                    ubo = std::make_unique<Buffer>(core->physicalDevice, core->logicalDevice.get(),
                                                        vk::BufferUsageFlagBits::eUniformTexelBuffer,
                                                        vk::MemoryPropertyFlagBits::eHostVisible |
                                                        vk::MemoryPropertyFlagBits::eHostCoherent, 1);
                    buffersData.try_emplace(resource.binding, std::move(ubo));
                    break;
                case vk::DescriptorType::eStorageBuffer:
                    bufferBindings.try_emplace(resource.name, resource);
                    ssbo = std::make_unique<Buffer>(core->physicalDevice, core->logicalDevice.get(),
                                                        vk::BufferUsageFlagBits::eStorageBuffer,
                                                        vk::MemoryPropertyFlagBits::eHostVisible |
                                                        vk::MemoryPropertyFlagBits::eHostCoherent, 1);
                    buffersData.try_emplace(resource.binding,std::move(ssbo));
                    break;
                }
                dstSetBuilder.AddBinding(resource.binding, resource.type);
            }
            
            assert(uniqueResources.size()==dstSetBuilder.bindings.size() && "Resources and builder must have the same size");
        }

        void BuildDescriptorsCache(DescriptorAllocator* descriptorAllocatorRef, vk::ShaderStageFlags stageFlags)
        {

            std::vector<vk::DescriptorBindingFlags> bindingFlags;
            for (int i = 0; i < dstSetBuilder.bindings.size(); ++i)
            {
                bindingFlags.push_back(
                    vk::DescriptorBindingFlagBits::ePartiallyBound);
            }
             auto bindingsFlagsCreateInfo = vk::DescriptorSetLayoutBindingFlagsCreateInfo()
            .setBindingCount((uint32_t)dstSetBuilder.bindings.size())
            .setPBindingFlags(bindingFlags.data());

            dstLayout = dstSetBuilder.BuildBindings(core->logicalDevice.get(), stageFlags, &bindingsFlagsCreateInfo, vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPoolEXT);

            dstSet = descriptorAllocatorRef->Allocate(core->logicalDevice.get(), dstLayout.get());

            for (auto& buffBinding : bufferBindings)
            {
                Buffer* buffer = buffersData.at(buffBinding.second.binding).get();
                writerBuilder.AddWriteBuffer(buffBinding.second.binding, buffer->descriptor, buffBinding.second.type);
            }

            for (auto imageBinding : imageBindings)
            {
                if (imageBinding.second.type == vk::DescriptorType::eCombinedImageSampler)
                {
                    ImageBinding& samplerBinding = imageBindingsData.at(imageBinding.second.binding);
                    writerBuilder.AddWriteImage(imageBinding.second.binding, samplerBinding.imageView, samplerBinding.sampler->samplerHandle.get(), vk::ImageLayout::eGeneral, imageBinding.second.type);
                }
                if (imageBinding.second.type == vk::DescriptorType::eStorageImage)
                {
                    ImageBinding& binding = imageBindingsData.at(imageBinding.second.binding);
                    writerBuilder.AddWriteImage(imageBinding.second.binding, binding.imageView, binding.sampler->samplerHandle.get(), vk::ImageLayout::eGeneral, imageBinding.second.type);
                }
            }
            writerBuilder.UpdateSet(core->logicalDevice.get(), dstSet.get());
        }

        void UpdateDescriptor()
        {
            for (auto& buffBinding : bufferBindings)
            {
                Buffer* buffer = buffersData.at(buffBinding.second.binding).get();
                writerBuilder.AddWriteBuffer(buffBinding.second.binding, buffer->descriptor, buffBinding.second.type);
            }

            for (auto imageBinding : imageBindings)
            {
                if (imageBinding.second.type == vk::DescriptorType::eSampledImage)
                {
                    ImageBinding& samplerBinding = imageBindingsData.at(imageBinding.second.binding);
                    writerBuilder.AddWriteImage(imageBinding.second.binding, samplerBinding.imageView, samplerBinding.sampler->samplerHandle.get(), vk::ImageLayout::eShaderReadOnlyOptimal, imageBinding.second.type);
                }
                if (imageBinding.second.type == vk::DescriptorType::eStorageBuffer)
                {
                    ImageBinding& binding = imageBindingsData.at(imageBinding.second.binding);
                    writerBuilder.AddWriteImage(imageBinding.second.binding, binding.imageView, binding.sampler->samplerHandle.get(), vk::ImageLayout::eGeneral, imageBinding.second.type);
                }
            }
            writerBuilder.UpdateSet(core->logicalDevice.get(), dstSet.get());
        }
        template<typename T>
        void SetBuffer(std::string name, std::vector<T> bufferData)
        {
            if (!bufferBindings.contains(name))
            {

                std::string text = "The buffer with name: " + name + ": does not exist on shaders\n";
                std::cout <<text;
                return;
            }
            ShaderResource binding = bufferBindings.at(name);
            Buffer& bufferRef = *buffersData.at(binding.binding); 
            if (sizeof(T) * bufferData.size()> bufferRef.deviceSize)
            {
                buffersData.at(binding.binding).reset(new Buffer(core->physicalDevice, core->logicalDevice.get(),
                                                     vk::BufferUsageFlagBits::eStorageBuffer,
                                                     vk::MemoryPropertyFlagBits::eHostVisible |
                                                     vk::MemoryPropertyFlagBits::eHostCoherent,
                                                     sizeof(T) * bufferData.size(), bufferData.data()));
                UpdateDescriptor();
            }
            
        }
        void SetImage(std::string name, ImageView* imageView, Sampler* sampler = nullptr)
        {
            if (!imageBindings.contains(name))
            {
                std::string text = "The Image with name: " + name + ": does not exist on shaders\n";
                std::cout << text;
                return;
            }
            ShaderResource& binding = imageBindings.at(name);
            if (imageBindingsData.at(binding.binding).imageView != imageView)
            {
                if (sampler)
                {
                    imageBindingsData.at(binding.binding).sampler = sampler;
                }
                imageBindingsData.at(binding.binding).imageView = imageView;
                UpdateDescriptor();
            }
        }


        struct ImageBinding
        {
            ImageView* imageView;
            Sampler* sampler;
        };
       
        std::map<std::string, ShaderResource> bufferBindings;
        std::map<std::string, ShaderResource> imageBindings;
        
        std::map<int, std::unique_ptr<Buffer>> buffersData;
        std::map<int, ImageBinding> imageBindingsData;
        
        DescriptorWriterBuilder writerBuilder;
        vk::UniqueDescriptorSetLayout dstLayout;
        vk::UniqueDescriptorSet dstSet;
        
        DescriptorLayoutBuilder dstSetBuilder;
        vk::DescriptorSetLayout* dstSetLayoutHandle;
        
        Core* core;
        ImageView* defaultImageView;
        Sampler* defaultSampler;
        
        ImageView* defaultStorageImageView;
        Sampler* defaultStorgeSampler;
         
    };
   
}

#endif //DESCRIPTORCACHE_HPP
