#include <iostream>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "Game.h"
#include "World/WorldConstants.h"

namespace
{
struct InputState
{
    Camera* camera;
    Game* game;
    bool mouseCaptured = true;
    bool firstMouse = true;
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;
};

InputState* inputState(GLFWwindow* window)
{
    return static_cast<InputState*>(glfwGetWindowUserPointer(window));
}

void setMouseCaptured(GLFWwindow* window, bool captured)
{
    InputState* input = inputState(window);
    input->mouseCaptured = captured;
    input->firstMouse = true;
    glfwSetInputMode(window, GLFW_CURSOR,
                     captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    InputState* input = inputState(window);
    if (!input->mouseCaptured)
        return;

    if (input->firstMouse) {
        input->lastMouseX = xpos;
        input->lastMouseY = ypos;
        input->firstMouse = false;
        return;
    }

    const float xoffset = static_cast<float>(xpos - input->lastMouseX);
    const float yoffset = static_cast<float>(input->lastMouseY - ypos);
    input->lastMouseX = xpos;
    input->lastMouseY = ypos;
    input->camera->ProcessMouseMovement(xoffset, yoffset);
}

void keyCallback(GLFWwindow* window, int key, int, int action, int)
{
    InputState* input = inputState(window);
    constexpr int KEY_COUNT = 1024;
    if (key >= 0 && key < KEY_COUNT)
        input->game->Keys[key] = action != GLFW_RELEASE;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        if (input->mouseCaptured)
            setMouseCaptured(window, false);
        else
            glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int)
{
    if (action != GLFW_PRESS)
        return;

    InputState* input = inputState(window);
    if (!input->mouseCaptured) {
        if (glfwGetWindowAttrib(window, GLFW_FOCUSED))
            setMouseCaptured(window, true);
        return;
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT)
        input->game->OnLeftClick();
    else if (button == GLFW_MOUSE_BUTTON_RIGHT)
        input->game->OnRightClick();
}

void scrollCallback(GLFWwindow* window, double, double yoffset)
{
    InputState* input = inputState(window);
    if (input->mouseCaptured)
        input->game->OnScroll(static_cast<float>(yoffset));
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    if (height > 0)
        inputState(window)->camera->UpdateAspectRatio(width, height);
}

void focusCallback(GLFWwindow* window, int focused)
{
    if (!focused && inputState(window)->mouseCaptured)
        setMouseCaptured(window, false);
}
}

int main()
{
    Config config;
    Camera camera(config);
    Game game(config, camera);

    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);

    GLFWwindow* window = glfwCreateWindow(config.windowX, config.windowY,
                                          "Minecraft", nullptr, nullptr);
    if (!window) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    InputState input{&camera, &game};
    glfwSetWindowUserPointer(window, &input);
    glfwSetCursorPosCallback(window, cursorPositionCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetWindowFocusCallback(window, focusCallback);
    setMouseCaptured(window, true);

    glfwSwapInterval(1);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
    framebufferSizeCallback(window, framebufferWidth, framebufferHeight);

    game.Init();

    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        const double currentTime = glfwGetTime();
        const float deltaTime = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;

        glfwPollEvents();
        game.ProcessInput(deltaTime);
        game.Update(deltaTime);

        if (game.isCameraUnderwater())
            glClearColor(UNDERWATER_FOG_R, UNDERWATER_FOG_G, UNDERWATER_FOG_B, 1.0f);
        else
            glClearColor(FOG_R, FOG_G, FOG_B, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
        game.Render(framebufferWidth, framebufferHeight);

        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
