#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "Allocator.hh"
#include "Device.hh"
#include "Pipeline.hh"
#include "Renderable.hh"
#include "pch.hh"

namespace letc
{
    struct RenderGraph
    {
        struct Resource
        {
            std::string id;

            enum Type
            {
                PersistentBuffer,
                PersistentImage,
                TransientBuffer,
                TransientImage
            };

            virtual void initialize(const Device &device, const Allocator &allocator) = 0;
            virtual void destroy(const Device &device, const Allocator &allocator) = 0;

            virtual ~Resource() = default;
        };

        struct PersistentBuffer : Resource
        {
            vk::Buffer buffer = nullptr;
            vk::DeviceSize size = {0};
            vk::BufferUsageFlags bufferUsage = {};
            VmaMemoryUsage memoryUsage = {};

            void initialize(const Device &device, const Allocator &allocator) override
            {
                return;
            }

            void destroy(const Device &device, const Allocator &allocator) override
            {
                return;
            }
        };

        struct PersistentImage : Resource
        {
            vk::Image image = nullptr;
            vk::Format format = {};
            vk::ImageLayout layout = {};

            void initialize(const Device &device, const Allocator &allocator) override
            {
                return;
            }

            void destroy(const Device &device, const Allocator &allocator) override
            {
                return;
            }
        };

        struct TransientBuffer : Resource
        {
            vk::Buffer buffer = nullptr;
            vk::DeviceSize size = {0};
            vk::BufferUsageFlags bufferUsage = {};
            VmaMemoryUsage memoryUsage = {};
            VmaAllocation allocation = nullptr;

            void initialize(const Device &device, const Allocator &allocator) override
            {
                vk::BufferCreateInfo bufferInfo = {};
                bufferInfo.size = size;
                bufferInfo.usage = bufferUsage;
                bufferInfo.sharingMode = vk::SharingMode::eExclusive;

                VmaAllocationCreateInfo allocCreateInfo = {};
                allocCreateInfo.usage = memoryUsage;

                vmaCreateBuffer(allocator.allocator, reinterpret_cast<VkBufferCreateInfo *>(&bufferInfo),
                                &allocCreateInfo, reinterpret_cast<VkBuffer *>(&buffer), &allocation, nullptr);
            }

            void destroy(const Device &device, const Allocator &allocator) override
            {
                vmaDestroyBuffer(allocator.allocator, buffer, allocation);
            }
        };

        struct TransientImage : Resource
        {
            vk::Image image = nullptr;
            vk::Format format = {};
            vk::ImageLayout layout = {};
            vk::ImageUsageFlags imageUsage = {};
            vk::Extent3D extent = {};
            VmaAllocation allocation = nullptr;

            void initialize(const Device &device, const Allocator &allocator) override
            {
                vk::ImageCreateInfo imageInfo = {};
                imageInfo.imageType = vk::ImageType::e2D;
                imageInfo.format = format;
                imageInfo.extent = extent;
                imageInfo.mipLevels = 1;
                imageInfo.arrayLayers = 1;
                imageInfo.samples = vk::SampleCountFlagBits::e1;
                imageInfo.tiling = vk::ImageTiling::eOptimal;
                imageInfo.usage = imageUsage;
                imageInfo.sharingMode = vk::SharingMode::eExclusive;
                imageInfo.initialLayout = vk::ImageLayout::eUndefined;

                VmaAllocationCreateInfo allocCreateInfo = {};
                allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

                vmaCreateImage(allocator.allocator, reinterpret_cast<VkImageCreateInfo *>(&imageInfo), &allocCreateInfo,
                               reinterpret_cast<VkImage *>(&image), &allocation, nullptr);
            }

            void destroy(const Device &device, const Allocator &allocator) override
            {
                vmaDestroyImage(allocator.allocator, image, allocation);
            }
        };

        struct Pass
        {
            std::string id;
            std::vector<std::string> resourceSrc;
            std::vector<std::string> resourceDst;

            virtual void setup(const RenderGraph& graph) = 0;
            virtual void execute(vk::CommandBuffer &commandBuffer) = 0;
            virtual ~Pass() = default;
        };

        struct Graphics : Pass
        {
            GraphicsPipeline &graphicsPipeline;
        };

        const Device &device;
        const Allocator &allocator;

        RenderGraph(const Device &device, const Allocator &allocator) : device(device), allocator(allocator) {};
    };
}; // namespace letc
