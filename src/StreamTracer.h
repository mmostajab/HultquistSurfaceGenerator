
/**
 *
 * Stream flow tracing library
 *
 * This class generates stream visualizations from vector fields.
 *
 */

#ifndef __STREAM_TRACER__
#define __STREAM_TRACER__

// STD
#include <vector>

// VTK
#include <vtkSmartPointer.h>
#include <vtkOpenFOAMReader.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

// RPE
#include "AABB.h"
#include "Grid.h"

class StreamTracer
{
public:

    StreamTracer();
    virtual ~StreamTracer();

    void loadOpenFOAM(std::string filename);

    void computeAccel();
    void computeStreamsurfaces(bool addition, bool remove, bool ripping);

    struct SurfaceParameters
    {
        // Tracing parameters
        enum TraceDirection { TD_FORWARD, TD_BACKWARD, TD_BOTH };

        TraceDirection  traceDirection;
        float           traceStepSize;
        unsigned int    traceMaxSteps;
        unsigned int    traceMaxSeeds;

        // Seeding plane
        std::vector< glm::vec3 >    seedingPoints;
        glm::vec3                   seedingLineCenter;
        glm::vec3                   seedingLineDirection;

        bool operator==(const SurfaceParameters& rval){
            if (traceDirection == rval.traceDirection    &&
                traceStepSize == rval.traceStepSize     &&
                traceMaxSteps == rval.traceMaxSteps     &&
                traceMaxSeeds == rval.traceMaxSeeds     &&
                seedingLineCenter == rval.seedingLineCenter &&
                seedingLineDirection == rval.seedingLineDirection){
                return true;
            }

            return false;
        }
    };

    void getParameters(SurfaceParameters &parameters);
    void setParameters(const SurfaceParameters &parameters);

    std::vector<glm::vec3> getVertices();
    std::vector<glm::vec3> getDerivatives();
    std::vector<float> getTexCoords();

    std::vector<unsigned int> getFaceIndices();

    std::vector<glm::vec3> getSeedingPoints();
    std::vector<glm::vec3> getAABB();

private:
    bool loadBinary(std::string filename);
    bool saveBinary(std::string filename);

    void generateSeedingPoints();
    bool traceRibbon(const unsigned int& ribbon_id, bool addition, bool remove, bool ripping);

    glm::vec3 derivate(const glm::vec3& point);

    bool seedIsValid(glm::vec3 seed);
    bool seedIsValid(glm::vec3 seed, size_t &i, size_t &j, size_t &k);

    SurfaceParameters m_surface_parameters;

    vtkSmartPointer<vtkOpenFOAMReader> m_reader;

    std::string m_filename;

    std::vector<AABB>      m_cellBoxes;
    std::vector<glm::vec3> m_cellPoints;
    std::vector<glm::vec3> m_cellVectors;

    AABB m_sceneBox;
    Grid m_sceneAccel;

    std::vector< glm::vec3 >    m_vertices;
    std::vector< glm::vec3 >    m_derivaties;
    std::vector< glm::vec3 >    m_normals;
    std::vector< glm::uint32 >  m_normal_counts;

    std::vector< float >        m_texCoords;
    std::vector< unsigned int>  m_faces;

    std::vector< int >  m_advancing_front;
    /*std::vector< std::vector< glm::vec3 > >  m_streamDerivs_forward;
    std::vector< std::vector< glm::vec3 > >  m_streamTexCoords_forward;

    std::vector< std::vector< glm::vec3 > >  m_streamLines_backward;
    std::vector< std::vector< glm::vec3 > >  m_streamDerivs_backward;
    std::vector< std::vector< glm::vec3 > >  m_streamTexCoords_backward;*/
};


#endif