#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <string>

#include "Cfg.h"

#define LOG_INDENT 4

using std::string;


class Logger {

	public:
		static int getIndent();
		static int indent( int indent );
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

	private:
		static int mIndent;
		static char mIndentChar[21];

};


#endif
