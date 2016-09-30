#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <clocale>
#include <stdexcept>

#include "Cfg.h"
#include "Logger.h"
#include "VulkanHandler.h"


int main( int argc, char** argv ) {
	setlocale( LC_ALL, "C" );
	Cfg::get().loadConfigFile( "config.json" );

	VulkanHandler vkHandler;

	try {
		vkHandler.setup();
	}
	catch( const std::runtime_error &err ) {
		Logger::logError( "[main] Vulkan setup failed. EXIT_FAILURE." );
		return EXIT_FAILURE;
	}

	glfwInit();
	glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
	glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );

	GLFWwindow* window = glfwCreateWindow( 800, 600, "PBR-Vulkan", nullptr, nullptr );

	while( !glfwWindowShouldClose( window ) ) {
		glfwPollEvents();
	}

	glfwDestroyWindow( window );

	return EXIT_SUCCESS;
}
