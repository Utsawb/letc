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
        m_buffer->proj[1][1] *= -1.0f; // Adjust for Vulkan coordinate system
        updateVectors();               // Initial calculation of orientation vectors
        sync();                        // Initial view matrix calculation
    }

    // Recalculates front, right, and up vectors based on yaw and pitch
    auto FirstPersonCamera::updateVectors() -> void
    {
        // Calculate the new Front vector
        glm::vec3 front;
        front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
        front.y = sin(glm::radians(m_pitch));
        front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
        m_front = glm::normalize(front);
        // Also re-calculate the Right and Up vector
        m_right = glm::normalize(
            glm::cross(m_front, m_worldUp)); // Normalize the vectors, because their length gets closer to 0 the more
                                             // you look up or down which results in slower movement.
        m_up = glm::normalize(glm::cross(m_right, m_front));
    }

    auto FirstPersonCamera::sync() -> void
    {
        // View matrix uses position and the calculated orientation vectors
        m_buffer->view = glm::lookAt(m_position, m_position + m_front, m_up);
        m_buffer.sync(); // Upload data to the GPU buffer
    }

    // Processes input received from any keyboard-like input system.
    // Accepts input parameter in the form of camera defined movement direction (scaled by time).
    auto FirstPersonCamera::move(const glm::vec3 &movementInput, float deltaTime) -> void
    {
        float velocity = m_positionSpeed * deltaTime;
        // movementInput.x controls forward(+) / backward(-)
        // movementInput.y controls right(+) / left(-)
        // movementInput.z controls up(+) / down(-) - Use m_up for FPS style, m_worldUp for "fly" style
        m_position += m_front * movementInput.x * velocity;
        m_position += m_right * movementInput.y * velocity;
        // Optional: Add vertical movement if needed
        // m_position += m_up * movementInput.z * velocity; // Use m_up for true FPS up/down relative to view
        m_position += m_worldUp * movementInput.z * velocity; // Use m_worldUp for flying up/down regardless of pitch

        // Note: No need to call sync() here typically.
        // sync() is usually called once per frame after all updates.
    }

    // Processes input received from a mouse input system.
    // Expects the offset value in both the x and y direction.
    auto FirstPersonCamera::pan(const float dx, const float dy) -> void
    {
        float xoffset = dx * m_rotationSpeed;
        float yoffset = dy * m_rotationSpeed; // Reversed since y-coordinates go from bottom to top

        m_yaw += xoffset;
        m_pitch += yoffset;

        // Make sure that when pitch is out of bounds, screen doesn't get flipped
        m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);

        // Update Front, Right and Up Vectors using the updated Euler angles
        updateVectors();

        // Note: No need to call sync() here typically.
        // sync() is usually called once per frame after all updates.
    }

    // --- Getters ---
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
