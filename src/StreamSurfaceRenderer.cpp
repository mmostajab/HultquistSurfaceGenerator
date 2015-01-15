#include "StreamSurfaceRenderer.h"

#include "helper.h"

#include <GL/glew.h>

StreamSurfaceRenderer::StreamSurfaceRenderer() {
    buffer_needs_update = true;
}

void StreamSurfaceRenderer::compileShaders() {
    GLenum e = glGetError();
    GLuint simple_vertex_shader, simple_fragment_shader, surface_vertex_shader, surface_fragment_shader;
    std::string simple_shader_vertex_source = convertFileToString("../../src/glsl/simple.vert");
    std::string simple_shader_fragment_source = convertFileToString("../../src/glsl/simple.frag");

    std::string surface_shader_vertex_source = convertFileToString("../../src/glsl/surface.vert");
    std::string surface_shader_fragment_source = convertFileToString("../../src/glsl/surface.frag");

    if (simple_shader_vertex_source.size() == 0 || simple_shader_fragment_source.size() == 0 || surface_shader_vertex_source.size() == 0 || surface_shader_fragment_source.size() == 0)
        return;

    simple_vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    simple_fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    e = glGetError();

    surface_vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    surface_fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    e = glGetError();

    const GLchar* simple_shader_vertex_sourcePtr = simple_shader_vertex_source.c_str();
    const GLchar* simple_shader_fragment_sourcePtr = simple_shader_fragment_source.c_str();
    const GLchar* surface_shader_vertex_sourcePtr = surface_shader_vertex_source.c_str();
    const GLchar* surface_shader_fragment_sourcePtr = surface_shader_fragment_source.c_str();

    glShaderSource(simple_vertex_shader, 1, &simple_shader_vertex_sourcePtr, NULL);
    glShaderSource(simple_fragment_shader, 1, &simple_shader_fragment_sourcePtr, NULL);
    glShaderSource(surface_vertex_shader, 1, &surface_shader_vertex_sourcePtr, NULL);
    glShaderSource(surface_fragment_shader, 1, &surface_shader_fragment_sourcePtr, NULL);
    e = glGetError();

    glCompileShader(simple_vertex_shader);
    glCompileShader(simple_fragment_shader);
    glCompileShader(surface_vertex_shader);
    glCompileShader(surface_fragment_shader);
    e = glGetError();

    simple_program = glCreateProgram();
    glAttachShader(simple_program, simple_vertex_shader);
    glAttachShader(simple_program, simple_fragment_shader);
    glLinkProgramARB(simple_program);

    render_program = glCreateProgram();
    glAttachShader(render_program, surface_vertex_shader);
    glAttachShader(render_program, surface_fragment_shader);
    glLinkProgramARB(render_program);

    glDeleteShader(simple_vertex_shader);
    glDeleteShader(simple_fragment_shader);
    glDeleteShader(surface_vertex_shader);
    glDeleteShader(surface_fragment_shader);
    e = glGetError();
}

void StreamSurfaceRenderer::loadOpenFOAM(std::string filename) {
    m_streamtracer.loadOpenFOAM(filename);
    m_streamtracer.computeAccel();
}

void StreamSurfaceRenderer::computeStreamSurface(bool addition, bool remove, bool ripping) {
    std::cout << "Computing the stream lines...\n";

    m_streamtracer.computeStreamsurfaces(addition, remove, ripping);

    std::cout << "Computing the stream lines...Done.\n";
}

void StreamSurfaceRenderer::update(const double& time, const double& timeSinceLastFrame, bool addition, bool remove, bool ripping) {
    if (buffer_needs_update){
        m_streamtracer.computeStreamsurfaces(addition, remove, ripping);
        update_buffers();
    }
}

void StreamSurfaceRenderer::setMode(Mode mode) {
    if (m_mode != mode){
        buffer_needs_update = true;
    }
    m_mode = mode;
}

