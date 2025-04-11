#pragma once

#include "pch.hh"

#include "Allocator.hh"
#include "Descriptor.hh"
#include "Pipeline.hh"
#include "Swapchain.hh"
#include "Material.hh"
#include "Device.hh"
#include "Buffer.hh"

// i have gotten very hacky, yes this is hacky
// but i need results and as some guy once said,
// make it work then make it fast
// but instead of make it fast ill make it readable later
// honestly the Renderable interface is a huge mental block
// i cant think through so ill just hack around it until
// i can architect a better solution
namespace letc
{
    struct PointsRenderer
    {
        const Allocator &allocator;
        const Device &device;

        std::unique_ptr<DescriptorLayout> layout;
        std::unique_ptr<Material> material;
        std::unique_ptr<GraphicsPipeline> pipeline;

        PointsRenderer(const Allocator &allocator, const Device &device, const Swapchain &swapchain, const std::vector<char> &vertexCode, const std::vector<char> &fragmentCode)
            : allocator(allocator), device(device)
        {
            layout = std::make_unique<DescriptorLayout>(device);
            layout->addBinding(0, 0, vk::DescriptorType::eUniformBuffer,
                               vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 1);
            layout->generateLayouts();

            material = std::make_unique<Material>(device, allocator, *layout); // add camera later?

            letc::GraphicsPipelineBuilder gpb;
            gpb.addShaderStage(vertexCode, vk::ShaderStageFlagBits::eVertex);
            gpb.addShaderStage(fragmentCode, vk::ShaderStageFlagBits::eFragment);
            gpb.addVertexInputBinding(0, sizeof(glm::vec3), vk::VertexInputRate::eVertex);
            gpb.addVertexInputAttribute(0, 0, vk::Format::eR32G32B32Sfloat, 0);
            gpb.setLayout(layout.get());
            gpb.inputAssemblyInfo.setTopology(vk::PrimitiveTopology::ePointList);
            gpb.rasterizationInfo.setPolygonMode(vk::PolygonMode::ePoint);
            gpb.renderingInfo.setColorAttachmentCount(1);
            gpb.renderingInfo.setPColorAttachmentFormats(&swapchain.format.format);
            gpb.rasterizationInfo.setCullMode(vk::CullModeFlagBits::eNone);
            pipeline = std::make_unique<GraphicsPipeline>(device, gpb);
        }
    };

    struct Points
    {
        const Allocator &allocator;

        std::size_t capacity = 1024;

        // attributes can be packed iirc
        std::vector<glm::vec3> points;
        std::unique_ptr<letc::Buffer> buffer;

        Points(const letc::Allocator &allocator) : allocator(allocator)
        {
            points.reserve(capacity);
            buffer = std::make_unique<letc::Buffer>(allocator, capacity * sizeof(glm::vec3), vk::BufferUsageFlagBits::eVertexBuffer,
                                                    VMA_MEMORY_USAGE_CPU_TO_GPU);
        }

        void cpy()
        {
            if (points.size() > capacity)
            {
                capacity = std::bit_ceil(points.size() * 2);
                buffer.reset();
                buffer = std::make_unique<letc::Buffer>(allocator, capacity * sizeof(glm::vec3), vk::BufferUsageFlagBits::eVertexBuffer,
                                                        VMA_MEMORY_USAGE_CPU_TO_GPU);
            }

            buffer->cpy(points.data(), points.size() * sizeof(glm::vec3));
        }

        void draw(const vk::CommandBuffer &commandBuffer)
        {
            if (points.empty())
                return;
            vk::DeviceSize offset = 0;
            commandBuffer.bindVertexBuffers(0, 1, &buffer->buffer, &offset);
            commandBuffer.draw(static_cast<uint32_t>(points.size()), 1, 0, 0);
        }
    };
}; // namespace letc
