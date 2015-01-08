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
    // Default tracing parameters
    m_parameters.traceDirection       = Parameters::TD_BOTH;
    m_parameters.traceStepSize        = 0.001f;
    m_parameters.traceMaxSteps        = 10000;
    m_parameters.traceMaxSeeds        = 1000;
    m_parameters.traceContSeeding     = false;

    // Default seed plane parameters
    m_parameters.seedPlaneSize        = 0.2f;
    m_parameters.seedPlaneCenter      = glm::vec3(0.0f, 0.0f, 0.0f);
    // m_parameters.seedPlaneOrientation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
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

void StreamTracer::exportCellsOSG()
{
    
}

void StreamTracer::exportGlyphsOSG()
{

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
        m_parameters.traceMaxSteps = 1000;
        m_parameters.traceStepSize = 0.01f;
    }
    else if (boost::filesystem::path(m_filename).filename() == "Numeca_StuetzLaufSaug_Q82_Lauf0.cgns")
    {
        m_sceneAccel.reset(accelBox, 50, 100, 100);
        m_parameters.traceMaxSteps = 1000;
        m_parameters.traceStepSize = 0.0001f;
    }
    //this should be adapted later. currently, we only have one cgns file that is to large to be loaded with the default parameters (leads to crash)
    else if (boost::filesystem::path(m_filename).extension() == ".cgns")
    {
        m_sceneAccel.reset(accelBox, 50, 100, 100);
        m_parameters.traceMaxSteps = 1000;
        m_parameters.traceStepSize = 0.0001f;
    }
    else
    {
        m_sceneAccel.reset(accelBox);
    }

    m_sceneAccel.insertPrimitiveList(m_cellBoxes);

    glm::vec3 center = 0.5f * ( 
        glm::vec3(m_sceneBox.min[0], m_sceneBox.min[1], m_sceneBox.min[2]) + 
        glm::vec3(m_sceneBox.max[0], m_sceneBox.max[1], m_sceneBox.max[2]) );
    m_parameters.seedPlaneCenter = center;

    std::cout << "Done\n";
}

void StreamTracer::computeStreamlines()
{
    clock_t streamComputation_start = clock();

    glm::vec3 xAxis;
    glm::vec3 yAxis;

    //m_parameters.seedPlaneOrientation.multVec(glm::vec3(1,0,0), xAxis);
    //m_parameters.seedPlaneOrientation.multVec(glm::vec3(0,1,0), yAxis);

    //if (!cont_seeding)
    //	srand(1);

    m_streamLines.clear();
    m_streamColors.clear();
    m_streamTexCoords.clear();

    unsigned lines = 0;

    int tid, numThreads, maxThreads=omp_get_max_threads();

    std::vector< std::vector<glm::vec3> > streamLines;
    std::vector< std::vector<glm::vec3> > streamColors;
    std::vector< std::vector<glm::vec3> > streamTexCoords;
    streamLines.resize(maxThreads);
    streamColors.resize(maxThreads);
    streamTexCoords.resize(maxThreads);

#ifdef STREAM_TRACER_USE_OMP
#pragma omp parallel private(numThreads, tid)
#endif
{
    tid        = omp_get_thread_num();
    numThreads = omp_get_num_threads();

    if (tid == 0)
    {
        std::cout << "computeStreamlines: OMP threads maximum: " << maxThreads << ", running: " << numThreads << std::endl;
    }

    boost::random::mt19937 rng;
    boost::random::uniform_real_distribution<> dist(-0.5f, +0.5f);
    rng.seed(tid);

    for (unsigned s=0; s<m_parameters.traceMaxSeeds/numThreads; s++)
    {
        glm::vec3 seed;
        bool valid = false;

        for (unsigned t=0; t<1000; t++)
        {
            const float u = dist(rng);
            const float v = dist(rng);

            seed = m_parameters.seedPlaneCenter
                 + u * m_parameters.seedPlaneSize * xAxis
                 + v * m_parameters.seedPlaneSize * yAxis;

            if ( (valid = seedIsValid( seed )) )
                break;
        }

        if (valid)
        {
            if (m_parameters.traceDirection==Parameters::TD_FORWARD || m_parameters.traceDirection==Parameters::TD_BOTH)
                traceStreamline(seed, +m_parameters.traceStepSize, streamLines[tid], streamColors[tid], streamTexCoords[tid]);

            if(streamLines[tid].size() > 0)
                lines++;

            if (m_parameters.traceDirection==Parameters::TD_BACKWARD || m_parameters.traceDirection==Parameters::TD_BOTH)
                traceStreamline(seed, -m_parameters.traceStepSize, streamLines[tid], streamColors[tid], streamTexCoords[tid]);

            if(streamLines[tid].size() > 0)
                lines++;
        }
    }
}

    for (size_t i=0; i<streamLines.size(); i++)
    {
        m_streamLines.insert(m_streamLines.end(), streamLines[i].begin(), streamLines[i].end());
        m_streamColors.insert(m_streamColors.end(), streamColors[i].begin(), streamColors[i].end());
        m_streamTexCoords.insert(m_streamTexCoords.end(), streamTexCoords[i].begin(), streamTexCoords[i].end());
    }
    
    float computationTime = ((float)(clock() - streamComputation_start) / CLOCKS_PER_SEC) * 1000.0f;
    std::cout << "Computation Time: " << computationTime << " and per line is " << computationTime / lines << std::endl;
    std::cout << "Number of Computed Stream lines: " << lines << std::endl;
    // lines_per_sec = lines/timer.GetSecondes();
}

void StreamTracer::traceStreamline(glm::vec3 seed, float stepsize, std::vector<glm::vec3> &streamLines, std::vector<glm::vec3> &streamColors, std::vector<glm::vec3> &streamTexCoords)
{
    glm::vec3 s0 = seed;

    for (unsigned s=0; s<m_parameters.traceMaxSteps; s++)
    {
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

        streamLines.push_back(s0);
        streamLines.push_back(s1);
        
        streamTexCoords.push_back(glm::vec3(d0.length(), d0.length(), d0.length()));
        streamTexCoords.push_back(glm::vec3(d0.length(), d0.length(), d0.length()));

        d0 = glm::normalize(d0);
        glm::vec3 color = 0.5f * (d0 + glm::vec3(1.0f, 1.0f, 1.0f));

        streamColors.push_back(color);
        streamColors.push_back(color);

        s0 = s1;
    }
}

bool StreamTracer::seedIsValid(glm::vec3 seed)
{
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

void StreamTracer::getParameters( Parameters &parameters )
{
    parameters = m_parameters;
}

void StreamTracer::setParameters( const Parameters &paramters )
{
    m_parameters = paramters;
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