void StreamSurfaceRenderer::update_buffers() {
    GLsizei nbuffers = sizeof(m_buffers) / sizeof(GLuint);
    if (m_buffers[0] > 0){
        glDeleteBuffers(nbuffers, m_buffers);
        glDeleteBuffers(1, &m_bounding_box_buffer);
        glDeleteBuffers(1, &m_seeding_line_buffer);
    }

    GLenum e = glGetError();

    std::vector<glm::vec3> vertices   = m_streamtracer.getVertices();
    std::vector<glm::vec3> colors     = m_streamtracer.getDerivatives();
    std::vector<glm::vec3> normals    = m_streamtracer.getDerivatives();

    std::vector<glm::vec3> seedingPoints = m_streamtracer.getSeedingPoints();
    std::vector<glm::vec3> boundingboxPoints = m_streamtracer.getAABB();

    std::vector<unsigned int> indices = m_streamtracer.getFaceIndices();


    glm::vec3 center(0.0f, 0.0f, 0.0f);
    for (size_t v = 0; v < vertices.size(); v++){
        center += vertices[v];
    }
    center /= vertices.size();
    for (size_t v = 0; v < vertices.size(); v++){
        vertices[v] -= center;
    }
    for (size_t s = 0; s < seedingPoints.size(); s++){
        seedingPoints[s] -= center;
    }
    for (size_t b = 0; b < boundingboxPoints.size(); b++){
        boundingboxPoints[b] -= center;
    }
    for (size_t c = 0; c < colors.size(); c++){
        colors[c] = glm::normalize(colors[c]) * 0.5f + glm::vec3(0.5f, 0.5f, 0.5f);
    }

    m_nIndices = indices.size();
    m_nSeedingPoints = seedingPoints.size();

    glGenBuffers(4, m_buffers);
    e = glGetError();

    glBindBuffer(GL_ARRAY_BUFFER, m_buffers[0]);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    e = glGetError();

    glBindBuffer(GL_ARRAY_BUFFER, m_buffers[1]);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec3), &colors[0], GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    e = glGetError();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_buffers[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(glm::uint32), &indices[0], GL_STATIC_DRAW);
    e = glGetError();

    glBindBuffer(GL_ARRAY_BUFFER, m_buffers[3]);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    e = glGetError();

    glGenBuffers(1, &m_bounding_box_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_bounding_box_buffer);
    glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(glm::vec3), &boundingboxPoints[0], GL_STATIC_DRAW);
    e = glGetError();

    glGenBuffers(1, &m_seeding_line_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_seeding_line_buffer);
    glBufferData(GL_ARRAY_BUFFER, seedingPoints.size() * sizeof(glm::vec3), &seedingPoints[0], GL_STATIC_DRAW);
    e = glGetError();

    buffer_needs_update = false;
}

void StreamSurfaceRenderer::draw() {

    GLenum e = glGetError();
    glUseProgram(render_program);

    e = glGetError();
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    e = glGetError();
    glBindBuffer(GL_ARRAY_BUFFER, m_buffers[0]);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    e = glGetError();
    glBindBuffer(GL_ARRAY_BUFFER, m_buffers[1]);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    glBindBuffer(GL_ARRAY_BUFFER, m_buffers[3]);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_buffers[2]);
    e = glGetError();

    if (m_mode == Mode::STREAM_SURFACE)
        glDrawElements(GL_TRIANGLES, m_nIndices, GL_UNSIGNED_INT, 0);
    else if (m_mode == Mode::STREAM_LINES)
        glDrawElements(GL_LINES, m_nIndices, GL_UNSIGNED_INT, 0);

    e = glGetError();
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    e = glGetError();
}

void StreamSurfaceRenderer::draw_2d(){
    GLenum e = glGetError();
    glPointSize(3.0f);

    glUseProgram(simple_program);
    e = glGetError();
    if (m_bounding_box_buffer > 0){

        glBindBuffer(GL_ARRAY_BUFFER, m_bounding_box_buffer);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

        glDrawArrays(GL_LINES, 0, 24);
        glDisableVertexAttribArray(0);
    }
    e = glGetError();

    if (m_seeding_line_buffer > 0){

        glBindBuffer(GL_ARRAY_BUFFER, m_seeding_line_buffer);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

        glDrawArrays(GL_LINE_STRIP, 0, m_nSeedingPoints);
        glDrawArrays(GL_POINTS, 0, m_nSeedingPoints);
        glDisableVertexAttribArray(0);
    }
    e = glGetError();
}

void StreamSurfaceRenderer::getParameters(unsigned int& maxseeds, unsigned int& maxsteps, float& stepsize, float center[3], float dir[3]) {
    StreamTracer::SurfaceParameters params;
    m_streamtracer.getParameters(params);

    maxseeds = params.traceMaxSeeds;
    maxsteps = params.traceMaxSteps;
    stepsize = params.traceStepSize;

    // should set and generate the line points again
    center[0] = params.seedingLineCenter[0];    center[1] = params.seedingLineCenter[1];    center[2] = params.seedingLineCenter[2];
    dir[0] = params.seedingLineDirection[0];    dir[1] = params.seedingLineDirection[1];    dir[2] = params.seedingLineDirection[2];
}

void StreamSurfaceRenderer::setParameters(const unsigned int& maxseeds, const unsigned int& maxsteps, const float& stepsize, const float center[3], const float dir[3]) {
    StreamTracer::SurfaceParameters params;
    StreamTracer::SurfaceParameters last_params;

    params.traceDirection = StreamTracer::SurfaceParameters::TraceDirection::TD_BOTH;
    params.traceMaxSeeds = maxseeds;
    params.traceMaxSteps = maxsteps;
    params.traceStepSize = stepsize;
    params.seedingLineCenter    = glm::vec3(center[0], center[1], center[2]);
    params.seedingLineDirection = glm::vec3(dir[0], dir[1], dir[2]);

    m_streamtracer.getParameters(last_params);

    if (!(last_params == params)){
        m_streamtracer.setParameters(params);
        buffer_needs_update = true;
    }
}

void StreamSurfaceRenderer::shutdown() {
    glDeleteBuffers(3, m_buffers);
}

StreamSurfaceRenderer::~StreamSurfaceRenderer() {

}