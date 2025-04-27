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
    struct ResourceLayout
    {
        std::string name;
        uint32_t binding;
        vk::DescriptorType type;
        vk::ShaderStageFlags stages;
        uint32_t count = 1;
    };

    struct ResourceSetLayout
    {
        std::unordered_map<uint32_t, ResourceLayout> resourceLayouts;
        vk::DescriptorSetLayout layout;
    };

    class ShaderManager;
    class Shader
    {
      private:
        friend ShaderManager;

        std::string m_entry;
        vk::ShaderStageFlagBits m_stage;
        std::vector<ResourceSetLayout> m_layouts;
        std::vector<vk::PushConstantRange> m_push;
        vk::ShaderModule m_module;
    };

    class ShaderManager
    {
      public:
        ShaderManager(std::weak_ptr<Device> device);

        auto add(const std::filesystem::path &path, const std::string &entry, const vk::ShaderStageFlagBits &stage)
            -> ShaderManager &;

        auto getShader(const std::filesystem::path &path, const std::string &entry) -> Shader;

      private:
        std::weak_ptr<Device> m_device;
        std::unordered_map<std::pair<std::filesystem::path, std::string>, Shader> m_shaders;
        std::unordered_map<std::filesystem::path, std::vector<uint32_t>> m_codes;
    };

} // namespace letc
