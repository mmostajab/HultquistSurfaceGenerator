#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include <gl/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>


// STD
#include <string>
#include <stdlib.h>
#include <stdio.h>

#include "AntTweakBarGUI.h"

class Application {
public:
    Application();

    void init(const unsigned int& width, const unsigned int& height);
    void run();
    void shutdown();

    ~Application();

private:
    void create();
    void update(float time, float elapsedTime);
    void draw();

    static std::string convertFileToString(const std::string& filename);
    void compileShaders();

    GLuint simple_program;


private:
    static void EventMouseButton(GLFWwindow* window, int button, int action, int mods);
    static void EventMousePos(GLFWwindow* window, double xpos, double ypos);
    static void EventMouseWheel(GLFWwindow* window, double xoffset, double yoffset);
    static void EventKey(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void EventChar(GLFWwindow* window, int codepoint);

    // Callback function called by GLFW when window size changes
    static void WindowSizeCB(GLFWwindow* window, int width, int height);
    static void error_callback(int error, const char* description);
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

private:
    GLFWwindow* m_window;
    static AntTweakBarGUI m_gui;
    glm::mat4 m_projmat, m_viewmat, m_worldmat;
    static unsigned int m_width, m_height;
};

#endif