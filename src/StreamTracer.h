
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

class CGNSStructuredMesh;

class StreamTracer
{
public:

    StreamTracer();
    virtual ~StreamTracer();

    void loadOpenFOAM(std::string filename);

    void computeAccel();
    void computeStreamlines();

    void exportGlyphsOSG();
    void exportCellsOSG();

    struct Parameters
    {
        // Tracing parameters
        enum TraceDirection {TD_FORWARD, TD_BACKWARD, TD_BOTH};

        TraceDirection  traceDirection;
        float           traceStepSize;
        unsigned int    traceMaxSteps;
        unsigned int    traceMaxSeeds;
        bool		    traceContSeeding;

        // Seeding plane
        float			seedPlaneSize;
        glm::vec3		seedPlaneCenter;
        glm::quat 		seedPlaneOrientation;
    };

    void getParameters( Parameters &parameters );
    void setParameters( const Parameters &paramters );

    bool loadBinary( std::string filename );
    bool saveBinary( std::string filename );

    std::vector<glm::vec3> getStreamLines();
    std::vector<glm::vec3> getStreamColors();

private:

    void traceStreamline(glm::vec3 seed, float stepsize, std::vector<glm::vec3> &streamLines, std::vector<glm::vec3> &streamColors, std::vector<glm::vec3> &streamTexCoords);
    bool seedIsValid(glm::vec3 seed);
    bool seedIsValid(glm::vec3 seed, size_t &i, size_t &j, size_t &k);
    
    Parameters m_parameters;

    vtkSmartPointer<vtkOpenFOAMReader> m_reader;

    std::string m_filename;

    std::vector<AABB>      m_cellBoxes;
    std::vector<glm::vec3> m_cellPoints;
    std::vector<glm::vec3> m_cellVectors;

    AABB m_sceneBox;
    Grid m_sceneAccel;

    // Result glyphs
    std::vector<glm::vec3>  m_streamLines;
    std::vector<glm::vec3>  m_streamColors;
    std::vector<glm::vec3>  m_streamTexCoords;
};

#endif