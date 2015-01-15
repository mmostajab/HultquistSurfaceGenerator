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

bool StreamTracer::traceRibbon(const unsigned int& ribbon_id, bool addition, bool remove, bool ripping) {
    
    if (ribbon_id >= (m_advancing_front.size() / 2) - 1)
        return false;

    bool caught_up = false;
    float prev_diagonal = 0.0f;
    while (true){

        unsigned int L0 = m_advancing_front[2 * ribbon_id];
        unsigned int R0 = m_advancing_front[2 * ribbon_id + 1];

        glm::vec3 d_l = derivate(m_vertices[L0]);
        if (glm::length(d_l) < 1e-14f){
            break;
        }

        glm::vec3 d_r = derivate(m_vertices[R0]);
        if (glm::length(d_r) < 1e-14f){
            break;
        }

        // Ripping
        if (ripping && glm::dot(glm::normalize(d_l), glm::normalize(d_r)) < 0.8f){
            return false;
        }

        glm::vec3 p_l = m_vertices[L0] + d_l * m_surface_parameters.traceStepSize;
        glm::vec3 p_r = m_vertices[R0] + d_r * m_surface_parameters.traceStepSize;

        if (p_l == m_vertices[L0])
            break;

        if (p_r == m_vertices[R0])
            break;

        if (addition){
            // Addition
            //glm::float32 maxW = glm::max(glm::length(m_vertices[L1] - m_vertices[R1]), glm::length(m_vertices[L0] - m_vertices[R0]));
            //glm::float32 minH = glm::min(glm::length(m_vertices[L0] - m_vertices[L1]), glm::length(m_vertices[R1] - m_vertices[R0]));
            glm::float32 maxW = glm::length(p_l - p_r) + glm::length(m_vertices[L0] - m_vertices[R0]);
            glm::float32 minH = glm::length(m_vertices[L0] - p_l) + glm::length(p_r - m_vertices[R0]);
            if (maxW / minH > 2.0f){
                glm::vec3 newVert = (p_l + p_r) / 2.0f;
                m_vertices.push_back(p_l);
                m_derivaties.push_back(d_l);
                m_texCoords.push_back(glm::length(d_l));
                m_advancing_front[2 * ribbon_id] = m_vertices.size() - 1;

                m_vertices.push_back(newVert);
                m_derivaties.push_back(derivate(newVert));
                m_texCoords.push_back(glm::length(m_derivaties.back()));
                m_advancing_front.insert((m_advancing_front.begin() + (2 * ribbon_id + 1)), m_vertices.size() - 1);
                m_advancing_front.insert((m_advancing_front.begin() + (2 * ribbon_id + 1)), m_vertices.size() - 1);
                
                m_vertices.push_back(p_r);
                m_derivaties.push_back(d_r);
                m_texCoords.push_back(glm::length(d_r));
                m_advancing_front[2 * ribbon_id + 3] = m_vertices.size() - 1;

                m_faces.push_back(L0); m_faces.push_back(m_vertices.size() - 2); m_faces.push_back(m_vertices.size() - 3);
                m_faces.push_back(L0); m_faces.push_back(R0);                    m_faces.push_back(m_vertices.size() - 2);
                m_faces.push_back(R0); m_faces.push_back(m_vertices.size() - 1); m_faces.push_back(m_vertices.size() - 2);

                return true;
            }
        }

        float left_diagonal = glm::length(p_l - m_vertices[R0]);
        float right_diagonal = glm::length(p_r - m_vertices[L0]);
        float min_diagonal = glm::min(left_diagonal, right_diagonal);
        bool trace_left = (left_diagonal == min_diagonal);

        if (caught_up && (trace_left || right_diagonal > prev_diagonal)) return false;

        if (trace_left){

            m_vertices.push_back(p_l);
            m_derivaties.push_back(d_l);
            m_texCoords.push_back(glm::length(d_l));
            m_advancing_front[2 * ribbon_id] = m_vertices.size() - 1;

            m_faces.push_back(L0);
            m_faces.push_back(R0);
            m_faces.push_back(m_vertices.size() - 1);

            caught_up = true;
        } else{
            m_vertices.push_back(p_r);
            m_derivaties.push_back(d_r);
            m_texCoords.push_back(glm::length(d_r));
            int newVertIdx = m_vertices.size() - 1;

            m_faces.push_back(L0);
            m_faces.push_back(R0);
            m_faces.push_back(newVertIdx);

            traceRibbon(ribbon_id + 1, addition, remove, ripping);

            m_advancing_front[2 * ribbon_id + 1] = newVertIdx;
        }
        
        prev_diagonal = min_diagonal;
    }

    return false;
}

glm::vec3 StreamTracer::derivate(const glm::vec3& point)
{
    glm::vec3 d;

    size_t i, j, k;
    if (!seedIsValid(point, i, j, k))
        return glm::vec3(0.0f, 0.0f, 0.0f);

    std::vector<Grid::PrimitiveIndex> &primitives = m_sceneAccel.getPrimitives(i, j, k);
    unsigned int primitiveIdx = primitives[0];

//    std::vector<Grid::PrimitiveIndex> &primitives = m_sceneAccel.getPrimitives(i, j, k);
//    size_t t = 0;
//
//#ifdef STREAM_TRACER_USE_CELL_LIST
//    for (; t<primitives.size(); t++)
//        if (m_cellBoxes[primitives[t]].contains((float*)(&s0)))
//            break;
//    if (t == primitives.size())
//        break;
//#endif

    d = m_cellVectors[primitiveIdx];

    return d;
}

