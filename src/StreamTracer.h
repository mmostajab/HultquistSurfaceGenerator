
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
    void computeStreamsurfaces();

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
            if (traceDirection       == rval.traceDirection    &&
                traceStepSize        == rval.traceStepSize     &&
                traceMaxSteps        == rval.traceMaxSteps     &&
                traceMaxSeeds        == rval.traceMaxSeeds     &&
                seedingLineCenter    == rval.seedingLineCenter &&
                seedingLineDirection == rval.seedingLineDirection){
                return true;
            }

            return false;
        }
    };

    void getParameters(SurfaceParameters &parameters);
    void setParameters(const SurfaceParameters &parameters);

    std::vector<std::vector<glm::vec3>> getStreamSurfaceLines_Forward();
    std::vector<std::vector<glm::vec3>> getStreamSurfaceDerivs_Forward();

private:
    bool loadBinary(std::string filename);
    bool saveBinary(std::string filename);

    void traceStreamline(glm::vec3 seed, float stepsize, std::vector<glm::vec3> &streamLines, std::vector<glm::vec3> &streamColors, std::vector<glm::vec3> &streamTexCoords);
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

    std::vector< std::vector< glm::vec3 > >  m_streamLines_forward;
    std::vector< std::vector< glm::vec3 > >  m_streamDerivs_forward;
    std::vector< std::vector< glm::vec3 > >  m_streamTexCoords_forward;

    std::vector< std::vector< glm::vec3 > >  m_streamLines_backward;
    std::vector< std::vector< glm::vec3 > >  m_streamDerivs_backward;
    std::vector< std::vector< glm::vec3 > >  m_streamTexCoords_backward;
};

#endif