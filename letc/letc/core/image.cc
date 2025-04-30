#include "letc/core/image.hh"

namespace letc
{
    Image::Image(std::weak_ptr<Allocator> allocator, const vk::ImageCreateInfo &info, const VmaMemoryUsage &usage)
    {
        m_allocator = allocator;
        m_info = info;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = usage;

        vmaCreateImage(allocator.lock()->get(), reinterpret_cast<VkImageCreateInfo *>(&m_info), &allocInfo,
                       reinterpret_cast<VkImage *>(&m_image), &m_allocation, nullptr);
    }

    Image::~Image()
    {
        vmaDestroyImage(m_allocator.lock()->get(), m_image, m_allocation);
    }

    auto Image::get() -> vk::Image
    {
        return m_image;
    }

    auto Image::getInfo() -> vk::ImageCreateInfo
    {
        return m_info;
    }

    ImageView::ImageView(std::weak_ptr<Device> device, const vk::ImageViewCreateInfo &info)
    {
        m_device = device;
        m_info = info;

        m_view = device.lock()->getLogical().createImageView(m_info);
    }

    ImageView::~ImageView()
    {
        m_device.lock()->getLogical().destroyImageView(m_view);
    }

    auto ImageView::get() -> vk::ImageView
    {
        return m_view;
    }

    auto ImageView::getInfo() -> vk::ImageViewCreateInfo
    {
        return m_info;
    }

} // namespace letc
