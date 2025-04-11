#pragma once

#ifndef LETC_PIPELINE_HH
#define LETC_PIPELINE_HH

#include "pch.hh"

#include "spirv_reflect.hh"

#include "Buffer.hh"
#include "Device.hh"
#include "Swapchain.hh"

// Helper function to convert reflection descriptor types to Vulkan descriptor types.
namespace
{
    vk::DescriptorType convertReflectionDescriptorType(SpvReflectDescriptorType type)
    {
        switch (type)
        {
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
            return vk::DescriptorType::eSampler;
        case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            return vk::DescriptorType::eCombinedImageSampler;
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            return vk::DescriptorType::eSampledImage;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            return vk::DescriptorType::eStorageImage;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
            return vk::DescriptorType::eUniformTexelBuffer;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            return vk::DescriptorType::eStorageTexelBuffer;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            return vk::DescriptorType::eUniformBuffer;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            return vk::DescriptorType::eStorageBuffer;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            return vk::DescriptorType::eUniformBufferDynamic;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            return vk::DescriptorType::eStorageBufferDynamic;
        case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            return vk::DescriptorType::eInputAttachment;
        default:
            // You might want to handle error cases more gracefully
            return vk::DescriptorType::eSampler; // fallback value
        }
    }
} // namespace

namespace letc
{
    struct IPipeline
    {
        virtual void bind(const vk::CommandBuffer &commandBuffer) = 0;
        virtual ~IPipeline() = default;
    };

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

    struct GraphicsPipeline : IPipeline
    {
        const Device &device;
        GraphicsPipelineBuilder builder;

        std::vector<vk::ShaderModule> shaders;
        std::map<uint32_t, std::map<uint32_t, vk::DescriptorSetLayoutBinding>> layoutBindings;
        std::set<uint32_t> sets;
        std::map<std::pair<uint32_t, uint32_t>, vk::PushConstantRange> pushConstants;
        std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
        vk::PipelineLayout layout;

        vk::Pipeline pipeline;

        GraphicsPipeline(const Device &device, const GraphicsPipelineBuilder &graphicsPipelineBuilder)
            : device(device), builder(graphicsPipelineBuilder)
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

            std::vector<spv_reflect::ShaderModule> shaderModules;
            for (const auto &code : builder.shaderCode)
            {
                shaderModules.emplace_back(code.size(), code.data());
            }

            // --- Reflection Code Begins Here ---
            // These containers will accumulate the descriptor set layout bindings and push constant ranges.
            // The layoutBindings map is keyed first by the descriptor set number and then by the binding number.
            // The pushConstants map is keyed by a pair (offset, size) so that multiple modules
            // referencing the same push constant range get their stage flags merged.
            for (const auto &module : shaderModules)
            {
                // Reflect descriptor sets for this module.
                uint32_t descriptorSetCount = 0;
                module.EnumerateDescriptorSets(&descriptorSetCount, nullptr);
                for (uint32_t setIndex = 0; setIndex < descriptorSetCount; ++setIndex)
                {
                    // Assume GetDescriptorSet returns a pointer to a reflection structure:
                    const auto *pSet = module.GetDescriptorSet(setIndex); // e.g. of type SpvReflectDescriptorSet*
                    uint32_t setNumber = pSet->set;
                    for (uint32_t i = 0; i < pSet->binding_count; ++i)
                    {
                        // Each binding in the set.
                        const auto *pBinding = pSet->bindings[i]; // e.g. of type SpvReflectDescriptorBinding*
                        vk::DescriptorSetLayoutBinding layoutBinding{};
                        layoutBinding.binding = pBinding->binding;
                        layoutBinding.descriptorCount = pBinding->count;
                        layoutBinding.descriptorType = convertReflectionDescriptorType(pBinding->descriptor_type);
                        // Use the shader stage flag from this module.
                        layoutBinding.stageFlags = static_cast<vk::ShaderStageFlagBits>(module.GetShaderStage());

                        std::cout << vk::to_string(layoutBinding.descriptorType)
                                  << " (Raw SpvReflect type: " << pBinding->descriptor_type << ")" << std::endl;

                        // If the same set and binding already exists, merge the stage flags.
                        auto &setBindingMap = layoutBindings[setNumber];
                        if (setBindingMap.find(layoutBinding.binding) != setBindingMap.end())
                        {
                            setBindingMap[layoutBinding.binding].stageFlags |= layoutBinding.stageFlags;
                        }
                        else
                        {
                            setBindingMap[layoutBinding.binding] = layoutBinding;
                        }
                    }
                }

                exit(-1);

                // Reflect push constant blocks for this module using a while loop.
                SpvReflectResult result = SPV_REFLECT_RESULT_SUCCESS;
                uint32_t pcIndex = 0;
                while (true)
                {
                    // Attempt to retrieve the push constant block at index pcIndex.
                    const SpvReflectBlockVariable *pPushConstant = module.GetPushConstantBlock(pcIndex, &result);
                    if (pPushConstant == nullptr || result != SPV_REFLECT_RESULT_SUCCESS)
                    {
                        // No more push constant blocks or an error occurred.
                        break;
                    }

                    // Create a key for merging: using offset and size as a unique identifier.
                    std::pair<uint32_t, uint32_t> key(pPushConstant->offset, pPushConstant->size);
                    vk::PushConstantRange pcRange{};
                    pcRange.offset = pPushConstant->offset;
                    pcRange.size = pPushConstant->size;
                    // Use the shader stage flag from this module.
                    pcRange.stageFlags = static_cast<vk::ShaderStageFlagBits>(module.GetShaderStage());

                    // Merge with an existing push constant range if present.
                    if (pushConstants.find(key) != pushConstants.end())
                    {
                        pushConstants[key].stageFlags |= pcRange.stageFlags;
                    }
                    else
                    {
                        pushConstants[key] = pcRange;
                    }
                    ++pcIndex;
                }
            }

            // Now create descriptor set layouts from the accumulated bindings.
            std::vector<vk::DescriptorSetLayout> setLayouts;
            for (auto &setPair : layoutBindings)
            {
                std::vector<vk::DescriptorSetLayoutBinding> bindings;
                for (auto &bindingPair : setPair.second)
                {
                    bindings.push_back(bindingPair.second);
                }
                vk::DescriptorSetLayoutCreateInfo dsLayoutInfo{};
                dsLayoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
                dsLayoutInfo.pBindings = bindings.data();
                vk::DescriptorSetLayout dsLayout = device.device.createDescriptorSetLayout(dsLayoutInfo);
                setLayouts.push_back(dsLayout);
            }
            // Store these descriptor set layouts as a member (so you can later clean them up).
            descriptorSetLayouts = setLayouts;

            // Collect push constant ranges into a vector.
            std::vector<vk::PushConstantRange> pushConstantRanges;
            for (auto &pc : pushConstants)
            {
                pushConstantRanges.push_back(pc.second);
            }

            // Create the pipeline layout using the descriptor set layouts and push constant ranges.
            vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.setSetLayouts(setLayouts);
            pipelineLayoutInfo.setPushConstantRanges(pushConstantRanges);
            layout = device.device.createPipelineLayout(pipelineLayoutInfo);
            builder.createInfo.setLayout(layout);
            // --- Reflection Code Ends Here ---

            // Continue with graphics pipeline creation.
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
            // Destroy descriptor set layouts.
            for (auto &dsLayout : descriptorSetLayouts)
            {
                device.device.destroyDescriptorSetLayout(dsLayout);
            }
        }
    };
}; // namespace letc

#endif // LETC_PIPELINE_HH
