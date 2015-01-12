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
    TwBar* generalBar       = TwNewBar("General");
    TwBar* seedinglineBar   = TwNewBar("Seeding Params");

    general_wireframe = false;
    TwAddVarRW(generalBar, "Wireframe", TW_TYPE_BOOL8, &general_wireframe, "");

    general_streamlines = false;
    TwAddVarRW(generalBar, "Streamlines", TW_TYPE_BOOL8, &general_streamlines, "");

    general_light_dir[0] = 1.0f;    general_light_dir[1] = 1.0f;    general_light_dir[2] = 1.0f;
    TwAddVarRW(generalBar, "Light Direction", TW_TYPE_DIR3F, general_light_dir, "");

    //TraceDirection  traceDirection;
    //float           traceStepSize;
    //unsigned int    traceMaxSteps;
    //unsigned int    traceMaxSeeds;

    //// Seeding plane
    //std::vector< glm::vec3 >    seedingPoints;
    //glm::vec3                   seedingLineCenter;

    seedingline_maxSeeds    = 0;
    seedingline_maxSteps    = 0;
    seedingline_stepSize    = 0.01f;
    seedingline_center[0]   = 0.0f;
    seedingline_center[1]   = 0.0f;
    seedingline_center[2]   = 0.0f;
    seedingline_dir[0]      = 0.0f;
    seedingline_dir[1]      = 0.0f;
    seedingline_dir[2]      = 0.0f;

    TwAddSeparator(seedinglineBar, "Segments", "");
    //TwAddVarRW(seedinglineBar, "Direction2", )
    TwAddVarRW(seedinglineBar, "Max Seeds", TW_TYPE_UINT32, &seedingline_maxSeeds, "");
    TwAddVarRW(seedinglineBar, "Max Steps", TW_TYPE_UINT32, &seedingline_maxSteps, "");
    TwAddVarRW(seedinglineBar, "Step Size", TW_TYPE_FLOAT,  &seedingline_stepSize, "");
    TwAddSeparator(seedinglineBar, "Spaital", "");
    TwAddVarRW(seedinglineBar, "Point",     TW_TYPE_DIR3F,  &seedingline_center, "");
    TwAddVarRW(seedinglineBar, "Direction", TW_TYPE_DIR3F,  &seedingline_dir, "");

    seedingline_dir[0] = 0.0f;
    seedingline_dir[1] = 1.0f;
    seedingline_dir[2] = 0.0f;
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