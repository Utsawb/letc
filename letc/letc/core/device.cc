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

    // this is my very bad code + gemini induced verbosity, fix later
    // but honestly device selection and queue handling will require like
    // a whole dp solution or something of that sort
    auto DeviceBuilder::build(std::weak_ptr<Instance> instance_weak) -> std::shared_ptr<Device>
    {
        auto instance = instance_weak.lock();
        ATHROW(instance, "Instance is expired");

        // 1. Select Physical Device if not already set
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

        // 2. Allocate Queues - This is the core logic change
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
            uint32_t firstQueueIndex; // Starting index within the family
        };
        std::vector<QueueAssignment> assignments;
        assignments.reserve(m_requestedQueues.size());

        bool all_requests_satisfied = true;
        for (const auto &[id, req] : m_requestedQueues)
        {
            const auto &flags = req.first;
            const auto &count = req.second;
            std::optional<uint32_t> found_family_index;

            // Try to find a suitable family
            for (uint32_t family_idx = 0; family_idx < numFamilies; ++family_idx)
            {
                // Check if family supports the required flags
                if (!(queueFamilyProps[family_idx].queueFlags & flags))
                {
                    continue; // Does not support the flags
                }

                // Check if the family has enough remaining queues
                uint32_t currently_allocated =
                    allocatedQueuesPerFamily.count(family_idx) ? allocatedQueuesPerFamily.at(family_idx) : 0;
                if (currently_allocated + count <= queueFamilyProps[family_idx].queueCount)
                {
                    // Found a suitable family
                    found_family_index = family_idx;
                    break;
                }
            }

            if (!found_family_index.has_value())
            {
                // Could not find *any* family for this request given current allocations
                // It's possible a different ordering or combination might work, but this greedy approach failed.
                // For a more robust solution, a backtracking or max-flow algorithm might be needed,
                // but this revised greedy approach is often sufficient.
                ATHROW(false, "Could not find suitable queue family for request ID: " + id);
                all_requests_satisfied = false; // Should not be reached due to ATHROW
                break;
            }

            // Assign queues from the found family
            uint32_t family_idx = found_family_index.value();
            uint32_t first_queue_idx =
                allocatedQueuesPerFamily.count(family_idx) ? allocatedQueuesPerFamily.at(family_idx) : 0;

            assignments.push_back({id, flags, count, family_idx, first_queue_idx});

            // Update allocated count for this family
            allocatedQueuesPerFamily[family_idx] = first_queue_idx + count;
        }
        // ATHROW already handles the failure case inside the loop

        // 3. Prepare Device Creation Info
        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        std::unordered_map<uint32_t, std::vector<float>> familyPriorities; // family_index -> priorities

        // Group assignments by family index to create the DeviceQueueCreateInfos
        for (const auto &assignment : assignments)
        {
            if (familyPriorities.find(assignment.familyIndex) == familyPriorities.end())
            {
                // Initialize priorities for this family if not already done
                familyPriorities[assignment.familyIndex] = std::vector<float>();
            }
            // Add 'count' priorities for this specific request
            for (uint32_t i = 0; i < assignment.count; ++i)
            {
                // Using a default priority of 1.0f. You could customize this.
                familyPriorities[assignment.familyIndex].push_back(1.0f);
            }
        }

        // Now create one vkDeviceQueueCreateInfo per family that has allocated queues
        for (const auto &pair : allocatedQueuesPerFamily)
        {
            uint32_t family_idx = pair.first;
            uint32_t total_queues_in_family = pair.second; // Total queues allocated from this family
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

        // Chain features structure for device creation
        vk::DeviceCreateInfo deviceInfo =
            vk::DeviceCreateInfo{}
                .setPNext(&m_deviceFeatures.get<vk::PhysicalDeviceFeatures2>()) // Get the head of the chain
                .setQueueCreateInfos(queueCreateInfos)
                .setPEnabledExtensionNames(extensionsCStr);

        // 4. Create Logical Device
        vk::Device logicalDevice = physicalDevice.createDevice(deviceInfo);
        ATHROW(logicalDevice, "Failed to create logical device");

        // 5. Create Device Object and Retrieve Queues
        auto device = std::make_shared<Device>();
        device->m_physicalDevice = physicalDevice;
        device->m_logicalDevice = logicalDevice;
        device->m_deviceBuilder = *this; // Store the builder configuration if needed

        // Retrieve queue handles based on assignments
        for (const auto &assignment : assignments)
        {
            Queues queueInfo;
            queueInfo.m_id = assignment.id;
            queueInfo.m_family = assignment.familyIndex;
            queueInfo.m_flags = assignment.flags; // Store the requested flags
            queueInfo.m_handles.resize(assignment.count);

            for (uint32_t i = 0; i < assignment.count; ++i)
            {
                queueInfo.m_handles[i] = logicalDevice.getQueue(assignment.familyIndex, assignment.firstQueueIndex + i);
            }
            device->m_queues.emplace(assignment.id, std::move(queueInfo)); // Use emplace and move
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
