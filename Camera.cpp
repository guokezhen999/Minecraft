//
// Created by 郭珂桢 on 2024/5/20.
//

#include "Camera.h"
#include <cmath>

Camera::Camera(const Config &config, glm::vec3 position, glm::vec3 up, float yaw, float pitch)
: m_config(config), Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY),
  Zoom(static_cast<float>(config.fov))
{
    m_projectionMatrix = makeProjectionMatrix(config);
    Position = position;
    WorldUp = up;
    Yaw = yaw;
    Pitch = pitch;
    updateCameraVectors();
    updateMatrices();
}

const glm::mat4& Camera::GetViewMatrix() const
{
    return m_viewMatrix;
}

const glm::mat4 &Camera::GetProjectionMatrix() const
{
    return m_projectionMatrix;
}

const glm::mat4 &Camera::GetProjectionViewMatrix() const
{
    return m_projectionViewMatrix;
}

void Camera::ProcessKeyboard(Camera_Movement direction, float deltaTime)
{
    float velocity = MovementSpeed * deltaTime;
    if (direction == FORWARD)
        Position += Front * velocity;
    if (direction == BACKWARD)
        Position -= Front * velocity;
    if (direction == LEFT)
        Position -= Right * velocity;
    if (direction == RIGHT)
        Position += Right * velocity;
    if (direction == UP)
        Position += Up * velocity;
    if (direction == DOWN)
        Position -= Up * velocity;
    updateMatrices();
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset)
{
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw += xoffset;
    Pitch = glm::clamp(Pitch + yoffset, -PITCH_LIMIT, PITCH_LIMIT);

    updateCameraVectors();
    updateMatrices();
}

void Camera::ProcessMouseScroll(float yoffset)
{
    Zoom -= (float)yoffset * 2.0f;
    if (Zoom < 30.0f)
        Zoom = 30.0f;
    if (Zoom > 110.0f)
        Zoom = 110.0f;
    updateMatrices();
}

void Camera::UpdateAspectRatio(int width, int height)
{
    m_config.windowX = width;
    m_config.windowY = height;
    updateMatrices();
}

void Camera::SetPosition(const glm::vec3& position)
{
    Position = position;
    updateMatrices();
}

void Camera::updateMatrices()
{
    m_projectionMatrix = glm::perspective(glm::radians(Zoom),
                                          (float)m_config.windowX / (float)m_config.windowY,
                                          0.1f, 1000.0f);
    m_viewMatrix = glm::lookAt(Position, Position + Front, Up);
    m_projectionViewMatrix = m_projectionMatrix * m_viewMatrix;
}

void Camera::updateCameraVectors()
{
    const float yawR = glm::radians(Yaw);
    const float pitchR = glm::radians(Pitch);

    glm::vec3 front;
    front.x = std::cos(yawR) * std::cos(pitchR);
    front.y = std::sin(pitchR);
    front.z = std::sin(yawR) * std::cos(pitchR);
    Front = glm::normalize(front);
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
}

glm::mat4 Camera::makeProjectionMatrix(const Config &config)
{
    float x = static_cast<float>(config.windowX);
    float y = static_cast<float>(config.windowY);
    float fov = (float)config.fov;
    return glm::perspective(glm::radians(fov), x / y, 0.1f, 1000.0f);
}

