#pragma once

#include "letc/pch.hh"

#include "letc/core/device.hh"

namespace std
{
    template <> struct hash<std::pair<std::filesystem::path, std::string>>
    {
        std::size_t operator()(const std::pair<std::filesystem::path, std::string> &key) const
        {
            return std::hash<std::string>()(key.first) ^ std::hash<std::string>()(key.second);
        }
    };
} // namespace std

namespace letc
{
    class ShaderManager;

    class Shader
    {

      private:
        friend ShaderManager;

        std::string m_entry;
        vk::ShaderStageFlagBits m_stage;
        std::map<uint32_t, std::map<uint32_t, vk::DescriptorSetLayoutBinding>> m_bindings;
        std::vector<vk::DescriptorSetLayout> m_layouts;
    };

    class ShaderManager
    {
      public:
        ShaderManager(std::weak_ptr<Device> device);

        auto add(const std::filesystem::path &path, const std::string &entry, const vk::ShaderStageFlagBits &stage)
            -> ShaderManager &;

      private:
        Slang::ComPtr<slang::IGlobalSession> m_slangSession;
        std::weak_ptr<Device> m_device;
        std::unordered_map<std::pair<std::filesystem::path, std::string>, Shader> m_shaders;
        std::unordered_map<std::filesystem::path, std::vector<char>> m_codes;
    };

} // namespace letc
