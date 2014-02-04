#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <string>

#include "Cfg.h"

using std::string;


class Logger {

	public:
		static void logDebug( const char* msg, const char* prefix = "* " );
		static void logDebug( string msg, const char* prefix = "* " );
		static void logDebugVerbose( const char* msg, const char* prefix = "* " );
		static void logDebugVerbose( string msg, const char* prefix = "* " );
		static void logError( const char* msg, const char* prefix = "* " );
		static void logError( string msg, const char* prefix = "* " );
		static void logInfo( const char* msg, const char* prefix = "* " );
		static void logInfo( string msg, const char* prefix = "* " );
		static void logWarning( const char* msg, const char* prefix = "* " );
		static void logWarning( string msg, const char* prefix = "* " );

};

#endif
