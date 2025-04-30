#include "letc/core/descriptor.hh"

namespace letc
{
    auto DescriptorLayout::operator|(const DescriptorLayout &rhs) const -> DescriptorLayout
    {
        ATHROW(m_name == rhs.m_name, "descriptor names don't match");
        ATHROW(m_binding == rhs.m_binding, "descriptor bindings don't match");
        ATHROW(m_type == rhs.m_type, "descriptor types don't match");
        ATHROW(m_count == rhs.m_count, "descriptor counts don't match");

        DescriptorLayout cpy = *this;
        cpy.m_stages |= rhs.m_stages;
        return cpy;
    }

    auto DescriptorLayout::toString() const -> std::string
    {
        return std::format("{}, {}, {}, {}", m_binding, m_name, vk::to_string(m_type), m_count);
    }

    DescriptorSetLayout::DescriptorSetLayout()
    {
    }

    auto DescriptorSetLayout::addBinding(const std::string &name, const uint32_t &binding,
                                         const vk::DescriptorType &type, const vk::ShaderStageFlags &stages,
                                         const uint32_t &count, const std::vector<vk::Sampler> samplers)
        -> DescriptorSetLayout &
    {
        auto dl = DescriptorLayout{};
        dl.m_name = name;
        dl.m_binding = binding;
        dl.m_type = type;
        dl.m_stages = stages;
        dl.m_count = count;
        dl.m_samplers = samplers;
        m_descriptors.push_back(dl);

        return *this;
    }

    auto DescriptorSetLayout::operator|(const DescriptorSetLayout &rhs) const -> DescriptorSetLayout
    {
        auto cpy = *this;
        for (const auto &dl : rhs.m_descriptors)
        {
            auto found = std::ranges::find_if(cpy.m_descriptors,
                                              [dl](const DescriptorLayout &i) { return i.m_binding == dl.m_binding; });
            if (found == cpy.m_descriptors.end())
            {
                cpy.m_descriptors.push_back(dl);
            }
            else
            {
                *found = *found | dl;
            }
        }

        return cpy;
    }

    auto DescriptorSetLayout::build(std::weak_ptr<Device> device) const -> vk::UniqueDescriptorSetLayout
    {
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        for (const auto &dl : m_descriptors)
        {
            auto dslb = vk::DescriptorSetLayoutBinding{}
                            .setImmutableSamplers(dl.m_samplers)
                            .setBinding(dl.m_binding)
                            .setDescriptorType(dl.m_type)
                            .setStageFlags(dl.m_stages)
                            .setDescriptorCount(dl.m_count);
            dslb.descriptorCount = dl.m_count;
            bindings.push_back(dslb);
        }

        auto dslInfo = vk::DescriptorSetLayoutCreateInfo{}.setBindings(bindings);
        return device.lock()->getLogical().createDescriptorSetLayoutUnique(dslInfo);
    }

    auto DescriptorSetLayout::build(std::weak_ptr<DescriptorSetPool> pool) -> std::shared_ptr<DescriptorSet>
    {
        vk::UniqueDescriptorSetLayout layoutHandle = build(pool.lock()->m_device);

        vk::DescriptorSetAllocateInfo allocateInfo{};
        allocateInfo.setDescriptorPool(pool.lock()->get());
        allocateInfo.setSetLayouts(*layoutHandle);

        // 3. Allocate the descriptor set handle
        vk::DescriptorSet descriptorSetHandle;
        auto result =
            pool.lock()->m_device.lock()->getLogical().allocateDescriptorSets(&allocateInfo, &descriptorSetHandle);

        auto descriptorSet = std::make_shared<DescriptorSet>();
        for (const auto &dsl : m_descriptors)
        {
            auto desc = Descriptor{};
            desc.m_name = dsl.m_name;
            desc.m_type = dsl.m_type;
            desc.m_count = dsl.m_count;
            desc.m_stages = dsl.m_stages;
            desc.m_binding = dsl.m_binding;
            desc.m_samplers = dsl.m_samplers;

            descriptorSet->m_descriptors.push_back(desc);
        }

        descriptorSet->m_handle = descriptorSetHandle;
        descriptorSet->m_layout = std::move(layoutHandle);
        descriptorSet->m_device = pool.lock()->m_device;

        return descriptorSet;
    }

