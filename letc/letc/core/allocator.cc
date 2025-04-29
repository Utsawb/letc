#include "letc/core/allocator.hh"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace letc
{
    Allocator::Allocator(std::weak_ptr<Instance> instance, std::weak_ptr<Device> device)
    {
        VmaAllocatorCreateInfo allocatorInfo{};
        allocatorInfo.instance = instance.lock()->get();
        allocatorInfo.physicalDevice = device.lock()->getPhysical();
        allocatorInfo.device = device.lock()->getLogical();
        auto result = vmaCreateAllocator(&allocatorInfo, &m_allocator);
    }

    Allocator::~Allocator()
    {
        vmaDestroyAllocator(m_allocator);
    }

    auto Allocator::get() -> VmaAllocator
    {
        return m_allocator;
    }
} // namespace letc
