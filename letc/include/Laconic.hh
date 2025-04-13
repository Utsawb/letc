#include "pch.hh"

namespace letc
{
    namespace laconic
    {
        inline vk::ImageCreateInfo DepthImageCreateInfo()
        {
            auto depthImageInfo = vk::ImageCreateInfo{};
            depthImageInfo.setImageType(vk::ImageType::e2D);
            depthImageInfo.setFormat(vk::Format::eD32Sfloat);
            depthImageInfo.setExtent({0, 0, 1});
            depthImageInfo.setMipLevels(1);
            depthImageInfo.setArrayLayers(1);
            depthImageInfo.setSamples(vk::SampleCountFlagBits::e1);
            depthImageInfo.setTiling(vk::ImageTiling::eOptimal);
            depthImageInfo.setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment);
            depthImageInfo.setSharingMode(vk::SharingMode::eExclusive);
            depthImageInfo.setQueueFamilyIndices(nullptr);
            depthImageInfo.setInitialLayout(vk::ImageLayout::eUndefined);
            return depthImageInfo;
        }

    } // namespace laconic
} // namespace letc
