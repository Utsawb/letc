#pragma once

#include <optional>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>
#ifndef LETC_MATERIAL_HH
#define LETC_MATERIAL_HH

#include "pch.hh"

#include "Allocator.hh"
#include "Buffer.hh"
#include "Descriptor.hh"
#include "Device.hh"
#include "Pipeline.hh"

namespace letc
{
    struct Material
    {
        const Device &device;
        const Allocator &allocator;
        const DescriptorLayout &descriptorLayout;

        std::vector<vk::DescriptorSet> descriptorSets;
        std::vector<std::optional<uint32_t>> dynamicOffsets;

        // bufferInfo[set][binding] = {bufferInfo, descriptorType}
        std::map<uint32_t, std::map<uint32_t, std::pair<vk::DescriptorBufferInfo, vk::DescriptorType>>> bufferInfos;

        // makes all the descriptor sets based on the layout provided
        Material(const Device &device, const Allocator &allocator, const DescriptorLayout &descriptorLayout)
            : device(device), allocator(allocator), descriptorLayout(descriptorLayout)
        {
            vk::DescriptorSetAllocateInfo allocateInfo{};
            allocateInfo.setDescriptorPool(allocator.descriptorPool);
            allocateInfo.setDescriptorSetCount(descriptorLayout.descriptorSetLayouts.size());
            allocateInfo.setPSetLayouts(descriptorLayout.descriptorSetLayouts.data());
            descriptorSets = device.device.allocateDescriptorSets(allocateInfo);

            for (const auto &setBindings : descriptorLayout.descriptorSetLayoutBindings)
            {
                for (const auto &bindingPair : setBindings.second)
                {
                    bufferInfos[setBindings.first][bindingPair.first] = {vk::DescriptorBufferInfo{},
                                                                         bindingPair.second.descriptorType};
                }
            }

            dynamicOffsets.resize(descriptorSets.size(), std::nullopt);
        }

        // set the buffer info to the corrisponding set, nothing has been updated yet
        void updateDescriptorBufferInfo(const uint32_t &set, const uint32_t &binding, const vk::Buffer &buffer,
                                        const vk::DeviceSize &offset, const vk::DeviceSize &range)
        {
            bufferInfos[set][binding].first.setBuffer(buffer);
            bufferInfos[set][binding].first.setOffset(offset);
            bufferInfos[set][binding].first.setRange(range);
        }

        // set the buffer info to the corrisponding set, nothing has been updated yet
        void updateDescriptorBufferInfo(const uint32_t &set, const uint32_t &binding, const letc::Buffer &buffer,
                                        const vk::DeviceSize &offset, const vk::DeviceSize &range)
        {
            bufferInfos[set][binding].first.setBuffer(buffer.buffer);
            bufferInfos[set][binding].first.setOffset(offset);
            bufferInfos[set][binding].first.setRange(range);
        }

        // update the sets to point at the buffers in bufferInfos
        void updateDescriptorSets()
        {
            std::vector<vk::WriteDescriptorSet> descriptorWrites;
            uint32_t index = 0;
            for (const auto &ds : descriptorSets)
            {
                for (const auto &bindingPair : bufferInfos[index])
                {
                    const auto &bufferInfo = bindingPair.second.first;
                    const auto &descriptorType = bindingPair.second.second;
                    auto write = vk::WriteDescriptorSet{}
                                     .setDstSet(ds)
                                     .setDstBinding(bindingPair.first)
                                     .setDescriptorCount(1)
                                     .setDescriptorType(descriptorType)
                                     .setPBufferInfo(&bufferInfo);
                    descriptorWrites.push_back(write);
                }
                ++index;
            }
            device.device.updateDescriptorSets(descriptorWrites, {});
        }

        // update a set using the bufferinfos
        void updateDescriptorSet(const uint32_t &set)
        {
            std::vector<vk::WriteDescriptorSet> descriptorWrites;
            for (const auto &bindingPair : bufferInfos[set])
            {
                const auto &bufferInfo = bindingPair.second.first;
                const auto &descriptorType = bindingPair.second.second;
                auto write = vk::WriteDescriptorSet{}
                                 .setDstSet(descriptorSets[set])
                                 .setDstBinding(bindingPair.first)
                                 .setDescriptorCount(1)
                                 .setDescriptorType(descriptorType)
                                 .setPBufferInfo(&bufferInfo);
                descriptorWrites.push_back(write);
            }
            device.device.updateDescriptorSets(descriptorWrites, {});
        }

        // update a binding only using the bufferInfos
        void updateDescriptorSet(const uint32_t &set, const uint32_t &binding)
        {
            std::vector<vk::WriteDescriptorSet> descriptorWrites;
            for (const auto &bindingPair : bufferInfos[set])
            {
                if (bindingPair.first == binding)
                {
                    const auto &bufferInfo = bindingPair.second.first;
                    const auto &descriptorType = bindingPair.second.second;
                    auto write = vk::WriteDescriptorSet{}
                                     .setDstSet(descriptorSets[set])
                                     .setDstBinding(bindingPair.first)
                                     .setDescriptorCount(1)
                                     .setDescriptorType(descriptorType)
                                     .setPBufferInfo(&bufferInfo);
                    descriptorWrites.push_back(write);
                }
            }
            device.device.updateDescriptorSets(descriptorWrites, {});
        }

        // change the dynamic offset of a set
        void updateDynamicOffset(const uint32_t &set, const uint32_t &dynamicOffset)
        {
            dynamicOffsets[set] = dynamicOffset;
        }

        // bind all the sets, use the dynamic offsets for the dynamic ones
        void bind(const vk::CommandBuffer &commandBuffer, const GraphicsPipeline &pipeline)
        {
            std::vector<uint32_t> offsets;
            for (const auto &offset : dynamicOffsets)
            {
                if (offset.has_value())
                {
                    offsets.push_back(offset.value());
                }
            }

            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.layout, 0,
                                             descriptorSets.size(), descriptorSets.data(), offsets.size(),
                                             offsets.data());
        }

        // bind only one set, use this maybe when u change the dynamic offset
        void bind(const vk::CommandBuffer &commandBuffer, const GraphicsPipeline &pipeline, const uint32_t &set)
        {
            std::vector<uint32_t> offsets;
            if (dynamicOffsets[set].has_value())
            {
                offsets.push_back(dynamicOffsets[set].value());
            }

            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.layout, set, 1,
                                             &descriptorSets[set], offsets.size(), offsets.data());
        }

        ~Material()
        {
            device.device.freeDescriptorSets(allocator.descriptorPool, descriptorSets);
        }
    };

}; // namespace letc

#endif // LETC_MATERIAL_HH
