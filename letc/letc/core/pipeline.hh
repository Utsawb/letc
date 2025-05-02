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
        using InputAssemblyCallback = std::function<void(vk::PipelineInputAssemblyStateCreateInfo &)>;
        using TessellationCallback = std::function<void(vk::PipelineTessellationStateCreateInfo &)>;
        using ViewportCallback = std::function<void(vk::PipelineViewportStateCreateInfo &)>;
        using RasterizationCallback = std::function<void(vk::PipelineRasterizationStateCreateInfo &)>;
        using MultisampleCallback = std::function<void(vk::PipelineMultisampleStateCreateInfo &)>;
        using DepthStencilCallback = std::function<void(vk::PipelineDepthStencilStateCreateInfo &)>;
        using ColorBlendCallback = std::function<void(vk::PipelineColorBlendStateCreateInfo &)>;
        using RenderingCallback = std::function<void(vk::PipelineRenderingCreateInfo &)>;
        using DynamicStateCallback = std::function<void(vk::PipelineDynamicStateCreateInfo &)>;

        GraphicsPipelineBuilder();

        auto addShader(const Shader &shader) -> GraphicsPipelineBuilder &;
        auto addVertexBinding(const uint32_t &binding, const uint32_t &stride, const vk::VertexInputRate &inputRate)
            -> GraphicsPipelineBuilder &;
        auto addVertexAttribute(const uint32_t &location, const uint32_t &binding, const vk::Format &format,
                                const uint32_t &offset) -> GraphicsPipelineBuilder &;

        auto setInputAssembly(InputAssemblyCallback func) -> GraphicsPipelineBuilder &;
        auto setTessellation(TessellationCallback func) -> GraphicsPipelineBuilder &;
        auto setViewport(ViewportCallback func) -> GraphicsPipelineBuilder &;
        auto setRasterization(RasterizationCallback func) -> GraphicsPipelineBuilder &;
        auto setMultisample(MultisampleCallback func) -> GraphicsPipelineBuilder &;
        auto setDepthStencil(DepthStencilCallback func) -> GraphicsPipelineBuilder &;

        auto clearColorBlendAttachments() -> GraphicsPipelineBuilder &;
        auto addColorBlendAttachment(const vk::PipelineColorBlendAttachmentState &attachment)
            -> GraphicsPipelineBuilder &;
        auto setColorBlend(ColorBlendCallback func) -> GraphicsPipelineBuilder &;
        auto setRendering(RenderingCallback func) -> GraphicsPipelineBuilder &;
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
        ~GraphicsPipeline();

        auto bind(const vk::CommandBuffer &commandBuffer) const -> void;
        auto getLayout() const -> vk::PipelineLayout;
        auto getSetLayouts() const -> std::map<uint32_t, DescriptorSetLayout>;

      private:
        friend class GraphicsPipelineBuilder;

        std::weak_ptr<Device> m_device;
        vk::PipelineLayout m_layout = nullptr;
        std::map<uint32_t, DescriptorSetLayout> m_setLayouts;
        vk::Pipeline m_handle = nullptr;
    };
} // namespace letc
