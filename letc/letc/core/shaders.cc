#include "letc/core/shaders.hh"

namespace letc
{
    auto Shader::get() -> vk::ShaderModule
    {
        return m_module;
    }

    auto Shader::getLayouts() -> std::map<uint32_t, DescriptorSetLayout>
    {
        return m_layouts;
    }

    ShaderManager::ShaderManager(std::weak_ptr<Device> device)
    {
        m_device = device;
    }

    ShaderManager::~ShaderManager()
    {
        auto device = m_device.lock();
        {
            for (const auto &[key, shader] : m_shaders)
                device->getLogical().destroyShaderModule(shader.m_module);
        }
    }

    auto ShaderManager::add(const std::filesystem::path &path, const std::string &entry) -> ShaderManager &
    {
        auto code = m_codes.try_emplace(path, readFile(path));
        auto cached = m_shaders.find({path, entry});

        if (cached != m_shaders.end())
        {
            return *this;
        }

        spirv_cross::Compiler compiler(m_codes.at(path));
        auto resources = compiler.get_shader_resources();
        auto ep = compiler.get_entry_points_and_stages();
        auto found = std::ranges::find_if(ep, [entry](const spirv_cross::EntryPoint &ep) { return ep.name == entry; });
        auto stages = spv2vk(found->execution_model);
        ATHROW(found != ep.end(), std::format("entry point {} not found", entry));

        std::map<uint32_t, DescriptorSetLayout> dsl;
        auto populateResource = [&compiler, &resources, &stages, &dsl](const spirv_cross::Resource &r,
                                                                       const vk::DescriptorType &type) {
            const auto &set = compiler.get_decoration(r.id, spv::DecorationDescriptorSet);
            const auto &binding = compiler.get_decoration(r.id, spv::DecorationBinding);
            const auto &count = 1;

            dsl[set].addBinding(r.name, binding, type, stages, count);
        };

        for (const auto &r : resources.uniform_buffers)
        {
            populateResource(r, vk::DescriptorType::eUniformBuffer);
        }

        for (const auto &r : resources.storage_buffers)
        {
            populateResource(r, vk::DescriptorType::eStorageBuffer);
        }

        for (const auto &r : resources.sampled_images)
        {
            populateResource(r, vk::DescriptorType::eCombinedImageSampler);
        }

        for (const auto &r : resources.storage_images)
        {
            populateResource(r, vk::DescriptorType::eStorageImage);
        }

        std::vector<vk::PushConstantRange> pcrs;
        for (const auto &r : resources.push_constant_buffers)
        {
            auto ranges = compiler.get_active_buffer_ranges(r.id);
            for (const auto &r : ranges)
            {
                pcrs.push_back(vk::PushConstantRange{}.setStageFlags(stages).setSize(r.range).setOffset(r.offset));
            }
        }

        auto shader = Shader{};
        auto shaderModuleInfo = vk::ShaderModuleCreateInfo{}.setCode(code.first->second);
        shader.m_entry = entry;
        shader.m_stage = stages;
        shader.m_layouts = dsl;
        shader.m_push = pcrs;
        shader.m_module = m_device.lock()->getLogical().createShaderModule(shaderModuleInfo);

        m_shaders.insert({{path, entry}, shader});

        return *this;
    }

    auto ShaderManager::getShader(const std::filesystem::path &path, const std::string &entry) -> Shader
    {
        return m_shaders.at({path, entry});
    }
} // namespace letc
