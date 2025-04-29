#include <iostream>

#include <vulkan/vulkan.hpp>

struct DL
{
    uint32_t m_binding = 0;
    vk::DescriptorType m_type = vk::DescriptorType::eUniformBuffer;
    vk::ShaderStageFlags m_stages = vk::ShaderStageFlagBits::eAllGraphics;
    uint32_t m_count = 4;
    std::vector<vk::Sampler> m_samplers = std::vector<vk::Sampler>{};
};

auto main(void) -> int
{
    DL dl;

    auto dslb = vk::DescriptorSetLayoutBinding{}
                    .setBinding(dl.m_binding)
                    .setDescriptorType(dl.m_type)
                    .setStageFlags(dl.m_stages)
                    .setDescriptorCount(dl.m_count)
                    .setImmutableSamplers(dl.m_samplers);

    std::cout << dslb.descriptorCount << std::endl;
}