    auto DescriptorSetLayout::toString() const -> std::string
    {
        std::string res;
        for (const auto &dl : m_descriptors)
        {
            res += dl.toString() + "\n";
        }
        return res;
    }

    auto DescriptorSet::get() -> vk::DescriptorSet &
    {
        return m_handle;
    }

    auto DescriptorSet::attachBuffer(const std::string &name, const vk::Buffer &buffer, const vk::DeviceSize &range,
                                     const vk::DeviceSize &offset) -> DescriptorSet &
    {
        vk::DescriptorBufferInfo bufferInfo{};
        bufferInfo.setBuffer(buffer);
        bufferInfo.setRange(range);
        bufferInfo.setOffset(offset);

        auto found = std::ranges::find_if(m_descriptors, [name](const Descriptor &d) { return d.m_name == name; });
        ATHROW(found != m_descriptors.end(), std::format("could not find {}", name));

        auto write = vk::WriteDescriptorSet{}
                         .setDstSet(m_handle)
                         .setDstBinding(found->m_binding)
                         .setDescriptorCount(1)
                         .setDescriptorType(found->m_type)
                         .setBufferInfo(bufferInfo);

        m_device.lock()->getLogical().updateDescriptorSets(write, {});

        return *this;
    }

    auto DescriptorSet::attachBuffer(const uint32_t &binding, const vk::Buffer &buffer, const vk::DeviceSize &range,
                                     const vk::DeviceSize &offset) -> DescriptorSet &
    {
        vk::DescriptorBufferInfo bufferInfo{};
        bufferInfo.setBuffer(buffer);
        bufferInfo.setRange(range);
        bufferInfo.setOffset(offset);

        auto found =
            std::ranges::find_if(m_descriptors, [binding](const Descriptor &d) { return d.m_binding == binding; });

        auto write = vk::WriteDescriptorSet{}
                         .setDstSet(m_handle)
                         .setDstBinding(binding)
                         .setDescriptorCount(1)
                         .setDescriptorType(found->m_type)
                         .setBufferInfo(bufferInfo);

        m_device.lock()->getLogical().updateDescriptorSets(write, {});

        return *this;
    }

    auto DescriptorSet::attachImage(const uint32_t &binding, const vk::ImageView &view, const vk::Sampler &sampler,
                                    const vk::ImageLayout layout) -> DescriptorSet &
    {

        return *this;
    }
    auto DescriptorSet::attachTexelBuffer(const uint32_t &binding, const vk::BufferView &view) -> DescriptorSet &
    {

        return *this;
    }

    DescriptorSetPool::DescriptorSetPool(std::weak_ptr<Device> device)
    {
        m_device = device;
        std::vector<vk::DescriptorPoolSize> descriptorPoolSizes = {
            vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, 1024},
            vk::DescriptorPoolSize{vk::DescriptorType::eUniformTexelBuffer, 1024},
            vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, 1024},
            vk::DescriptorPoolSize{vk::DescriptorType::eStorageTexelBuffer, 1024},
            vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, 1024},
        };
        vk::DescriptorPoolCreateInfo descriptorPoolInfo{};
        descriptorPoolInfo.setMaxSets(1024);
        descriptorPoolInfo.setPoolSizes(descriptorPoolSizes);
        descriptorPoolInfo.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
        m_pool = device.lock()->getLogical().createDescriptorPool(descriptorPoolInfo);
    }

    DescriptorSetPool::~DescriptorSetPool()
    {
        m_device.lock()->getLogical().destroyDescriptorPool(m_pool);
    }

    auto DescriptorSetPool::get() -> vk::DescriptorPool
    {
        return m_pool;
    }

} // namespace letc
