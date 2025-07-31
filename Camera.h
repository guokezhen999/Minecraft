//
// Created by 郭珂桢 on 2024/5/20.
//

#ifndef MINECRAFT_CAMERA_H
#define MINECRAFT_CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Config.h"

enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

// Default camera values
const float YAW         = -90.0f;
const float PITCH       =  0.0f;
const float SPEED       =  2.5f;
const float SENSITIVITY =  0.1f;
const float ZOOM        =  45.0f;

class Camera
{
public:
    glm::vec3 Position;
    glm::vec3 Front, Up, Right;
    glm::vec3 WorldUp;

    float Yaw;
    float Pitch;

    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    Camera() = default;
    Camera(const Config &config, glm::vec3 position = glm::vec3(0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = YAW, float  pitch = PITCH);

    glm::mat4& GetViewMatrix();
    glm::mat4& GetProjectionMatrix();
    glm::mat4& GetProjectionViewMatrix();

    void ProcessKeyboard(Camera_Movement direction, float deltaTime);
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
    void ProcessMouseScroll(float yoffset);

private:
    void updateCameraVectors();
    void updateMatrices();
    static glm::mat4 makeProjectionMatrix(const Config &config);

    glm::mat4 m_projectionMatrix;
    glm::mat4 m_viewMatrix;
    glm::mat4 m_projectionViewMatrix;

    Config m_config;

};


#endif //MINECRAFT_CAMERA_H
