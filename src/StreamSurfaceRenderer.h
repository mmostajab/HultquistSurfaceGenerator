/**
 *
 * Stream surface generator and renderer class.
 *
 * This class is an interface to generate and render the stream surfaces.
 *
 */

#ifndef __STREAM_TRACER_RENDERER__
#define __STREAM_TRACER_RENDERER__

// STD
#include <vector>
#include <string>

// GLM
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

// GL
#include <GL/glew.h>

// Stream Tracer
#include "StreamTracer.h"

// Camera
#include "Camera.h"

class StreamSurfaceRenderer
{
public:
    StreamSurfaceRenderer();

    void loadOpenFOAM(std::string filename);
    void computeStreamSurface();

    void compileShaders();
    void update(const double& time, const double& timeSinceLastFrame);
    void draw();

    void shutdown();

    enum Mode { STREAM_LINES, STREAM_SURFACE };
    void setMode(Mode);
    void getParameters(unsigned int& maxseeds, unsigned int& maxsteps, float& stepsize, float center[3], float dir[3]);
    void setParameters(const unsigned int& maxseeds, const unsigned int& maxsteps, const float& stepsize, const float center[3], const float dir[3]);

    virtual ~StreamSurfaceRenderer();

private:
    StreamTracer m_streamtracer;
    void update_buffers();

    bool buffer_needs_update;
    Mode m_mode;
    GLuint render_program;
    GLuint m_buffers[4];    // vertexbuffer, m_colorbuffer, m_indicesbuffer, m_normalbuffer
    size_t m_nIndices;
};

#endif