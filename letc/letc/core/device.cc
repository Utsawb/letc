#include "letc/core/device.hh"

#include <ranges>
#include <vulkan/vulkan_handles.hpp>

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
        m_requestedQueues.try_emplace(id, flags, count);
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
            // just pick the first device that is dedicated,
            // or fallback to whatever is first
            // how many people rly have more than one gpu out there
            // and even if they do ill just let them pick in the json
            // config later
            auto physicalDevices = instance.lock().get()->get().enumeratePhysicalDevices();
            auto dgpu = std::ranges::find_if(physicalDevices, [](const vk::PhysicalDevice &pd) {
                return pd.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
            });
            m_physicalDevice = dgpu != physicalDevices.end() ? *dgpu : physicalDevices.at(0);
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
                return (flags & qp.queueFlags) && (count <= qp.queueCount);
            });
            ATHROW(found != queueProps.end(), "could not find suitable queue");

            auto queue = Queues{};
            queue.m_id = id;
            queue.m_handles = std::vector<vk::Queue>{count, nullptr};
            queue.m_flags = flags;
            queue.m_family = found - queueProps.begin();
            device->m_queues.emplace(id, queue);
            queueProps.erase(found);
        }

        std::vector<std::vector<float>> priorities;
        std::vector<vk::DeviceQueueCreateInfo> queueInfos;
        for (const auto &[id, queues] : device->m_queues)
        {
            priorities.push_back(std::vector<float>(queues.m_handles.size(), 1.0f));
            queueInfos.push_back(vk::DeviceQueueCreateInfo{}
                                     .setQueueCount(queues.m_handles.size())
                                     .setQueuePriorities(priorities.back())
                                     .setQueueFamilyIndex(queues.m_family));
        }

        std::vector<const char *> extensionsCStr;
        std::ranges::for_each(m_extensions,
                              [&extensionsCStr](const std::string &e) { extensionsCStr.push_back(e.c_str()); });
        auto deviceInfo = vk::DeviceCreateInfo{}
                              .setPNext(&m_deviceFeatures.get())
                              .setQueueCreateInfos(queueInfos)
                              .setPEnabledExtensionNames(extensionsCStr);

        device->m_logicalDevice = m_physicalDevice->createDevice(deviceInfo);

        for (auto &[id, q] : device->m_queues)
        {
            for (std::size_t i = 0; i < q.m_handles.size(); ++i)
            {
                q.m_handles.at(i) = device->m_logicalDevice.getQueue(q.m_family, i);
            }
        }

        return device;
    }

    auto Queues::get() -> std::vector<vk::Queue>
    {
        return m_handles;
    }

    auto Queues::getId() -> std::string
    {
        return m_id;
    }

    auto Queues::getFamily() -> uint32_t
    {
        return m_family;
    }

    auto Device::getPhysical() -> vk::PhysicalDevice
    {
        return m_physicalDevice;
    }

    auto Device::getLogical() -> vk::Device
    {
        return m_logicalDevice;
    }

    auto Device::getQueue(const std::string &id) -> Queues
    {
        return m_queues.at(id);
    }

    Device::~Device()
    {
        m_logicalDevice.destroy();
    }

} // namespace letc
