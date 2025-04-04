#pragma once
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>
#ifndef LETC_DEVICE_HH
#define LETC_DEVICE_HH

#include "pch.hh"

// custom hash for the queue flags
namespace std
{
    template <> struct hash<vk::QueueFlags>
    {
        std::size_t operator()(const vk::QueueFlags &flags) const noexcept
        {
            return std::hash<uint32_t>{}(static_cast<uint32_t>(flags));
        }
    };
}; // namespace std

namespace letc
{
    struct Device
    {
        vk::PhysicalDevice physicalDevice;
        vk::Device device;
        uint32_t graphicsQueueFamilyIndex;

        operator const vk::Device &()
        {
            return device;
        }
        operator const vk::PhysicalDevice &() const
        {
            return physicalDevice;
        }

        Device(const vk::Instance &instance)
        {
            std::vector<const char *> deviceExtensions{};
            deviceExtensions.push_back(vk::KHRDynamicRenderingExtensionName);
            deviceExtensions.push_back(vk::KHRSwapchainExtensionName);

            /*
                Physical Device
            */
            auto physicalDevices = instance.enumeratePhysicalDevices();
            bool found = false;
            for (const auto &pd : physicalDevices)
            {
                auto availableExtensions = pd.enumerateDeviceExtensionProperties();
                bool extensionsSupported = true;
                for (auto requiredExt : deviceExtensions)
                {
                    bool extFound = false;
                    for (const auto &ext : availableExtensions)
                    {
                        if (std::strcmp(ext.extensionName, requiredExt) == 0)
                        {
                            extFound = true;
                            break;
                        }
                    }
                    if (!extFound)
                    {
                        extensionsSupported = false;
                        break;
                    }
                }
                if (!extensionsSupported)
                {
                    continue;
                }

                auto queueFamilies = pd.getQueueFamilyProperties();
                for (uint32_t i = 0; i < queueFamilies.size(); i++)
                {
                    if ((queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) && queueFamilies[i].queueCount > 0)
                    {
                        graphicsQueueFamilyIndex = i;
                        physicalDevice = pd;
                        found = true;
                        break;
                    }
                }
                if (found)
                    break;
            }

            assertThrow(found, "no suitable physical device found");

            float queuePriority = 1.0f;
            vk::DeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.setQueueFamilyIndex(graphicsQueueFamilyIndex);
            queueCreateInfo.setQueueCount(1);
            queueCreateInfo.setPQueuePriorities(&queuePriority);

            vk::PhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeatures{};
            dynamicRenderingFeatures.setDynamicRendering(true);
            vk::PhysicalDeviceFeatures deviceFeatures{};
            deviceFeatures.setFillModeNonSolid(true);

            vk::DeviceCreateInfo deviceCreateInfo{};
            deviceCreateInfo.setQueueCreateInfoCount(1);
            deviceCreateInfo.setPQueueCreateInfos(&queueCreateInfo);
            deviceCreateInfo.setPEnabledExtensionNames(deviceExtensions);
            deviceCreateInfo.setPEnabledFeatures(&deviceFeatures);
            deviceCreateInfo.setPNext(&dynamicRenderingFeatures);

            // Create the logical device.
            device = physicalDevice.createDevice(deviceCreateInfo);
        }

        ~Device()
        {
            device.destroy();
        }
    };
}; // namespace letc

#endif // LETC_DEVICE_HH
