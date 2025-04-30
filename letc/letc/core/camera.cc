#include "letc/core/camera.hh"

namespace letc
{
    FirstPersonCamera::FirstPersonCamera(std::weak_ptr<Allocator> allocator, const float &aspect, const float &fovy,
                                         const float &near, const float &far, const glm::vec3 &position,
                                         const glm::quat &rotation, const float positionSpeed,
                                         const float rotationSpeed)
        : Camera<FirstPersonCamera>(allocator), m_position(position), m_rotation(rotation),
          m_positionSpeed(positionSpeed), m_rotationSpeed(rotationSpeed)
    {
        m_buffer->proj = glm::perspective(fovy, aspect, near, far);
        sync();
    }

    auto FirstPersonCamera::sync() -> void
    {
        glm::vec3 front = m_rotation * glm::vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 up = m_rotation * glm::vec3(0.0f, 1.0f, 0.0f);

        m_buffer->view = glm::lookAt(m_position, m_position + front, up);

        m_buffer.sync();
    }

    auto FirstPersonCamera::move(const glm::vec3 &translation) -> void
    {
        m_position += translation * m_rotation;
    }

    auto FirstPersonCamera::pan(const glm::vec2 &translation) -> void
    {
        m_rotation = glm::angleAxis(translation.x * m_rotationSpeed, glm::vec3(0.0, 1.0, 0.0)) * m_rotation;
        m_rotation = glm::angleAxis(translation.y * m_rotationSpeed, glm::vec3(1.0, 0.0, 0.0)) * m_rotation;
    }

} // namespace letc
