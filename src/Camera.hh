#pragma once
#include <memory>
#include <vulkan/vulkan_enums.hpp>
#ifndef LETC_CAMERA_HH
#define LETC_CAMERA_HH

#include "Allocator.hh"
#include "Buffer.hh"

namespace letc
{
    struct Camera
    {
        glm::vec4 eye;
        glm::vec4 center;
        glm::vec4 up;

        float fovy;
        float aspect;
        float near;
        float far;

        struct Uniform
        {
            glm::mat4 view;
            glm::mat4 proj;
        };
        Uniform uniform;
        std::unique_ptr<Buffer> buffer;

        Camera(const Allocator &allocator, const glm::vec4 &eye, const glm::vec4 &center, const glm::vec4 &up,
               const float &fovy, const float &aspect, const float &near = 0.001f, const float &far = 10000.0f)
            : eye(eye), center(center), up(up), fovy(fovy), aspect(aspect), near(near), far(far)
        {
            this->buffer = std::make_unique<Buffer>(allocator, sizeof(Uniform), vk::BufferUsageFlagBits::eUniformBuffer,
                                                    VMA_MEMORY_USAGE_CPU_TO_GPU);

            this->uniform.view = glm::lookAt(glm::vec3(eye), glm::vec3(center), glm::vec3(up));
            this->uniform.proj = glm::perspectiveZO(glm::radians(fovy), aspect, near, far);
            this->uniform.proj[1][1] *= -1;
        }

        void updateView()
        {
            this->uniform.view = glm::lookAt(glm::vec3(eye), glm::vec3(center), glm::vec3(up));
        }

        void updateProj()
        {
            this->uniform.proj = glm::perspectiveZO(glm::radians(fovy), aspect, near, far);
            this->uniform.proj[1][1] *= -1;
        }

        void cpy()
        {
            this->buffer->cpy(&this->uniform, sizeof(Uniform));
        }

        void orbit(const float &x, const float &y)
        {
            glm::vec3 center = glm::vec3(this->center);
            glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f); // constant up vector

            // Compute the offset from the center to the eye and its length.
            glm::vec3 offset = glm::vec3(this->eye) - center;
            float radius = glm::length(offset);
            glm::vec3 view = glm::normalize(offset);

            // Horizontal rotation: rotate the view vector around the constant up vector.
            glm::mat4 rotX = glm::rotate(glm::mat4(1.0f), glm::radians(-x), worldUp);
            view = glm::vec3(rotX * glm::vec4(view, 0.0f));

            // Recalculate the right vector using the updated view vector.
            glm::vec3 right = glm::normalize(glm::cross(view, worldUp));

            // Vertical rotation: rotate the view vector around the right vector.
            glm::mat4 rotY = glm::rotate(glm::mat4(1.0f), glm::radians(y), right);
            view = glm::vec3(rotY * glm::vec4(view, 0.0f));

            // Update the eye position while maintaining the same distance (radius) from the center.
            eye = glm::vec4(center + view * radius, 1.0f);
            // Keep the up vector constant.
            up = glm::vec4(worldUp, 0.0f);
        }

        void zoom(const float &z)
        {
            glm::vec3 eye = glm::vec3(this->eye);
            glm::vec3 center = glm::vec3(this->center);
            glm::vec3 view = glm::normalize(center - eye);

            float current_distance = glm::length(center - eye);
            float threshold_distance = 10.0f;
            float decay_factor = 1.0f;
            float min_zoom_factor = 0.1f;

            if (current_distance < threshold_distance)
            {
                // Linear decay: zoom factor decreases linearly with distance
                decay_factor = glm::max(min_zoom_factor, current_distance / threshold_distance);
            }

            eye += view * z * decay_factor;
            this->eye = glm::vec4(eye, 1.0f);
        }
    };
}; // namespace letc

#endif // LETC_CAMERA_HH
