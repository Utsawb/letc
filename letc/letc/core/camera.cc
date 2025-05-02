#include "letc/core/camera.hh"
#include <algorithm> // For std::clamp

namespace letc
{
    FirstPersonCamera::FirstPersonCamera(std::weak_ptr<Allocator> allocator, const float &aspect, const float &fovy,
                                         const float &near, const float &far, const glm::vec3 &position,
                                         const float yaw, const float pitch, const float positionSpeed,
                                         const float rotationSpeed)
        : Camera<FirstPersonCamera>(allocator), m_position(position), m_yaw(yaw), m_pitch(pitch),
          m_positionSpeed(positionSpeed), m_rotationSpeed(rotationSpeed)
    {
        m_buffer->proj = glm::perspectiveZO(fovy, aspect, near, far);
        m_buffer->proj[1][1] *= -1.0f;
        updateVectors();
        sync();
    }

    auto FirstPersonCamera::updateVectors() -> void
    {
        glm::vec3 front;
        front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
        front.y = sin(glm::radians(m_pitch));
        front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
        m_front = glm::normalize(front);
        m_right = glm::normalize(glm::cross(m_front, m_worldUp));

        m_up = glm::normalize(glm::cross(m_right, m_front));
    }

    auto FirstPersonCamera::sync() -> void
    {
        m_buffer->view = glm::lookAt(m_position, m_position + m_front, m_up);
        m_buffer.sync();
    }

    auto FirstPersonCamera::move(const glm::vec3 &movementInput, float deltaTime) -> void
    {
        float velocity = m_positionSpeed * deltaTime;
        m_position += m_front * movementInput.x * velocity;
        m_position += m_right * movementInput.y * velocity;
        m_position += m_worldUp * movementInput.z * velocity;
    }

    auto FirstPersonCamera::pan(const float dx, const float dy) -> void
    {
        float xoffset = dx * m_rotationSpeed;
        float yoffset = dy * m_rotationSpeed;

        m_yaw += xoffset;
        m_pitch += yoffset;

        m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);

        updateVectors();
    }

    auto FirstPersonCamera::getPosition() const -> const glm::vec3 &
    {
        return m_position;
    }

    auto FirstPersonCamera::getFront() const -> const glm::vec3 &
    {
        return m_front;
    }

    auto FirstPersonCamera::getUp() const -> const glm::vec3 &
    {
        return m_up;
    }

    auto FirstPersonCamera::getRight() const -> const glm::vec3 &
    {
        return m_right;
    }

} // namespace letc
