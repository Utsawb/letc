#pragma once

#include "letc/pch.hh"

#include "letc/core/descriptor.hh"
#include "letc/core/device.hh"
#include "letc/core/shaders.hh"

namespace letc
{
    class GraphicsPipeline;

    class GraphicsPipelineBuilder
    {
      public:
        GraphicsPipelineBuilder();

        auto addShader(const Shader &shader) -> GraphicsPipelineBuilder &;
        auto addVertexBinding(const uint32_t &binding, const uint32_t &stride, const vk::VertexInputRate &inputRate)
            -> GraphicsPipelineBuilder &;
        auto addVertexAttribute(const uint32_t &location, const uint32_t &binding, const vk::Format &format,
                                const uint32_t &offset) -> GraphicsPipelineBuilder &;
        auto setInputAssembly(const vk::PipelineInputAssemblyStateCreateInfo &info) -> GraphicsPipelineBuilder &;
        auto setTessellation(const vk::PipelineTessellationStateCreateInfo &info) -> GraphicsPipelineBuilder &;
        auto setViewport(const vk::PipelineViewportStateCreateInfo &info) -> GraphicsPipelineBuilder &;
        auto setRasterization(const vk::PipelineRasterizationStateCreateInfo &info) -> GraphicsPipelineBuilder &;
        auto setMultisample(const vk::PipelineMultisampleStateCreateInfo &info) -> GraphicsPipelineBuilder &;
        auto setDepthStencil(const vk::PipelineDepthStencilStateCreateInfo &info) -> GraphicsPipelineBuilder &;
        auto clearColorBlendAttachments() -> GraphicsPipelineBuilder &;
        auto addColorBlendAttachment(const vk::PipelineColorBlendAttachmentState &attachment)
            -> GraphicsPipelineBuilder &;
        auto setColorBlend(const vk::PipelineColorBlendStateCreateInfo &info) -> GraphicsPipelineBuilder &;
        auto setRendering(const vk::PipelineRenderingCreateInfo &info) -> GraphicsPipelineBuilder &;
        auto clearDynamicStates() -> GraphicsPipelineBuilder &;
        auto addDynamicState(const vk::DynamicState &state) -> GraphicsPipelineBuilder &;

        auto build(std::weak_ptr<Device> device) const -> std::shared_ptr<GraphicsPipeline>;

      private:
        std::vector<Shader> m_shaders;
        std::vector<vk::VertexInputBindingDescription> m_vertexBinding;
        std::vector<vk::VertexInputAttributeDescription> m_vertexAttribute;

        vk::PipelineVertexInputStateCreateInfo m_vertexInputInfo;
        vk::PipelineInputAssemblyStateCreateInfo m_inputAssemblyInfo;
        vk::PipelineTessellationStateCreateInfo m_tessellationInfo;
        vk::PipelineViewportStateCreateInfo m_viewportInfo;
        vk::PipelineRasterizationStateCreateInfo m_rasterizationInfo;
        vk::PipelineMultisampleStateCreateInfo m_multisampleInfo;
        vk::PipelineDepthStencilStateCreateInfo m_depthStencilInfo;
        vk::PipelineColorBlendStateCreateInfo m_colorBlendInfo;
        std::vector<vk::PipelineColorBlendAttachmentState> m_colorBlendAttachments;
        vk::PipelineRenderingCreateInfo m_renderingInfo;
        vk::PipelineDynamicStateCreateInfo m_dynamicStateInfo;
        std::vector<vk::DynamicState> m_dynamicStates;
        vk::PipelineCreateFlags m_createFlags;
    };

    class GraphicsPipeline
    {
      public:
        ~GraphicsPipeline(); // Destructor needed for cleanup

        // --- Runtime Usage ---
        auto bind(const vk::CommandBuffer &commandBuffer) const -> void;
        auto getLayout() const -> vk::PipelineLayout; // Optional: if needed externally

      private:
        friend class GraphicsPipelineBuilder; // Allow builder to populate private members

        // Private constructor - only builder can create
        GraphicsPipeline() = default;

        std::weak_ptr<Device> m_device; // Keep weak_ptr to avoid cycles if Device holds Pipelines
        vk::PipelineLayout m_layout = VK_NULL_HANDLE;
        vk::Pipeline m_handle = VK_NULL_HANDLE;
        // Store shader modules if they are created uniquely here, otherwise rely on ShaderManager
        // std::vector<vk::ShaderModule> m_shaderModules; // Uncomment if needed
    };
} // namespace letc
