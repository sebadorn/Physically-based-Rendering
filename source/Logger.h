#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <string>

// Enable (1) or disable (0) logging.
#define DO_LOG 1


class Logger {

	public:
		static void logDebug( const char* msg );
		static void logDebug( std::string msg );
		static void logError( const char* msg );
		static void logError( std::string msg );
		static void logInfo( const char* msg );
		static void logInfo( std::string msg );
		static void logWarning( const char* msg );
		static void logWarning( std::string msg );

};

#endif
