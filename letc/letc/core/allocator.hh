#pragma once

#include "letc/pch.hh"

#include "letc/core/device.hh"
#include "letc/core/instance.hh"

namespace letc
{
    class Allocator
    {
      public:
        Allocator(std::weak_ptr<Instance> instance, std::weak_ptr<Device> device);
        ~Allocator();
        auto get() -> VmaAllocator;

      private:
        VmaAllocator m_allocator = nullptr;
    };
} // namespace letc
