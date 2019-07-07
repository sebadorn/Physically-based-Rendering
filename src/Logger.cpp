#include "Logger.h"

using std::string;


int Logger::mIndent = 0;
char Logger::mIndentChar[21];
bool Logger::mIsMute = false;
int Logger::mLogLevel = 4;


/**
 * Build the log message from a format string and values.
 * @param  {const char*} format
 * @param  {va_list}     args
 * @return {const std::string}
 */
const string Logger::buildLogMessage( const char* format, va_list args ) {
	const size_t size = std::strlen( format ) + 1024;
	char msg[size];
	vsnprintf( msg, size, format, args );

	return string( msg );
}


/**
 * Get the currently set indentation level.
 * @return {int}
 */
int Logger::getIndent() {
	return mIndent;
}


/**
 * Set the new indent value for log entries.
 * @param  {int} indent New value for indentation.
 * @return {int}        The new indentation length.
 */
int Logger::indent( int indent ) {
	mIndent = ( indent < 0 ) ? 0 : indent;
	snprintf( mIndentChar, 21, "%d", mIndent );

	return mIndent;
}


/**
 * Set the new indent value relative to the current value.
 * @param  {int} diff Relative indentation change.
 * @return {int}      The new absolute indentation length.
 */
int Logger::indentChange( int diff ) {
	return Logger::indent( mIndent + diff );
}


/**
 * Central log method.
 * @param {const char*} format
 * @param {const char*} prefix
 * @param {const char*} msg
 */
void Logger::log( const char* format, const char* prefix, const char* msg ) {
	if( mIsMute ) {
		return;
	}

	printf( format, "", prefix, msg );
}


/**
 * Log messages of level "debug".
 * @param {const char*} msg    Message to log.
 * @param {const char*} prefix Prefix for the line.
 */
void Logger::logDebug( const char* msg, const char* prefix ) {
	if( mLogLevel < 3 || mIsMute ) {
		return;
	}

	string p = string( "\033[32m%" ).append( mIndentChar ).append( "s%s%s\033[0m\n" );
	Logger::log( p.c_str(), prefix, msg );
}


/**
 * Log messages of level "debug".
 * @param {std::string} msg    Message to log.
 * @param {const char*} prefix Prefix for the line.
 */
void Logger::logDebug( string msg, const char* prefix ) {
	Logger::logDebug( msg.c_str(), prefix );
}


/**
 * Log message of level "debug".
 * @param {const char*} format
 */
void Logger::logDebugf( const char* format, ... ) {
	va_list args;
	va_start( args, format );
	Logger::logDebug( Logger::buildLogMessage( format, args ) );
	va_end( args );
}


/**
 * Log messages of level "debug and extra verbose".
 * @param {const char*} msg    Message to log.
 * @param {const char*} prefix Prefix for the line.
 */
void Logger::logDebugVerbose( const char* msg, const char* prefix ) {
	if( mLogLevel < 4 || mIsMute ) {
		return;
	}

	string p = string( "\033[36m%" ).append( mIndentChar ).append( "s%s%s\033[0m\n" );
	Logger::log( p.c_str(), prefix, msg );
}


/**
 * Log messages of level "debug and extra verbose".
 * @param {std::string} msg    Message to log.
 * @param {const char*} prefix Prefix for the line.
 */
void Logger::logDebugVerbose( string msg, const char* prefix ) {
	Logger::logDebugVerbose( msg.c_str(), prefix );
}


/**
 * Log message of level "debug verbose".
 * @param {const char*} format
 */
void Logger::logDebugVerbosef( const char* format, ... ) {
	va_list args;
	va_start( args, format );
	Logger::logDebugVerbose( Logger::buildLogMessage( format, args ) );
	va_end( args );
}


/**
 * Log messages of level "error".
 * @param {const char*} msg    Message to log.
 * @param {const char*} prefix Prefix for the line.
 */
void Logger::logError( const char* msg, const char* prefix ) {
	if( mLogLevel < 1 || mIsMute ) {
		return;
	}

	string p = string( "\033[31;1m%" ).append( mIndentChar ).append( "s%s%s\033[0m\n" );
	Logger::log( p.c_str(), prefix, msg );
}


/**
 * Log messages of level "error".
 * @param {std::string} msg    Message to log.
 * @param {const char*} prefix Prefix for the line.
 */
void Logger::logError( string msg, const char* prefix ) {
	Logger::logError( msg.c_str(), prefix );
}


/**
 * Log message of level "error".
 * @param {const char*} format
 */
void Logger::logErrorf( const char* format, ... ) {
	va_list args;
	va_start( args, format );
	Logger::logError( Logger::buildLogMessage( format, args ) );
	va_end( args );
}


/**
 * Log messages of level "info".
 * @param {const char*} msg    Message to log.
 * @param {const char*} prefix Prefix for the line.
 */
void Logger::logInfo( const char* msg, const char* prefix ) {
	if( mLogLevel < 2 || mIsMute ) {
		return;
	}

	string p = string( "%" ).append( mIndentChar ).append( "s%s%s\n" );
	Logger::log( p.c_str(), prefix, msg );
}


/**
 * Log messages of level "info".
 * @param {std::string} msg    Message to log.
 * @param {const char*} prefix Prefix for the line.
 */
void Logger::logInfo( string msg, const char* prefix ) {
	Logger::logInfo( msg.c_str(), prefix );
}


/**
 * Log message of level "info".
 * @param {const char*} format
 */
void Logger::logInfof( const char* format, ... ) {
	va_list args;
	va_start( args, format );
	Logger::logInfo( Logger::buildLogMessage( format, args ) );
	va_end( args );
}


/**
 * Log messages of level "warnning".
 * @param {const char*} msg    Message to log.
 * @param {const char*} prefix Prefix for the line.
 */
void Logger::logWarning( const char* msg, const char* prefix ) {
	if( mLogLevel < 1 || mIsMute ) {
		return;
	}

	string p = string( "\033[33;1m%" ).append( mIndentChar ).append( "s%s%s\033[0m\n" );
	Logger::log( p.c_str(), prefix, msg );
}


/**
 * Log messages of level "warning".
 * @param {std::string} msg    Message to log.
 * @param {const char*} prefix Prefix for the line.
 */
void Logger::logWarning( string msg, const char* prefix ) {
	Logger::logWarning( msg.c_str(), prefix );
}


/**
 * Log message of level "warning".
 * @param {const char*} format
 */
void Logger::logWarningf( const char* format, ... ) {
	va_list args;
	va_start( args, format );
	Logger::logWarning( Logger::buildLogMessage( format, args ) );
	va_end( args );
}


/**
 * Mute.
 */
void Logger::mute() {
	mIsMute = true;
}


/**
 * Set the log level.
 * @param {int} level New log level.
 */
void Logger::setLogLevel( int level ) {
	mLogLevel = level;
}


/**
 * Unmute.
 */
void Logger::unmute() {
	mIsMute = false;
}
