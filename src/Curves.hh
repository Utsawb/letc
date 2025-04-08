#pragma once

#ifndef LETC_CURVES_HH
#define LETC_CURVES_HH

#include "pch.hh"

#include "Allocator.hh"
#include "Descriptor.hh"
#include "Pipeline.hh"
#include "Swapchain.hh"
#include "Material.hh"
#include "Device.hh"
#include "Buffer.hh"

namespace letc
{
    struct CurvesRenderer
    {
        const Allocator &allocator;
        const Device &device;

        std::unique_ptr<DescriptorLayout> layout;
        std::unique_ptr<Material> material;
        std::unique_ptr<GraphicsPipeline> pipeline;

        CurvesRenderer(const Allocator &allocator, const Device &device, const Swapchain &swapchain, 
            const std::vector<char> &vertexCode, const std::vector<char> &fragmentCode)
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
            gpb.inputAssemblyInfo.setTopology(vk::PrimitiveTopology::eLineList);
            gpb.rasterizationInfo.setPolygonMode(vk::PolygonMode::eLine);
            gpb.renderingInfo.setColorAttachmentCount(1);
            gpb.renderingInfo.setPColorAttachmentFormats(&swapchain.format.format);
            gpb.rasterizationInfo.setCullMode(vk::CullModeFlagBits::eNone);
            pipeline = std::make_unique<GraphicsPipeline>(device, gpb);
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