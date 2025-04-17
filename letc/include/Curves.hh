#pragma once

#ifndef LETC_CURVES_HH
#define LETC_CURVES_HH

#include "pch.hh"

#include "Allocator.hh"
#include "Buffer.hh"
#include "Device.hh"
#include "Material.hh"
#include "Pipeline.hh"
#include "Swapchain.hh"

namespace letc
{
    struct CurvesRenderer
    {
        const Allocator &allocator;
        const Device &device;

        std::unique_ptr<Material> material;
        std::unique_ptr<GraphicsPipeline> pipeline;

        CurvesRenderer(const Allocator &allocator, const Device &device, const Swapchain &swapchain,
                       const std::vector<char> &vertexCode, const std::vector<char> &fragmentCode)
            : allocator(allocator), device(device)
        {
            letc::GraphicsPipelineBuilder gpb;
            gpb.addShaderStage(vertexCode, vk::ShaderStageFlagBits::eVertex);
            gpb.addShaderStage(fragmentCode, vk::ShaderStageFlagBits::eFragment);
            gpb.addVertexInputBinding(0, sizeof(glm::vec3), vk::VertexInputRate::eVertex);
            gpb.addVertexInputAttribute(0, 0, vk::Format::eR32G32B32Sfloat, 0);
            gpb.inputAssemblyInfo.setTopology(vk::PrimitiveTopology::eLineList);
            gpb.rasterizationInfo.setPolygonMode(vk::PolygonMode::eLine);
            gpb.renderingInfo.setColorAttachmentCount(1);
            gpb.renderingInfo.setPColorAttachmentFormats(&swapchain.format.format);
            gpb.rasterizationInfo.setCullMode(vk::CullModeFlagBits::eNone);
            pipeline = std::make_unique<GraphicsPipeline>(device, gpb);

            material = std::make_unique<Material>(device.device, allocator.allocator, pipeline);
        }
    };

    struct Curves
    {
        const Allocator &allocator;

        std::vector<glm::vec3> cpoints;
        std::size_t capacity = 1024;
        std::unique_ptr<Buffer> buffer;

        Curves(const Allocator &allocator) : allocator(allocator)
        {
            buffer = std::make_unique<Buffer>(allocator, capacity * sizeof(glm::vec3),
                                              vk::BufferUsageFlagBits::eIndirectBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
        }

        void cpy(const std::vector<glm::vec3> &cpts)
        {
        }
    };

}; // namespace letc

#endif
