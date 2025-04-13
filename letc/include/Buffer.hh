#pragma once

#ifndef LETC_BUFFER_HH
#define LETC_BUFFER_HH

#include "pch.hh"

#include "Allocator.hh"

namespace letc
{
    struct Buffer
    {
        const Allocator &allocator;
        vk::Buffer buffer;
        VmaAllocation allocation;

        Buffer(const Allocator &allocator, const vk::DeviceSize &size, const vk::BufferUsageFlagBits &bufferUsage,
               const VmaMemoryUsage &memoryUsage, const vk::SharingMode shareMode = vk::SharingMode::eExclusive)
            : allocator(allocator)
        {
            vk::BufferCreateInfo bufferCreateInfo{};
            bufferCreateInfo.size = size;
            bufferCreateInfo.usage = bufferUsage;
            bufferCreateInfo.sharingMode = shareMode;

            VmaAllocationCreateInfo allocCreateInfo = {};
            allocCreateInfo.usage = memoryUsage;

            assertThrow(vmaCreateBuffer(allocator.allocator, reinterpret_cast<VkBufferCreateInfo *>(&bufferCreateInfo),
                                        &allocCreateInfo, reinterpret_cast<VkBuffer *>(&buffer), &allocation,
                                        nullptr) == VK_SUCCESS,
                        "failed to create buffer");
        }

        void cpy(const void *data, const vk::DeviceSize &size, const vk::DeviceSize offset = 0)
        {
            void *gpuPtr;
            vmaMapMemory(allocator.allocator, allocation, &gpuPtr);
            std::memcpy(static_cast<char *>(gpuPtr) + offset, data, size);
            vmaUnmapMemory(allocator.allocator, allocation);
        }

        ~Buffer()
        {
            vmaDestroyBuffer(allocator.allocator, buffer, allocation);
        }
    };

    template <typename T> struct BufferObject
    {
        const Allocator &allocator;
        T object;
        vk::Buffer buffer;
        VmaAllocation allocation;

        BufferObject(const Allocator &allocator, const vk::BufferUsageFlagBits &bufferUsage,
                     const VmaMemoryUsage &memoryUsage, const vk::SharingMode shareMode = vk::SharingMode::eExclusive)
            : allocator(allocator)
        {
            vk::BufferCreateInfo bufferCreateInfo{};
            bufferCreateInfo.size = sizeof(T);
            bufferCreateInfo.usage = bufferUsage;
            bufferCreateInfo.sharingMode = shareMode;

            VmaAllocationCreateInfo allocCreateInfo = {};
            allocCreateInfo.usage = memoryUsage;

            assertThrow(vmaCreateBuffer(allocator.allocator, reinterpret_cast<VkBufferCreateInfo *>(&bufferCreateInfo),
                                        &allocCreateInfo, reinterpret_cast<VkBuffer *>(&buffer), &allocation,
                                        nullptr) == VK_SUCCESS,
                        "failed to create buffer");
        }

        void sync()
        {
            void *deviceAddress = nullptr;
            vmaMapMemory(allocator.allocator, allocation, &deviceAddress);
            std::memcpy(deviceAddress, &object, sizeof(T));
            vmaUnmapMemory(allocator.allocator, allocation);
        }

        ~BufferObject()
        {
            vmaDestroyBuffer(allocator.allocator, buffer, allocation);
        }
    };

    template <typename T, std::size_t N> struct BufferArray
    {
        const Allocator &allocator;
        std::array<T, N> array;
        vk::Buffer buffer;
        VmaAllocation allocation;

        BufferArray(const Allocator &allocator, const vk::BufferUsageFlagBits &bufferUsage,
                    const VmaMemoryUsage &memoryUsage, const vk::SharingMode shareMode = vk::SharingMode::eExclusive)
            : allocator(allocator)
        {
            vk::BufferCreateInfo bufferCreateInfo{};
            bufferCreateInfo.size = sizeof(T) * N;
            bufferCreateInfo.usage = bufferUsage;
            bufferCreateInfo.sharingMode = shareMode;

            VmaAllocationCreateInfo allocCreateInfo = {};
            allocCreateInfo.usage = memoryUsage;

            assertThrow(vmaCreateBuffer(allocator.allocator, reinterpret_cast<VkBufferCreateInfo *>(&bufferCreateInfo),
                                        &allocCreateInfo, reinterpret_cast<VkBuffer *>(&buffer), &allocation,
                                        nullptr) == VK_SUCCESS,
                        "failed to create buffer");
        }

        void sync()
        {
            void *deviceAddress = nullptr;
            vmaMapMemory(allocator.allocator, allocation, &deviceAddress);
            std::memcpy(deviceAddress, array.data(), N * sizeof(T));
            vmaUnmapMemory(allocator.allocator, allocation);
        }

        ~BufferArray()
        {
            vmaDestroyBuffer(allocator.allocator, buffer, allocation);
        }
    };

