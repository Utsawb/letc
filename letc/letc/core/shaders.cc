#include "letc/core/shaders.hh"

namespace letc
{

    ShaderManager::ShaderManager(std::weak_ptr<Device> device)
    {
        slang::createGlobalSession(m_slangSession.writeRef());
        m_device = device;
    }

    auto ShaderManager::add(const std::filesystem::path &path, const std::string &entry,
                            const vk::ShaderStageFlagBits &stage) -> ShaderManager &
    {
        auto code = m_codes.try_emplace(path, readFile(path));

        auto sessionDesc = slang::SessionDesc{};
        auto targetDesc = slang::TargetDesc{};
        targetDesc.format = SLANG_SPIRV;
        targetDesc.profile = m_slangSession->findProfile("spirv_1_5");
        sessionDesc.targets = &targetDesc;
        sessionDesc.targetCount = 1;

        return *this;
    }
} // namespace letc
