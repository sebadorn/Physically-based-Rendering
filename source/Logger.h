#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <string>

#include "Cfg.h"


class Logger {

	public:
		static void logDebug( const char* msg, const char* prefix = "* " );
		static void logDebug( std::string msg, const char* prefix = "* " );
		static void logError( const char* msg, const char* prefix = "* " );
		static void logError( std::string msg, const char* prefix = "* " );
		static void logInfo( const char* msg, const char* prefix = "* " );
		static void logInfo( std::string msg, const char* prefix = "* " );
		static void logWarning( const char* msg, const char* prefix = "* " );
		static void logWarning( std::string msg, const char* prefix = "* " );

};

#endif
