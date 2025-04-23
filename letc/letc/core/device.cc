#include "letc/core/device.hh"

namespace letc
{
    DeviceBuilder::DeviceBuilder()
    {
    }

    auto DeviceBuilder::addExtension(const std::string &extension) -> DeviceBuilder &
    {
        m_extensions.insert(extension);
        return *this;
    }

    auto DeviceBuilder::requestQueues(const std::string &id, const vk::QueueFlags &flags, const uint32_t &count)
        -> DeviceBuilder &
    {
        m_requestedQueues.emplace(id, flags, count);
        return *this;
    }

    auto DeviceBuilder::setDevice(const vk::PhysicalDevice &physicalDevice) -> DeviceBuilder &
    {
        m_physicalDevice = physicalDevice;
        return *this;
    }

    auto DeviceBuilder::setDeviceFeatures(void func(FeatureChain &)) -> DeviceBuilder &
    {
        func(m_deviceFeatures);
        return *this;
    }

    auto DeviceBuilder::build(std::weak_ptr<Instance> instance) -> std::shared_ptr<Device>
    {
        if (!m_physicalDevice.has_value())
        {
            // find a suitable physical device
        }

        auto device = std::make_shared<Device>();
        device->m_physicalDevice = m_physicalDevice.value();
        device->m_deviceBuilder = *this;

        auto queueProps = m_physicalDevice->getQueueFamilyProperties();
        for (const auto &[id, pair] : m_requestedQueues)
        {
            const auto &flags = pair.first;
            const auto &count = pair.second;
            auto found = std::ranges::find_if(queueProps, [flags, count](const vk::QueueFamilyProperties &qp) {
                return static_cast<bool>(flags & qp.queueFlags) && (count <= qp.queueCount);
            });

            auto queue = Queue{};
            queue.m_id = id;
            queue.m_handle = std::vector<vk::Queue>(count, nullptr);
            queue.m_flags = flags;
            queue.m_family = found - queueProps.begin();
            device->m_queues.emplace(id, queue);
        }

        return device;
    }

} // namespace letc
