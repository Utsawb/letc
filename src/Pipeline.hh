#pragma once

#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>
#ifndef LETC_PIPELINE_HH
#define LETC_PIPELINE_HH

#include "pch.hh"

#include "Buffer.hh"
#include "Descriptor.hh"
#include "Device.hh"
#include "Swapchain.hh"

namespace letc
{
    struct GraphicsPipelineBuilder
    {
        vk::GraphicsPipelineCreateInfo createInfo;
        vk::PipelineCreateFlags createFlags;

        std::vector<std::vector<char>> shaderCode;
        std::vector<std::vector<char>> shaderNames;
        std::vector<vk::PipelineShaderStageCreateInfo> shaderStageInfos;

        vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
        std::vector<vk::VertexInputBindingDescription> vertexInputBindings;
        std::vector<vk::VertexInputAttributeDescription> vertexInputAttributes;

        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
        vk::PipelineTessellationStateCreateInfo tessellationInfo;
        vk::PipelineViewportStateCreateInfo viewportInfo;
        vk::PipelineRasterizationStateCreateInfo rasterizationInfo;
        vk::PipelineMultisampleStateCreateInfo multisampleInfo;
        vk::PipelineDepthStencilStateCreateInfo depthStencilInfo;

        vk::PipelineColorBlendStateCreateInfo colorBlendInfo;
        std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments;

        vk::PipelineRenderingCreateInfo renderingInfo;

        vk::PipelineDynamicStateCreateInfo dynamicStateInfo;
        std::vector<vk::DynamicState> dynamicStates;

        vk::PipelineLayoutCreateInfo layoutInfo;
        const DescriptorLayout *descriptorLayout;
        std::vector<vk::PushConstantRange> pushConstantRanges;

        GraphicsPipelineBuilder &addShaderStage(const std::vector<char> &code, const vk::ShaderStageFlagBits &stage,
                                                const std::string &entryPoint = "main")
        {
            shaderCode.push_back(code);

            shaderNames.push_back(std::vector<char>(entryPoint.begin(), entryPoint.end()));
            shaderNames.back().push_back('\0');

            vk::PipelineShaderStageCreateInfo shaderStageInfo{};
            shaderStageInfo.setStage(stage);
            shaderStageInfo.setPName(shaderNames.back().data());
            shaderStageInfos.push_back(shaderStageInfo);

            return *this;
        }

        GraphicsPipelineBuilder &addVertexInputBinding(const uint32_t &binding, const uint32_t &stride,
                                                       const vk::VertexInputRate &inputRate)
        {
            vk::VertexInputBindingDescription vertexInputBinding{};
            vertexInputBinding.setBinding(binding);
            vertexInputBinding.setStride(stride);
            vertexInputBinding.setInputRate(inputRate);
            vertexInputBindings.push_back(vertexInputBinding);

            return *this;
        }

        GraphicsPipelineBuilder &addVertexInputAttribute(const uint32_t &location, const uint32_t &binding,
                                                         const vk::Format &format, const uint32_t &offset)
        {
            vk::VertexInputAttributeDescription vertexInputAttribute{};
            vertexInputAttribute.setLocation(location);
            vertexInputAttribute.setBinding(binding);
            vertexInputAttribute.setFormat(format);
            vertexInputAttribute.setOffset(offset);
            vertexInputAttributes.push_back(vertexInputAttribute);

            return *this;
        }

        GraphicsPipelineBuilder &setInputAssembly(const vk::PipelineInputAssemblyStateCreateInfo &info)
        {
            inputAssemblyInfo = info;
            return *this;
        }

        GraphicsPipelineBuilder &setTessellation(const vk::PipelineTessellationStateCreateInfo &info)
        {
            tessellationInfo = info;
            return *this;
        }

        GraphicsPipelineBuilder &setRasterization(const vk::PipelineRasterizationStateCreateInfo &info)
        {
            rasterizationInfo = info;
            return *this;
        }

        GraphicsPipelineBuilder &setMultisample(const vk::PipelineMultisampleStateCreateInfo &info)
        {
            multisampleInfo = info;
            return *this;
        }

        GraphicsPipelineBuilder &setDepthStencil(const vk::PipelineDepthStencilStateCreateInfo &info)
        {
            depthStencilInfo = info;
            return *this;
        }

        GraphicsPipelineBuilder &clearColorBlendAttachments()
        {
            colorBlendAttachments.clear();
            return *this;
        }

        GraphicsPipelineBuilder &addColorBlendAttachment(const vk::PipelineColorBlendAttachmentState &attachment)
        {
            colorBlendAttachments.push_back(attachment);
            return *this;
        }

        GraphicsPipelineBuilder &setColorBlend(const vk::PipelineColorBlendStateCreateInfo &info)
        {
            colorBlendInfo = info;
            return *this;
        }

        GraphicsPipelineBuilder &setRendering(const vk::PipelineRenderingCreateInfo &info)
        {
            renderingInfo = info;
            return *this;
        }

        GraphicsPipelineBuilder &clearDynamicStates()
        {
            dynamicStates.clear();
            return *this;
        }

        GraphicsPipelineBuilder &addDynamicState(const vk::DynamicState &state)
        {
            dynamicStates.push_back(state);
            return *this;
        }

        GraphicsPipelineBuilder &setLayout(const DescriptorLayout * const& layout)
        {
            descriptorLayout = layout;
            return *this;
        }

