#pragma once

#include "letc/pch.hh"

#include "letc/core/device.hh"
#include "letc/core/instance.hh"
#include "letc/core/window.hh"

namespace letc
{
    class Swapchain;

    class SwapchainBuilder
    {
      public:
        SwapchainBuilder();

        auto setPresentMode(const vk::PresentModeKHR &presentMode) -> SwapchainBuilder &;
        auto setFormat(const vk::SurfaceFormatKHR &format) -> SwapchainBuilder &;
        auto build(std::weak_ptr<Window> window, std::weak_ptr<Instance> instance, std::weak_ptr<Device> device)
            -> std::shared_ptr<Swapchain>;

      private:
        vk::PresentModeKHR m_presentMode;
        vk::SurfaceFormatKHR m_format;
    };

    class Swapchain
    {
      public:
        ~Swapchain();
        auto get() -> vk::SwapchainKHR &;
        auto getFormat() const -> vk::SurfaceFormatKHR;
        auto getExtent() const -> vk::Extent2D;
        auto getImageCount() const -> uint32_t;
        auto getImages() -> std::vector<vk::Image> &;
        auto getImageViews() -> std::vector<vk::ImageView> &;

      private:
        friend SwapchainBuilder;

        std::weak_ptr<Instance> m_instance;
        std::weak_ptr<Device> m_device;
        vk::SwapchainCreateInfoKHR m_info;
        vk::SurfaceFormatKHR m_format;
        vk::PresentModeKHR m_presentMode;
        vk::SurfaceKHR m_surface;
        vk::SurfaceCapabilitiesKHR m_surfaceCapabilites;
        vk::SwapchainKHR m_handle;
        std::vector<vk::Image> m_images;
        std::vector<vk::ImageView> m_views;
    };

} // namespace letc
