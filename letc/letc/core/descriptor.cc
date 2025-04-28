#include "letc/core/descriptor.hh"

namespace letc
{
    DescriptorSetLayoutBuilder::DescriptorSetLayoutBuilder()
    {
    }

    auto DescriptorSetLayoutBuilder::addBinding(const std::string &name, const uint32_t &binding,
                                                const vk::DescriptorType &type, const vk::ShaderStageFlags &stages,
                                                const uint32_t &count, const std::vector<vk::Sampler> samplers)
        -> DescriptorSetLayoutBuilder &
    {
        auto dslb = vk::DescriptorSetLayoutBinding{}
                        .setBinding(binding)
                        .setDescriptorType(type)
                        .setStageFlags(stages)
                        .setDescriptorCount(count)
                        .setImmutableSamplers(samplers);

        m_bindings[binding] = dslb;
        m_names[name] = m_bindings[binding];

        return *this;
    }

    auto DescriptorSetLayoutBuilder::build(std::weak_ptr<Device> device) const -> std::shared_ptr<DescriptorSetLayout>
    {
        std::vector<vk::DescriptorSetLayoutBinding> serialBindings;
        for (const auto &[set, binding] : m_bindings)
        {
            serialBindings.push_back(binding);
        }

        auto dsl = std::make_shared<DescriptorSetLayout>();
        dsl->m_device = device;
        dsl->m_handle = device.lock()->getLogical().createDescriptorSetLayout(
            vk::DescriptorSetLayoutCreateInfo{}.setBindings(serialBindings));
        dsl->m_bindings = m_bindings;
        dsl->m_names = m_names;

        return dsl;
    }

    DescriptorSetLayout::~DescriptorSetLayout()
    {
        m_device.lock()->getLogical().destroyDescriptorSetLayout(m_handle);
    }

    auto DescriptorSetLayout::get() -> vk::DescriptorSetLayout
    {
        return m_handle;
    }

} // namespace letc
