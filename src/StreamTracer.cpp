#include "StreamTracer.h"

// STD
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

// Boost
#include <boost/filesystem.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_real_distribution.hpp>

// OpenMP
#include <omp.h>

// VTK
#include <vtkSmartPointer.h>
#include <vtkProperty.h>
#include <vtkUnstructuredGrid.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkCellTypes.h>
#include <vtkDataSetMapper.h>
#include <vtkOpenFOAMReader.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkIdList.h>

// RPE
#include "AABB.h"
#include "Grid.h"

// #define STREAM_TRACER_USE_CELL_LIST // Test all primitives in an acceleration cell
#define STREAM_TRACER_USE_OMP       // Use OpenMP multi-threading

StreamTracer::StreamTracer()
{
    // Default surface tracing parameters
    m_surface_parameters.traceDirection = SurfaceParameters::TD_BOTH;
    m_surface_parameters.traceStepSize  = 0.001f;
    m_surface_parameters.traceMaxSteps  = 1000;
    m_surface_parameters.traceMaxSeeds  = 100;

    m_surface_parameters.seedingLineCenter = glm::vec3(0.0f, 0.0f, 0.0f);
    m_surface_parameters.seedingLineDirection = glm::vec3(0.0f, 0.0f, 1.0f); 
}

StreamTracer::~StreamTracer()
{
}

void StreamTracer::loadOpenFOAM(std::string filename)
{
    // Load binary, if available
    m_filename = filename;
    if (loadBinary(filename + ".bin"))
        return;

    std::cout << "loadOpenFOAM: loading " << filename << std::endl;

    // Read the file
    m_reader = vtkSmartPointer<vtkOpenFOAMReader>::New();
    m_reader->SetFileName(filename.c_str());
    m_reader->Update();
    //m_reader->SetTimeValue(0.5);
    m_reader->SetTimeValue(2700);
    //m_reader->SetTimeValue(8000.5);
    m_reader->ReadZonesOn();
    m_reader->Update();

    // Parse data
    vtkDataSet   *block0    = vtkDataSet::SafeDownCast(m_reader->GetOutput()->GetBlock(0));
    vtkCellData  *cellData  = block0->GetCellData();
    vtkDataArray *dataArray = cellData->GetVectors("U");

    vtkIdType numCells  = block0->GetNumberOfCells();
    vtkIdType numPoints = block0->GetNumberOfPoints();
    vtkIdType numTuples = dataArray->GetNumberOfTuples();

    std::cout << std::endl
           << "  number of cells: "     << numCells                                                                   << std::endl
           << "  number of points: "    << numPoints                                                                  << std::endl
           << "  number of tuples: U: " << numTuples << " (components: " << dataArray->GetNumberOfComponents() << ")" << std::endl;

    // Construct cell boxes
    for (vtkIdType i=0; i<numCells; i++)
    {
        vtkSmartPointer<vtkIdList> cellPointIds = vtkSmartPointer<vtkIdList>::New();
        block0->GetCellPoints(i, cellPointIds);

        AABB box;
        for (vtkIdType j=0; j<cellPointIds->GetNumberOfIds(); j++)
        {
            double point[3];
            block0->GetPoint(cellPointIds->GetId(j), point);
            box.extend((float)point[0], (float)point[1], (float)point[2]);
        }
        m_cellBoxes.push_back(box);
    }

    // Import point data
    for (vtkIdType i=0; i<numPoints; i++)
    {
        double point[3];
        block0->GetPoint(i, point);
        m_cellPoints.push_back(glm::vec3((float)point[0], (float)point[1], (float)point[2]));
    }

    // Import cell data
    for (vtkIdType i=0; i<numTuples; i++)
    {
        double tuple[3];
        dataArray->GetTuple(i, tuple);
        m_cellVectors.push_back(glm::vec3((float)tuple[0], (float)tuple[1], (float)tuple[2]));
    }

    saveBinary(filename + ".bin");
}

std::vector<std::vector<glm::vec3>> StreamTracer::getStreamSurfaceLines_Forward() {
    return m_streamLines_forward;
}

std::vector<std::vector<glm::vec3>> StreamTracer::getStreamSurfaceDerivs_Forward() {
    return m_streamDerivs_forward;
}

