#pragma once

#include "letc/pch.hh"

#include "letc/core/instance.hh"

// forced myself to comment this out properly so I did not work on it any longer
// such a waste of time man
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

        /**
         *  @brief Add a device extension to enable on device creation
         *  @param Extension to add
         *  @return Reference to builder
         */
        auto addExtension(const std::string &extension) -> DeviceBuilder &;

        /**
         *  @brief Request a queue from the device
         *  @param id A unique id to be used to retrieve this queue at a later point
         *  @param flags The flags the queue should have, please make this as specific as possible
         *  @param count The number of flags type to retrieve, these are forced to be from the same family
         *  @return Reference to builder
         */
        auto requestQueues(const std::string &id, const vk::QueueFlags &flags, const uint32_t &count = 1)
            -> DeviceBuilder &;

        /**
         *  @brief Override the internal device selection, useful when you already know
         *      the physical device you want to use
         *  @param Handle to the physical device
         *  @return Reference to builder
         */
        auto setDevice(const vk::PhysicalDevice &physicalDevice) -> DeviceBuilder &;

        /**
         *  @brief Set device features that may be needed, currently chains all features up to 1.4
         *  @param func A callback that gives access to the internal mutable reference to the feature chain
         *  @return Reference to builder
         */
        auto setDeviceFeatures(void func(FeatureChain &)) -> DeviceBuilder &;

        /**
         *  @brief Build the device
         *  @param instance Borrowed handle to the Instance class
         *  @return A shared pointer to the create Device
         */
        auto build(std::weak_ptr<Instance> instance) -> std::shared_ptr<Device>;

      private:
        std::unordered_set<std::string> m_extensions;
        std::unordered_map<std::string, std::pair<vk::QueueFlags, uint32_t>> m_requestedQueues;
        FeatureChain m_deviceFeatures;
        std::optional<vk::PhysicalDevice> m_physicalDevice;
    };

    class Queues
    {
      public:
        auto get() -> std::vector<vk::Queue>;
        auto getId() -> std::string;
        auto getFamily() -> uint32_t;

      private:
        friend DeviceBuilder;
        friend Device;

        std::string m_id;
        std::vector<vk::Queue> m_handles;
        vk::QueueFlags m_flags;
        uint32_t m_family;
    };

    class Device
    {
      public:
        ~Device();

        auto getPhysical() -> vk::PhysicalDevice;
        auto getLogical() -> vk::Device;
        auto getQueue(const std::string &id) -> Queues;

      private:
        friend DeviceBuilder;
        DeviceBuilder m_deviceBuilder;

        vk::PhysicalDevice m_physicalDevice;
        vk::Device m_logicalDevice;
        std::unordered_map<std::string, Queues> m_queues;
    };
} // namespace letc
