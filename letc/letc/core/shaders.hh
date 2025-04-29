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
        auto getLayouts() -> std::map<uint32_t, DescriptorSetLayout>;

      private:
        friend ShaderManager;
        friend GraphicsPipelineBuilder;

        std::string m_entry;
        vk::ShaderStageFlagBits m_stage;
        std::map<uint32_t, DescriptorSetLayout> m_layouts;
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

/**
 * @brief Merges two vectors of push constant ranges.
 *
 * Combines push constant ranges from two vectors. If ranges with the same
 * offset and size exist in both vectors, their stage flags are merged using
 * bitwise OR. Otherwise, unique ranges from the right-hand side are appended.
 *
 * @param lhs The left-hand side vector of push constant ranges.
 * @param rhs The right-hand side vector of push constant ranges.
 * @return A new vector containing the merged push constant ranges.
 */
inline auto operator|(const std::vector<vk::PushConstantRange> &lhs, const std::vector<vk::PushConstantRange> &rhs)
    -> std::vector<vk::PushConstantRange>
{
    // Create a copy of the left-hand side vector to store the result.
    auto merged_ranges = lhs; //

    // Iterate through each push constant range in the right-hand side vector.
    for (const auto &rhs_range : rhs) //
    {
        // Try to find a range in the merged vector with the same offset and size.
        auto found_it = std::ranges::find_if(merged_ranges, [&rhs_range](const vk::PushConstantRange &existing_range) {
            return existing_range.offset == rhs_range.offset && existing_range.size == rhs_range.size; //
        });

        // If a matching range is found...
        if (found_it != merged_ranges.end())
        {
            // Merge the stage flags using bitwise OR.
            found_it->stageFlags |= rhs_range.stageFlags;
        }
        else // If no matching range is found...
        {
            // Add the range from the right-hand side vector to the merged vector.
            merged_ranges.push_back(rhs_range); //
        }
    }

    // Return the vector containing the merged ranges.
    return merged_ranges;
}
