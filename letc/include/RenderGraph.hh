#pragma once

#include "Allocator.hh"
#include "Device.hh"
#include "Pipeline.hh"
#include "Renderable.hh"
#include "pch.hh"
#include <cassert>
#include <format>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace letc
{
    struct RenderGraph
    {
        struct Resource
        {
            std::string id;
            virtual void initialize(const Device &device, const Allocator &allocator) = 0;
            virtual void destroy(const Device &device, const Allocator &allocator) = 0;
            virtual ~Resource() = default;
        };

        struct PersistentBuffer : Resource
        {
            vk::Buffer buffer = nullptr;
            vk::DeviceSize size = {0};

            void initialize(const Device &device, const Allocator &allocator) override
            {
            }

            void destroy(const Device &device, const Allocator &allocator) override
            {
            }
        };

        struct PersistentImage : Resource
        {
            vk::Image image = nullptr;
            vk::Format format = {};
            vk::ImageLayout layout = {};

            void initialize(const Device &device, const Allocator &allocator) override
            {
            }

            void destroy(const Device &device, const Allocator &allocator) override
            {
            }
        };

        struct PersistentImageView : Resource
        {
            std::string imageId;
            vk::ImageView imageView = nullptr;

            void initialize(const Device &device, const Allocator &allocator) override
            {
            }

            void destroy(const Device &device, const Allocator &allocator) override
            {
            }
        };

        struct TransientBuffer : Resource
        {
            vk::Buffer buffer = nullptr;
            vk::BufferCreateInfo bufferInfo = {};
            VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
            VmaAllocation allocation = nullptr;

            void initialize(const Device &device, const Allocator &allocator) override
            {
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
            vk::ImageCreateInfo imageInfo = {};
            VmaAllocation allocation = nullptr;

            void initialize(const Device &device, const Allocator &allocator) override
            {
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

        struct TransientImageView : Resource
        {
            std::string imageId;
            vk::ImageView imageView = nullptr;
            vk::ImageViewCreateInfo viewCreateInfo = {};

            void initialize(const Device &device, const Allocator &allocator) override
            {
                imageView = device.device.createImageView(viewCreateInfo);
            }

            void destroy(const Device &device, const Allocator &allocator) override
            {
                device.device.destroyImageView(imageView);
            }
        };

        struct Pass
        {
            std::string id;
            // Map binding slot -> (sub-binding slot -> resource id)
            std::map<uint32_t, std::map<uint32_t, std::string>> resourceBindings;
            std::unordered_map<std::string, std::variant<vk::BufferMemoryBarrier, vk::ImageMemoryBarrier>>
                resourceBarriers;

            virtual void setup(const RenderGraph &graph) = 0;
            virtual void execute(vk::CommandBuffer &commandBuffer) = 0;
            virtual ~Pass() = default;
        };

        struct Graphics : Pass
        {
            const GraphicsPipeline *graphicsPipeline = nullptr;
            std::vector<IRenderable *> renderables;

            Graphics(const std::string id, const GraphicsPipeline *graphicsPipeline)
                : graphicsPipeline(graphicsPipeline)
            {
                this->id = id;
            }

            void setup(const RenderGraph &graph) override
            {
                // Setup render pass resources, viewports, etc.
            }

            void execute(vk::CommandBuffer &commandBuffer) override
            {
                // Bind pipeline and draw renderables.
            }
        };

        const Device &device;
        const Allocator &allocator;
        std::unordered_map<std::string, std::unique_ptr<Resource>> resources;
        std::unordered_map<std::string, std::unique_ptr<Pass>> passes;

        RenderGraph(const Device &device, const Allocator &allocator) : device(device), allocator(allocator)
        {
        }

        // Add a persistent buffer resource created externally.
        RenderGraph &addPersistentBuffer(const std::string &id, const vk::Buffer &buffer, const vk::DeviceSize &size)
        {
            // Check for duplicate id.
            assert(resources.find(id) == resources.end() && "duplicate persistent buffer id");
            auto pb = std::make_unique<PersistentBuffer>();
            pb->id = id;
            pb->buffer = buffer;
            pb->size = size;
            resources.emplace(id, std::move(pb));
            return *this;
        }

        // Add a persistent image resource created externally.
        RenderGraph &addPersistentImage(const std::string &id, const vk::Image &image)
        {
            assert(resources.find(id) == resources.end() && "duplicate persistent image id");
            auto pi = std::make_unique<PersistentImage>();
            pi->id = id;
            pi->image = image;
            resources.emplace(id, std::move(pi));
            return *this;
        }

        // Add a transient buffer resource that the render graph will create/destroy.
        RenderGraph &addTransientBuffer(const std::string &id, const vk::DeviceSize &size,
                                        const vk::BufferUsageFlags &bufferUsage,
                                        const VmaMemoryUsage &memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU)
        {
            assert(resources.find(id) == resources.end() && "duplicate transient buffer id");
            auto tb = std::make_unique<TransientBuffer>();
            tb->id = id;
            tb->bufferInfo =
                vk::BufferCreateInfo{}.setSize(size).setUsage(bufferUsage).setSharingMode(vk::SharingMode::eExclusive);
            tb->memoryUsage = memoryUsage;
            resources.emplace(id, std::move(tb));
            return *this;
        }

        // Add a transient image resource.
        // Note: We extend the API to take an id and image create info.
        RenderGraph &addTransientImage(const std::string &id, const vk::ImageCreateInfo &imageCreateInfo)
        {
            assert(resources.find(id) == resources.end() && "duplicate transient image id");
            auto ti = std::make_unique<TransientImage>();
            ti->id = id;
            ti->imageInfo = imageCreateInfo;
            resources.emplace(id, std::move(ti));
            return *this;
        }

        // Add a graphics pass.
        RenderGraph &addGraphicsPass(const std::string &id, const GraphicsPipeline *graphicsPipeline)
        {
            assert(passes.find(id) == passes.end() && "duplicate pass id");
            passes.emplace(id, std::make_unique<Graphics>(id, graphicsPipeline));
            return *this;
        }

        RenderGraph &addPersistentImageView(const std::string &id, const std::string &sourceImageId,
                                            const vk::ImageView &view)
        {
            assert(resources.find(id) == resources.end() && "duplicate persistent image view id");
            auto piv = std::make_unique<PersistentImageView>();
            piv->id = id;
            piv->imageId = sourceImageId;
            piv->imageView = view;
            resources.emplace(id, std::move(piv));
            return *this;
        }

        // Add a transient image view.
        RenderGraph &addTransientImageView(const std::string &id, const std::string &srcImageId,
                                           const vk::ImageViewCreateInfo &viewCreateInfo)
        {
            assert(resources.find(id) == resources.end() && "duplicate transient image view id");
            auto tiv = std::make_unique<TransientImageView>();
            tiv->id = id;
            tiv->imageId = srcImageId;
            tiv->viewCreateInfo = viewCreateInfo;
            resources.emplace(id, std::move(tiv));
            return *this;
        }

        // Optional: Initialize all resources. Typically called before executing passes.
        void initializeResources()
        {
            for (auto &[id, resource] : resources)
            {
                resource->initialize(device, allocator);
            }
        }

        // Optional: Execute all passes. In a real-world scenario you might need to order these.
        void execute(vk::CommandBuffer &commandBuffer)
        {
            // Setup each pass if needed.
            for (auto &[id, pass] : passes)
            {
                pass->setup(*this);
            }
            // Execute passes.
            for (auto &[id, pass] : passes)
            {
                pass->execute(commandBuffer);
            }
        }

        ~RenderGraph()
        {
            // When the RenderGraph is destroyed, all resources are destroyed.
            for (auto &[id, resource] : resources)
            {
                resource->destroy(device, allocator);
            }
        }
    };
}; // namespace letc
