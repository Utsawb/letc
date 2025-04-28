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

        // Sensible defaults - often overridden
        m_renderingInfo.setColorAttachmentCount(1); // Assuming one color attachment by default
        // m_renderingInfo.setPColorAttachmentFormats(...); // Needs format info, set via callback or later
        m_renderingInfo.setDepthAttachmentFormat(vk::Format::eD32Sfloat); // Common default

        m_depthStencilInfo.setDepthTestEnable(VK_TRUE);
        m_depthStencilInfo.setDepthWriteEnable(VK_TRUE);
        m_depthStencilInfo.setDepthCompareOp(vk::CompareOp::eLess);
        m_depthStencilInfo.setDepthBoundsTestEnable(VK_FALSE);
        m_depthStencilInfo.setStencilTestEnable(VK_FALSE);

        // Dynamic states often include viewport and scissor
        m_dynamicStates.push_back(vk::DynamicState::eViewport);
        m_dynamicStates.push_back(vk::DynamicState::eScissor);

        // Default tessellation state (no tessellation)
        m_tessellationInfo = vk::PipelineTessellationStateCreateInfo{};

        // Default dynamic state info (will be updated in build)
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
        // The callback modifies the main ColorBlendStateCreateInfo.
        // Attachments are still managed separately via add/clear methods.
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

    /* // Optional: Callback for the main dynamic state struct
    auto GraphicsPipelineBuilder::setDynamicState(DynamicStateCallback func) -> GraphicsPipelineBuilder& {
        func(m_dynamicStateInfo);
        return *this;
    }
    */

    // --- Build Implementation (largely unchanged, but uses the modified internal structs) ---
    auto GraphicsPipelineBuilder::build(std::weak_ptr<Device> device_weak) const -> std::shared_ptr<GraphicsPipeline>
    {
        auto device = device_weak.lock();
        ATHROW(device, "Device weak_ptr expired during GraphicsPipeline build");
        ATHROW(!m_shaders.empty(), "Cannot build GraphicsPipeline without shaders");

        std::vector<vk::PipelineShaderStageCreateInfo> shaderStageInfos;
        shaderStageInfos.reserve(m_shaders.size());
        std::map<uint32_t, std::shared_ptr<DescriptorSetLayout>> combinedLayouts; // set -> layout
        std::vector<vk::PushConstantRange> combinedPushConstants;

        // --- Shader Processing (Unchanged) ---
        for (const auto &shader : m_shaders)
        {
            shaderStageInfos.push_back(vk::PipelineShaderStageCreateInfo{}
                                           .setStage(shader.m_stage)
                                           .setModule(shader.m_module)
                                           .setPName(shader.m_entry.c_str()));

            for (const auto &[setIndex, layout] : shader.m_layouts)
            {
                combinedLayouts[setIndex] = layout;
            }

            combinedPushConstants.insert(combinedPushConstants.end(), shader.m_push.begin(), shader.m_push.end());
        }

        std::vector<vk::DescriptorSetLayout> setLayouts;
        for (const auto &[setIndex, layoutPtr] : combinedLayouts)
        {
            setLayouts.push_back(layoutPtr->get());
        }

        vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.setSetLayouts(setLayouts);
        pipelineLayoutInfo.setPushConstantRanges(combinedPushConstants);

        vk::PipelineLayout pipelineLayout = device->getLogical().createPipelineLayout(pipelineLayoutInfo);
        ATHROW(pipelineLayout, "Failed to create pipeline layout");

        // --- Prepare State Create Infos (using potentially modified member structs) ---

        // Vertex Input (binding/attributes added separately)
        vk::PipelineVertexInputStateCreateInfo vertexInputInfo = m_vertexInputInfo; // Start with defaults if any
        vertexInputInfo.setVertexBindingDescriptions(m_vertexBinding);
        vertexInputInfo.setVertexAttributeDescriptions(m_vertexAttribute);

        // Color Blend (attachments added separately)
        vk::PipelineColorBlendStateCreateInfo colorBlendInfo =
            m_colorBlendInfo;                                   // Uses the (potentially modified) member
        colorBlendInfo.setAttachments(m_colorBlendAttachments); // Set attachments managed separately

        // Dynamic State (states added separately)
        // Use a const_cast ONLY if absolutely necessary and safe. It's generally better
        // to make build() non-const if it needs to modify members temporarily for setup.
        // However, for setting the pDynamicStates pointer, it's common practice here.
        vk::PipelineDynamicStateCreateInfo dynamicStateInfo =
            m_dynamicStateInfo;                             // Uses the (potentially modified) member
        dynamicStateInfo.setDynamicStates(m_dynamicStates); // Set states managed separately

        // --- Graphics Pipeline Create Info ---
        vk::GraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.setFlags(m_createFlags);
        pipelineInfo.setStages(shaderStageInfos);
        pipelineInfo.setPVertexInputState(&vertexInputInfo);
        pipelineInfo.setPInputAssemblyState(&m_inputAssemblyInfo); // Use member directly
        pipelineInfo.setPTessellationState(&m_tessellationInfo);   // Use member directly
        pipelineInfo.setPViewportState(&m_viewportInfo);           // Use member directly
        pipelineInfo.setPRasterizationState(&m_rasterizationInfo); // Use member directly
        pipelineInfo.setPMultisampleState(&m_multisampleInfo);     // Use member directly
        pipelineInfo.setPDepthStencilState(&m_depthStencilInfo);   // Use member directly
        pipelineInfo.setPColorBlendState(&colorBlendInfo);         // Use locally prepared struct
        pipelineInfo.setPDynamicState(&dynamicStateInfo);          // Use locally prepared struct
        pipelineInfo.setLayout(pipelineLayout);

        // Dynamic Rendering setup
        // Use a const_cast ONLY if absolutely necessary and safe.
        // See comment above for dynamicStateInfo.
        vk::PipelineRenderingCreateInfo renderingInfo = m_renderingInfo; // Use member directly
        pipelineInfo.setPNext(&renderingInfo);
        pipelineInfo.setRenderPass(VK_NULL_HANDLE); // Explicitly null for dynamic rendering
        pipelineInfo.setSubpass(0);                 // Subpass index is 0 for dynamic rendering

        // --- Create Pipeline ---
        auto result = device->getLogical().createGraphicsPipeline(VK_NULL_HANDLE, pipelineInfo);
        ATHROW(result.result == vk::Result::eSuccess, "Failed to create graphics pipeline");
        vk::Pipeline pipeline = result.value;

        // --- Create and Return Shared Pointer ---
        auto graphicsPipeline = std::shared_ptr<GraphicsPipeline>(new GraphicsPipeline()); // Use private constructor

        graphicsPipeline->m_device = device_weak;
        graphicsPipeline->m_layout = pipelineLayout;
        graphicsPipeline->m_handle = pipeline;

        return graphicsPipeline;
    }

    // --- GraphicsPipeline Implementation (Unchanged) ---

    GraphicsPipeline::~GraphicsPipeline()
    {
        if (auto device = m_device.lock()) // Check if device still exists
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

} // namespace letc
