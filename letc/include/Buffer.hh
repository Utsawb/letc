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
}; // namespace letc

#endif // LETC_BUFFER_HH
