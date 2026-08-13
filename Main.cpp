#include <iostream>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "Game.h"
#include "World/WorldConstants.h"
#include "World/WorldSave.h"

namespace
{
struct InputState
{
    Camera* camera;
    Game* game;
    bool mouseCaptured = false;
    bool firstMouse = true;
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;
    bool needsRedraw = true;
};

InputState* inputState(GLFWwindow* window)
{
    return static_cast<InputState*>(glfwGetWindowUserPointer(window));
}

void requestMenuRedraw(GLFWwindow* window)
{
    InputState* input = inputState(window);
    if (input && input->game && !input->game->isPlaying())
        input->needsRedraw = true;
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
    input->game->setCursorPos(xpos, ypos);
    if (!input->mouseCaptured)
        requestMenuRedraw(window);

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

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        input->game->OnEscape();
    else
        input->game->OnKey(key, action);
    requestMenuRedraw(window);
}

void charCallback(GLFWwindow* window, unsigned int codepoint)
{
    inputState(window)->game->OnChar(codepoint);
    requestMenuRedraw(window);
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int)
{
    InputState* input = inputState(window);
    if (input->game->isPlaying()) {
        if (!input->mouseCaptured || action != GLFW_PRESS)
            return;
        if (button == GLFW_MOUSE_BUTTON_LEFT)
            input->game->OnLeftClick();
        else if (button == GLFW_MOUSE_BUTTON_RIGHT)
            input->game->OnRightClick();
        return;
    }

    input->game->OnMouseButton(button, action);
    requestMenuRedraw(window);
}

void scrollCallback(GLFWwindow* window, double, double yoffset)
{
    inputState(window)->game->OnScroll(static_cast<float>(yoffset));
    requestMenuRedraw(window);
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    if (height > 0)
        inputState(window)->camera->UpdateAspectRatio(width, height);
    requestMenuRedraw(window);
}

void focusCallback(GLFWwindow* window, int focused)
{
    InputState* input = inputState(window);
    if (!focused && input->game->isPlaying())
        input->game->OnEscape();
    requestMenuRedraw(window);
}

void applyStartupVideo(GLFWwindow* window, const Config& config)
{
    glfwSwapInterval(config.vsync ? 1 : 0);
    if (!config.isFullscreen)
        return;
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if (!monitor)
        return;
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    if (!mode)
        return;
    glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height,
                         mode->refreshRate);
    glfwSwapInterval(config.vsync ? 1 : 0);
}

int displayRefreshRate(GLFWwindow* window)
{
    GLFWmonitor* monitor = glfwGetWindowMonitor(window);
    if (!monitor)
        monitor = glfwGetPrimaryMonitor();
    if (!monitor)
        return 60;
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    if (!mode || mode->refreshRate < 30)
        return 60;
    return mode->refreshRate;
}

void capPlayFrameRate(GLFWwindow* window, bool vsync, double frameStart)
{
    if (!vsync)
        return;
    const double minDt = 1.0 / static_cast<double>(displayRefreshRate(window));
    const double elapsed = glfwGetTime() - frameStart;
    if (elapsed < minDt)
        glfwWaitEventsTimeout(minDt - elapsed);
}

// Menus are static: sleep until input, and never present faster than 30 Hz
// while the mouse is moving. Continuous presents keep the M-series GPU
// clocked up even for a cheap sky pass (macmon ~90% at <1W).
bool waitForMenuFrame(GLFWwindow* window, InputState& input, double& lastPresent)
{
    constexpr double kMinDt = 1.0 / 30.0;
    while (!glfwWindowShouldClose(window) && !input.game->isPlaying()) {
        const double now = glfwGetTime();
        if (!input.needsRedraw) {
            glfwWaitEvents();
            continue;
        }
        const double wait = lastPresent + kMinDt - now;
        if (wait > 0.0) {
            glfwWaitEventsTimeout(wait);
            continue;
        }
        glfwPollEvents();
        return true;
    }
    return !glfwWindowShouldClose(window);
}
}

int main()
{
    Config config;
    WorldSave::loadSettings(config);
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

    glfwSetWindowPos(window, config.windowPosX, config.windowPosY);
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    InputState input{&camera, &game, false, true, 0.0, 0.0, true};
    glfwSetWindowUserPointer(window, &input);
    glfwSetCursorPosCallback(window, cursorPositionCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCharCallback(window, charCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetWindowFocusCallback(window, focusCallback);
    setMouseCaptured(window, false);
    applyStartupVideo(window, config);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
    framebufferSizeCallback(window, framebufferWidth, framebufferHeight);

    game.setWindow(window);
    game.Init();

    double lastTime = glfwGetTime();
    double lastMenuPresent = 0.0;
    while (!glfwWindowShouldClose(window)) {
        if (!game.isPlaying()) {
            if (!waitForMenuFrame(window, input, lastMenuPresent))
                break;
        } else {
            glfwPollEvents();
        }

        const double frameStart = glfwGetTime();
        const float deltaTime = static_cast<float>(frameStart - lastTime);
        lastTime = frameStart;

        const bool wantCapture = game.isPlaying();
        if (wantCapture != input.mouseCaptured)
            setMouseCaptured(window, wantCapture);

        if (game.isPlaying()) {
            game.ProcessInput(deltaTime);
            game.Update(deltaTime);
        }

        if (game.isCameraUnderwater())
            glClearColor(UNDERWATER_FOG_R, UNDERWATER_FOG_G, UNDERWATER_FOG_B, 1.0f);
        else {
            const glm::vec3 cc = game.clearColor();
            glClearColor(cc.r, cc.g, cc.b, 1.0f);
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
        const GameScreen screenBefore = game.screen();
        game.Render(framebufferWidth, framebufferHeight);
        game.endFrame();

        glfwSwapBuffers(window);

        if (game.isPlaying()) {
            capPlayFrameRate(window, game.m_Config.vsync, frameStart);
        } else {
            lastMenuPresent = glfwGetTime();
            input.needsRedraw = game.screen() != screenBefore;
        }
    }

    game.shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
