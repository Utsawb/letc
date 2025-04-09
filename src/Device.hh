#pragma once

#ifndef LETC_DEVICE_HH
#define LETC_DEVICE_HH

#include "Instance.hh"

namespace letc
{

    struct DeviceBuilder
    {
        std::set<std::string> deviceExtensions;
        std::unordered_map<std::string, vk::QueueFlagBits> requestedQueues;
        std::optional<vk::PhysicalDevice> physicalDevice;

        DeviceBuilder &addExtension(const char *extension)
        {
            deviceExtensions.insert(extension);
            return *this;
        }

        DeviceBuilder &addExtensions(const std::vector<std::string> &extensions)
        {
            for (const auto &e : extensions)
            {
                deviceExtensions.insert(e);
            }
            return *this;
        }

        DeviceBuilder &requestQueue(const std::string &uniqueIdentifier, const vk::QueueFlagBits &queueFlags)
        {
            requestedQueues.emplace(uniqueIdentifier, queueFlags);
            return *this;
        }

        DeviceBuilder &requestDevice(const vk::PhysicalDevice &physicalDevice)
        {
            this->physicalDevice = physicalDevice;
            return *this;
        }

        DeviceBuilder()
        {
            deviceExtensions.insert(vk::KHRDynamicRenderingExtensionName);
            deviceExtensions.insert(vk::KHRSwapchainExtensionName);
        }
    };

    struct Queue
    {
        std::string id;
        uint32_t queueFamilyIndex;
        vk::Queue queue;
    };

    struct Device
    {
        const Instance &instance;
        DeviceBuilder deviceBuilder;
        vk::PhysicalDevice physicalDevice;
        vk::Device device;
        std::unordered_map<std::string, Queue> queues;

        Device(const Instance &instance, const DeviceBuilder &builder) : instance(instance), deviceBuilder(builder)
        {
            if (deviceBuilder.physicalDevice.has_value())
            {
                physicalDevice = *deviceBuilder.physicalDevice;
            }
            else
            {
                auto physicalDevices = instance.instance.enumeratePhysicalDevices();
                for (const auto &pd : physicalDevices)
                {
                    auto pdExtensions = pd.enumerateDeviceExtensionProperties();
                    std::set<std::string> availableExtensions;
                    std::transform(pdExtensions.begin(), pdExtensions.end(),
                                   std::inserter(availableExtensions, availableExtensions.end()),
                                   [](const vk::ExtensionProperties &ext) { return std::string(ext.extensionName); });
                    std::set<std::string> intersection;
                    std::set_intersection(deviceBuilder.deviceExtensions.begin(), deviceBuilder.deviceExtensions.end(),
                                          availableExtensions.begin(), availableExtensions.end(),
                                          std::inserter(intersection, intersection.end()));
                    if (intersection.size() == deviceBuilder.deviceExtensions.size())
                    {
                        physicalDevice = pd;
                        break;
                    }
                }
            }

            const auto allQueueProps = physicalDevice.getQueueFamilyProperties();
            struct FamilyInfo
            {
                uint32_t index;
                vk::QueueFamilyProperties properties;
                uint32_t allocated;
            };
            std::vector<FamilyInfo> families;
            for (uint32_t i = 0; i < allQueueProps.size(); ++i)
            {
                families.push_back({i, allQueueProps[i], 0});
            }

            // Structure to record assignment for each requested queue.
            struct Assignment
            {
                std::string id;
                uint32_t familyIndex;
                uint32_t offset;
                vk::QueueFlagBits flag;
            };
            std::vector<Assignment> assignments;

            for (const auto &req : deviceBuilder.requestedQueues)
            {
                const std::string &identifier = req.first;
                vk::QueueFlagBits requestedFlags = req.second;
                bool found = false;
                for (auto &family : families)
                {
                    if ((family.properties.queueFlags & requestedFlags) &&
                        (family.allocated < family.properties.queueCount))
                    {
                        assignments.push_back({identifier, family.index, family.allocated, requestedFlags});
                        family.allocated += 1;
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    throw std::runtime_error("No suitable queue family found for request: " + identifier);
                }
            }

            std::unordered_map<uint32_t, uint32_t> familyQueueCount;
            for (const auto &assignment : assignments)
            {
                familyQueueCount[assignment.familyIndex]++;
            }
            std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
            std::vector<std::vector<float>> prioritiesStorage;
            for (const auto &entry : familyQueueCount)
            {
                uint32_t familyIndex = entry.first;
                uint32_t queueCount = entry.second;
                prioritiesStorage.push_back(std::vector<float>(queueCount, 1.0f));
                vk::DeviceQueueCreateInfo dqci{};
                dqci.queueFamilyIndex = familyIndex;
                dqci.queueCount = queueCount;
                dqci.pQueuePriorities = prioritiesStorage.back().data();
                queueCreateInfos.push_back(dqci);
            }

            std::vector<const char *> extensions;
            for (const auto &ext : deviceBuilder.deviceExtensions)
            {
                extensions.push_back(ext.c_str());
            }

            vk::PhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeatures{};
            dynamicRenderingFeatures.dynamicRendering = VK_TRUE;
            vk::PhysicalDeviceFeatures deviceFeatures{};
            deviceFeatures.fillModeNonSolid = VK_TRUE;

            vk::DeviceCreateInfo deviceCreateInfo{};
            deviceCreateInfo.setQueueCreateInfos(queueCreateInfos);
            deviceCreateInfo.setPEnabledExtensionNames(extensions);
            deviceCreateInfo.setPEnabledFeatures(&deviceFeatures);
            deviceCreateInfo.setPNext(&dynamicRenderingFeatures);

            device = physicalDevice.createDevice(deviceCreateInfo);

            for (const auto &assignment : assignments)
            {
                vk::Queue vkQueue = device.getQueue(assignment.familyIndex, assignment.offset);
                Queue q{assignment.id, assignment.familyIndex, vkQueue};
                queues.emplace(assignment.id, q);
            }
        }
    };
}; // namespace letc

#endif // LETC_DEVICE_HH
