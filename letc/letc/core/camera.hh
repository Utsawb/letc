#pragma once

#include "letc/pch.hh"

#include "letc/core/buffer.hh"

namespace letc
{
    struct UCamera
    {
        glm::mat4 proj;
        glm::mat4 view;
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
        FirstPersonCamera(std::weak_ptr<Allocator> allocator, const float &aspect,
                          const float &fovy = glm::radians(60.0f), const float &near = 0.1f, const float &far = 1000.0f,
                          const glm::vec3 &position = glm::vec3{0.0f}, const float yaw = -90.0f,
                          const float pitch = 0.0f, const float positionSpeed = 2.5f, const float rotationSpeed = 0.1f);
        auto sync() -> void;

        auto pan(const float dx, const float dy) -> void;
        auto move(const glm::vec3 &movementInput, float deltaTime = 0.0142857143f) -> void;

        auto getPosition() const -> const glm::vec3 &;
        auto getFront() const -> const glm::vec3 &;
        auto getUp() const -> const glm::vec3 &;
        auto getRight() const -> const glm::vec3 &;

      private:
        void updateVectors();

        float m_positionSpeed;
        float m_rotationSpeed;
        glm::vec3 m_position;

        glm::vec3 m_front;
        glm::vec3 m_up;
        glm::vec3 m_right;
        glm::vec3 m_worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

        float m_yaw;
        float m_pitch;
    };
} // namespace letc
