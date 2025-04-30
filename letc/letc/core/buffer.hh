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
            // Only attempt destruction if the buffer handle is valid (not moved from)
            if (m_buffer != nullptr)
            {
                // Lock the weak_ptr to get the allocator
                if (auto locked_allocator = m_allocator.lock())
                {
                    // If allocator is valid, destroy the buffer
                    vmaDestroyBuffer(locked_allocator->get(), m_buffer, m_allocation);
                }
                else
                {
                    // Optional: Log an error if the allocator expired before cleanup
                    // std::cerr << "Warning: Allocator expired before ObjectBuffer cleanup for buffer " << m_buffer <<
                    // std::endl;
                }
                // Prevent double deletion in case of exceptions during destruction
                m_buffer = nullptr;
                m_allocation = nullptr;
            }
        }
        ObjectBuffer(const ObjectBuffer &) = delete;
        ObjectBuffer &operator=(const ObjectBuffer &) = delete;

        ObjectBuffer(ObjectBuffer &&other) noexcept
            : m_allocator(std::move(other.m_allocator)), m_object(std::move(other.m_object)),
              m_buffer(std::exchange(other.m_buffer, nullptr)), m_allocation(std::exchange(other.m_allocation, nullptr))
        {
        }

        // 4. Move Assignment Operator
        ObjectBuffer &operator=(ObjectBuffer &&other) noexcept
        {
            // Prevent self-assignment
            if (this != &other)
            {
                if (auto locked_allocator = m_allocator.lock())
                {
                    if (m_buffer != nullptr)
                    {
                        vmaDestroyBuffer(locked_allocator->get(), m_buffer, m_allocation);
                    }
                }

                m_allocator = std::move(other.m_allocator);
                m_object = std::move(other.m_object);
                m_buffer = std::exchange(other.m_buffer, nullptr);
                m_allocation = std::exchange(other.m_allocation, nullptr);
            }
            return *this;
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

        vk::BufferCreateInfo m_bufferInfo = {};
        VmaAllocationCreateInfo m_allocInfo = {};

      public:
        VectorBuffer(std::weak_ptr<Allocator> allocator, const std::size_t &initialCount,
                     const vk::BufferUsageFlagBits &bufferUsage, const VmaMemoryUsage &memoryUsage,
                     const vk::SharingMode shareMode = vk::SharingMode::eExclusive)
            : m_allocator(allocator)
        {

            m_bufferInfo.size = initialCount == 0 ? 0 : std::bit_ceil(sizeof(Object) * initialCount);
            m_bufferInfo.usage = bufferUsage;
            m_bufferInfo.sharingMode = shareMode;

            m_allocInfo.usage = memoryUsage;

            if (initialCount != 0)
            {
                auto res =
                    vmaCreateBuffer(allocator.lock()->get(), reinterpret_cast<VkBufferCreateInfo *>(&m_bufferInfo),
                                    &m_allocInfo, reinterpret_cast<VkBuffer *>(&m_buffer), &m_allocation, nullptr);
                ATHROW(res == VK_SUCCESS, "failed to create buffer");
            }

            m_vector.resize(initialCount);
        }

        ~VectorBuffer()
        {
            // Only attempt destruction if the buffer handle is valid (not moved from)
            if (m_buffer != nullptr)
            {
                // Lock the weak_ptr to get the allocator
                if (auto locked_allocator = m_allocator.lock())
                {
                    // If allocator is valid, destroy the buffer
                    vmaDestroyBuffer(locked_allocator->get(), m_buffer, m_allocation);
                }
                else
                {
                    // Optional: Log an error if the allocator expired before cleanup
                    // std::cerr << "Warning: Allocator expired before VectorBuffer cleanup for buffer " << m_buffer <<
                    // std::endl;
                }
                // Prevent double deletion in case of exceptions during destruction
                m_buffer = nullptr;
                m_allocation = nullptr;
            }
        }
        VectorBuffer(const VectorBuffer &) = delete;
        VectorBuffer &operator=(const VectorBuffer &) = delete;

        // Move Constructor
        VectorBuffer(VectorBuffer &&other) noexcept
            : m_allocator(std::move(other.m_allocator)),                // Move weak_ptr
              m_vector(std::move(other.m_vector)),                      // Move vector contents
              m_buffer(std::exchange(other.m_buffer, nullptr)),         // Steal buffer handle
              m_allocation(std::exchange(other.m_allocation, nullptr)), // Steal allocation handle
              m_bufferInfo(other.m_bufferInfo),                         // Copy buffer info
              m_allocInfo(other.m_allocInfo)                            // Copy alloc info
        {
        }

        // Move Assignment Operator
        VectorBuffer &operator=(VectorBuffer &&other) noexcept
        {
            if (this != &other)
            {
                if (auto locked_allocator = m_allocator.lock())
                {
                    if (m_buffer != nullptr)
                    {
                        vmaDestroyBuffer(locked_allocator->get(), m_buffer, m_allocation);
                    }
                }

                m_allocator = std::move(other.m_allocator);
                m_vector = std::move(other.m_vector);
                m_buffer = std::exchange(other.m_buffer, nullptr);
                m_allocation = std::exchange(other.m_allocation, nullptr);
                m_bufferInfo = other.m_bufferInfo;
                m_allocInfo = other.m_allocInfo;
            }
            return *this;
        }
        auto sync() -> void
        {
            if (m_bufferInfo.size < (sizeof(Object) * m_vector.size()))
            {
                if (m_buffer != nullptr)
                    vmaDestroyBuffer(m_allocator.lock()->get(), m_buffer, m_allocation);
                m_bufferInfo.size = std::bit_ceil(sizeof(Object) * m_vector.size());
                auto res =
                    vmaCreateBuffer(m_allocator.lock()->get(), reinterpret_cast<VkBufferCreateInfo *>(&m_bufferInfo),
                                    &m_allocInfo, reinterpret_cast<VkBuffer *>(&m_buffer), &m_allocation, nullptr);
                ATHROW(res == VK_SUCCESS, "failed to create buffer");
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

        auto getContainer() -> std::vector<Object> &
        {
            return m_vector;
        }
    };

} // namespace letc
