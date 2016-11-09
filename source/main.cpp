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

	Logger::setLogLevel( Cfg::get().value<int>( Cfg::LOG_LEVEL ) );
	Logger::logInfo( "[main] Configuration loaded." );

	VulkanHandler vkHandler;

	try {
		vkHandler.setup();
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

	vkHandler.mainLoop();

	Logger::logInfo( "[main] Main loop stopped." );
	Logger::logInfo( "--------------------" );

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
