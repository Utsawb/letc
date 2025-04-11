#pragma once

#include "pch.hh"

#include "spirv_reflect.hh"

#include "Device.hh"

namespace letc
{
    struct DescriptorLayout
    {
        const Device &device;
        std::map<uint32_t, std::map<uint32_t, vk::DescriptorSetLayoutBinding>> descriptorSetLayoutBindings;
        std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;

        DescriptorLayout(const Device &device) : device(device)
        {
        }

        DescriptorLayout &addBinding(const uint32_t &set, const uint32_t &binding, const vk::DescriptorType &type,
                                     const vk::ShaderStageFlags &stageFlags, const uint32_t &descriptorCount)
        {
            descriptorSetLayoutBindings[set][binding] = vk::DescriptorSetLayoutBinding{}
                                                            .setBinding(binding)
                                                            .setDescriptorType(type)
                                                            .setStageFlags(stageFlags)
                                                            .setDescriptorCount(descriptorCount);

            return *this;
        }

        void generateLayouts()
        {
            descriptorSetLayouts.clear();
            for (const auto &setBindings : descriptorSetLayoutBindings)
            {
                std::vector<vk::DescriptorSetLayoutBinding> bindings;
                for (const auto &bindingPair : setBindings.second)
                {
                    bindings.push_back(bindingPair.second);
                }
                vk::DescriptorSetLayoutCreateInfo layoutInfo({}, bindings);
                descriptorSetLayouts.push_back(device.device.createDescriptorSetLayout(layoutInfo));
            }
        }

        ~DescriptorLayout()
        {
            for (const auto &layout : descriptorSetLayouts)
            {
                device.device.destroyDescriptorSetLayout(layout);
            }
        }
    };
} // namespace letc
