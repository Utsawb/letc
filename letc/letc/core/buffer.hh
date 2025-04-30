#pragma once

#include "letc/pch.hh"

#include "letc/core/allocator.hh"

namespace letc
{
    template <typename Object> struct ObjectBuffer
    {
      private:
        std::weak_ptr<Allocator> m_allocator;
        Object m_object;
        vk::Buffer m_buffer;
        VmaAllocation m_allocation;

      public:
        ObjectBuffer(std::weak_ptr<Allocator> allocator, const vk::BufferUsageFlagBits &bufferUsage,
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

        ~ObjectBuffer()
        {
            vmaDestroyBuffer(m_allocator.lock()->get(), m_buffer, m_allocation);
        }

        auto containedSize() -> std::size_t
        {
            return sizeof(Object);
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

    template <typename Object> class VectorBuffer
    {
      private:
        std::weak_ptr<Allocator> m_allocator;
        std::vector<Object> m_vector;

        vk::Buffer m_buffer = nullptr;
        VmaAllocation m_allocation = nullptr;

        vk::BufferCreateInfo m_bufferInfo;
        VmaAllocationCreateInfo m_allocInfo;

      public:
        VectorBuffer(std::weak_ptr<Allocator> allocator, const std::size_t &initialCount,
                     const vk::BufferUsageFlagBits &bufferUsage, const VmaMemoryUsage &memoryUsage,
                     const vk::SharingMode shareMode = vk::SharingMode::eExclusive)
            : m_allocator(allocator)
        {

            m_bufferInfo.size = std::bit_ceil(sizeof(Object) * initialCount);
            m_bufferInfo.usage = bufferUsage;
            m_bufferInfo.sharingMode = shareMode;

            m_allocInfo.usage = memoryUsage;

            if (initialCount != 0)
            {
                vmaCreateBuffer(allocator.lock()->get(), reinterpret_cast<VkBufferCreateInfo *>(&m_bufferInfo),
                                &m_allocInfo, reinterpret_cast<VkBuffer *>(&m_buffer), &m_allocation, nullptr);
            }

            m_vector.resize(initialCount);
        }

        ~VectorBuffer()
        {
            if (m_buffer != nullptr)
                vmaDestroyBuffer(m_allocator.lock()->get(), m_buffer, m_allocation);
        }

        auto sync() -> void
        {
            if (m_bufferInfo.size < (sizeof(Object) * m_vector.size()))
            {
                if (m_buffer != nullptr)
                    vmaDestroyBuffer(m_allocator.lock()->get(), m_buffer, m_allocation);
                m_bufferInfo.size = std::bit_ceil(sizeof(Object) * m_vector.size());
                vmaCreateBuffer(m_allocator.lock()->get(), reinterpret_cast<VkBufferCreateInfo *>(&m_bufferInfo),
                                &m_allocInfo, reinterpret_cast<VkBuffer *>(&m_buffer), &m_allocation, nullptr);
            }

            void *dva = nullptr;
            vmaMapMemory(m_allocator.lock()->get(), m_allocation, &dva);
            std::memcpy(dva, m_vector.data(), sizeof(Object) * m_vector.size());
            vmaUnmapMemory(m_allocator.lock()->get(), m_allocation);
        }

        auto containedSize() -> std::size_t
        {
            return sizeof(Object) * m_vector.size();
        }

        auto get() -> vk::Buffer
        {
            return m_buffer;
        }

        auto operator->() -> std::vector<Object> *
        {
            return &m_vector;
        }

        auto operator->() const -> const std::vector<Object> *
        {
            return &m_vector;
        }
    };

} // namespace letc