    template <typename T> struct BufferVector
    {
        const Allocator &allocator;
        std::vector<T> vector;
        vk::Buffer buffer;
        VmaAllocation allocation;

        vk::BufferUsageFlagBits bufferUsage;
        VmaMemoryUsage memoryUsage;
        vk::SharingMode shareMode;
        vk::DeviceSize allocatedSize;

        BufferVector(const Allocator &allocator, const vk::DeviceSize &size, const vk::BufferUsageFlagBits &bufferUsage,
                     const VmaMemoryUsage &memoryUsage, const vk::SharingMode shareMode = vk::SharingMode::eExclusive)
            : allocator(allocator), bufferUsage(bufferUsage), memoryUsage(memoryUsage), shareMode(shareMode)
        {
            vk::BufferCreateInfo bufferCreateInfo{};
            allocatedSize = size * sizeof(T);
            bufferCreateInfo.size = allocatedSize;
            bufferCreateInfo.usage = bufferUsage;
            bufferCreateInfo.sharingMode = shareMode;

            VmaAllocationCreateInfo allocCreateInfo = {};
            allocCreateInfo.usage = memoryUsage;

            vector.resize(size);

            assertThrow(vmaCreateBuffer(allocator.allocator, reinterpret_cast<VkBufferCreateInfo *>(&bufferCreateInfo),
                                        &allocCreateInfo, reinterpret_cast<VkBuffer *>(&buffer), &allocation,
                                        nullptr) == VK_SUCCESS,
                        "failed to create buffer");
        }

        void sync()
        {
            const vk::DeviceSize newSize = vector.size() * sizeof(T);

            if (newSize != allocatedSize)
            {
                vmaDestroyBuffer(allocator.allocator, buffer, allocation);

                vk::BufferCreateInfo bufferCreateInfo{};
                bufferCreateInfo.size = newSize;
                bufferCreateInfo.usage = bufferUsage;
                bufferCreateInfo.sharingMode = shareMode;

                VmaAllocationCreateInfo allocCreateInfo = {};
                allocCreateInfo.usage = memoryUsage;

                assertThrow(vmaCreateBuffer(allocator.allocator,
                                            reinterpret_cast<VkBufferCreateInfo *>(&bufferCreateInfo), &allocCreateInfo,
                                            reinterpret_cast<VkBuffer *>(&buffer), &allocation, nullptr) == VK_SUCCESS,
                            "failed to create buffer");

                allocatedSize = newSize;
            }

            void *deviceAddress = nullptr;
            vmaMapMemory(allocator.allocator, allocation, &deviceAddress);
            std::memcpy(deviceAddress, vector.data(), newSize);
            vmaUnmapMemory(allocator.allocator, allocation);
        }

        ~BufferVector()
        {
            vmaDestroyBuffer(allocator.allocator, buffer, allocation);
        }
    };

    template <typename T> struct ImageBuffer
    {
        std::vector<T> m_cpuBuffer;
        vk::Image m_gpuImage;
        vk::ImageUsageFlags m_usage;
        vk::Format m_format;
        uint32_t m_width, m_height;
        vk::ImageTiling m_tiling;
        const VmaAllocator &m_allocator;
        VmaAllocation m_allocation;

        ImageBuffer(const VmaAllocator &allocator, uint32_t width, uint32_t height, vk::Format format,
                    const std::vector<T> &cpuBuffer, vk::ImageUsageFlags usage,
                    vk::ImageTiling tiling = vk::ImageTiling::eOptimal)
            : m_cpuBuffer(cpuBuffer), m_usage(usage), m_format(format), m_width(width), m_height(height),
              m_tiling(tiling), m_allocator(allocator)
        {
            vk::ImageCreateInfo imageCreateInfo{};
            imageCreateInfo.imageType = vk::ImageType::e2D;
            imageCreateInfo.extent = vk::Extent3D{width, height, 1};
            imageCreateInfo.mipLevels = 1;
            imageCreateInfo.arrayLayers = 1;
            imageCreateInfo.format = format;
            imageCreateInfo.tiling = tiling;
            imageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
            imageCreateInfo.usage = usage;
            imageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
            imageCreateInfo.samples = vk::SampleCountFlagBits::e1;

            VmaAllocationCreateInfo allocCreateInfo = {};
            // If the image is created with linear tiling, we allow CPU mapping.
            // Otherwise (optimal tiling), use GPU-only memory.
            allocCreateInfo.usage =
                (tiling == vk::ImageTiling::eLinear) ? VMA_MEMORY_USAGE_CPU_TO_GPU : VMA_MEMORY_USAGE_GPU_ONLY;

            assertThrow(vmaCreateImage(m_allocator, reinterpret_cast<VkImageCreateInfo *>(&imageCreateInfo),
                                       &allocCreateInfo, reinterpret_cast<VkImage *>(&m_gpuImage), &m_allocation,
                                       nullptr) == VK_SUCCESS,
                        "failed to create image");
        }

        void syncImage()
        {
            assertThrow(m_tiling == vk::ImageTiling::eLinear, "syncImage only supported for linear tiled images");
            void *gpuPtr;
            vmaMapMemory(m_allocator, m_allocation, &gpuPtr);
            std::memcpy(gpuPtr, m_cpuBuffer.data(), m_cpuBuffer.size() * sizeof(T));
            vmaUnmapMemory(m_allocator, m_allocation);
        }

        ~ImageBuffer()
        {
            vmaDestroyImage(m_allocator, m_gpuImage, m_allocation);
        }

        ImageBuffer(const ImageBuffer &other) = delete;
        ImageBuffer &operator=(const ImageBuffer &other) = delete;
    };
}; // namespace letc

#endif // LETC_BUFFER_HH
