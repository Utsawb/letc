#include "letc/core/swapchain.hh"
#include <limits>

namespace letc
{
    SwapchainBuilder::SwapchainBuilder()
    {
        m_presentMode = vk::PresentModeKHR::eFifo;
        m_format = vk::Format::eR8G8B8A8Srgb;
    }

    auto SwapchainBuilder::setPresentMode(const vk::PresentModeKHR &presentMode) -> SwapchainBuilder &
    {
        m_presentMode = presentMode;
        return *this;
    }

    auto SwapchainBuilder::setFormat(const vk::SurfaceFormatKHR &format) -> SwapchainBuilder &
    {
        m_format = format;
        return *this;
    }

    auto SwapchainBuilder::build(std::weak_ptr<Window> window, std::weak_ptr<Instance> instance,
                                 std::weak_ptr<Device> device) -> std::shared_ptr<Swapchain>
    {
        auto swapchain = std::make_shared<Swapchain>();
        swapchain->m_instance = instance;
        swapchain->m_device = device;
        swapchain->m_surface = window.lock()->createSurface(instance);
        swapchain->m_surfaceCapabilites = device.lock()->getPhysical().getSurfaceCapabilitiesKHR(swapchain->m_surface);

        auto swapchainInfo = vk::SwapchainCreateInfoKHR{}
                                 .setPresentMode(m_presentMode)
                                 .setClipped(true)
                                 .setSurface(swapchain->m_surface)
                                 .setImageFormat(m_format.format)
                                 .setImageColorSpace(m_format.colorSpace)
                                 .setMinImageCount(swapchain->m_surfaceCapabilites.minImageCount + 1)
                                 .setImageArrayLayers(1)
                                 .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
                                 .setImageSharingMode(vk::SharingMode::eExclusive)
                                 .setPreTransform(swapchain->m_surfaceCapabilites.currentTransform)
                                 .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);

        if (swapchain->m_surfaceCapabilites.currentExtent ==
            vk::Extent2D{std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::max()})
        {
            swapchainInfo.setImageExtent(vk::Extent2D{static_cast<uint32_t>(window.lock()->get().getWidth()),
                                                      static_cast<uint32_t>(window.lock()->get().getWidth())});
        }
        else
        {
            swapchainInfo.setImageExtent(swapchain->m_surfaceCapabilites.currentExtent);
        }

        swapchain->m_handle = device.lock()->getLogical().createSwapchainKHR(swapchainInfo);

        return swapchain;
    }

    Swapchain::~Swapchain()
    {
        m_device.lock()->getLogical().destroySwapchainKHR(m_handle);
        m_instance.lock()->get().destroySurfaceKHR(m_surface);
    }

} // namespace letc
