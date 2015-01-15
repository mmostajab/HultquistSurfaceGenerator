#include "application.h"

// STD
#include <iostream>
#include <fstream>
#include <time.h>

// GL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>

// Static Members
GLFWwindow*     Application::m_window           = 0;
AntTweakBarGUI  Application::m_gui;
unsigned int    Application::m_width            = 0;
unsigned int    Application::m_height           = 0;
bool            Application::m_controlKeyHold   = false;
bool            Application::m_altKeyHold       = false;
Camera          Application::m_camera;
bool            Application::m_w_pressed        = false;
bool            Application::m_s_pressed        = false;
bool            Application::m_a_pressed        = false;
bool            Application::m_d_pressed        = false;
bool            Application::m_q_pressed        = false;
bool            Application::m_e_pressed        = false;

Application::Application() {
    m_addition = true;
    m_remove = true;
    m_ripping = true;
}

void Application::init(const unsigned int& width, const unsigned int& height) {

    m_width = width; m_height = height;

    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
        exit(EXIT_FAILURE);

    m_window = glfwCreateWindow(width, height, "Stream Surface Generator (Demo)", NULL, NULL);
    if (!m_window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(m_window);

    glfwSetKeyCallback(m_window, key_callback);
    glfwSetWindowSizeCallback(m_window, WindowSizeCB);
    glfwSetCursorPosCallback(m_window, EventMousePos);
    glfwSetScrollCallback(m_window, EventMouseWheel);
    glfwSetMouseButtonCallback(m_window, (GLFWmousebuttonfun)EventMouseButton);
    glfwSetKeyCallback(m_window, (GLFWkeyfun)EventKey);

    // - Directly redirect GLFW char events to AntTweakBar
    glfwSetCharCallback(m_window, (GLFWcharfun)EventChar);

    if (glewInit() != GLEW_OK){
        std::cout << "cannot intialize the glew.\n";
        exit(EXIT_FAILURE);
    }

    m_gui.init(m_width, m_height);

    init();

    GLenum e = glGetError();
    //glEnable(GL_DEPTH);
    e = glGetError();
    glEnable(GL_DEPTH_TEST);
    e = glGetError();
}

void Application::init() {
    GLenum e = glGetError();
    glClearColor(19 / 255.0, 9 / 255.0, 99 / 255.0, 1.0f);
    e = glGetError();
    m_camera.init(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::ivec2(m_width, m_height), 3.14 / 4, 0.01, 1000.0);

}

void Application::create() {
    compileShaders();

    m_streamtracer_renderer.loadOpenFOAM("../../data/Fraunhofer/othmer.foam");

    m_streamtracer_renderer.getParameters(m_gui.seedingline_maxSeeds, m_gui.seedingline_maxSteps, m_gui.seedingline_stepSize, m_gui.seedingline_center, m_gui.seedingline_dir);
    
    m_addition = m_gui.tracing_addition;
    m_remove   = m_gui.tracing_remove;
    m_ripping  = m_gui.tracing_ripping;
}

void Application::update(float time, float timeSinceLastFrame) {

    m_camera.moveCamera(timeSinceLastFrame, m_w_pressed, m_s_pressed);

    m_viewmat = m_camera.getViewMat();
    m_projmat = m_camera.getProjMat();
    m_worldmat = glm::mat4(1.0f);

    glUniformMatrix4fv(0, 1, GL_FALSE, (float*)&m_projmat);
    glUniformMatrix4fv(1, 1, GL_FALSE, (float*)&m_viewmat);
    glUniformMatrix4fv(2, 1, GL_FALSE, (float*)&m_worldmat);
    glUniform3fv      (3, 1, (float*)m_gui.general_light_dir);

    if (m_gui.general_streamlines)
        m_streamtracer_renderer.setMode(StreamSurfaceRenderer::Mode::STREAM_LINES);
    else
        m_streamtracer_renderer.setMode(StreamSurfaceRenderer::Mode::STREAM_SURFACE);

    if (m_addition != m_gui.tracing_addition || m_remove != m_gui.tracing_remove || m_ripping != m_gui.tracing_ripping){
        m_streamtracer_renderer.setAsDirty();

        m_addition = m_gui.tracing_addition;
        m_remove = m_gui.tracing_remove;
        m_ripping = m_gui.tracing_ripping;
    }

    m_streamtracer_renderer.setParameters(m_gui.seedingline_maxSeeds, m_gui.seedingline_maxSteps, m_gui.seedingline_stepSize, m_gui.seedingline_center, m_gui.seedingline_dir);
    m_streamtracer_renderer.update(time, timeSinceLastFrame, m_gui.tracing_addition, m_gui.tracing_remove, m_gui.tracing_ripping);
}

void Application::draw() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_gui.general_wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glUniformMatrix4fv(0, 1, GL_FALSE, (float*)&m_projmat);
    glUniformMatrix4fv(1, 1, GL_FALSE, (float*)&m_viewmat);
    glUniformMatrix4fv(2, 1, GL_FALSE, (float*)&m_worldmat);
    glUniform3fv(3, 1, (float*)m_gui.general_light_dir);
    m_streamtracer_renderer.draw();

    glUniformMatrix4fv(0, 1, GL_FALSE, (float*)&m_projmat);
    glUniformMatrix4fv(1, 1, GL_FALSE, (float*)&m_viewmat);
    glUniformMatrix4fv(2, 1, GL_FALSE, (float*)&m_worldmat);
    glUniform3fv(3, 1, (float*)m_gui.general_light_dir);
    m_streamtracer_renderer.draw_2d();
}

