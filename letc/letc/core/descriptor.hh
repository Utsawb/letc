#pragma once

#include "letc/pch.hh"

#include "letc/core/device.hh"

namespace letc
{
    class DescriptorSetLayout;
    class Descriptor;
    class DescriptorSet;
    class DescriptorSetPool;

    struct DescriptorLayout
    {
      public:
        auto operator|(const DescriptorLayout &rhs) const -> DescriptorLayout;
        auto toString() const -> std::string;

      private:
        friend DescriptorSetLayout;

        std::string m_name;
        uint32_t m_binding;
        vk::DescriptorType m_type;
        vk::ShaderStageFlags m_stages;
        uint32_t m_count = 1;
        std::vector<vk::Sampler> m_samplers;
    };

    class DescriptorSetLayout
    {
      public:
        DescriptorSetLayout();

        auto addBinding(const std::string &name, const uint32_t &binding, const vk::DescriptorType &type,
                        const vk::ShaderStageFlags &stages, const uint32_t &count = 1,
                        const std::vector<vk::Sampler> samplers = std::vector<vk::Sampler>{}) -> DescriptorSetLayout &;

        auto operator|(const DescriptorSetLayout &rhs) const -> DescriptorSetLayout;

        auto build(std::weak_ptr<Device> device) const -> vk::UniqueDescriptorSetLayout;
        auto build(std::weak_ptr<DescriptorSetPool> pool) -> std::shared_ptr<DescriptorSet>;

        auto toString() const -> std::string;

      private:
        std::vector<DescriptorLayout> m_descriptors;
    };

    class Descriptor
    {

      private:
        friend DescriptorSetLayout;
        friend DescriptorSet;

        std::string m_name;
        uint32_t m_binding;
        vk::DescriptorType m_type;
        vk::ShaderStageFlags m_stages;
        uint32_t m_count = 1;
        std::vector<vk::Sampler> m_samplers;
    };

    class DescriptorSet
    {
      public:
        auto get() -> vk::DescriptorSet;

        auto attachBuffer(std::weak_ptr<Device> device, const uint32_t &binding, const vk::Buffer &buffer,
                          const vk::DeviceSize &range, const vk::DeviceSize &offset = 0) -> DescriptorSet &;
        auto attachImage(std::weak_ptr<Device> device, const uint32_t &binding, const vk::ImageView &view,
                         const vk::Sampler &sampler,
                         const vk::ImageLayout layout = vk::ImageLayout::eShaderReadOnlyOptimal) -> DescriptorSet &;
        auto attachTexelBuffer(std::weak_ptr<Device> device, const uint32_t &binding, const vk::BufferView &view)
            -> DescriptorSet &;

      private:
        friend DescriptorSetLayout;

        std::vector<Descriptor> m_descriptors;
        vk::DescriptorSet m_handle;
        vk::UniqueDescriptorSetLayout m_layout;
    };

    class DescriptorSetPool
    {
      public:
        DescriptorSetPool(std::weak_ptr<Device> device);
        ~DescriptorSetPool();
        auto get() -> vk::DescriptorPool;

      private:
        friend DescriptorSetLayout;

        std::weak_ptr<Device> m_device;
        vk::DescriptorPool m_pool;
    };
} // namespace letc

inline auto operator|(const std::map<uint32_t, letc::DescriptorSetLayout> &lhs,
                      const std::map<uint32_t, letc::DescriptorSetLayout> &rhs)
    -> std::map<uint32_t, letc::DescriptorSetLayout>
{
    auto cpy = lhs;
    for (const auto &[set, dsl] : rhs)
    {
        cpy[set] = cpy[set] | dsl;
    }
    return cpy;
}
