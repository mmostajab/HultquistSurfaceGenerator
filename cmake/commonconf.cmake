IF(NOT CommonconfProcessed)

SET(StreamSurfaceGeneratorHome ${CMAKE_CURRENT_SOURCE_DIR})
SET(StreamSurfaceGeneratorBinaryDir ${CMAKE_BINARY_DIR})

MESSAGE(STATUS "StreamSurfaceGenerator Source Directory: ${StreamSurfaceGeneratorHome}")
MESSAGE(STATUS "StreamSurfaceGenerator Binary Directory: ${StreamSurfaceGeneratorBinaryDir}")

# include macros
INCLUDE(${StreamSurfaceGeneratorHome}/cmake/macros.cmake)

# detect compiler and architecture
IF(${CMAKE_GENERATOR} STREQUAL "Visual Studio 12 2013")
    SET(StreamSurfaceGenerator_MSVC2013 TRUE)
    SET(StreamSurfaceGenerator_WIN32 TRUE)
    MESSAGE(STATUS "Visual Studio 2013 Build (32 Bit)")
ELSE()
    MESSAGE(WARNING "Unsupported or unknown generator: ${CMAKE_GENERATOR}")
ENDIF()

SET(Boost_USE_STATIC_LIBS   ON)

# set binary output path
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# common include directories
LIST(APPEND StreamSurfaceGeneratorIncludeDirs ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}) 

LIST(APPEND StreamSurfaceGeneratorGlobalDefinitions "-DMOBIRFIDVIS_SOURCE_DIR=\"${StreamSurfaceGeneratorHome}\"")

# platform-dependent configuration
IF(WIN32)
    LIST(APPEND StreamSurfaceGeneratorGlobalDefinitions "-DNOMINMAX" "-D_CRT_SECURE_NO_DEPRECATE")

    # Disable warnings for Microsoft compiler:
    # C4290: C++ exception specification ignored except to indicate a function is
    #        not __declspec(nothrow)
    # C4390: ';' : empty controlled statement found; is this the intent?
    #        occurs when OpenGL error logging macros are disabled
    # C4503: The decorated name was longer than the compiler limit (4096), and was truncated.
    #        Occurs in AutoEvaluatePipeline due to some nested nested map-iterator-map. Could
    #        not be deactivated locally...
    LIST(APPEND StreamSurfaceGeneratorGlobalDefinitions /wd4290 /wd4390 /wd4503)

    # prevent error: number of sections exceeded object file format limit
    LIST(APPEND StreamSurfaceGeneratorGlobalDefinitions /bigobj)
    
    # allows 32 Bit builds to use more than 2GB RAM (VC++ only)
    SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /LARGEADDRESSAWARE")
    SET(CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS} /LARGEADDRESSAWARE")
    
    LIST(APPEND StreamSurfaceGenerator_GlobalExternalLibs netapi32 version)
ENDIF(WIN32)

ADD_DEFINITIONS(/WX)

IF(CMAKE_COMPILER_IS_GNUCXX)
    # enable C++11 support in GCC
    LIST(APPEND CMAKE_CXX_FLAGS "-std=c++11")
ENDIF()

IF(STREAM_SURFACE_GENERATOR)

    # OPENGL 
    FIND_PACKAGE(AntTweakBar REQUIRED)
    FIND_PACKAGE(OpenGL REQUIRED)
    FIND_PACKAGE(GLFW3 REQUIRED)
    FIND_PACKAGE(GLEW REQUIRED)
    FIND_PACKAGE(GLM REQUIRED)

    INCLUDE_DIRECTORIES(${ANT_TWEAK_BAR_INCLUDE_PATH})	
    INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIRS})
    INCLUDE_DIRECTORIES(${GLFW3_INCLUDE_DIR})
    INCLUDE_DIRECTORIES(${GLEW_INCLUDE_DIRS})
    INCLUDE_DIRECTORIES(${GLM_INCLUDE_DIR})
    
    # VTK
    FIND_PACKAGE(VTK REQUIRED NO_MODULE)
    INCLUDE(${VTK_USE_FILE})
    
    # Boost
    FIND_PACKAGE(Boost REQUIRED COMPONENTS filesystem system)
    INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})

ENDIF(STREAM_SURFACE_GENERATOR)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${StreamSurfaceGeneratorHome}/cmake")

SET(CommonconfProcessed TRUE)

ENDIF(NOT CommonconfProcessed)