void Application::run() {
    create();
    clock_t start_time = clock();
    clock_t start_frame = clock();

    while (!glfwWindowShouldClose(m_window))
    {
        draw();

        m_gui.draw();

        glfwSwapBuffers(m_window);
        glfwPollEvents();

        clock_t current_time = clock();
        float elapsed_since_start       = float(current_time - start_time)  / CLOCKS_PER_SEC;
        float elapsed_since_last_frame  = float(current_time - start_frame) / CLOCKS_PER_SEC;
        start_frame = clock();

        update(elapsed_since_start, elapsed_since_last_frame);
    }
}

void Application::shutdown() {
    m_streamtracer_renderer.shutdown();
    m_gui.shutdown();
    glfwDestroyWindow(m_window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}

Application::~Application() {
}

void Application::compileShaders() { 
    m_streamtracer_renderer.compileShaders();
}

void Application::EventMouseButton(GLFWwindow* window, int button, int action, int mods) {
    m_gui.TwEventMouseButtonGLFW3(button, action, mods);
}

void Application::EventMousePos(GLFWwindow* window, double xpos, double ypos) {
    m_gui.TwEventMousePosGLFW3(int(xpos), int(ypos));

    if (m_controlKeyHold){
        double x_mouse_mov = xpos - m_width / 2.0, y_mouse_mov = ypos - m_height / 2.0;
        m_camera.rotateCamera(1.0f, x_mouse_mov / m_width, y_mouse_mov / m_height);

        glfwSetCursorPos(m_window, m_width / 2.0, m_height / 2.0);
    }
}

void Application::EventMouseWheel(GLFWwindow* window, double xoffset, double yoffset) {
    m_gui.TwEventMouseWheelGLFW3(xoffset, yoffset);
}

void Application::EventKey(GLFWwindow* window, int key, int scancode, int action, int mods) {
    m_gui.TwEventKeyGLFW3(key, scancode, action, mods);

    if (action == GLFW_PRESS){
        if (m_controlKeyHold && key == GLFW_KEY_W)  m_w_pressed = true;
        if (m_controlKeyHold && key == GLFW_KEY_S)  m_s_pressed = true;
        if (m_controlKeyHold && key == GLFW_KEY_A)  m_a_pressed = true;
        if (m_controlKeyHold && key == GLFW_KEY_D)  m_d_pressed = true;
        if (m_controlKeyHold && key == GLFW_KEY_Q)  m_q_pressed = true;
        if (m_controlKeyHold && key == GLFW_KEY_E)  m_e_pressed = true;

        if (key == GLFW_KEY_LEFT_CONTROL)           m_controlKeyHold = true;
        if (key == GLFW_KEY_LEFT_ALT)               m_altKeyHold     = true;
    }

    if (action == GLFW_RELEASE){
        if (key == GLFW_KEY_W)  m_w_pressed = false;
        if (key == GLFW_KEY_S)  m_s_pressed = false;
        if (key == GLFW_KEY_A)  m_a_pressed = false;
        if (key == GLFW_KEY_D)  m_d_pressed = false;
        if (key == GLFW_KEY_Q)  m_q_pressed = false;
        if (key == GLFW_KEY_E)  m_e_pressed = false;

        if (key == GLFW_KEY_LEFT_CONTROL)           m_controlKeyHold    = false;
        if (key == GLFW_KEY_LEFT_ALT)               m_altKeyHold        = false;
    }
}

void Application::EventChar(GLFWwindow* window, int codepoint) {
    m_gui.TwEventCharGLFW3(codepoint);
}

// Callback function called by GLFW when window size changes
void Application::WindowSizeCB(GLFWwindow* window, int width, int height) {
    m_width = width; m_height = height;
    glViewport(0, 0, width, height);

    // Send the new window size to AntTweakBar
    m_gui.WindowSizeCB(width, height);
}
void Application::error_callback(int error, const char* description) {
    fputs(description, stderr);
}
void Application::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_LEFT_CONTROL && action == GLFW_PRESS){
        glfwSetCursorPos(m_window, m_width / 2.0f, m_height / 2.0f);
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        m_controlKeyHold = true;
    }

    if (key == GLFW_KEY_LEFT_CONTROL && action == GLFW_RELEASE){
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        m_controlKeyHold = false;
    }
}