void StreamTracer::computeAccel()
{
    std::cout << "Computing Acceleration Structure...";
    for (size_t i=0; i<m_cellBoxes.size(); i++)
        m_sceneBox.extend(m_cellBoxes[i]);

    // Enlarge grid box to avoid primitives on boundaries
    const float EPSILON = 0.0001f;
    AABB accelBox = m_sceneBox;
    accelBox.enlarge(EPSILON);

    // Use optimized parameters for known scenes
    if (boost::filesystem::path(m_filename).filename() == "othmer.foam")
    {
        m_sceneAccel.reset(accelBox, 1000);
        m_surface_parameters.traceMaxSteps = 1000;
        m_surface_parameters.traceStepSize = 0.01f;
    }
    else if (boost::filesystem::path(m_filename).filename() == "Numeca_StuetzLaufSaug_Q82_Lauf0.cgns")
    {
        m_sceneAccel.reset(accelBox, 50, 100, 100);
        m_surface_parameters.traceMaxSteps = 1000;
        m_surface_parameters.traceStepSize = 0.0001f;
    }
    //this should be adapted later. currently, we only have one cgns file that is to large to be loaded with the default parameters (leads to crash)
    else if (boost::filesystem::path(m_filename).extension() == ".cgns")
    {
        m_sceneAccel.reset(accelBox, 50, 100, 100);
        m_surface_parameters.traceMaxSteps = 1000;
        m_surface_parameters.traceStepSize = 0.0001f;
    }
    else
    {
        m_sceneAccel.reset(accelBox);
    }

    m_sceneAccel.insertPrimitiveList(m_cellBoxes);

    glm::vec3 center = 0.5f * ( 
        glm::vec3(m_sceneBox.min[0], m_sceneBox.min[1], m_sceneBox.min[2]) + 
        glm::vec3(m_sceneBox.max[0], m_sceneBox.max[1], m_sceneBox.max[2]) );
    m_surface_parameters.seedingLineCenter = center;

    std::cout << "Done\n";
}

void StreamTracer::traceStreamline(glm::vec3 seed, float stepsize, std::vector<glm::vec3> &streamLines, std::vector<glm::vec3> &streamColors, std::vector<glm::vec3> &streamTexCoords)
{
    glm::vec3 s0 = seed;

    for (unsigned s=0; s<m_surface_parameters.traceMaxSteps; s++)
    {
        // TODO First point should be stored

        size_t i, j, k;
        if (!seedIsValid( s0, i, j, k ))
            break;

        std::vector<Grid::PrimitiveIndex> &primitives = m_sceneAccel.getPrimitives( i, j, k );
        size_t t=0;

#ifdef STREAM_TRACER_USE_CELL_LIST
        for (; t<primitives.size(); t++)
            if ( m_cellBoxes[primitives[t]].contains( (float*)(&s0) ) )
                break;
        if (t==primitives.size())
            break;
#endif

        glm::vec3 d0 = m_cellVectors[primitives[t]];
        glm::vec3 s1 = s0 + stepsize * d0;

        //streamLines.push_back(s0);
        streamLines.push_back(s1);
        
        streamTexCoords.push_back(glm::vec3(d0.length(), d0.length(), d0.length()));
        //streamTexCoords.push_back(glm::vec3(d0.length(), d0.length(), d0.length()));

        d0 = glm::normalize(d0);

        streamColors.push_back(d0);
        //streamColors.push_back(d0);

        s0 = s1;
    }
}

