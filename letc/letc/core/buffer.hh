#pragma once

#include "letc/pch.hh"

#include "letc/core/allocator.hh"

namespace letc
{
    template <typename Object> struct BufferObject
    {
      private:
        std::weak_ptr<Allocator> m_allocator;
        Object m_object;
        vk::Buffer m_buffer;
        VmaAllocation m_allocation;

      public:
        BufferObject(std::weak_ptr<Allocator> allocator, const vk::BufferUsageFlagBits &bufferUsage,
                     const VmaMemoryUsage &memoryUsage, const vk::SharingMode shareMode = vk::SharingMode::eExclusive)
        {
            m_allocator = allocator;
            vk::BufferCreateInfo bufferInfo{};
            bufferInfo.size = sizeof(Object);
            bufferInfo.usage = bufferUsage;
            bufferInfo.sharingMode = shareMode;

            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.usage = memoryUsage;

            vmaCreateBuffer(allocator.lock()->get(), reinterpret_cast<VkBufferCreateInfo *>(&bufferInfo), &allocInfo,
                            reinterpret_cast<VkBuffer *>(&m_buffer), &m_allocation, nullptr);
        }

        ~BufferObject()
        {
            vmaDestroyBuffer(m_allocator.lock()->get(), m_buffer, m_allocation);
        }

        auto sync() -> void
        {
            void *dva = nullptr;
            vmaMapMemory(m_allocator.lock()->get(), m_allocation, &dva);
            std::memcpy(dva, &m_object, sizeof(Object));
            vmaUnmapMemory(m_allocator.lock()->get(), m_allocation);
        }

        auto get() -> vk::Buffer
        {
            return m_buffer;
        }

        auto operator->() -> Object *
        {
            return &m_object;
        }

        auto operator->() const -> const Object *
        {
            return &m_object;
        }
    };

} // namespace letc
