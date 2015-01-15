#ifndef __ANT_TWEAK_BAR_GUI__
#define __ANT_TWEAK_BAR_GUI__

#include <AntTweakBar.h>
#include <GLFW/glfw3.h>

class AntTweakBarGUI {
public:
    AntTweakBarGUI();

    void init(const unsigned int& width, const unsigned int& height);
    void draw();
    void shutdown();

    ~AntTweakBarGUI();

    static void TwEventMouseButtonGLFW3(int button, int action, int mods);
    static void TwEventMousePosGLFW3(double xpos, double ypos);
    static void TwEventMouseWheelGLFW3(double xoffset, double yoffset);
    static void TwEventKeyGLFW3(int key, int scancode, int action, int mods);
    static void TwEventCharGLFW3(int codepoint);

    // Callback function called by GLFW when window size changes
    static void WindowSizeCB(int width, int height);

//private:
public:
    unsigned int m_width, m_height;

    bool  general_wireframe;
    bool  general_streamlines;
    float general_light_dir[3];

    //TraceDirection  traceDirection;
    float           seedingline_stepSize;
    unsigned int    seedingline_maxSteps;
    unsigned int    seedingline_maxSeeds;

    // Seeding plane
    float           seedingline_center[3];
    float           seedingline_dir[3];

    bool            tracing_addition;
    bool            tracing_remove; 
    bool            tracing_ripping;
};

#endif