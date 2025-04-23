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

    auto DeviceBuilder::requestQueue(const std::string &id, const vk::QueueFlagBits &flags) -> DeviceBuilder &
    {
        m_requestedQueues.emplace(id, flags);
        return *this;
    }

    auto DeviceBuilder::setDevice(const vk::PhysicalDevice &physicalDevice) -> DeviceBuilder &
    {
        m_physicalDevice = physicalDevice;
        return *this;
    }

    auto DeviceBuilder::build() -> std::shared_ptr<Device>
    {
        
    }

} // namespace letc
