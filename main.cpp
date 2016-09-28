#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <clocale>

// #include "source/Cfg.h"


int main( int argc, char** argv ) {
	setlocale( LC_ALL, "C" );
	// Cfg::get().loadConfigFile( "config.json" );

	glfwInit();
	glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
	glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );

	GLFWwindow* window = glfwCreateWindow( 800, 600, "PBR-Vulkan", nullptr, nullptr );

	while( !glfwWindowShouldClose( window ) ) {
		glfwPollEvents();
	}
}
