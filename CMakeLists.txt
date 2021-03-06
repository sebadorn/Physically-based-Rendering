cmake_minimum_required( VERSION 3.1 )
project( PBR )
set( TRUNK ${PROJECT_SOURCE_DIR}/source )
set( CMAKE_CXX_STANDARD 11 )
set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake_modules/" )
set( CMAKE_AUTOMOC ON )
set( CMAKE_INCLUDE_CURRENT_DIR ON )

file( GLOB SOURCES_CPP ${TRUNK}/*.cpp ${TRUNK}/qt/*.cpp ${TRUNK}/accelstructures/*.cpp )
add_executable( ${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/main.cpp ${SOURCES_CPP} ${SOURCES_MOC} )


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

find_package( GLM REQUIRED )
include_directories( ${GLM_INCLUDE_DIRS} )


# OpenCL
find_package( OpenCL REQUIRED )
include_directories( ${OPENCL_INCLUDE_DIRS} )
set( LIBRARIES ${LIBRARIES} ${OPENCL_LIBRARIES} )

# Ignore deprecated OpenCL 1.1 headers warning
add_definitions( -DCL_USE_DEPRECATED_OPENCL_1_1_APIS )


# Qt5
find_package( Qt5Widgets REQUIRED )
qt5_use_modules( ${PROJECT_NAME} Widgets OpenGL )


# DEVIL
find_package( DEVIL REQUIRED )
include_directories( ${IL_INCLUDE_DIR} )
set( LIBRARIES ${LIBRARIES} ${IL_LIBRARIES} )


target_link_libraries( ${PROJECT_NAME} ${LIBRARIES} )
