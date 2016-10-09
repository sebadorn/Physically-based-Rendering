#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <clocale>
#include <stdexcept>

#include "Cfg.h"
#include "Logger.h"
#include "VulkanHandler.h"


/**
 * Main.
 * @param  {int}    argc
 * @param  {char**} argv
 * @return {int}
 */
int main( int argc, char** argv ) {
	setlocale( LC_ALL, "C" );
	Cfg::get().loadConfigFile( "config.json" );

	Logger::logInfo( "[main] Configuration loaded." );

	glfwInit();

	int glfwVersionMajor = 0;
	int glfwVersionMinor = 0;
	int glfwVersionRev = 0;
	glfwGetVersion( &glfwVersionMajor, &glfwVersionMinor, &glfwVersionRev );

	Logger::logInfof(
		"[main] GLFW version: %d.%d.%d",
		glfwVersionMajor, glfwVersionMinor, glfwVersionRev
	);

	if( !glfwVulkanSupported() ) {
		Logger::logError( "[main] GLFW says it doesn't support Vulkan." );
		glfwTerminate();

		return EXIT_FAILURE;
	}

	glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
	glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );

	GLFWwindow* window = glfwCreateWindow(
		Cfg::get().value<int>( Cfg::WINDOW_WIDTH ),
		Cfg::get().value<int>( Cfg::WINDOW_HEIGHT ),
		"PBR-Vulkan", nullptr, nullptr
	);

	Logger::logInfo( "--------------------" );

	VulkanHandler vkHandler;

	try {
		vkHandler.setup( window );
	}
	catch( const std::runtime_error &err ) {
		Logger::logError( "[main] Vulkan setup failed. EXIT_FAILURE." );

		try {
			vkHandler.teardown();
		}
		catch( const std::runtime_error &err2 ) {
			Logger::logError( "[main] Vulkan teardown failed too." );
		}

		return EXIT_FAILURE;
	}

	Logger::logInfo( "--------------------" );
	Logger::logInfo( "[main] Starting main loop." );

	while( !glfwWindowShouldClose( window ) ) {
		glfwPollEvents();
	}

	Logger::logInfo( "[main] Main loop stopped." );
	Logger::logInfo( "--------------------" );

	glfwDestroyWindow( window );
	glfwTerminate();

	try {
		vkHandler.teardown();
	}
	catch( const std::runtime_error &err ) {
		Logger::logError( "[main] Vulkan teardown failed. EXIT_FAILURE." );
		return EXIT_FAILURE;
	}

	Logger::logInfo( "--------------------" );
	Logger::logInfo( "[main] EXIT_SUCCESS." );

	return EXIT_SUCCESS;
}
