#pragma once

#include "pch.hh"

#include "Allocator.hh"
#include "Buffer.hh"
#include "Descriptor.hh"
#include "Device.hh"
#include "Pipeline.hh"

namespace letc
{
    struct FrenetFramesRenderer
    {
        const Device &device;
        const Allocator &allocator;

        std::unique_ptr<DescriptorLayout> layout;
        std::unique_ptr<GraphicsPipeline> pipeline;

        FrenetFramesRenderer(const Allocator &allocator, const Device &device, const std::vector<char> &code)
            : allocator(allocator), device(device)
        {
            layout = std::make_unique<DescriptorLayout>(device);
            layout->addBinding(0, 0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eAllGraphics, 1);
            layout->generateLayouts();

            letc::GraphicsPipelineBuilder gpb;
            gpb.addShaderStage(code, vk::ShaderStageFlagBits::eVertex, "vs");
            gpb.addShaderStage(code, vk::ShaderStageFlagBits::eFragment, "fs");

            gpb.addVertexInputBinding(0, sizeof(glm::mat4), vk::VertexInputRate::eInstance);
            gpb.addVertexInputAttribute(0, 1, vk::Format::eR32G32B32A32Sfloat, 0 * sizeof(glm::vec4));
            gpb.addVertexInputAttribute(0, 2, vk::Format::eR32G32B32A32Sfloat, 1 * sizeof(glm::vec4));
            gpb.addVertexInputAttribute(0, 3, vk::Format::eR32G32B32A32Sfloat, 2 * sizeof(glm::vec4));
            gpb.addVertexInputAttribute(0, 4, vk::Format::eR32G32B32A32Sfloat, 3 * sizeof(glm::vec4));

            auto fmt = vk::Format::eR8G8B8A8Srgb;
            gpb.setLayout(layout.get());
            gpb.inputAssemblyInfo.setTopology(vk::PrimitiveTopology::eLineList);
            gpb.rasterizationInfo.setPolygonMode(vk::PolygonMode::eLine);
            gpb.renderingInfo.setColorAttachmentCount(1);
            gpb.renderingInfo.setColorAttachmentFormats(fmt);
            gpb.rasterizationInfo.setCullMode(vk::CullModeFlagBits::eNone);
            pipeline = std::make_unique<GraphicsPipeline>(device, gpb);
        }
    };

    struct FrenetFrame
    {
        const Allocator &allocator;

        std::size_t capacity = 1024;

        std::vector<glm::mat4> frames;
        std::unique_ptr<letc::Buffer> buffer;

        FrenetFrame(const letc::Allocator &allocator) : allocator(allocator)
        {
            frames.reserve(capacity);
            buffer =
                std::make_unique<letc::Buffer>(allocator, capacity * sizeof(glm::mat4),
                                               vk::BufferUsageFlagBits::eVertexBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
        }

        void cpy()
        {
            if (frames.size() > capacity)
            {
                capacity = std::bit_ceil(frames.size() * 2);
                buffer.reset();
                buffer =
                    std::make_unique<letc::Buffer>(allocator, capacity * sizeof(glm::mat4),
                                                   vk::BufferUsageFlagBits::eVertexBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
            }

            buffer->cpy(frames.data(), frames.size() * sizeof(glm::mat4));
        }

        void draw(const vk::CommandBuffer &commandBuffer)
        {
            if (frames.empty())
                return;

            vk::DeviceSize offset = 0;
            commandBuffer.bindVertexBuffers(0, 1, &buffer->buffer, &offset);
            commandBuffer.drawIndexed(6, frames.size(), 0, 0, 0);
        }
    };
}; // namespace letc
