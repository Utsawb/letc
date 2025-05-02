#include "letc/core/pipeline.hh"

namespace letc
{
    GraphicsPipelineBuilder::GraphicsPipelineBuilder()
    {
        m_inputAssemblyInfo.setTopology(vk::PrimitiveTopology::eTriangleList);
        m_inputAssemblyInfo.setPrimitiveRestartEnable(false);

        m_viewportInfo.setViewportCount(1);
        m_viewportInfo.setScissorCount(1);

        m_rasterizationInfo.setDepthClampEnable(VK_FALSE);
        m_rasterizationInfo.setRasterizerDiscardEnable(VK_FALSE);
        m_rasterizationInfo.setPolygonMode(vk::PolygonMode::eFill);
        m_rasterizationInfo.setLineWidth(1.0f);
        m_rasterizationInfo.setCullMode(vk::CullModeFlagBits::eBack);
        m_rasterizationInfo.setFrontFace(vk::FrontFace::eCounterClockwise);
        m_rasterizationInfo.setDepthBiasEnable(VK_FALSE);

        m_multisampleInfo.setSampleShadingEnable(VK_FALSE);
        m_multisampleInfo.setRasterizationSamples(vk::SampleCountFlagBits::e1);

        m_colorBlendInfo.setLogicOpEnable(VK_FALSE);
        m_colorBlendInfo.setLogicOp(vk::LogicOp::eCopy);
        m_colorBlendAttachments.push_back(
            vk::PipelineColorBlendAttachmentState{}
                .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                   vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
                .setBlendEnable(VK_FALSE));

        m_renderingInfo.setColorAttachmentCount(1);
        m_renderingInfo.setDepthAttachmentFormat(vk::Format::eD32Sfloat);

        m_depthStencilInfo.setDepthTestEnable(VK_TRUE);
        m_depthStencilInfo.setDepthWriteEnable(VK_TRUE);
        m_depthStencilInfo.setDepthCompareOp(vk::CompareOp::eLess);
        m_depthStencilInfo.setDepthBoundsTestEnable(VK_FALSE);
        m_depthStencilInfo.setStencilTestEnable(VK_FALSE);

        m_dynamicStates.push_back(vk::DynamicState::eViewport);
        m_dynamicStates.push_back(vk::DynamicState::eScissor);

        m_tessellationInfo = vk::PipelineTessellationStateCreateInfo{};

        m_dynamicStateInfo = vk::PipelineDynamicStateCreateInfo{};
    }

    auto GraphicsPipelineBuilder::addShader(const Shader &shader) -> GraphicsPipelineBuilder &
    {
        m_shaders.push_back(shader);
        return *this;
    }

    auto GraphicsPipelineBuilder::addVertexBinding(const uint32_t &binding, const uint32_t &stride,
                                                   const vk::VertexInputRate &inputRate) -> GraphicsPipelineBuilder &
    {
        vk::VertexInputBindingDescription vertexInputBinding{};
        vertexInputBinding.setBinding(binding);
        vertexInputBinding.setStride(stride);
        vertexInputBinding.setInputRate(inputRate);
        m_vertexBinding.push_back(vertexInputBinding);
        return *this;
    }

    auto GraphicsPipelineBuilder::addVertexAttribute(const uint32_t &location, const uint32_t &binding,
                                                     const vk::Format &format, const uint32_t &offset)
        -> GraphicsPipelineBuilder &
    {
        vk::VertexInputAttributeDescription vertexInputAttribute{};
        vertexInputAttribute.setLocation(location);
        vertexInputAttribute.setBinding(binding);
        vertexInputAttribute.setFormat(format);
        vertexInputAttribute.setOffset(offset);
        m_vertexAttribute.push_back(vertexInputAttribute);
        return *this;
    }

    auto GraphicsPipelineBuilder::setInputAssembly(InputAssemblyCallback func) -> GraphicsPipelineBuilder &
    {
        func(m_inputAssemblyInfo);
        return *this;
    }

    auto GraphicsPipelineBuilder::setTessellation(TessellationCallback func) -> GraphicsPipelineBuilder &
    {
        func(m_tessellationInfo);
        return *this;
    }
    auto GraphicsPipelineBuilder::setViewport(ViewportCallback func) -> GraphicsPipelineBuilder &
    {
        func(m_viewportInfo);
        return *this;
    }

    auto GraphicsPipelineBuilder::setRasterization(RasterizationCallback func) -> GraphicsPipelineBuilder &
    {
        func(m_rasterizationInfo);
        return *this;
    }

    auto GraphicsPipelineBuilder::setMultisample(MultisampleCallback func) -> GraphicsPipelineBuilder &
    {
        func(m_multisampleInfo);
        return *this;
    }

    auto GraphicsPipelineBuilder::setDepthStencil(DepthStencilCallback func) -> GraphicsPipelineBuilder &
    {
        func(m_depthStencilInfo);
        return *this;
    }

    auto GraphicsPipelineBuilder::clearColorBlendAttachments() -> GraphicsPipelineBuilder &
    {
        m_colorBlendAttachments.clear();
        return *this;
    }

    auto GraphicsPipelineBuilder::addColorBlendAttachment(const vk::PipelineColorBlendAttachmentState &attachment)
        -> GraphicsPipelineBuilder &
    {
        m_colorBlendAttachments.push_back(attachment);
        return *this;
    }

    auto GraphicsPipelineBuilder::setColorBlend(ColorBlendCallback func) -> GraphicsPipelineBuilder &
    {
        func(m_colorBlendInfo);
        return *this;
    }

