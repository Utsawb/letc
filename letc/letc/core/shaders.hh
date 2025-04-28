#pragma once

#include "letc/pch.hh"

#include "letc/core/descriptor.hh"
#include "letc/core/device.hh"

/**
 * @brief Converts a SPIR-V Execution Model to the corresponding Vulkan Shader Stage Bit.
 *
 * Provides a direct mapping for known execution models to shader stages.
 * Assumes the input ExecutionModel corresponds to a valid Vulkan shader stage.
 * No runtime error checking for unmappable values is performed, as requested.
 * NV aliases for KHR ray tracing stages map to the KHR flags.
 * ExecutionModelKernel is mapped to Compute as the closest equivalent.
 *
 * @param em The SPIR-V Execution Model.
 * @return The corresponding vk::ShaderStageFlagBits.
 */
inline auto spv2vk(const spv::ExecutionModel em) -> vk::ShaderStageFlagBits
{
    switch (em)
    {
    case spv::ExecutionModel::ExecutionModelVertex:
        return vk::ShaderStageFlagBits::eVertex;
    case spv::ExecutionModel::ExecutionModelTessellationControl:
        return vk::ShaderStageFlagBits::eTessellationControl;
    case spv::ExecutionModel::ExecutionModelTessellationEvaluation:
        return vk::ShaderStageFlagBits::eTessellationEvaluation;
    case spv::ExecutionModel::ExecutionModelGeometry:
        return vk::ShaderStageFlagBits::eGeometry;
    case spv::ExecutionModel::ExecutionModelFragment:
        return vk::ShaderStageFlagBits::eFragment;
    case spv::ExecutionModel::ExecutionModelGLCompute:
    case spv::ExecutionModel::ExecutionModelKernel:
        return vk::ShaderStageFlagBits::eCompute;
    case spv::ExecutionModel::ExecutionModelTaskNV:
        return vk::ShaderStageFlagBits::eTaskNV;
    case spv::ExecutionModel::ExecutionModelMeshNV:
        return vk::ShaderStageFlagBits::eMeshNV;
    case spv::ExecutionModel::ExecutionModelRayGenerationKHR:
        return vk::ShaderStageFlagBits::eRaygenKHR;
    case spv::ExecutionModel::ExecutionModelIntersectionKHR:
        return vk::ShaderStageFlagBits::eIntersectionKHR;
    case spv::ExecutionModel::ExecutionModelAnyHitKHR:
        return vk::ShaderStageFlagBits::eAnyHitKHR;
    case spv::ExecutionModel::ExecutionModelClosestHitKHR:
        return vk::ShaderStageFlagBits::eClosestHitKHR;
    case spv::ExecutionModel::ExecutionModelMissKHR:
        return vk::ShaderStageFlagBits::eMissKHR;
    case spv::ExecutionModel::ExecutionModelCallableKHR:
        return vk::ShaderStageFlagBits::eCallableKHR;
    case spv::ExecutionModel::ExecutionModelTaskEXT:
        return vk::ShaderStageFlagBits::eTaskEXT;
    case spv::ExecutionModel::ExecutionModelMeshEXT:
        return vk::ShaderStageFlagBits::eMeshEXT;
    default:
        throw std::runtime_format("unmappable spv::ExecutionModel");
        return vk::ShaderStageFlagBits::eAll;
    }
}
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
    class GraphicsPipelineBuilder;

    class Shader
    {
      public:
        auto get() -> vk::ShaderModule;
        auto getLayouts() -> std::map<uint32_t, std::shared_ptr<DescriptorSetLayout>>;

      private:
        friend ShaderManager;
        friend GraphicsPipelineBuilder;

        std::string m_entry;
        vk::ShaderStageFlagBits m_stage;
        std::map<uint32_t, std::shared_ptr<DescriptorSetLayout>> m_layouts;
        std::vector<vk::PushConstantRange> m_push;
        vk::ShaderModule m_module;
    };

    class ShaderManager
    {
      public:
        ShaderManager(std::weak_ptr<Device> device);
        ~ShaderManager();

        ShaderManager(const ShaderManager &) = delete;
        auto operator=(const ShaderManager &) -> ShaderManager & = delete;
        ShaderManager(ShaderManager &&) noexcept = default;
        auto operator=(ShaderManager &&) noexcept -> ShaderManager & = default;

        auto add(const std::filesystem::path &path, const std::string &entry) -> ShaderManager &;
        auto getShader(const std::filesystem::path &path, const std::string &entry) -> Shader;

      private:
        std::weak_ptr<Device> m_device;
        std::unordered_map<std::pair<std::filesystem::path, std::string>, Shader> m_shaders;
        std::unordered_map<std::filesystem::path, std::vector<uint32_t>> m_codes;
    };

} // namespace letc
