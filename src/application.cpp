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

    glEnable(GL_DEPTH);
    glEnable(GL_DEPTH_TEST);
}

void Application::init() {
    glClearColor(19 / 255.0, 9 / 255.0, 99 / 255.0, 1.0f);
    m_camera.init(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::ivec2(m_width, m_height), 3.14 / 4, 0.01, 1000.0);

    std::cout << "Computing the stream lines...";
    m_streamtracer.loadOpenFOAM("../../data/Fraunhofer/othmer.foam");
    m_streamtracer.computeAccel();
    //m_streamtracer.computeStreamlines();
    m_streamtracer.computeStreamsurfaces();
    std::cout << "Done.\n";
}

void Application::create() {
    compileShaders();

    /*std::vector<glm::vec3> vertices = m_streamtracer.getStreamLines();
    std::vector<glm::vec3> colors   = m_streamtracer.getStreamColors();
    m_nVertices = vertices.size();*/

    std::vector<std::vector<glm::vec3>> streamsurface_vertices = m_streamtracer.getStreamSurfaceLines_Forward();
    std::vector<std::vector<glm::vec3>> streamsurface_colors   = m_streamtracer.getStreamSurfaceColors_Forward();

    std::vector<glm::vec3> vertices, colors;
    std::vector<glm::uint32> indices;

    glm::vec3 center(0.0f, 0.0f, 0.0f);
    unsigned int count = 0;
    for (size_t i = 0; i < streamsurface_vertices.size(); i++){
        for (size_t j = 0; j < streamsurface_vertices[i].size(); j++){
            vertices.push_back(streamsurface_vertices[i][j]);
            colors.push_back(streamsurface_colors[i][j]);

            if (i != streamsurface_vertices.size() - 1 && j < streamsurface_vertices[i].size() - 1 && j < streamsurface_vertices[i + 1].size() - 1){
                indices.push_back(count);
                indices.push_back(count + streamsurface_vertices[i].size());
                indices.push_back(count + 1);

                indices.push_back(count + 1);
                indices.push_back(count + streamsurface_vertices[i].size());
                indices.push_back(count + streamsurface_vertices[i].size() + 1);
            }

            center += streamsurface_vertices[i][j];
            count++;
        }
    }
    m_nVertices = indices.size();
    center /= count;
    for (size_t i = 0; i < vertices.size(); i++){
            vertices[i] -= center;
    }

    GLuint buffers[3]; 
    glGenBuffers(3, buffers);

    glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    glBindBuffer(GL_ARRAY_BUFFER, buffers[1]);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec3), &colors[0], GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(glm::uint32), &indices[0], GL_STATIC_DRAW);

    m_projmat = m_camera.getProjMat();
    m_viewmat = m_camera.getViewMat();
    m_worldmat = glm::mat4(1.0f);

    glUseProgram(simple_program);
    GLuint world_mat_loc = glGetUniformLocation(simple_program,  "world_mat");
    GLuint view_mat_loc  = glGetUniformLocation( simple_program, "view_mat");
    GLuint proj_mat_loc  = glGetUniformLocation( simple_program, "proj_mat");

    glUniformMatrix4fv(world_mat_loc, 1, GL_FALSE, (float*)&m_worldmat);
    glUniformMatrix4fv(view_mat_loc,  1, GL_FALSE, (float*)&m_viewmat);
    glUniformMatrix4fv(proj_mat_loc,  1, GL_FALSE, (float*)&m_projmat);
}

void Application::update(float time, float timeSinceLastFrame) {

    m_camera.moveCamera(timeSinceLastFrame, m_w_pressed, m_s_pressed);

    m_viewmat = m_camera.getViewMat();
    m_projmat = m_camera.getProjMat();

    GLuint world_mat_loc = glGetUniformLocation(simple_program, "world_mat");
    GLuint view_mat_loc  = glGetUniformLocation(simple_program, "view_mat");
    GLuint proj_mat_loc  = glGetUniformLocation(simple_program, "proj_mat");
    
    glUniformMatrix4fv(world_mat_loc, 1, GL_FALSE, (float*)&m_worldmat);
    glUniformMatrix4fv(view_mat_loc,  1, GL_FALSE, (float*)&m_viewmat);
    glUniformMatrix4fv(proj_mat_loc,  1, GL_FALSE, (float*)&m_projmat);
}

void Application::draw() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(simple_program);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    //glDrawArrays(GL_LINES, 0, m_nVertices);
    glDrawElements(GL_TRIANGLES, m_nVertices, GL_UNSIGNED_INT, 0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
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

    if (simple_shader_vertex_source.size() == 0 || simple_shader_fragment_source.size() == 0)
        return;

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
    }

    if (action == GLFW_RELEASE){
        if (key == GLFW_KEY_W)  m_w_pressed = false;
        if (key == GLFW_KEY_S)  m_s_pressed = false;
        if (key == GLFW_KEY_A)  m_a_pressed = false;
        if (key == GLFW_KEY_D)  m_d_pressed = false;
        if (key == GLFW_KEY_Q)  m_q_pressed = false;
        if (key == GLFW_KEY_E)  m_e_pressed = false;

        if (key == GLFW_KEY_LEFT_CONTROL)           m_controlKeyHold = false;
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
