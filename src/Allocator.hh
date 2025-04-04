#pragma once

#include <vulkan/vulkan_structs.hpp>
#ifndef LETC_ALLOCATOR_HH
#define LETC_ALLOCATOR_HH

#include "pch.hh"

#include "Device.hh"
#include "Instance.hh"

namespace letc
{
    struct Allocator
    {
        const Instance &instance;
        const Device &device;

        VmaAllocator allocator;
        vk::DescriptorPool descriptorPool;

        Allocator(const Instance &instance, Device &device)
            : instance(instance), device(device)
        {
            VmaAllocatorCreateInfo allocatorInfo{};
            allocatorInfo.instance = instance.instance;
            allocatorInfo.physicalDevice = device.physicalDevice;
            allocatorInfo.device = device.device;
            allocatorInfo.vulkanApiVersion = instance.instanceBuilder.applicationInfo.apiVersion;
            assertThrow(vmaCreateAllocator(&allocatorInfo, &allocator) || true, "failed to create allocator");

            // stupid amounts that I should not be reaching anytime soon
            std::vector<vk::DescriptorPoolSize> descriptorPoolSizes;
            descriptorPoolSizes.push_back(vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, 1024});
            descriptorPoolSizes.push_back(vk::DescriptorPoolSize{vk::DescriptorType::eUniformBufferDynamic, 1024});
            descriptorPoolSizes.push_back(vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, 1024});
            descriptorPoolSizes.push_back(vk::DescriptorPoolSize{vk::DescriptorType::eStorageBufferDynamic, 1024});
            descriptorPoolSizes.push_back(vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, 1024});


            vk::DescriptorPoolCreateInfo descriptorPoolInfo{};
            descriptorPoolInfo.setMaxSets(1024);
            descriptorPoolInfo.setPoolSizes(descriptorPoolSizes);
            descriptorPoolInfo.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
            descriptorPool = device.device.createDescriptorPool(descriptorPoolInfo);
        }

        ~Allocator()
        {
            device.device.destroyDescriptorPool(descriptorPool);
            vmaDestroyAllocator(allocator);
        }
    };
}; // namespace letc

#endif