        GraphicsPipelineBuilder &addPushConstantRange(const vk::PushConstantRange &range)
        {
            pushConstantRanges.push_back(range);
            return *this;
        }

        GraphicsPipelineBuilder()
        {
            inputAssemblyInfo.setTopology(vk::PrimitiveTopology::eTriangleList);

            viewportInfo.setViewportCount(1);
            viewportInfo.setScissorCount(1);

            rasterizationInfo.setDepthClampEnable(VK_FALSE);
            rasterizationInfo.setRasterizerDiscardEnable(VK_FALSE);
            rasterizationInfo.setPolygonMode(vk::PolygonMode::eFill);
            rasterizationInfo.setLineWidth(1.0f);
            rasterizationInfo.setCullMode(vk::CullModeFlagBits::eBack);
            rasterizationInfo.setFrontFace(vk::FrontFace::eCounterClockwise);
            rasterizationInfo.setDepthBiasEnable(VK_FALSE);

            multisampleInfo.setSampleShadingEnable(VK_FALSE);
            multisampleInfo.setRasterizationSamples(vk::SampleCountFlagBits::e1);

            colorBlendInfo.setLogicOpEnable(VK_FALSE);
            colorBlendInfo.setLogicOp(vk::LogicOp::eCopy);
            colorBlendAttachments.push_back(
                vk::PipelineColorBlendAttachmentState{}
                    .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                       vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
                    .setBlendEnable(VK_FALSE));

            renderingInfo.setColorAttachmentCount(1);
            renderingInfo.setDepthAttachmentFormat(vk::Format::eD32Sfloat);

            depthStencilInfo.setDepthTestEnable(VK_TRUE);
            depthStencilInfo.setDepthWriteEnable(VK_TRUE);
            depthStencilInfo.setDepthCompareOp(vk::CompareOp::eLess);
            depthStencilInfo.setDepthBoundsTestEnable(VK_FALSE);
            depthStencilInfo.setStencilTestEnable(VK_FALSE);

            dynamicStates.push_back(vk::DynamicState::eViewport);
            dynamicStates.push_back(vk::DynamicState::eScissor);
        }
    };

    struct GraphicsPipeline
    {
        const Device &device;
        const Swapchain &swapchain;
        GraphicsPipelineBuilder builder;
        std::vector<vk::ShaderModule> shaders;
        vk::PipelineLayout layout;
        vk::Pipeline pipeline;

        GraphicsPipeline(const Device &device, const Swapchain &swapchain,
                  const GraphicsPipelineBuilder &graphicsPipelineBuilder)
            : device(device), swapchain(swapchain), builder(graphicsPipelineBuilder)
        {
            for (auto &code : builder.shaderCode)
            {
                shaders.push_back(
                    device.device.createShaderModule(vk::ShaderModuleCreateInfo{}
                                                          .setCodeSize(code.size())
                                                          .setPCode(reinterpret_cast<uint32_t *>(code.data()))));
            }
            for (size_t i = 0; i < builder.shaderStageInfos.size(); i++)
            {
                builder.shaderStageInfos[i].setModule(shaders[i]);
            }
            builder.createInfo.setStages(builder.shaderStageInfos);

            builder.vertexInputInfo.setVertexBindingDescriptionCount(builder.vertexInputBindings.size());
            builder.vertexInputInfo.setVertexBindingDescriptions(builder.vertexInputBindings);
            builder.vertexInputInfo.setVertexAttributeDescriptionCount(builder.vertexInputAttributes.size());
            builder.vertexInputInfo.setVertexAttributeDescriptions(builder.vertexInputAttributes);
            builder.createInfo.setPVertexInputState(&builder.vertexInputInfo);

            builder.createInfo.setPInputAssemblyState(&builder.inputAssemblyInfo);
            builder.createInfo.setPViewportState(&builder.viewportInfo);
            builder.createInfo.setPRasterizationState(&builder.rasterizationInfo);
            builder.createInfo.setPMultisampleState(&builder.multisampleInfo);
            builder.createInfo.setPDepthStencilState(&builder.depthStencilInfo);

            builder.colorBlendInfo.setAttachments(builder.colorBlendAttachments);
            builder.createInfo.setPColorBlendState(&builder.colorBlendInfo);

            builder.createInfo.setPNext(&builder.renderingInfo);

            builder.dynamicStateInfo.setDynamicStates(builder.dynamicStates);
            builder.createInfo.setPDynamicState(&builder.dynamicStateInfo);

            vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.setSetLayouts(builder.descriptorLayout->descriptorSetLayouts);
            pipelineLayoutInfo.setPushConstantRanges(builder.pushConstantRanges);
            layout = device.device.createPipelineLayout(pipelineLayoutInfo);
            builder.createInfo.setLayout(layout);

            pipeline = device.device.createGraphicsPipeline(VK_NULL_HANDLE, builder.createInfo).value;
        }

        void bind(const vk::CommandBuffer &commandBuffer)
        {
            commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
        }

        ~GraphicsPipeline()
        {
            device.device.destroyPipeline(pipeline);
            device.device.destroyPipelineLayout(layout);
            for (auto &shader : shaders)
            {
                device.device.destroyShaderModule(shader);
            }
        }
    };
}; // namespace letc

#endif // LETC_PIPELINE_HH
