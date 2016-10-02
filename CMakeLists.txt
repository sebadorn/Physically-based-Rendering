cmake_minimum_required( VERSION 3.1 )
project( PBR )
set( TRUNK ${PROJECT_SOURCE_DIR}/source )
set( CMAKE_CXX_STANDARD 11 )
set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake_modules/" )
set( CMAKE_AUTOMOC ON )
set( CMAKE_INCLUDE_CURRENT_DIR ON )


# # OpenGL
# find_package( OpenGL REQUIRED )
# include_directories( ${OPENGL_INCLUDE_DIRS} )
# set( LIBRARIES ${LIBRARIES} ${OPENGL_LIBRARIES} )

# find_package( GLUT REQUIRED )
# include_directories( ${GLUT_INCLUDE_DIRS} )
# set( LIBRARIES ${LIBRARIES} ${GLUT_LIBRARIES} )

# set( GLEW_FIND_QUIETLY 1 )
# find_package( GLEW REQUIRED )
# include_directories( ${GLEW_INCLUDE_DIRS} )
# set( LIBRARIES ${LIBRARIES} ${GLEW_LIBRARIES} )


# GLM
find_package( GLM REQUIRED )
include_directories( ${GLM_INCLUDE_DIRS} )


# # OpenCL
# find_package( OpenCL REQUIRED )
# include_directories( ${OPENCL_INCLUDE_DIRS} )
# set( LIBRARIES ${LIBRARIES} ${OPENCL_LIBRARIES} )

# # Ignore deprecated OpenCL 1.1 headers warning
# add_definitions( -DCL_USE_DEPRECATED_OPENCL_1_1_APIS )


# dear imgui
set( IMGUI_DIR ../imgui/ )
include_directories( ${IMGUI_DIR} )


# Vulkan (LunarG SDK)
set( VULKAN_SDK "/home/seba/programming/VulkanSDK/1.0.26.0/x86_64" )
include_directories( ${VULKAN_SDK}/include )
set( LIBRARIES ${LIBRARIES} ${VULKAN_SDK}/lib/libvulkan.so.1 )


# GLFW (required version >= 3.2)
set( GLFW_DIR ../glfw )
option( GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF )
option( GLFW_BUILD_TESTS "Build the GLFW test programs" OFF )
option( GLFW_BUILD_DOCS "Build the GLFW documentation" OFF )
add_subdirectory( ${GLFW_DIR} glfw_binary_dir )
include_directories( ${GLFW_DIR}/include )
set( LIBRARIES ${LIBRARIES} "glfw" )


# # DEVIL
# find_package( DEVIL REQUIRED )
# include_directories( ${IL_INCLUDE_DIR} )
# set( LIBRARIES ${LIBRARIES} ${IL_LIBRARIES} )


file( GLOB SOURCES_CPP ${TRUNK}/*.cpp ${TRUNK}/accelstructures/*.cpp )
add_executable(
	${PROJECT_NAME} ${TRUNK}/main.cpp
	${SOURCES_CPP}
	${IMGUI_DIR}/imgui.cpp ${IMGUI_DIR}/imgui_draw.cpp ${IMGUI_DIR}/imgui_demo.cpp
)

target_link_libraries( ${PROJECT_NAME} ${LIBRARIES} )
