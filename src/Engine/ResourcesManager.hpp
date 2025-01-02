//



// Created by carlo on 2024-11-22.
//






#ifndef RESOURCESMANAGER_HPP
#define RESOURCESMANAGER_HPP

#define BASE_SIZE 10000

namespace ENGINE 
{
    class ResourcesManager : SYSTEMS::Subject
    {
    public:
        enum BufferState
        {
            VALID,
            INVALID
        };
        struct BufferUpdateInfo 
        {
            BufferState state;
            size_t size;
            void* data;
        };

        struct ImagesUpdateInfo 
        {
            std::string path;
            uint32_t arrayLayersCount;
            uint32_t mipsCount;
            vk::Format format;
            LayoutPatterns dstPattern;
            BufferState bufferState;
        };
        ImageShipper* GetShipper(std::string name, std::string path, uint32_t arrayLayersCount, uint32_t mipsCount, vk::Format format,
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
                imagesUpdateInfos.emplace_back(ImagesUpdateInfo{path, arrayLayersCount, mipsCount, format, dstPattern, VALID});
                imageShipper = GetShipperFromName(name);
            }
            if (imageShipper->image == nullptr)
            {
                imageShipper->SetDataFromPath(path);
                imageShipper->BuildImage(core, arrayLayersCount, mipsCount, format, dstPattern);
            }
            return imageShipper;
        }
         ImageShipper* GetShipper(std::string name, void* data, int width, int height, vk::DeviceSize size, uint32_t arrayLayersCount, uint32_t mipsCount, vk::Format format,
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
                imagesUpdateInfos.emplace_back(ImagesUpdateInfo{"", arrayLayersCount, mipsCount, format, dstPattern, VALID});
                imageShipper = GetShipperFromName(name);
            }
            if (imageShipper->image == nullptr)
            {
                imageShipper->SetDataRaw(data, width, height, size);
                imageShipper->BuildImage(core, arrayLayersCount, mipsCount, format, dstPattern);
            }
            return imageShipper;
        }
        
        ImageShipper* BatchShipper(std::string name, std::string path, uint32_t arrayLayersCount, uint32_t mipsCount, vk::Format format,
                                 LayoutPatterns dstPattern)
        {
            if (imagesShippersNames.contains(name))
            {
                SYSTEMS::Logger::GetInstance()->SetLogPreferences(SYSTEMS::LogLevel::L_INFO);
                SYSTEMS::Logger::GetInstance()->Log("Using texture that already exist: " + name, SYSTEMS::LogLevel::L_INFO);
                ImageShipper* shipper = GetShipperFromName(name);
                return shipper;
            }
            imagesShippersNames.try_emplace(name, (int32_t)imageShippers.size());
            imageShippers.emplace_back(std::make_unique<ImageShipper>());
            imagesUpdateInfos.emplace_back(ImagesUpdateInfo{path, arrayLayersCount, mipsCount, format, dstPattern, INVALID});
            updateImagesShippers = true;
            return GetShipperFromName(name);
            
        }
        
        
        ImageView* GetImage(std::string name, vk::ImageCreateInfo imageInfo, int baseMipLevel, int baseArrayLayer)
        {
            assert(core!= nullptr &&"core must be set");
            ImageView* imageViewRef = nullptr;
            if (imagesNames.contains(name) && (imageInfo.usage & vk::ImageUsageFlagBits::eStorage))
            {
                imageViewRef = GetStorageFromName(name);
                return imageViewRef;
                
            }else if(imagesNames.contains(name))
            {
                imageViewRef = GetImageViewFromName(name);
                return imageViewRef;
            }
            
            auto image = std::make_unique<
                Image>(core->physicalDevice, core->logicalDevice.get(), imageInfo);
            if (imageInfo.usage & vk::ImageUsageFlagBits::eStorage)
            {
                assert(!storageImagesNames.contains(name) && "Image name already exist");
                storageImagesNames.try_emplace(name, (int32_t)storageImagesViews.size());
                storageImagesViews.emplace_back(std::make_unique<ImageView>(
                    core->logicalDevice.get(), image->imageData.get(),
                    baseMipLevel, imageInfo.mipLevels, baseArrayLayer,
                    imageInfo.arrayLayers));
                images.emplace_back(std::move(image));
                return storageImagesViews.back().get();
            }
            else
            {
                assert(!imagesNames.contains(name) && "Image name already exist");
                imagesNames.try_emplace(name, (int32_t)imageViews.size());
                imageViews.emplace_back(std::make_unique<ImageView>(core->logicalDevice.get(), image->imageData.get(),
                                                                    baseMipLevel, imageInfo.mipLevels, baseArrayLayer,
                                                                    imageInfo.arrayLayers));
                images.emplace_back(std::move(image));
                return imageViews.back().get();
            }
        }
        
        Buffer* GetBuffer(std::string name, vk::BufferUsageFlags bufferUsageFlags,
                          vk::MemoryPropertyFlags memPropertyFlags, vk::DeviceSize deviceSize
                          , void* data = nullptr)
        {
            assert(core!= nullptr &&"core must be set");
            if (bufferNames.contains(name))
            {
                return GetBuffFromName(name);
            }
        
            auto buffer = std::make_unique<Buffer>(
                core->physicalDevice, core->logicalDevice.get(), bufferUsageFlags, memPropertyFlags, deviceSize,
                data);
        
            bufferNames.try_emplace(name, (int32_t)buffers.size());
            buffers.emplace_back(std::move(buffer));
            buffersState.push_back({VALID, deviceSize, data});
            return buffers.back().get();
        }
        
        StagedBuffer* GetStageBuffer(std::string name, vk::BufferUsageFlags bufferUsageFlags, vk::DeviceSize deviceSize, void* data = nullptr)
        {
            assert(core!= nullptr &&"core must be set");
            if (stagedBufferNames.contains(name))
            {
                return GetStagedBuffFromName(name);
            }
        
            auto buffer = std::make_unique<StagedBuffer>(
                core->physicalDevice, core->logicalDevice.get(), bufferUsageFlags, deviceSize);

            if (data != nullptr)
            {
                void* mappedMem= buffer->Map();
                memcpy(mappedMem, data, deviceSize);
                auto commandExecutor = std::make_unique<ExecuteOnceCommand>(core);
                auto commandBuffer = commandExecutor->BeginCommandBuffer();
                buffer->Unmap(commandBuffer);
                commandExecutor->EndCommandBuffer();
            }
        
            stagedBufferNames.try_emplace(name, (int32_t)stagedBuffers.size());
            stagedBuffers.emplace_back(std::move(buffer));
            stagedBuffersState.push_back({VALID, deviceSize});
            return stagedBuffers.back().get();
        }
        
        Buffer* SetBuffer(std::string name, vk::DeviceSize deviceSize
                          , void* data)
        {
            assert(core!= nullptr &&"core must be set");
            assert(bufferNames.contains(name) && "Buffer dont exist");

            Buffer* bufferRef = buffers.at(bufferNames.at(name)).get();
            if (deviceSize > bufferRef->deviceSize)
            {
                buffersState.at(bufferNames.at(name)) = {INVALID, deviceSize, data};
                invalidateBuffers = true;
                
            }else
            {
                //pending to handle this if is a staged resource
                Buffer* bufferRef = GetBuffFromName(name);
                if (bufferRef->mappedMem == nullptr)
                {
                    bufferRef->Map();
                }
                memcpy(bufferRef->mappedMem, data, deviceSize);
                if (bufferRef->usageFlags == vk::BufferUsageFlagBits::eStorageBuffer)
                {
                    bufferRef->Unmap();
                }
            }

            return buffers.at(bufferNames.at(name)).get();
        }
        
        StagedBuffer* SetStageBuffer(std::string name, vk::DeviceSize deviceSize, void* data)
        {
            //todo
            assert(core!= nullptr &&"core must be set");
            assert(stagedBufferNames.contains(name) && "staged buffer dont exist");

            if (deviceSize > stagedBuffers.at(stagedBufferNames.at(name))->size)
            {
                stagedBuffersState.at(stagedBufferNames.at(name)) = {INVALID, deviceSize, data};
                invalidateBuffers = true;
            }else
            {
                StagedBuffer* buffer = GetStagedBuffFromName(name);
                void* mappedMem = buffer->Map();
                memcpy(mappedMem, data, deviceSize);
                auto commandExecutor = std::make_unique<ExecuteOnceCommand>(core);
                auto commandBuffer = commandExecutor->BeginCommandBuffer();
                buffer->Unmap(commandBuffer);
                commandExecutor->EndCommandBuffer();
            }
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
        
        ImageView* GetStorageFromName(std::string name)
        {
            if (!storageImagesNames.contains(name))
            {
                return nullptr;
            }
            return storageImagesViews.at(storageImagesNames.at(name)).get();
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

        int GetShipperID(std::string name)
        {
            return imagesShippersNames.at(name);
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
        void UpdateImages()
        {
            if (!updateImagesShippers){return;}
            for (int i = 0; i < imageShippers.size(); i++)
            {
                ImagesUpdateInfo& updateInfo = imagesUpdateInfos[i];
                if (updateInfo.bufferState == INVALID)
                {
                    imageShippers[i]->SetDataFromPath(updateInfo.path);
                    imageShippers[i]->BuildImage(core, updateInfo.arrayLayersCount, updateInfo.mipsCount,
                                                 updateInfo.format, updateInfo.dstPattern);
                    updateInfo.bufferState = VALID;
                }
            }
            
        }
        void UpdateBuffers()
        {
            if (!invalidateBuffers){return;}
            for (auto& name : bufferNames)
            {
                BufferUpdateInfo& bufferUpdateInfo = buffersState.at(name.second);
                if (bufferUpdateInfo.state == INVALID)
                {
                    buffers.at(bufferNames.at(name.first)).reset(new Buffer(core->physicalDevice, core->logicalDevice.get(),
                                                                  buffers.at(name.second)->usageFlags, buffers.at(name.second)->memPropertyFlags, bufferUpdateInfo.size,
                                                                  bufferUpdateInfo.data));
                    bufferUpdateInfo.state = VALID;
                }
            }
            auto commandExecutor = std::make_unique<ExecuteOnceCommand>(core);
            auto commandBuffer = commandExecutor->BeginCommandBuffer();
            for (auto& name : stagedBufferNames)
            {
                BufferUpdateInfo& bufferUpdateInfo = stagedBuffersState.at(name.second);
                if (bufferUpdateInfo.state == INVALID)
                {
                    stagedBuffers.at(stagedBufferNames.at(name.first)).reset(new StagedBuffer(
                        core->physicalDevice, core->logicalDevice.get(),
                        stagedBuffers.at(name.second)->deviceBuffer->usageFlags,
                        bufferUpdateInfo.size));
                    StagedBuffer* buffer = GetStagedBuffFromName(name.first);
                    void* mappedMem = buffer->Map();
                    memcpy(mappedMem, bufferUpdateInfo.data, bufferUpdateInfo.size);
                    buffer->Unmap(commandBuffer);
                    
                    bufferUpdateInfo.state = VALID;
                }               
                
            }
            commandExecutor->EndCommandBuffer();
            Notify();
            invalidateBuffers = false;
        }
        void EndFrameDynamicUpdates(vk::CommandBuffer commandBuffer)
        {
            BufferAccessPattern src = GetSrcBufferAccessPattern(BufferUsageTypes::B_GRAPHICS_WRITE);
            BufferAccessPattern dst = GetSrcBufferAccessPattern(BufferUsageTypes::B_TRANSFER_DST);
            CreateMemBarrier(src, dst, commandBuffer);
            for (int i = 0; i < storageImagesToClear.size(); ++i)
            {
                ImageView* imageView = GetStorageFromName(storageImagesToClear[i]);
                vk::ClearColorValue clearColor(std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f});

                commandBuffer.clearColorImage(
                    imageView->imageData->imageHandle,
                    vk::ImageLayout::eGeneral,
                    clearColor,
                   imageView->GetSubresourceRange() 
                );

            }
            storageImagesToClear.clear();
        }

        ResourcesManager(Core* coreRefs)
        {
            this->core = coreRefs;
            stagedBuffers.reserve(BASE_SIZE);
            buffers.reserve(BASE_SIZE);
            storageImagesViews.reserve(BASE_SIZE);
            imageViews.reserve(BASE_SIZE);
            images.reserve(BASE_SIZE);
            std::string defaultTexturePath = SYSTEMS::OS::GetInstance()->GetEngineResourcesPath() + "\\Images\\default_texture.jpg";
            
            ImageShipper* shipper = GetShipper("default_tex", defaultTexturePath, 1,1, g_ShipperFormat, LayoutPatterns::GRAPHICS_READ);


            auto imageInfo = ENGINE::Image::CreateInfo2d(
                glm::uvec2(core->swapchainRef->extent.width, core->swapchainRef->extent.height), 1, 1,
                                                         ENGINE::g_32bFormat,
                                                         vk::ImageUsageFlagBits::eStorage);

            ImageView* defaultStorage = GetImage("default_storage", imageInfo, 0, 0);
            
        }

        void RequestStorageImageClear(std::string name)
        {
            if (!storageImagesNames.contains(name))
            {
                SYSTEMS::Logger::GetInstance()->LogMessage("Storage Image with name does not exist: " + name);
                return;
            }
            storageImagesToClear.push_back(name);
        }
        
        static ResourcesManager* GetInstance(Core* coreRef = nullptr)
        {
            if (instance == nullptr && coreRef != nullptr)
            {
                instance = new ResourcesManager(coreRef);
            }
            return instance;
        }
        
        ~ResourcesManager() = default;

        void Attach(SYSTEMS::Watcher* watcher) override
        {
           watchers.push_back(watcher); 
        }

        void Detach(SYSTEMS::Watcher* watcher) override
        {
           watchers.erase(std::remove(watchers.begin(), watchers.end(), watcher), watchers.end()); 
        }

        void Notify() override
        {
            for (auto& watcher : watchers)
            {
                watcher->UpdateWatcher();
            }
        }

        std::vector<SYSTEMS::Watcher*> watchers;
        
        std::unordered_map<std::string, int32_t> bufferNames;
        std::unordered_map<std::string, int32_t> stagedBufferNames;
        std::unordered_map<std::string, int32_t> imagesNames;
        std::unordered_map<std::string, int32_t> storageImagesNames;
        std::unordered_map<std::string, int32_t> imagesShippersNames;
        
        
        std::vector<std::unique_ptr<Buffer>> buffers;
        std::vector<std::unique_ptr<StagedBuffer>> stagedBuffers;
        std::vector<BufferUpdateInfo> buffersState;
        std::vector<BufferUpdateInfo> stagedBuffersState;
        std::vector<std::unique_ptr<ImageView>> imageViews;
        std::vector<std::unique_ptr<ImageView>> storageImagesViews;
        std::vector<std::unique_ptr<ImageShipper>> imageShippers;
        std::vector<ImagesUpdateInfo> imagesUpdateInfos;
        std::vector<std::unique_ptr<Image>> images;
        std::vector<std::string> storageImagesToClear;
      
        bool invalidateBuffers = false;
        bool updateImagesShippers = false;
        
        Core* core;
        static ResourcesManager* instance;

    };

    ResourcesManager* ResourcesManager::instance = nullptr;
}


#endif //RESOURCESMANAGER_HPP
