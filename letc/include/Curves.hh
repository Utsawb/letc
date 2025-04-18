#pragma once

#ifndef LETC_CURVES_HH
#define LETC_CURVES_HH

#include "pch.hh"

#include "Allocator.hh"
#include "Buffer.hh"
#include "Device.hh"
#include "Pipeline.hh"

namespace letc
{
    struct CurvesRenderer
    {
        const Allocator &allocator;
        const Device &device;

        std::unique_ptr<DescriptorLayout> layout;
        std::unique_ptr<GraphicsPipeline> pipeline;

        CurvesRenderer(const Allocator &allocator, const Device &device, const std::vector<char> &vertexCode,
                       const std::vector<char> &fragmentCode)
            : allocator(allocator), device(device)
        {
            layout = std::make_unique<DescriptorLayout>(device);
            layout->addBinding(0, 0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eAllGraphics, 1);
            layout->generateLayouts();

            letc::GraphicsPipelineBuilder gpb;
            gpb.addShaderStage(vertexCode, vk::ShaderStageFlagBits::eVertex, "vs");
            gpb.addShaderStage(fragmentCode, vk::ShaderStageFlagBits::eFragment, "fs");
            gpb.addVertexInputBinding(0, sizeof(glm::vec3), vk::VertexInputRate::eVertex);
            gpb.addVertexInputAttribute(0, 0, vk::Format::eR32G32B32Sfloat, 0);
            gpb.setLayout(layout.get());
            gpb.inputAssemblyInfo.setTopology(vk::PrimitiveTopology::eLineList);
            gpb.rasterizationInfo.setPolygonMode(vk::PolygonMode::eLine);
            gpb.renderingInfo.setColorAttachmentCount(1);
            auto fmt = vk::Format::eR8G8B8A8Srgb;
            gpb.renderingInfo.setColorAttachmentFormats(fmt);
            gpb.rasterizationInfo.setCullMode(vk::CullModeFlagBits::eNone);
            pipeline = std::make_unique<GraphicsPipeline>(device, gpb);
        }
    };

    struct Curves
    {
        BufferVector<glm::vec3> bufferVec;
        const float alpha = 0.5f;

        Curves(const Allocator &allocator, vk::DeviceSize initial_capacity = 1024)
            : bufferVec(allocator, initial_capacity, vk::BufferUsageFlagBits::eVertexBuffer,
                        VMA_MEMORY_USAGE_CPU_TO_GPU, vk::SharingMode::eExclusive)
        {
            bufferVec.vector.clear();
        }

        static glm::vec3 pointOnCurve(const glm::vec3 &p0, const glm::vec3 &p1, const glm::vec3 &p2,
                                      const glm::vec3 &p3, float t)
        {
            float t2 = t * t;
            float t3 = t2 * t;
            return (p1 + (-0.5f * p0 + 0.5f * p2) * t + (p0 - 2.5f * p1 + 2.0f * p2 - 0.5f * p3) * t2 +
                    (-0.5f * p0 + 1.5f * p1 - 1.5f * p2 + 0.5f * p3) * t3);
        }

        void generateCurve(const std::vector<glm::vec3> &pts, uint32_t k)
        {
            auto &curvePoints = bufferVec.vector;
            curvePoints.clear();

            if (pts.empty() || k == 0)
            {
                return;
            }
            if (pts.size() == 1)
            {
                curvePoints.push_back(pts[0]);
                return;
            }
            if (k == 1)
            {
                curvePoints.push_back(pts[0]);
                return;
            }

            curvePoints.reserve(k);

            size_t num_control_points = pts.size();
            size_t num_segments = num_control_points - 1;

            std::vector<std::pair<float, float>> arcLengthLUT;
            arcLengthLUT.reserve((num_segments * 20) + 1);
            arcLengthLUT.push_back({0.0f, 0.0f});
            float total_arc_length = 0.0f;
            const int arc_approx_steps_per_segment = 20;
            glm::vec3 prev_sampled_point = pts[0];

            for (size_t i = 0; i < num_segments; ++i)
            {
                const glm::vec3 &p1 = pts[i];
                const glm::vec3 &p2 = pts[i + 1];
                const glm::vec3 &p0 = (i == 0) ? p1 : pts[i - 1];
                const glm::vec3 &p3 = (i >= num_control_points - 2) ? p2 : pts[i + 2];

                for (int step = 1; step <= arc_approx_steps_per_segment; ++step)
                {
                    float t_local = static_cast<float>(step) / static_cast<float>(arc_approx_steps_per_segment);
                    glm::vec3 current_sampled_point = pointOnCurve(p0, p1, p2, p3, t_local);
                    float segment_dist = glm::distance(prev_sampled_point, current_sampled_point);
                    total_arc_length += segment_dist;
                    float t_global = static_cast<float>(i) + t_local;
                    arcLengthLUT.push_back({t_global, total_arc_length});
                    prev_sampled_point = current_sampled_point;
                }
            }

            if (total_arc_length < 1e-6f)
            {
                for (uint32_t i = 0; i < k; ++i)
                {
                    curvePoints.push_back(pts[0]);
                }
                return;
            }

            curvePoints.push_back(pts[0]);

            float target_length_step = total_arc_length / static_cast<float>(k - 1);
            size_t lut_idx = 1;

            for (uint32_t i = 1; i < k - 1; ++i)
            {
                float current_target_length = static_cast<float>(i) * target_length_step;

                while (lut_idx < arcLengthLUT.size() && arcLengthLUT[lut_idx].second < current_target_length)
                {
                    lut_idx++;
                }
                if (lut_idx >= arcLengthLUT.size())
                {
                    curvePoints.push_back(pts.back());
                    continue;
                }

                const auto &p_prev = arcLengthLUT[lut_idx - 1];
                const auto &p_curr = arcLengthLUT[lut_idx];
                float length_diff = p_curr.second - p_prev.second;
                float t_diff = p_curr.first - p_prev.first;
                float interpolation_factor =
                    (std::abs(length_diff) > 1e-6f) ? (current_target_length - p_prev.second) / length_diff : 0.0f;
                float t_global = p_prev.first + t_diff * interpolation_factor;
                size_t segment_idx = static_cast<size_t>(std::floor(t_global));
                float t_local = t_global - static_cast<float>(segment_idx);
                segment_idx = std::min(segment_idx, num_segments - 1);

                const glm::vec3 &p1 = pts[segment_idx];
                const glm::vec3 &p2 = pts[segment_idx + 1];
                const glm::vec3 &p0 = (segment_idx == 0) ? p1 : pts[segment_idx - 1];
                const glm::vec3 &p3 = (segment_idx >= num_control_points - 2) ? p2 : pts[segment_idx + 2];

                curvePoints.push_back(pointOnCurve(p0, p1, p2, p3, t_local));
            }

            curvePoints.push_back(pts.back());
        }

        void sync()
        {
            bufferVec.sync();
        }

        void draw(const vk::CommandBuffer &commandBuffer)
        {
            if (bufferVec.vector.empty() || bufferVec.buffer == VK_NULL_HANDLE)
            {
                return;
            }

            vk::DeviceSize offset = 0;
            commandBuffer.bindVertexBuffers(0, 1, &bufferVec.buffer, &offset);
            commandBuffer.draw(static_cast<uint32_t>(bufferVec.vector.size()), 1, 0, 0);
        }
    };

}; // namespace letc

#endif
