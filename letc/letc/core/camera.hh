#pragma once

#include "letc/pch.hh"

#include "letc/core/buffer.hh"

namespace letc
{
    struct UCamera
    {
        glm::mat4 view;
        glm::mat4 proj;
    };

    template <typename Derived> class Camera
    {
      public:
        auto bSync() -> void
        {
            static_cast<Derived *>(this)->sync();
        }

        auto getBuffer() -> vk::Buffer
        {
            return m_buffer.get();
        }

        auto containedSize() -> std::size_t
        {
            return m_buffer.containedSize();
        }

      protected:
        ObjectBuffer<UCamera> m_buffer;
        Camera(std::weak_ptr<Allocator> allocator)
            : m_buffer(allocator, vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU,
                       vk::SharingMode::eExclusive)
        {
        }
    };

    class FirstPersonCamera : public Camera<FirstPersonCamera>
    {
      public:
        FirstPersonCamera(std::weak_ptr<Allocator> allocator, const float &aspect, const float &fovy = 60.0f,
                          const float &near = 0.1f, const float &far = 1000.0f,
                          const glm::vec3 &position = glm::vec3{0.0f},
                          const glm::quat &rotation = glm::identity<glm::quat>(), const float positionSpeed = 1.0f,
                          const float rotationSpeed = 1.0f);
        auto sync() -> void;

        auto move(const glm::vec3 &translation) -> void;
        auto pan(const glm::vec2 &translation) -> void;

      private:
        float m_positionSpeed;
        float m_rotationSpeed;
        glm::vec3 m_position;
        glm::quat m_rotation;
    };
} // namespace letc
