cmake_minimum_required( VERSION 2.8 )
project( PBR )
set( TRUNK ${PROJECT_SOURCE_DIR}/source )
set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake_modules/" )

file( GLOB SOURCES_CPP ${TRUNK}/*.cpp ${TRUNK}/qt/*.cpp ${TRUNK}/tinyobjloader/*.cpp )
file( GLOB HEADERS_QT ${TRUNK}/qt/*.h )


# OpenGL
find_package( OpenGL REQUIRED )
include_directories( ${OPENGL_INCLUDE_DIRS} )
set( LIBRARIES ${LIBRARIES} ${OPENGL_LIBRARIES} )

find_package( GLUT REQUIRED )
include_directories( ${GLUT_INCLUDE_DIRS} )
set( LIBRARIES ${LIBRARIES} ${GLUT_LIBRARIES} )

set( GLEW_FIND_QUIETLY 1 )
find_package( GLEW REQUIRED )
include_directories( ${GLEW_INCLUDE_DIRS} )
set( LIBRARIES ${LIBRARIES} ${GLEW_LIBRARIES} )


# OpenCL
find_package( OpenCL REQUIRED )
include_directories( ${OPENCL_INCLUDE_DIRS} )
set( LIBRARIES ${LIBRARIES} ${OPENCL_LIBRARIES} )


# Qt4
find_package( Qt4 REQUIRED )
include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )
include_directories( ${QT_INCLUDE_DIR} )
include_directories( ${QT_QTOPENGL_INCLUDE_DIR} )
qt4_wrap_cpp( SOURCES_MOC ${HEADERS_QT} )
set( LIBRARIES ${LIBRARIES} ${QT_LIBRARIES} ${QT_QTOPENGL_LIBRARIES} )


# ASSIMP
find_package( ASSIMP REQUIRED )
include_directories( ${ASSIMP_INCLUDE_DIRS} )
set( LIBRARIES ${LIBRARIES} ${ASSIMP_LIBRARIES} )


add_executable( ${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/main.cpp ${SOURCES_CPP} ${SOURCES_MOC} )
target_link_libraries( ${PROJECT_NAME} ${LIBRARIES} )
