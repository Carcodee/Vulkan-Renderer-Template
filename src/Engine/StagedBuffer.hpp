//

// Created by carlo on 2024-10-02.
//

#ifndef STAGEDBUFFER_HPP
#define STAGEDBUFFER_HPP

namespace ENGINE
{
    class StagedBuffer
    {
    public:
        StagedBuffer(vk::PhysicalDevice physicalDevice, vk::Device logicalDevice, vk::BufferUsageFlags usage,
                     vk::DeviceSize size)
        {
            stagingBuffer = std::make_unique<ENGINE::Buffer>(physicalDevice, logicalDevice,
                                                             vk::BufferUsageFlagBits::eTransferDst,
                                                             vk::MemoryPropertyFlagBits::eHostVisible |
                                                             vk::MemoryPropertyFlagBits::eHostCoherent,
                                                             size);
            deviceBuffer = std::make_unique<ENGINE::Buffer>(physicalDevice, logicalDevice,
                                                            usage | vk::BufferUsageFlagBits::eTransferDst,
                                                            vk::MemoryPropertyFlagBits::eDeviceLocal, size);
        }

        void* Map()
        {
            return stagingBuffer->Map();
        }

        void Unmap(vk::CommandBuffer commandBuffer)
        {
            stagingBuffer->Unmap();

            auto copyRegion = vk::BufferCopy()
                              .setDstOffset(0)
                              .setDstOffset(0)
                              .setSize(size);

            commandBuffer.copyBuffer(stagingBuffer->bufferHandle.get(), deviceBuffer->bufferHandle.get(), {copyRegion});
        }
        

        vk::Buffer GetBuffer()
        {
            return deviceBuffer->bufferHandle.get();
        }

        std::unique_ptr<ENGINE::Buffer> stagingBuffer;
        std::unique_ptr<ENGINE::Buffer> deviceBuffer;
        vk::DeviceSize size;
    };
    
    
}
#endif //STAGEDBUFFER_HPP
