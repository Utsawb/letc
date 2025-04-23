#pragma once

#include "letc/pch.hh"

#include "letc/core/instance.hh"

namespace letc
{
    class Device;

    class DeviceBuilder
    {
      public:
        DeviceBuilder();

        auto addExtension(const std::string &extension) -> DeviceBuilder &;
        auto requestQueue(const std::string &id, const vk::QueueFlagBits &flags) -> DeviceBuilder &;
        auto setDevice(const vk::PhysicalDevice &physicalDevice) -> DeviceBuilder &;
        auto setDynamicRendering(const bool &enable) -> DeviceBuilder &;

        auto build() -> std::shared_ptr<Device>;

      private:
        std::unordered_set<std::string> m_extensions;
        std::unordered_map<std::string, vk::QueueFlagBits> m_requestedQueues;
        vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceDynamicRenderingFeatures> m_pNextChain;
        std::optional<vk::PhysicalDevice> m_physicalDevice;
    };

    class Device
    {
      public:
        ~Device();

      private:
        friend DeviceBuilder;
        DeviceBuilder m_deviceBuilder;

        vk::PhysicalDevice m_physicalDevice;
        vk::Device m_logicalDevice;
    };
} // namespace letc
