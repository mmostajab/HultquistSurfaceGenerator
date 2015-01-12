#include "StreamSurfaceRenderer.h"

#include "helper.h"

#include <GL/glew.h>

StreamSurfaceRenderer::StreamSurfaceRenderer() {
    buffer_needs_update = true;
}

void StreamSurfaceRenderer::compileShaders() {
    GLuint simple_vertex_shader, simple_fragment_shader;
    std::string simple_shader_vertex_source = convertFileToString("../../src/glsl/simple.vert");
    std::string simple_shader_fragment_source = convertFileToString("../../src/glsl/simple.frag");

    if (simple_shader_vertex_source.size() == 0 || simple_shader_fragment_source.size() == 0)
        return;

    simple_vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    simple_fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    const GLchar* simple_shader_vertex_sourcePtr = simple_shader_vertex_source.c_str();
    const GLchar* simple_shader_fragment_sourcePtr = simple_shader_fragment_source.c_str();

    glShaderSource(simple_vertex_shader, 1, &simple_shader_vertex_sourcePtr, NULL);
    glShaderSource(simple_fragment_shader, 1, &simple_shader_fragment_sourcePtr, NULL);
    glCompileShader(simple_vertex_shader);
    glCompileShader(simple_fragment_shader);

    render_program = glCreateProgram();
    glAttachShader(render_program, simple_vertex_shader);
    glAttachShader(render_program, simple_fragment_shader);
    glLinkProgramARB(render_program);

    glDeleteShader(simple_vertex_shader);
    glDeleteShader(simple_fragment_shader);
}

void StreamSurfaceRenderer::loadOpenFOAM(std::string filename) {
    m_streamtracer.loadOpenFOAM(filename);
    m_streamtracer.computeAccel();
}

void StreamSurfaceRenderer::computeStreamSurface() {
    std::cout << "Computing the stream lines...\n";

    m_streamtracer.computeStreamsurfaces();

    std::cout << "Computing the stream lines...Done.\n";
}

void StreamSurfaceRenderer::update(const double& time, const double& timeSinceLastFrame) {
    if (buffer_needs_update){
        m_streamtracer.computeStreamsurfaces();
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
    }

    std::vector<std::vector<glm::vec3>> streamsurface_vertices = m_streamtracer.getStreamSurfaceLines_Forward();
    std::vector<std::vector<glm::vec3>> streamsurface_derivs = m_streamtracer.getStreamSurfaceDerivs_Forward();

    std::vector<glm::vec3> vertices, colors, normals;
    std::vector<glm::uint32> indices;

    glm::vec3 center(0.0f, 0.0f, 0.0f);
    unsigned int count = 0;
    for (size_t i = 0; i < streamsurface_vertices.size(); i++){
        for (size_t j = 0; j < streamsurface_vertices[i].size(); j++){
            vertices.push_back(streamsurface_vertices[i][j]);
            colors.push_back(glm::normalize(streamsurface_derivs[i][j]) * 0.5f + glm::vec3(0.5f, 0.5f, 0.5f));
            
            bool has_left_neighbour = false, has_right_neighbour = false;
            if (i > 0 && j < streamsurface_vertices[i - 1].size())
                has_left_neighbour = true;

            if (i < streamsurface_vertices.size() - 1 && j < streamsurface_vertices[i + 1].size())
                has_right_neighbour = true;

            if (has_left_neighbour && has_right_neighbour){
                float h = glm::length(streamsurface_vertices[i + 1][j] - streamsurface_vertices[i][j]);
                float h_prime = glm::length(streamsurface_vertices[i][j] - streamsurface_vertices[i - 1][j]);

                glm::vec3 v1 = (h_prime * streamsurface_vertices[i + 1][j] + (h - h_prime) * streamsurface_vertices[i][j] - h * streamsurface_vertices[i - 1][j]);
                glm::vec3 v2 = streamsurface_derivs[i][j];
                normals.push_back( glm::cross(v1, v2) );
            } else if (has_left_neighbour){
                glm::vec3 v1 = (streamsurface_vertices[i][j] - streamsurface_vertices[i - 1][j]) / glm::length(streamsurface_vertices[i][j] - streamsurface_vertices[i - 1][j]);
                glm::vec3 v2 = streamsurface_derivs[i][j];
                normals.push_back( glm::cross(v1, v2) );
            } else if (has_right_neighbour){
                glm::vec3 v1 = (streamsurface_vertices[i + 1][j] - streamsurface_vertices[i][j]) / glm::length(streamsurface_vertices[i + 1][j] - streamsurface_vertices[i][j]);
                glm::vec3 v2 = streamsurface_derivs[i][j];
                normals.push_back( glm::cross(v1, v2) );
            } else normals.push_back(glm::vec3(0.0f, 0.0f, 0.0f));

            if (m_mode == Mode::STREAM_SURFACE){
                if (i != streamsurface_vertices.size() - 1 && j < streamsurface_vertices[i].size() - 1 && j < streamsurface_vertices[i + 1].size() - 1){
                    indices.push_back(count);
                    indices.push_back(count + streamsurface_vertices[i].size());
                    indices.push_back(count + 1);

                    indices.push_back(count + 1);
                    indices.push_back(count + streamsurface_vertices[i].size());
                    indices.push_back(count + streamsurface_vertices[i].size() + 1);
                }
            } 
            else if (m_mode == Mode::STREAM_LINES){
                if (j < streamsurface_vertices[i].size() - 1){
                    indices.push_back(count);
                    indices.push_back(count + 1);
                }
            }

            center += streamsurface_vertices[i][j];
            count++;
        }
    }
    m_nIndices = indices.size();
    center /= count;

    for (size_t i = 0; i < vertices.size(); i++){
        vertices[i] -= center;
    }

    glGenBuffers(4, m_buffers);

    glBindBuffer(GL_ARRAY_BUFFER, m_buffers[0]);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    glBindBuffer(GL_ARRAY_BUFFER, m_buffers[1]);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec3), &colors[0], GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_buffers[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(glm::uint32), &indices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_buffers[3]);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    buffer_needs_update = false;
}

void StreamSurfaceRenderer::draw() {
    glUseProgram(render_program);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, m_buffers[0]);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    glBindBuffer(GL_ARRAY_BUFFER, m_buffers[1]);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_buffers[2]);

    if (m_mode == Mode::STREAM_SURFACE)
        glDrawElements(GL_TRIANGLES, m_nIndices, GL_UNSIGNED_INT, 0);
    else if (m_mode == Mode::STREAM_LINES)
        glDrawElements(GL_LINES, m_nIndices, GL_UNSIGNED_INT, 0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
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