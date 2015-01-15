#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include "StreamSurfaceRenderer.h"

#include <gl/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>


// STD
#include <string>
#include <stdlib.h>
#include <stdio.h>

#include "AntTweakBarGUI.h"
#include "Camera.h"

class Application {
public:
    Application();

    void init(const unsigned int& width, const unsigned int& height);
    void init();
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

    static bool m_controlKeyHold;
    static bool m_altKeyHold;
    static bool m_w_pressed, m_s_pressed, m_a_pressed, m_d_pressed, m_q_pressed, m_e_pressed;

private:
    static GLFWwindow* m_window;
    static AntTweakBarGUI m_gui;
    glm::mat4 m_projmat, m_viewmat, m_worldmat;
    glm::vec3 m_light_dir;
    static unsigned int m_width, m_height;

    StreamSurfaceRenderer m_streamtracer_renderer;
    static Camera m_camera;
    unsigned int m_nVertices;
    
    bool m_addition, m_ripping, m_remove;
};

#endif