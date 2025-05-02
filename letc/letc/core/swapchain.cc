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
        auto surface = window.lock()->createSurface(instance);
        auto surfaceCapabilites = device.lock()->getPhysical().getSurfaceCapabilitiesKHR(surface);

        auto surfaceFormats = device.lock()->getPhysical().getSurfaceFormatsKHR(surface);
        vk::SurfaceFormatKHR selectedFormat = surfaceFormats[0];
        for (const auto &availableFormat : surfaceFormats)
        {
            if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
                availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            {
                selectedFormat = availableFormat;
                break;
            }
            if (availableFormat.format == vk::Format::eR8G8B8A8Srgb &&
                availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            {
                selectedFormat = availableFormat;
            }
        }
        m_format = selectedFormat;

        auto swapchainInfo =
            vk::SwapchainCreateInfoKHR{}
                .setPresentMode(m_presentMode)
                .setClipped(true)
                .setSurface(surface)
                .setImageFormat(m_format.format)
                .setImageColorSpace(m_format.colorSpace)
                .setMinImageCount(surfaceCapabilites.minImageCount + 1)
                .setImageArrayLayers(1)
                .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst)
                .setImageSharingMode(vk::SharingMode::eExclusive)
                .setPreTransform(surfaceCapabilites.currentTransform)
                .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);

        if (surfaceCapabilites.currentExtent ==
            vk::Extent2D{std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::max()})
        {
            swapchainInfo.setImageExtent(vk::Extent2D{static_cast<uint32_t>(window.lock()->get().getWidth()),
                                                      static_cast<uint32_t>(window.lock()->get().getWidth())});
        }
        else
        {
            swapchainInfo.setImageExtent(surfaceCapabilites.currentExtent);
        }

        auto handle = device.lock()->getLogical().createSwapchainKHR(swapchainInfo);
        auto images = device.lock()->getLogical().getSwapchainImagesKHR(handle);
        auto views = std::vector<vk::ImageView>{};
        views.reserve(images.size());
        std::ranges::for_each(images, [this, &views, &device](const vk::Image &img) {
            auto imgInfo = vk::ImageViewCreateInfo{}
                               .setImage(img)
                               .setViewType(vk::ImageViewType::e2D)
                               .setFormat(m_format.format)
                               .setSubresourceRange(vk::ImageSubresourceRange{}
                                                        .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                        .setBaseMipLevel(0)
                                                        .setLevelCount(1)
                                                        .setBaseArrayLayer(0)
                                                        .setLayerCount(1));
            views.push_back(device.lock()->getLogical().createImageView(imgInfo));
        });

        auto swapchain = std::make_shared<Swapchain>();
        swapchain->m_instance = instance;
        swapchain->m_device = device;
        swapchain->m_info = swapchainInfo;
        swapchain->m_format = m_format;
        swapchain->m_presentMode = m_presentMode;
        swapchain->m_surface = surface;
        swapchain->m_surfaceCapabilites = surfaceCapabilites;
        swapchain->m_handle = handle;
        swapchain->m_images = images;
        swapchain->m_views = views;
        return swapchain;
    }

    Swapchain::~Swapchain()
    {
        for (const auto &view : m_views)
        {
            m_device.lock()->getLogical().destroyImageView(view);
        }

        m_device.lock()->getLogical().destroySwapchainKHR(m_handle);
        m_instance.lock()->get().destroySurfaceKHR(m_surface);
    }

    auto Swapchain::get() -> vk::SwapchainKHR &
    {
        return m_handle;
    }

    auto Swapchain::getFormat() const -> vk::SurfaceFormatKHR
    {
        return m_format;
    }

    auto Swapchain::getExtent() const -> vk::Extent2D
    {
        return m_info.imageExtent;
    }
    auto Swapchain::getImageCount() const -> uint32_t
    {
        return m_images.size();
    }
    auto Swapchain::getImages() -> std::vector<vk::Image> &
    {
        return m_images;
    }
    auto Swapchain::getImageViews() -> std::vector<vk::ImageView> &
    {
        return m_views;
    }

} // namespace letc
