#pragma once

#include "letc/pch.hh"

#include "letc/core/instance.hh"

namespace letc
{
    class Device;

    class DeviceBuilder
    {
      public:
        using FeatureChain = vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features,
                                                vk::PhysicalDeviceVulkan12Features, vk::PhysicalDeviceVulkan13Features,
                                                vk::PhysicalDeviceVulkan14Features>;

        DeviceBuilder();

        auto addExtension(const std::string &extension) -> DeviceBuilder &;
        auto requestQueues(const std::string &id, const vk::QueueFlags &flags, const uint32_t &count = 1)
            -> DeviceBuilder &;
        auto setDevice(const vk::PhysicalDevice &physicalDevice) -> DeviceBuilder &;
        auto setDeviceFeatures(void func(FeatureChain &)) -> DeviceBuilder &;

        auto build(std::weak_ptr<Instance>) -> std::shared_ptr<Device>;

      private:
        std::unordered_set<std::string> m_extensions;
        std::unordered_map<std::string, std::pair<vk::QueueFlags, uint32_t>> m_requestedQueues;
        FeatureChain m_deviceFeatures;
        std::optional<vk::PhysicalDevice> m_physicalDevice;
    };

    class Queue
    {
      public:
        auto get(const std::size_t &idx) -> vk::Queue;
        auto getId() -> std::string;
        auto getFamily() -> uint32_t;

      private:
        friend DeviceBuilder;
        friend Device;

        std::string m_id;
        std::vector<vk::Queue> m_handle;
        vk::QueueFlags m_flags;
        uint32_t m_family;
    };

    class Device
    {
      public:
        ~Device();

        auto getPhysical() -> vk::PhysicalDevice;
        auto getLogical() -> vk::Device;
        auto getQueue(const std::string &id) -> Queue;

      private:
        friend DeviceBuilder;
        DeviceBuilder m_deviceBuilder;

        vk::PhysicalDevice m_physicalDevice;
        vk::Device m_logicalDevice;
        std::unordered_map<std::string, Queue> m_queues;
    };
} // namespace letc
