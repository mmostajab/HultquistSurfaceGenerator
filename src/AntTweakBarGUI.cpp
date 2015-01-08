#include "AntTweakBarGUI.h"

AntTweakBarGUI::AntTweakBarGUI() {
    m_width  = -1;
    m_height = -1;
}

void AntTweakBarGUI::init(const unsigned int& width, const unsigned int& height) {
    m_width = width;    m_height = height;
    
    if (!TwInit(TwGraphAPI::TW_OPENGL, NULL))
        exit(EXIT_FAILURE);

    TwWindowSize(m_width, m_height);
    TwBar* myBar = TwNewBar("Tools");
    bool myVar = 0;
    TwAddVarRW(myBar, "OPENMP", TW_TYPE_BOOL8, &myVar, "");
}

void AntTweakBarGUI::draw() {
    TwDraw();
}

void AntTweakBarGUI::shutdown() {
    TwTerminate();
}

AntTweakBarGUI::~AntTweakBarGUI() {

}

void AntTweakBarGUI::TwEventMouseButtonGLFW3(int button, int action, int mods) {
    TwEventMouseButtonGLFW(button, action); 
}

void AntTweakBarGUI::TwEventMousePosGLFW3(double xpos, double ypos) { 
    TwMouseMotion(int(xpos), int(ypos)); 
}

void AntTweakBarGUI::TwEventMouseWheelGLFW3(double xoffset, double yoffset) { 
    TwEventMouseWheelGLFW(int(yoffset)); 
}

void AntTweakBarGUI::TwEventKeyGLFW3(int key, int scancode, int action, int mods) { 
    TwEventKeyGLFW(key, action); 
}

void AntTweakBarGUI::TwEventCharGLFW3(int codepoint) { 
    TwEventCharGLFW(codepoint, GLFW_PRESS); 
}

// Callback function called by GLFW when window size changes
void AntTweakBarGUI::WindowSizeCB(int width, int height) {
    // Send the new window size to AntTweakBar
    TwWindowSize(width, height);
}