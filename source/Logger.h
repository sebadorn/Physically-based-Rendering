#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <string>

#include "Cfg.h"


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
