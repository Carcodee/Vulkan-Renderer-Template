//
// Created by carlo on 2024-10-08.
//

#ifndef DESCRIPTORS_HPP
#define DESCRIPTORS_HPP

namespace ENGINE
{
    struct DescriptorLayoutBuilder
    {
        void AddBinding(uint32_t binding, vk::DescriptorType type)
        {
            auto newBinding = vk::DescriptorSetLayoutBinding()
            .setBinding(binding)
            .setDescriptorCount(1)
            .setDescriptorType(type);
            bindings.push_back(newBinding);
        }


        vk::UniqueDescriptorSetLayout BuildBindings(vk::Device device, vk::ShaderStageFlags stageFlags,
                                                    void* pNext = nullptr,
                                                    vk::DescriptorSetLayoutCreateFlags flags =
                                                        vk::DescriptorSetLayoutCreateFlags())
        {
            for (auto& binding : bindings)
            {
                binding.stageFlags |= stageFlags;
            }
            auto dstSetLayoutCreateInfo = vk::DescriptorSetLayoutCreateInfo()
                                          .setPNext(pNext)
                                          .setPBindings(bindings.data())
                                          .setBindingCount(static_cast<uint32_t>(bindings.size()))
                                          .setFlags(flags);

            clear();
            return device.createDescriptorSetLayoutUnique(dstSetLayoutCreateInfo);
        }
         void clear()
        {
            bindings.clear();
        }       
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        
    };

    struct DescriptorWriterBuilder
    {
        void AddWriteImage(int binding, ImageView* imageView, vk::Sampler sampler,vk::ImageLayout layout, vk::DescriptorType type)
        {

            vk::DescriptorImageInfo& imageInfo = imageInfos.emplace_back(vk::DescriptorImageInfo()
                .setSampler(sampler)
                .setImageLayout(layout)
                .setImageView(imageView->imageView.get()));

            auto dstWrite = vk::WriteDescriptorSet()
            .setDstBinding(binding)
            .setDstSet(VK_NULL_HANDLE)
            .setDescriptorCount(1)
            .setDescriptorType(type)
            .setPImageInfo(&imageInfo);
            
            writes.push_back(dstWrite);
        }
        
         void AddWriteBuffer(int binding, Buffer* buffer, vk::DescriptorType type)
        {
            vk::DescriptorBufferInfo& bufferInfo = bufferInfos.emplace_back(buffer->descriptor);

            auto dstWrite = vk::WriteDescriptorSet()
            .setDstBinding(binding)
            .setDstSet(VK_NULL_HANDLE)
            .setDescriptorCount(1)
            .setDescriptorType(type)
            .setPBufferInfo(&bufferInfo);

            writes.push_back(dstWrite);
        }
        
        void Clear()
        {
            imageInfos.clear();
            writes.clear();
            bufferInfos.clear();
        }
        
        void UpdateSet(vk::Device device, vk::DescriptorSet set)
        {
            for (auto& write : writes)
            {
                write.setDstSet(set);
            }
            device.updateDescriptorSets(static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
            Clear();
        }
        
        std::map<std::string, vk::DescriptorBufferInfo*> bufferInfosRef;
        std::deque<vk::DescriptorImageInfo> imageInfos;
        std::deque<vk::DescriptorBufferInfo> bufferInfos;
        std::vector<vk::WriteDescriptorSet> writes;
    };
}

#endif //DESCRIPTORS_HPP
