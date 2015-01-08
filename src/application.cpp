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
AntTweakBarGUI  Application::m_gui;
unsigned int    Application::m_width = 0;
unsigned int    Application::m_height = 0;

Application::Application() {
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
}

void Application::create() {
    compileShaders();

    glm::vec3 vertices[3] = { glm::vec3(-0.6f, -0.4f, 0.0f), glm::vec3(0.6f, -0.4f, 0.0f), glm::vec3(0.0f, 0.6f, 0.0f) };
    glm::vec3 colors[3] = { glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) };

    GLuint buffers[2];
    glGenBuffers(2, buffers);

    glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    glBindBuffer(GL_ARRAY_BUFFER, buffers[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    float ratio = m_width / (float)m_height;
    m_projmat = glm::perspective(45.0f, ratio, 0.0f, 1000.0f);
    m_viewmat = glm::lookAt(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    m_worldmat = glm::mat4(1.0f);

    glUseProgram(simple_program);
    GLuint world_mat_loc = glGetUniformLocation(simple_program, "world_mat");
    GLuint view_mat_loc = glGetUniformLocation(simple_program, "view_mat");
    GLuint proj_mat_loc = glGetUniformLocation(simple_program, "proj_mat");

    glUniformMatrix4fv(world_mat_loc, 1, GL_FALSE, (float*)&m_worldmat);
    glUniformMatrix4fv(view_mat_loc, 1, GL_FALSE, (float*)&m_viewmat);
    glUniformMatrix4fv(proj_mat_loc, 1, GL_FALSE, (float*)&m_projmat);
}

void Application::update(float time, float elapsedTime) {
    m_viewmat = glm::lookAt(glm::vec3(sin(time), cos(time), 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    GLuint view_mat_loc = glGetUniformLocation(simple_program, "view_mat");
    glUniformMatrix4fv(view_mat_loc, 1, GL_FALSE, (float*)&m_viewmat);
}

void Application::draw() {
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(simple_program);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
}

void Application::run() {
    create();
    clock_t start_time = clock();

    while (!glfwWindowShouldClose(m_window))
    {
        clock_t start_frame = clock();
        draw();

        m_gui.draw();

        glfwSwapBuffers(m_window);
        glfwPollEvents();

        clock_t current_time = clock();
        float elapsed_since_start       = float(current_time - start_time)  / CLOCKS_PER_SEC;
        float elapsed_since_last_frame  = float(current_time - start_frame) / CLOCKS_PER_SEC;
        
        update(elapsed_since_start, elapsed_since_last_frame);
    }
}

void Application::shutdown() {
    m_gui.shutdown();
    glfwDestroyWindow(m_window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}

Application::~Application() {
}

std::string Application::convertFileToString(const std::string& filename) {    
    std::ifstream ifile(filename);
    if (!ifile){
        return std::string("");
    }

    return std::string(std::istreambuf_iterator<char>(ifile), (std::istreambuf_iterator<char>()));

}

void Application::compileShaders() { 
    GLuint simple_vertex_shader, simple_fragment_shader;
    std::string simple_shader_vertex_source = convertFileToString("../../src/glsl/simple.vert");
    std::string simple_shader_fragment_source = convertFileToString("../../src/glsl/simple.frag");

    simple_vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    simple_fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    const GLchar* simple_shader_vertex_sourcePtr    = simple_shader_vertex_source.c_str();
    const GLchar* simple_shader_fragment_sourcePtr  = simple_shader_fragment_source.c_str();

    glShaderSource(simple_vertex_shader, 1, &simple_shader_vertex_sourcePtr, NULL);
    glShaderSource(simple_fragment_shader, 1, &simple_shader_fragment_sourcePtr, NULL);
    glCompileShader(simple_vertex_shader);
    glCompileShader(simple_fragment_shader);
    
    simple_program = glCreateProgram();
    glAttachShader(simple_program, simple_vertex_shader);
    glAttachShader(simple_program, simple_fragment_shader);
    glLinkProgram(simple_program);
    
    glDeleteShader(simple_vertex_shader);
    glDeleteShader(simple_fragment_shader);
}

void Application::EventMouseButton(GLFWwindow* window, int button, int action, int mods) {
    m_gui.TwEventMouseButtonGLFW3(button, action, mods);
}

void Application::EventMousePos(GLFWwindow* window, double xpos, double ypos) {
    m_gui.TwEventMousePosGLFW3(int(xpos), int(ypos));
}

void Application::EventMouseWheel(GLFWwindow* window, double xoffset, double yoffset) {
    m_gui.TwEventMouseWheelGLFW3(xoffset, yoffset);
}

void Application::EventKey(GLFWwindow* window, int key, int scancode, int action, int mods) {
    m_gui.TwEventKeyGLFW3(key, scancode, action, mods);
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
}
