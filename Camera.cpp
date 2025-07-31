//
// Created by 郭珂桢 on 2024/5/20.
//

#include "Camera.h"
#include <iostream>

Camera::Camera(const Config &config, glm::vec3 position, glm::vec3 up, float yaw, float pitch)
: m_config(config), Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
{
    m_projectionMatrix = makeProjectionMatrix(config);
    Position = position;
    WorldUp = up;
    Yaw = yaw;
    Pitch = pitch;
    updateCameraVectors();
}

glm::mat4& Camera::GetViewMatrix()
{
    return m_viewMatrix;
}

glm::mat4 &Camera::GetProjectionMatrix()
{
    return m_projectionMatrix;
}

glm::mat4 &Camera::GetProjectionViewMatrix()
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

void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch)
{
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw   += xoffset;
    Pitch += yoffset;

    // make sure that when pitch is out of bounds, screen doesn't get flipped
    if (constrainPitch)
    {
        if (Pitch > 89.0f)
            Pitch = 89.0f;
        if (Pitch < -89.0f)
            Pitch = -89.0f;
    }

    // update Front, Right and Up Vectors using the updated Euler angles
    updateCameraVectors();
    updateMatrices();
}

void Camera::ProcessMouseScroll(float yoffset)
{
    Zoom -= (float)yoffset;
    if (Zoom < 0.5f)
        Zoom = 0.5f;
    if (Zoom > 45.0f)
        Zoom = 45.0f;
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
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);
    // also re-calculate the Right and Up vector
    Right = glm::normalize(glm::cross(Front, WorldUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    Up    = glm::normalize(glm::cross(Right, Front));
}

glm::mat4 Camera::makeProjectionMatrix(const Config &config)
{
    float x = static_cast<float>(config.windowX);
    float y = static_cast<float>(config.windowY);
    float fov = (float)config.fov;
    return glm::perspective(glm::radians(fov), x / y, 0.1f, 1000.0f);
}

