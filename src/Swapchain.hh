#pragma once

#ifndef LETC_SWAPCHAIN_HH
#define LETC_SWAPCHAIN_HH

#include "pch.hh"

namespace letc
{
    struct Swapchain
    {
        const vk::SurfaceKHR &surface;
        const vk::Device &device;

        vk::SurfaceCapabilitiesKHR capabilities;
        vk::SurfaceFormatKHR format;
        vk::PresentModeKHR presentMode;
        vk::SwapchainKHR swapchain;
        std::vector<vk::Image> images;
        std::vector<vk::ImageView> imageViews;

        operator const vk::SwapchainKHR &()
        {
            return swapchain;
        }

        Swapchain(const vkfw::Window &window, const vk::SurfaceKHR &surface, const vk::PhysicalDevice &physicalDevice,
                  const vk::Device &device)
            : surface(surface), device(device)
        {
            /*
                Surface -> Swapchain values
            */
            capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
            format = physicalDevice.getSurfaceFormatsKHR(surface).front();
            presentMode = vk::PresentModeKHR::eFifo;

            /*
                Swapchain
            */
            vk::SwapchainCreateInfoKHR swapchainCreateInfo{};
            swapchainCreateInfo.setSurface(surface);
            swapchainCreateInfo.setMinImageCount(capabilities.minImageCount);
            swapchainCreateInfo.setImageFormat(format.format);
            swapchainCreateInfo.setImageColorSpace(format.colorSpace);

            if (capabilities.currentExtent == vk::Extent2D{UINT32_MAX, UINT32_MAX})
            {
                swapchainCreateInfo.setImageExtent(
                    vk::Extent2D{(uint32_t)window.getWidth(), (uint32_t)window.getHeight()});
            }
            else
            {
                swapchainCreateInfo.setImageExtent(capabilities.currentExtent);
            }

            swapchainCreateInfo.setImageArrayLayers(1);
            swapchainCreateInfo.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);
            swapchainCreateInfo.setImageSharingMode(vk::SharingMode::eExclusive);
            swapchainCreateInfo.setPreTransform(capabilities.currentTransform);
            swapchainCreateInfo.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
            swapchainCreateInfo.setPresentMode(presentMode);
            swapchainCreateInfo.setClipped(VK_TRUE);
            swapchain = device.createSwapchainKHR(swapchainCreateInfo);

            /*
                Swapchain Images & Views
            */
            images = device.getSwapchainImagesKHR(swapchain);
            imageViews.reserve(images.size());
            for (const vk::Image &image : images)
            {
                vk::ImageViewCreateInfo imageViewCreateInfo{};
                imageViewCreateInfo.setImage(image);
                imageViewCreateInfo.setViewType(vk::ImageViewType::e2D);
                imageViewCreateInfo.setFormat(format.format);
                imageViewCreateInfo.setComponents(vk::ComponentMapping{});

                vk::ImageSubresourceRange imageSubresourceRange{};
                imageSubresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor);
                imageSubresourceRange.setBaseMipLevel(0);
                imageSubresourceRange.setLevelCount(1);
                imageSubresourceRange.setBaseArrayLayer(0);
                imageSubresourceRange.setLayerCount(1);
                imageViewCreateInfo.setSubresourceRange(imageSubresourceRange);

                imageViews.push_back(device.createImageView(imageViewCreateInfo));
            }
        }

        ~Swapchain()
        {
            for (const vk::ImageView &imageView : imageViews)
            {
                device.destroyImageView(imageView);
            }
            device.destroySwapchainKHR(swapchain);
        }
    };
}; // namespace letc

#endif // LETC_SWAPCHAIN_HH
