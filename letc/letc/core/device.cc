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

    auto DeviceBuilder::build(std::weak_ptr<Instance> instance_weak) -> std::shared_ptr<Device>
    {
        auto instance = instance_weak.lock();
        ATHROW(instance, "Instance is expired");

        if (!m_physicalDevice.has_value())
        {
            auto physicalDevices = instance->get().enumeratePhysicalDevices();
            ATHROW(!physicalDevices.empty(), "No physical devices found");

            auto dgpu_it = std::ranges::find_if(physicalDevices, [](const vk::PhysicalDevice &pd) {
                return pd.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
            });
            m_physicalDevice = (dgpu_it != physicalDevices.end()) ? *dgpu_it : physicalDevices.front();
        }

        vk::PhysicalDevice physicalDevice = m_physicalDevice.value();
        auto queueFamilyProps = physicalDevice.getQueueFamilyProperties();
        uint32_t numFamilies = static_cast<uint32_t>(queueFamilyProps.size());

        std::unordered_map<uint32_t, uint32_t> allocatedQueuesPerFamily; // family_index -> count_allocated
        std::vector<uint32_t> remainingQueuesPerFamily(numFamilies);
        for (uint32_t i = 0; i < numFamilies; ++i)
        {
            remainingQueuesPerFamily[i] = queueFamilyProps[i].queueCount;
        }

        struct QueueAssignment
        {
            std::string id;
            vk::QueueFlags flags;
            uint32_t count;
            uint32_t familyIndex;
            uint32_t firstQueueIndex;
        };
        std::vector<QueueAssignment> assignments;
        assignments.reserve(m_requestedQueues.size());

        bool all_requests_satisfied = true;
        for (const auto &[id, req] : m_requestedQueues)
        {
            const auto &flags = req.first;
            const auto &count = req.second;
            std::optional<uint32_t> found_family_index;

            for (uint32_t family_idx = 0; family_idx < numFamilies; ++family_idx)
            {
                if (!(queueFamilyProps[family_idx].queueFlags & flags))
                {
                    continue;
                }

                uint32_t currently_allocated =
                    allocatedQueuesPerFamily.count(family_idx) ? allocatedQueuesPerFamily.at(family_idx) : 0;
                if (currently_allocated + count <= queueFamilyProps[family_idx].queueCount)
                {
                    found_family_index = family_idx;
                    break;
                }
            }

            if (!found_family_index.has_value())
            {
                ATHROW(false, "Could not find suitable queue family for request ID: " + id);
                all_requests_satisfied = false;
                break;
            }

            uint32_t family_idx = found_family_index.value();
            uint32_t first_queue_idx =
                allocatedQueuesPerFamily.count(family_idx) ? allocatedQueuesPerFamily.at(family_idx) : 0;

            assignments.push_back({id, flags, count, family_idx, first_queue_idx});

            allocatedQueuesPerFamily[family_idx] = first_queue_idx + count;
        }

        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        std::unordered_map<uint32_t, std::vector<float>> familyPriorities;

        for (const auto &assignment : assignments)
        {
            if (familyPriorities.find(assignment.familyIndex) == familyPriorities.end())
            {
                familyPriorities[assignment.familyIndex] = std::vector<float>();
            }
            for (uint32_t i = 0; i < assignment.count; ++i)
            {
                familyPriorities[assignment.familyIndex].push_back(1.0f);
            }
        }

        for (const auto &pair : allocatedQueuesPerFamily)
        {
            uint32_t family_idx = pair.first;
            uint32_t total_queues_in_family = pair.second;
            if (total_queues_in_family > 0)
            {
                queueCreateInfos.push_back(vk::DeviceQueueCreateInfo{}
                                               .setQueueFamilyIndex(family_idx)
                                               .setQueueCount(total_queues_in_family)
                                               .setPQueuePriorities(familyPriorities.at(family_idx).data()));
            }
        }

        std::vector<const char *> extensionsCStr;
        extensionsCStr.reserve(m_extensions.size());
        std::ranges::for_each(m_extensions,
                              [&extensionsCStr](const std::string &e) { extensionsCStr.push_back(e.c_str()); });

        vk::DeviceCreateInfo deviceInfo =
            vk::DeviceCreateInfo{}
                .setPNext(&m_deviceFeatures.get<vk::PhysicalDeviceFeatures2>())
                .setQueueCreateInfos(queueCreateInfos)
                .setPEnabledExtensionNames(extensionsCStr);

        vk::Device logicalDevice = physicalDevice.createDevice(deviceInfo);
        ATHROW(logicalDevice, "Failed to create logical device");

        auto device = std::make_shared<Device>();
        device->m_physicalDevice = physicalDevice;
        device->m_logicalDevice = logicalDevice;
        device->m_deviceBuilder = *this;

        for (const auto &assignment : assignments)
        {
            Queues queueInfo;
            queueInfo.m_id = assignment.id;
            queueInfo.m_family = assignment.familyIndex;
            queueInfo.m_flags = assignment.flags;
            queueInfo.m_handles.resize(assignment.count);

            for (uint32_t i = 0; i < assignment.count; ++i)
            {
                queueInfo.m_handles[i] = logicalDevice.getQueue(assignment.familyIndex, assignment.firstQueueIndex + i);
            }
            device->m_queues.emplace(assignment.id, std::move(queueInfo));
        }

        VULKAN_HPP_DEFAULT_DISPATCHER.init(device->m_logicalDevice);

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
