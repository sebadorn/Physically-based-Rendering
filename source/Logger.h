#ifndef LOGGER_H
#define LOGGER_H

#include <stdarg.h>
#include <string>

#include "Cfg.h"

#define LOG_INDENT 4

using std::string;


class Logger {

	public:
		static int getIndent();
		static int indent( int indent );
		static int indentChange( int diff );
		static void logDebug( const char* msg, const char* prefix = "* " );
		static void logDebug( string msg, const char* prefix = "* " );
		static void logDebugf( const char* format, ... );
		static void logDebugVerbose( const char* msg, const char* prefix = "* " );
		static void logDebugVerbose( string msg, const char* prefix = "* " );
		static void logDebugVerbosef( const char* format, ... );
		static void logError( const char* msg, const char* prefix = "* " );
		static void logError( string msg, const char* prefix = "* " );
		static void logErrorf( const char* format, ... );
		static void logInfo( const char* msg, const char* prefix = "* " );
		static void logInfo( string msg, const char* prefix = "* " );
		static void logInfof( const char* format, ... );
		static void logWarning( const char* msg, const char* prefix = "* " );
		static void logWarning( string msg, const char* prefix = "* " );
		static void logWarningf( const char* format, ... );

	protected:
		static const string buildLogMessage( const char* format, va_list args );

	private:
		static int mIndent;
		static char mIndentChar[21];

};


#endif
