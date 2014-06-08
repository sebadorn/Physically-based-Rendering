#include "Logger.h"

using std::cerr;
using std::cout;
using std::endl;
using std::string;


int Logger::mIndent = 0;
char Logger::mIndentChar[21];


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
 * Log messages of level "debug".
 * @param {const char*} msg    Message to log.
 * @param {const char*} prefix Prefix for the line.
 */
void Logger::logDebug( const char* msg, const char* prefix ) {
	if( Cfg::get().value<int>( Cfg::LOG_LEVEL ) < 3 ) { return; }
	string p = string( "\033[36m%" ).append( mIndentChar ).append( "s%s%s\033[0m\n" );
	printf( p.c_str(), "", prefix, msg );
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
 * Log messages of level "debug and extra verbose".
 * @param {const char*} msg    Message to log.
 * @param {const char*} prefix Prefix for the line.
 */
void Logger::logDebugVerbose( const char* msg, const char* prefix ) {
	if( Cfg::get().value<int>( Cfg::LOG_LEVEL ) < 4 ) { return; }
	string p = string( "\033[36m%" ).append( mIndentChar ).append( "s%s%s\033[0m\n" );
	printf( p.c_str(), "", prefix, msg );
}


/**
 * Log messages of level "debug and extra verbose".
 * @param {std::string} msg    Message to log.
 * @param {const char*} prefix Prefix for the line.
 */
void Logger::logDebugVerbose( string msg, const char* prefix ) {
	Logger::logDebug( msg.c_str(), prefix );
}


/**
 * Log messages of level "error".
 * @param {const char*} msg    Message to log.
 * @param {const char*} prefix Prefix for the line.
 */
void Logger::logError( const char* msg, const char* prefix ) {
	if( Cfg::get().value<int>( Cfg::LOG_LEVEL ) < 1 ) { return; }
	string p = string( "\033[31;1m%" ).append( mIndentChar ).append( "s%s%s\033[0m\n" );
	printf( p.c_str(), "", prefix, msg );
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
 * Log messages of level "info".
 * @param {const char*} msg    Message to log.
 * @param {const char*} prefix Prefix for the line.
 */
void Logger::logInfo( const char* msg, const char* prefix ) {
	if( Cfg::get().value<int>( Cfg::LOG_LEVEL ) < 2 ) { return; }
	string p = string( "%" ).append( mIndentChar ).append( "s%s%s\n" );
	printf( p.c_str(), "", prefix, msg );
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
 * Log messages of level "warnning".
 * @param {const char*} msg    Message to log.
 * @param {const char*} prefix Prefix for the line.
 */
void Logger::logWarning( const char* msg, const char* prefix ) {
	if( Cfg::get().value<int>( Cfg::LOG_LEVEL ) < 1 ) { return; }
	string p = string( "\033[33;1m%" ).append( mIndentChar ).append( "s%s%s\033[0m\n" );
	printf( p.c_str(), "", prefix, msg );
}


/**
 * Log messages of level "warning".
 * @param {std::string} msg    Message to log.
 * @param {const char*} prefix Prefix for the line.
 */
void Logger::logWarning( string msg, const char* prefix ) {
	Logger::logWarning( msg.c_str(), prefix );
}
