//


// Created by carlo on 2024-11-22.
//


#ifndef RESOURCESMANAGER_HPP
#define RESOURCESMANAGER_HPP

#define BASE_SIZE 10000

namespace ENGINE 
{
    class ResourcesManager
    {
    public:

        
        ImageShipper* SetShipperPath(std::string name, std::string path)
        {
            assert(core!= nullptr &&"core must be set");
            ImageShipper* imageShipper;
            if (imagesShippersNames.contains(name))
            {
                imageShipper = imageShippers.at(imagesShippersNames.at(name)).get();
            }
            else
            {
                imagesShippersNames.try_emplace(name, (int32_t)imageShippers.size());
                imageShippers.emplace_back(std::make_unique<ImageShipper>());
                imageShipper = GetShipperFromName(name);
            }
            imageShipper->SetDataFromPath(path);
            return imageShipper;
        }
        
        ~ResourcesManager() = default;
        ImageShipper* SetShipperDataRaw(std::string name, void* data, int width, int height, vk::DeviceSize size)
        {
            assert(core!= nullptr &&"core must be set");
            ImageShipper* imageShipper;
            if (imagesShippersNames.contains(name))
            {
                imageShipper = imageShippers.at(imagesShippersNames.at(name)).get();
            }
            else
            {
                imagesShippersNames.try_emplace(name, (int32_t)imageShippers.size());
                imageShippers.emplace_back(std::make_unique<ImageShipper>());
                imageShipper = GetShipperFromName(name);
            }
            imageShipper->SetDataRaw(data, width, width, size);
            return imageShipper;
        }
        
        ImageShipper* GetShipper(std::string name, uint32_t arrayLayersCount, uint32_t mipsCount, vk::Format format,
                                 LayoutPatterns dstPattern)
        {
            assert(core!= nullptr &&"core must be set");
            ImageShipper* imageShipper;
            if (imagesShippersNames.contains(name))
            {
                imageShipper = imageShippers.at(imagesShippersNames.at(name)).get();
            }
            else
            {
                imagesShippersNames.try_emplace(name, (int32_t)imageShippers.size());
                imageShippers.emplace_back(std::make_unique<ImageShipper>());
                imageShipper = GetShipperFromName(name);
                return imageShipper;
            }
            imageShipper->BuildImage(core, arrayLayersCount, mipsCount, format, dstPattern);
            return imageShipper;
        }
        
        
        ImageView* GetImage(std::string name, vk::ImageCreateInfo imageInfo, int baseMipLevel,
                            int mipLevelCount, int baseArrayLayer, int arrayLayerCount)
        {
            assert(core!= nullptr &&"core must be set");
            auto image = std::make_unique<
                Image>(core->physicalDevice, core->logicalDevice.get(), imageInfo);
            if (imageInfo.usage == vk::ImageUsageFlagBits::eStorage)
            {
                assert(!storageImagesNames.contains(name) && "Image name already exist");
                storageImagesNames.try_emplace(name, (int32_t)storageImagesViews.size());
                storageImagesViews.emplace_back(std::make_unique<ImageView>(
                    core->logicalDevice.get(), image->imageData.get(),
                    baseMipLevel, mipLevelCount, baseArrayLayer,
                    arrayLayerCount));
                images.emplace_back(std::move(image));
                return storageImagesViews.back().get();
            }
            else
            {
                assert(!imagesNames.contains(name) && "Image name already exist");
                imagesNames.try_emplace(name, (int32_t)imageViews.size());
                imageViews.emplace_back(std::make_unique<ImageView>(core->logicalDevice.get(), image->imageData.get(),
                                                                    baseMipLevel, mipLevelCount, baseArrayLayer,
                                                                    arrayLayerCount));
                images.emplace_back(std::move(image));
                return imageViews.back().get();
            }
        }
        
        Buffer* GetBuffer(std::string name, vk::BufferUsageFlags bufferUsageFlags,
                          vk::MemoryPropertyFlags memPropertyFlags, vk::DeviceSize deviceSize
                          , void* data = nullptr)
        {
            assert(core!= nullptr &&"core must be set");
            assert(!bufferNames.contains(name) && "Buffer name already exist");
        
            auto buffer = std::make_unique<Buffer>(
                core->physicalDevice, core->logicalDevice.get(), bufferUsageFlags, memPropertyFlags, deviceSize,
                data);
        
            bufferNames.try_emplace(name, (int32_t)buffers.size());
            buffers.emplace_back(std::move(buffer));
            return buffers.back().get();
        }
        