void StreamTracer::computeStreamsurfaces() {
    clock_t streamComputation_start = clock();

    m_streamLines_forward.clear();
    m_streamDerivs_forward.clear();
    m_streamTexCoords_forward.clear();

    m_streamLines_backward.clear();
    m_streamDerivs_backward.clear();
    m_streamTexCoords_backward.clear();

    glm::vec3 line_direction(0.0f, 0.0f, 1.0f);
    boost::random::mt19937 rng;
    boost::random::uniform_real_distribution<> dist(-0.5f, +0.5f);
    float len = .4f;
    for (size_t s = 0; s < m_surface_parameters.traceMaxSeeds; s++){
        glm::vec3 seed;
        //do
        seed = m_surface_parameters.seedingLineCenter + (s * len / m_surface_parameters.traceMaxSeeds - .2f) * m_surface_parameters.seedingLineDirection;
        //while (!seedIsValid(seed));
        if (seedIsValid(seed))
            m_surface_parameters.seedingPoints.push_back( seed );
    }

    m_streamLines_forward.resize(m_surface_parameters.seedingPoints.size());
    m_streamDerivs_forward.resize(m_surface_parameters.seedingPoints.size());
    m_streamTexCoords_forward.resize(m_surface_parameters.seedingPoints.size());

    m_streamLines_backward.resize(m_surface_parameters.seedingPoints.size());
    m_streamDerivs_backward.resize(m_surface_parameters.seedingPoints.size());
    m_streamTexCoords_backward.resize(m_surface_parameters.seedingPoints.size());

    unsigned lines = 0;
    for (unsigned s = 0; s<m_surface_parameters.seedingPoints.size(); s++)
    {
        glm::vec3 seed = m_surface_parameters.seedingPoints[s];
        bool valid = true;

        if (valid)
        {
            if (m_surface_parameters.traceDirection == SurfaceParameters::TD_FORWARD || m_surface_parameters.traceDirection == SurfaceParameters::TD_BOTH)
                traceStreamline(seed, +m_surface_parameters.traceStepSize, m_streamLines_forward[s], m_streamDerivs_forward[s], m_streamTexCoords_forward[s]);

            if (m_streamLines_forward[s].size() > 0)
                lines++;

            if (m_surface_parameters.traceDirection == SurfaceParameters::TD_BACKWARD || m_surface_parameters.traceDirection == SurfaceParameters::TD_BOTH)
                traceStreamline(seed, -m_surface_parameters.traceStepSize, m_streamLines_backward[s], m_streamDerivs_backward[s], m_streamTexCoords_backward[s]);

            if (m_streamLines_backward[s].size() > 0)
                lines++;
        }
    }

    float computationTime = ((float)(clock() - streamComputation_start) / CLOCKS_PER_SEC) * 1000.0f;
    std::cout << "Computation Time: " << computationTime << " and per line is " << computationTime / lines << std::endl;
    std::cout << "Number of Computed Stream lines: " << lines << std::endl;
    // lines_per_sec = lines/timer.GetSecondes();
}

bool StreamTracer::seedIsValid(glm::vec3 seed) {
    size_t i, j, k;
    return seedIsValid(seed, i, j, k);
}

bool StreamTracer::seedIsValid(glm::vec3 seed, size_t &i, size_t &j, size_t &k)
{
    float *point = (float*)(&seed);

    if (!m_sceneBox.contains( point ))
        return false;

    m_sceneAccel.locateCell( i, j, k, point );
    return !( m_sceneAccel.emptyCell( i, j, k ) );
}

void StreamTracer::getParameters( SurfaceParameters &parameters )
{
    parameters = m_surface_parameters;
}

void StreamTracer::setParameters( const SurfaceParameters &paramters )
{
    m_surface_parameters = paramters;
}

bool StreamTracer::loadBinary( std::string filename )
{
    std::ifstream in(filename.c_str(), std::ios_base::binary);

    if (in)
    {
        std::cout << "loadBinary: loading " << filename << std::endl;

        size_t cellBoxesNumber;
        size_t cellPointsNumber;
        size_t cellVectorsNumber;

        in >> cellBoxesNumber >> std::ws;
        in >> cellPointsNumber >> std::ws;
        in >> cellVectorsNumber >> std::ws;

        m_cellBoxes.resize(cellBoxesNumber);
        m_cellPoints.resize(cellPointsNumber);
        m_cellVectors.resize(cellVectorsNumber);

        in.read((char*)(&m_cellBoxes[0]), sizeof(AABB)*cellBoxesNumber);
        in.read((char*)(&m_cellPoints[0]), sizeof(glm::vec3)*cellPointsNumber);
        in.read((char*)(&m_cellVectors[0]), sizeof(glm::vec3)*cellVectorsNumber);
    }

    return !!in;
}

bool StreamTracer::saveBinary( std::string filename )
{
    std::ofstream out(filename.c_str(), std::ios_base::binary);

    if (out)
    {
        std::cout << "saveBinary: saving " << filename << std::endl;

        size_t cellBoxesNumber   = m_cellBoxes.size();
        size_t cellPointsNumber  = m_cellPoints.size();
        size_t cellVectorsNumber = m_cellVectors.size();

        out << cellBoxesNumber << std::endl;
        out << cellPointsNumber << std::endl;
        out << cellVectorsNumber << std::endl;

        out.write((char*)(&m_cellBoxes[0]), sizeof(AABB)*cellBoxesNumber);
        out.write((char*)(&m_cellPoints[0]), sizeof(glm::vec3)*cellPointsNumber);
        out.write((char*)(&m_cellVectors[0]), sizeof(glm::vec3)*cellVectorsNumber);
    }

    return !!out;
}