void StreamTracer::computeStreamsurfaces(bool addition, bool remove, bool ripping) {
    clock_t streamComputation_start = clock();

    m_vertices.clear();
    m_faces.clear();
    m_derivaties.clear();
    m_texCoords.clear();
    m_advancing_front.clear();

    generateSeedingPoints();
    //m_advancing_front.resize(m_surface_parameters.seedingPoints.size());

    std::vector<int> lastConnectedPoint(m_surface_parameters.seedingPoints.size() * 2, 0);
    for (size_t p = 0; p < m_surface_parameters.seedingPoints.size(); p++){
        m_vertices.push_back(m_surface_parameters.seedingPoints[p]);
        m_derivaties.push_back(derivate(m_surface_parameters.seedingPoints[p]));
        m_texCoords.push_back(glm::length(m_derivaties[p]));
        

        if (p < m_surface_parameters.seedingPoints.size() - 1){
            m_advancing_front.push_back(p);
            m_advancing_front.push_back(p + 1);
        }
        
    }
    
    int nSeedingPoints = 20; //m_advancing_front.size();
    for (size_t i = 0; i < nSeedingPoints * m_surface_parameters.traceMaxSeeds; i++){
        if (traceRibbon(i % (m_advancing_front.size() - 1), addition, remove, ripping))
            ;//i++;
    }

    float computationTime = ((float)(clock() - streamComputation_start) / CLOCKS_PER_SEC) * 1000.0f;
    std::cout << "Computation Time: " << computationTime << std::endl;

    // compute normals
    /*for (size_t i = 0; i < m_streamLines.size(); i++){

    }*/
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

std::vector<glm::vec3> StreamTracer::getVertices(){
    return m_vertices;
}

std::vector<glm::vec3> StreamTracer::getDerivatives(){
    return m_derivaties;
}

std::vector<float> StreamTracer::getTexCoords(){
    return m_texCoords;
}

std::vector<unsigned int> StreamTracer::getFaceIndices(){
    return m_faces;
}

std::vector<glm::vec3> StreamTracer::getSeedingPoints(){
    return m_surface_parameters.seedingPoints;
}

std::vector<glm::vec3> StreamTracer::getAABB(){
    std::vector<glm::vec3> points;

    glm::vec3 min_point(m_sceneBox.min[0], m_sceneBox.min[1], m_sceneBox.min[2]);
    glm::vec3 max_point(m_sceneBox.max[0], m_sceneBox.max[1], m_sceneBox.max[2]);

    // 1
    points.push_back(glm::vec3(min_point.x, min_point.y, min_point.z));
    points.push_back(glm::vec3(max_point.x, min_point.y, min_point.z));

    // 2
    points.push_back(glm::vec3(min_point.x, min_point.y, min_point.z));
    points.push_back(glm::vec3(min_point.x, max_point.y, min_point.z));

    // 3
    points.push_back(glm::vec3(min_point.x, min_point.y, min_point.z));
    points.push_back(glm::vec3(min_point.x, min_point.y, max_point.z));

    // 4
    points.push_back(glm::vec3(max_point.x, max_point.y, max_point.z)); 
    points.push_back(glm::vec3(min_point.x, max_point.y, max_point.z));

    // 5
    points.push_back(glm::vec3(max_point.x, max_point.y, max_point.z));
    points.push_back(glm::vec3(max_point.x, min_point.y, max_point.z));

    // 6
    points.push_back(glm::vec3(max_point.x, max_point.y, max_point.z));
    points.push_back(glm::vec3(max_point.x, max_point.y, min_point.z));


    // 7
    points.push_back(glm::vec3(min_point.x, min_point.y, max_point.z));
    points.push_back(glm::vec3(max_point.x, min_point.y, max_point.z));

    // 8
    points.push_back(glm::vec3(min_point.x, min_point.y, max_point.z));
    points.push_back(glm::vec3(min_point.x, max_point.y, max_point.z));

    // 9
    points.push_back(glm::vec3(min_point.x, max_point.y, min_point.z));
    points.push_back(glm::vec3(max_point.x, max_point.y, min_point.z));

    // 10
    points.push_back(glm::vec3(min_point.x, max_point.y, min_point.z));
    points.push_back(glm::vec3(min_point.x, max_point.y, max_point.z));

    // 11
    points.push_back(glm::vec3(max_point.x, min_point.y, min_point.z));
    points.push_back(glm::vec3(max_point.x, min_point.y, max_point.z));

    // 12
    points.push_back(glm::vec3(max_point.x, min_point.y, min_point.z));
    points.push_back(glm::vec3(max_point.x, max_point.y, min_point.z));

    return points;
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

void StreamTracer::generateSeedingPoints() {
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
            m_surface_parameters.seedingPoints.push_back(seed);
    }
}