        StagedBuffer* GetStageBuffer(std::string name, vk::BufferUsageFlags bufferUsageFlags, vk::DeviceSize deviceSize
        )
        {
            assert(core!= nullptr &&"core must be set");
            assert(!stagedBufferNames.contains(name) && "Buffer name already exist");
        
            auto buffer = std::make_unique<StagedBuffer>(
                core->physicalDevice, core->logicalDevice.get(), bufferUsageFlags, deviceSize);
        
            stagedBufferNames.try_emplace(name, (int32_t)buffers.size());
            stagedBuffers.emplace_back(std::move(buffer));
            return stagedBuffers.back().get();
        }
        
        Buffer* SetBuffer(std::string name, vk::BufferUsageFlags bufferUsageFlags,
                          vk::MemoryPropertyFlags memPropertyFlags, vk::DeviceSize deviceSize
                          , void* data = nullptr)
        {
            assert(core!= nullptr &&"core must be set");
            assert(bufferNames.contains(name) && "Buffer dont exist");
        
            auto buffer = std::make_unique<Buffer>(
                core->physicalDevice, core->logicalDevice.get(), bufferUsageFlags, memPropertyFlags, deviceSize,
                data);
        
        
            buffers.at(bufferNames.at(name)).reset(new Buffer(core->physicalDevice, core->logicalDevice.get(),
                                                              bufferUsageFlags, memPropertyFlags, deviceSize, data));
            return buffers.at(bufferNames.at(name)).get();
        }
        
        StagedBuffer* SetStageBuffer(std::string name, vk::BufferUsageFlags bufferUsageFlags,
                                     vk::DeviceSize deviceSize
        )
        {
            assert(core!= nullptr &&"core must be set");
            assert(!stagedBufferNames.contains(name) && "staged buffer dont exist");
        
            auto buffer = std::make_unique<StagedBuffer>(
                core->physicalDevice, core->logicalDevice.get(), bufferUsageFlags, deviceSize);
        
            stagedBuffers.at(stagedBufferNames.at(name)).reset(
                new StagedBuffer(core->physicalDevice, core->logicalDevice.get(), bufferUsageFlags, deviceSize));
            return stagedBuffers.at(stagedBufferNames.at(name)).get();
        }
        
        
        ImageView* GetImageViewFromName(std::string name)
        {
            if (!imagesNames.contains(name))
            {
                return nullptr;
            }
            return imageViews.at(imagesNames.at(name)).get();
        }
        
        ImageShipper* GetShipperFromName(std::string name)
        {
            if (!imagesShippersNames.contains(name))
            {
                return nullptr;
            }
            return imageShippers.at(imagesShippersNames.at(name)).get();
        }
        
        ImageShipper* GetStorageFromName(std::string name)
        {
            if (!storageImagesNames.contains(name))
            {
                return nullptr;
            }
            return imageShippers.at(storageImagesNames.at(name)).get();
        }
        
        Buffer* GetBuffFromName(std::string name)
        {
            if (!bufferNames.contains(name))
            {
                return nullptr;
            }
            return buffers.at(bufferNames.at(name)).get();
        }
        
        StagedBuffer* GetStagedBuffFromName(std::string name)
        {
            if (!stagedBufferNames.contains(name))
            {
                return nullptr;
            }
            return stagedBuffers.at(stagedBufferNames.at(name)).get();
        }


        void DestroyResources()
        {
            buffers.clear();
            stagedBuffers.clear();
            storageImagesViews.clear();
            imageViews.clear();
            imageShippers.clear();
            images.clear();
        }

        ResourcesManager(Core* coreRefs)
        {
            this->core = coreRefs;
            stagedBuffers.reserve(BASE_SIZE);
            buffers.reserve(BASE_SIZE);
            storageImagesViews.reserve(BASE_SIZE);
            imageViews.reserve(BASE_SIZE);
            images.reserve(BASE_SIZE);
        }
        static ResourcesManager* GetInstance(Core* coreRef = nullptr)
        {
            if (instance == nullptr && coreRef != nullptr)
            {
                instance = new ResourcesManager(coreRef);
            }
            return instance;
        }
        
        std::unordered_map<std::string, int32_t> bufferNames;
        std::unordered_map<std::string, int32_t> stagedBufferNames;
        std::unordered_map<std::string, int32_t> imagesNames;
        std::unordered_map<std::string, int32_t> storageImagesNames;
        std::unordered_map<std::string, int32_t> imagesShippersNames;
        
        
        std::vector<std::unique_ptr<Buffer>> buffers;
        std::vector<std::unique_ptr<StagedBuffer>> stagedBuffers;
        std::vector<std::unique_ptr<ImageView>> imageViews;
        std::vector<std::unique_ptr<ImageView>> storageImagesViews;
        std::vector<std::unique_ptr<ImageShipper>> imageShippers;
        std::vector<std::unique_ptr<Image>> images;
        
        Core* core;
        static ResourcesManager* instance;


    };

    ResourcesManager* ResourcesManager::instance = nullptr;
}


#endif //RESOURCESMANAGER_HPP
