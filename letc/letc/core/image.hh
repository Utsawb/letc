#pragma once

#include "letc/pch.hh"

#include "letc/core/allocator.hh"
#include "letc/core/device.hh"

namespace letc
{
    class Image
    {
      public:
        Image(std::weak_ptr<Allocator> allocator, const vk::ImageCreateInfo &info, const VmaMemoryUsage &usage);
        ~Image();
        auto get() -> vk::Image;
        auto getInfo() -> vk::ImageCreateInfo;

      private:
        std::weak_ptr<Allocator> m_allocator;
        vk::Image m_image;
        vk::ImageCreateInfo m_info;
        VmaAllocation m_allocation;
    };

    class ImageView
    {
      public:
        ImageView(std::weak_ptr<Device> device, const vk::ImageViewCreateInfo &info);
        ~ImageView();
        auto get() -> vk::ImageView;
        auto getInfo() -> vk::ImageViewCreateInfo;

      private:
        std::weak_ptr<Device> m_device;
        vk::ImageViewCreateInfo m_info;
        vk::ImageView m_view;
    };

    namespace laconic
    {
        inline auto depthImage(std::weak_ptr<Allocator> allocator, const uint32_t &width, const uint32_t &height)
            -> std::shared_ptr<Image>
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
            return std::make_shared<Image>(allocator, depthImageCreateInfo, VMA_MEMORY_USAGE_GPU_ONLY);
        }

        inline auto depthImageView(std::weak_ptr<Device> device, std::weak_ptr<Image> depthImage)
            -> std::shared_ptr<ImageView>
        {
            auto depthImageViewCreateInfo = vk::ImageViewCreateInfo{};
            depthImageViewCreateInfo.setImage(depthImage.lock()->get());
            depthImageViewCreateInfo.setViewType(vk::ImageViewType::e2D);
            depthImageViewCreateInfo.setFormat(vk::Format::eD32Sfloat);

            auto subresourceRange = vk::ImageSubresourceRange{};
            subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eDepth);
            subresourceRange.setBaseMipLevel(0);
            subresourceRange.setLevelCount(1);
            subresourceRange.setBaseArrayLayer(0);
            subresourceRange.setLayerCount(1);
            depthImageViewCreateInfo.setSubresourceRange(subresourceRange);

            return std::make_shared<ImageView>(device, depthImageViewCreateInfo);
        }

        inline auto transitionImageLayout(const vk::CommandBuffer &commandBuffer, vk::Image image,
                                          vk::ImageAspectFlags aspectMask, vk::ImageLayout oldLayout,
                                          vk::ImageLayout newLayout, vk::AccessFlags srcAccessMask,
                                          vk::AccessFlags dstAccessMask, vk::PipelineStageFlags srcStageMask,
                                          vk::PipelineStageFlags dstStageMask, uint32_t baseMipLevel = 0,
                                          uint32_t levelCount = 1, uint32_t baseArrayLayer = 0, uint32_t layerCount = 1)
            -> void
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

        inline auto transitionImageLayout(const vk::CommandBuffer &commandBuffer, std::weak_ptr<Image> image,
                                          vk::ImageAspectFlags aspectMask, vk::ImageLayout oldLayout,
                                          vk::ImageLayout newLayout, vk::AccessFlags srcAccessMask,
                                          vk::AccessFlags dstAccessMask, vk::PipelineStageFlags srcStageMask,
                                          vk::PipelineStageFlags dstStageMask, uint32_t baseMipLevel = 0,
                                          uint32_t levelCount = 1, uint32_t baseArrayLayer = 0, uint32_t layerCount = 1)
            -> void
        {
            vk::ImageMemoryBarrier barrier{};
            barrier.setOldLayout(oldLayout);
            barrier.setNewLayout(newLayout);
            barrier.setSrcQueueFamilyIndex(vk::QueueFamilyIgnored);
            barrier.setDstQueueFamilyIndex(vk::QueueFamilyIgnored);
            barrier.setImage(image.lock()->get());
            barrier.setSubresourceRange(
                vk::ImageSubresourceRange{aspectMask, baseMipLevel, levelCount, baseArrayLayer, layerCount});
            barrier.setSrcAccessMask(srcAccessMask);
            barrier.setDstAccessMask(dstAccessMask);

            commandBuffer.pipelineBarrier(srcStageMask, dstStageMask, {}, nullptr, nullptr, barrier);
        }
    } // namespace laconic
} // namespace letc
