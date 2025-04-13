#pragma once

#include "pch.hh"

namespace letc
{
    struct Image
    {
        const VmaAllocator &allocator;
        vk::ImageCreateInfo imageCreateInfo;
        VmaAllocation allocation;
        vk::Image image;

        const vk::ImageCreateFlags &flags = imageCreateInfo.flags;
        const vk::ImageType &type = imageCreateInfo.imageType;
        const vk::Format &format = imageCreateInfo.format;
        const vk::Extent3D &extent = imageCreateInfo.extent;
        const uint32_t &mipLevels = imageCreateInfo.mipLevels;
        const uint32_t &arrayLayers = imageCreateInfo.arrayLayers;
        const vk::SampleCountFlagBits &samples = imageCreateInfo.samples;
        const vk::ImageTiling &tiling = imageCreateInfo.tiling;
        const vk::ImageUsageFlags &usage = imageCreateInfo.usage;
        const vk::SharingMode &sharingMode = imageCreateInfo.sharingMode;
        const uint32_t &queueFamilyIndexCount = imageCreateInfo.queueFamilyIndexCount;
        const uint32_t *&pQueueFamilyIndices = imageCreateInfo.pQueueFamilyIndices;
        const vk::ImageLayout &initialLayout = imageCreateInfo.initialLayout;

        operator const vk::Image &()
        {
            return image;
        }

        Image(const VmaAllocator &allocator, const vk::ImageCreateInfo &imageCreateInfo,
              const VmaMemoryUsage &memoryUsage)
            : allocator(allocator), imageCreateInfo(imageCreateInfo)
        {
            VmaAllocationCreateInfo allocationCreateInfo{};
            allocationCreateInfo.usage = memoryUsage;

            vmaCreateImage(allocator, reinterpret_cast<VkImageCreateInfo *>(&this->imageCreateInfo),
                           &allocationCreateInfo, reinterpret_cast<VkImage *>(&image), &allocation, nullptr);
        }

        ~Image()
        {
            vmaDestroyImage(allocator, image, allocation);
        }
    };

    struct ImageView
    {
        const vk::Device &device;
        vk::ImageViewCreateInfo imageViewCreateInfo;
        vk::ImageView imageView;

        operator const vk::ImageView &()
        {
            return imageView;
        }

        ImageView(const vk::Device &device, const vk::ImageViewCreateInfo &imageViewCreateInfo)
            : device(device), imageViewCreateInfo(imageViewCreateInfo)
        {
            imageView = device.createImageView(imageViewCreateInfo);
        }

        ~ImageView()
        {
            device.destroyImageView(imageView);
        }
    };

    namespace laconic
    {
        inline std::unique_ptr<Image> depthImage(const VmaAllocator &allocator, const uint32_t &width,
                                                 const uint32_t &height)
        {
            auto depthImageCreateInfo = vk::ImageCreateInfo{};
            depthImageCreateInfo.setImageType(vk::ImageType::e2D);
            depthImageCreateInfo.setFormat(vk::Format::eD32Sfloat);
            depthImageCreateInfo.setExtent({width, height, 1});
            depthImageCreateInfo.setMipLevels(1);
            depthImageCreateInfo.setArrayLayers(1);
            depthImageCreateInfo.setSamples(vk::SampleCountFlagBits::e1);
            depthImageCreateInfo.setTiling(vk::ImageTiling::eOptimal);
            depthImageCreateInfo.setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment);
            depthImageCreateInfo.setSharingMode(vk::SharingMode::eExclusive);
            depthImageCreateInfo.setQueueFamilyIndices(nullptr);
            depthImageCreateInfo.setInitialLayout(vk::ImageLayout::eUndefined);
            return std::make_unique<Image>(allocator, depthImageCreateInfo, VMA_MEMORY_USAGE_GPU_ONLY);
        }

        inline std::unique_ptr<ImageView> depthImageView(const vk::Device &device, const vk::Image &depthImage)
        {
            auto depthImageViewCreateInfo = vk::ImageViewCreateInfo{};
            depthImageViewCreateInfo.setImage(depthImage);
            depthImageViewCreateInfo.setViewType(vk::ImageViewType::e2D);
            depthImageViewCreateInfo.setFormat(vk::Format::eD32Sfloat);

            auto subresourceRange = vk::ImageSubresourceRange{};
            subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eDepth);
            subresourceRange.setBaseMipLevel(0);
            subresourceRange.setLevelCount(1);
            subresourceRange.setBaseArrayLayer(0);
            subresourceRange.setLayerCount(1);
            depthImageViewCreateInfo.setSubresourceRange(subresourceRange);

            return std::make_unique<ImageView>(device, depthImageViewCreateInfo);
        }

        inline void transitionImageLayout(const vk::CommandBuffer &commandBuffer, vk::Image image,
                                          vk::ImageAspectFlags aspectMask, vk::ImageLayout oldLayout,
                                          vk::ImageLayout newLayout, vk::AccessFlags srcAccessMask,
                                          vk::AccessFlags dstAccessMask, vk::PipelineStageFlags srcStageMask,
                                          vk::PipelineStageFlags dstStageMask, uint32_t baseMipLevel = 0,
                                          uint32_t levelCount = 1, uint32_t baseArrayLayer = 0, uint32_t layerCount = 1)
        {
            vk::ImageMemoryBarrier barrier{};
            barrier.setOldLayout(oldLayout);
            barrier.setNewLayout(newLayout);
            barrier.setSrcQueueFamilyIndex(vk::QueueFamilyIgnored);
            barrier.setDstQueueFamilyIndex(vk::QueueFamilyIgnored);
            barrier.setImage(image);
            barrier.setSubresourceRange(
                vk::ImageSubresourceRange{aspectMask, baseMipLevel, levelCount, baseArrayLayer, layerCount});
            barrier.setSrcAccessMask(srcAccessMask);
            barrier.setDstAccessMask(dstAccessMask);

            commandBuffer.pipelineBarrier(srcStageMask, dstStageMask, {}, nullptr, nullptr, barrier);
        }

    }; // namespace laconic

}; // namespace letc
