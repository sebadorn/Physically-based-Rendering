#include "Logger.h"

using std::cerr;
using std::cout;
using std::endl;
using std::string;


/**
 * Log messages of level "debug".
 * @param {const char*} msg    Message to log.
 * @param {const char*} prefix Prefix for the line.
 */
void Logger::logDebug( const char* msg, const char* prefix ) {
	if( Cfg::get().value<int>( Cfg::LOGGING ) < 3 ) { return; }
	cout << "\033[36m" << prefix << msg << "\033[0m" << endl;
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
	if( Cfg::get().value<int>( Cfg::LOGGING ) < 4 ) { return; }
	cout << "\033[36m" << prefix << msg << "\033[0m" << endl;
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
	if( Cfg::get().value<int>( Cfg::LOGGING ) < 1 ) { return; }
	cerr << "\033[31;1m" << prefix << msg << "\033[0m" << endl;
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
	if( Cfg::get().value<int>( Cfg::LOGGING ) < 2 ) { return; }
	cout << prefix << msg << endl;
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
	if( Cfg::get().value<int>( Cfg::LOGGING ) < 1 ) { return; }
	cout << "\033[33;1m" << prefix << msg << "\033[0m" << endl;
}


/**
 * Log messages of level "warning".
 * @param {std::string} msg    Message to log.
 * @param {const char*} prefix Prefix for the line.
 */
void Logger::logWarning( string msg, const char* prefix ) {
	Logger::logWarning( msg.c_str(), prefix );
}