    auto GraphicsPipelineBuilder::setRendering(RenderingCallback func) -> GraphicsPipelineBuilder &
    {
        func(m_renderingInfo);
        return *this;
    }

    auto GraphicsPipelineBuilder::clearDynamicStates() -> GraphicsPipelineBuilder &
    {
        m_dynamicStates.clear();
        return *this;
    }

    auto GraphicsPipelineBuilder::addDynamicState(const vk::DynamicState &state) -> GraphicsPipelineBuilder &
    {
        m_dynamicStates.push_back(state);
        return *this;
    }

    auto GraphicsPipelineBuilder::build(std::weak_ptr<Device> device_weak) const -> std::shared_ptr<GraphicsPipeline>
    {
        auto device = device_weak.lock();
        ATHROW(device, "Device weak_ptr expired during GraphicsPipeline build");
        ATHROW(!m_shaders.empty(), "Cannot build GraphicsPipeline without shaders");

        std::vector<vk::PipelineShaderStageCreateInfo> shaderStageInfos;
        shaderStageInfos.reserve(m_shaders.size());

        std::map<uint32_t, DescriptorSetLayout> combinedDescriptorSetLayouts;
        std::vector<vk::PushConstantRange> combinedPushConstants;

        for (const auto &shader : m_shaders)
        {
            shaderStageInfos.push_back(vk::PipelineShaderStageCreateInfo{}
                                           .setStage(shader.m_stage)
                                           .setModule(shader.m_module)
                                           .setPName(shader.m_entry.c_str()));

            combinedDescriptorSetLayouts = combinedDescriptorSetLayouts | shader.m_layouts;
            combinedPushConstants = combinedPushConstants | shader.m_push;
        }

        std::vector<vk::DescriptorSetLayout> setLayouts;
        std::vector<vk::UniqueDescriptorSetLayout> uniqueSetLayouts;
        for (const auto &[set, layout] : combinedDescriptorSetLayouts)
        {
            uniqueSetLayouts.push_back(layout.build(device));
            setLayouts.push_back(uniqueSetLayouts.back().get());
        }

        vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.setSetLayouts(setLayouts);
        pipelineLayoutInfo.setPushConstantRanges(combinedPushConstants);

        vk::PipelineLayout pipelineLayout = device->getLogical().createPipelineLayout(pipelineLayoutInfo);
        ATHROW(pipelineLayout, "Failed to create pipeline layout");

        vk::PipelineVertexInputStateCreateInfo vertexInputInfo = m_vertexInputInfo;
        vertexInputInfo.setVertexBindingDescriptions(m_vertexBinding);
        vertexInputInfo.setVertexAttributeDescriptions(m_vertexAttribute);

        vk::PipelineColorBlendStateCreateInfo colorBlendInfo = m_colorBlendInfo;
        colorBlendInfo.setAttachments(m_colorBlendAttachments);

        vk::PipelineDynamicStateCreateInfo dynamicStateInfo = m_dynamicStateInfo;
        dynamicStateInfo.setDynamicStates(m_dynamicStates);

        vk::GraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.setFlags(m_createFlags);
        pipelineInfo.setStages(shaderStageInfos);
        pipelineInfo.setPVertexInputState(&vertexInputInfo);
        pipelineInfo.setPInputAssemblyState(&m_inputAssemblyInfo);
        pipelineInfo.setPTessellationState(&m_tessellationInfo);
        pipelineInfo.setPViewportState(&m_viewportInfo);
        pipelineInfo.setPRasterizationState(&m_rasterizationInfo);
        pipelineInfo.setPMultisampleState(&m_multisampleInfo);
        pipelineInfo.setPDepthStencilState(&m_depthStencilInfo);
        pipelineInfo.setPColorBlendState(&colorBlendInfo);
        pipelineInfo.setPDynamicState(&dynamicStateInfo);
        pipelineInfo.setLayout(pipelineLayout);

        vk::PipelineRenderingCreateInfo renderingInfo = m_renderingInfo; 
        pipelineInfo.setPNext(&renderingInfo);
        pipelineInfo.setRenderPass(VK_NULL_HANDLE); 
        pipelineInfo.setSubpass(0);              

        auto result = device->getLogical().createGraphicsPipeline(VK_NULL_HANDLE, pipelineInfo);
        ATHROW(result.result == vk::Result::eSuccess, "Failed to create graphics pipeline");
        vk::Pipeline pipeline = result.value;

        auto graphicsPipeline = std::shared_ptr<GraphicsPipeline>(new GraphicsPipeline()); 

        graphicsPipeline->m_device = device_weak;
        graphicsPipeline->m_layout = pipelineLayout;
        graphicsPipeline->m_setLayouts = combinedDescriptorSetLayouts;
        graphicsPipeline->m_handle = pipeline;

        return graphicsPipeline;
    }

    GraphicsPipeline::~GraphicsPipeline()
    {
        if (auto device = m_device.lock())
        {
            if (m_handle)
                device->getLogical().destroyPipeline(m_handle);
            if (m_layout)
                device->getLogical().destroyPipelineLayout(m_layout);
        }
    }

    auto GraphicsPipeline::bind(const vk::CommandBuffer &commandBuffer) const -> void
    {
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_handle);
    }

    auto GraphicsPipeline::getLayout() const -> vk::PipelineLayout
    {
        return m_layout;
    }

    auto GraphicsPipeline::getSetLayouts() const -> std::map<uint32_t, DescriptorSetLayout>
    {
        return m_setLayouts;
    }

} // namespace letc
