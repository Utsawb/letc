#pragma once

#include "letc/pch.hh"

#include "letc/core/device.hh"

namespace letc
{
    class DescriptorSetLayout;
    class DescriptorSet;

    class DescriptorSetLayoutBuilder
    {
      public:
        DescriptorSetLayoutBuilder();

        auto addBinding(const std::string &name, const uint32_t &binding, const vk::DescriptorType &type,
                        const vk::ShaderStageFlags &stages, const uint32_t &count = 1,
                        const std::vector<vk::Sampler> samplers = std::vector<vk::Sampler>{})
            -> DescriptorSetLayoutBuilder &;

        auto build(std::weak_ptr<Device> device) const -> std::shared_ptr<DescriptorSetLayout>;

      private:
        std::map<uint32_t, vk::DescriptorSetLayoutBinding> m_bindings;
        std::map<std::string, vk::DescriptorSetLayoutBinding> m_names;
    };

    class DescriptorSetLayout
    {
      public:
        ~DescriptorSetLayout();
        auto get() -> vk::DescriptorSetLayout;
        auto getInfo() -> std::map<std::string, vk::DescriptorSetLayoutBinding>;

      private:
        friend DescriptorSetLayoutBuilder;

        std::weak_ptr<Device> m_device;
        vk::DescriptorSetLayout m_handle;
        std::map<uint32_t, vk::DescriptorSetLayoutBinding> m_bindings;
        std::map<std::string, vk::DescriptorSetLayoutBinding> m_names;
    };

    class DescriptorSet
    {
      private:
        vk::DescriptorSet m_handle;
    };
} // namespace letc
