#include "Logger.h"


/**
 * Log messages of level "debug".
 * @param {const char*} msg    Message to log.
 * @param {const char*} prefix Prefix for the line.
 */
void Logger::logDebug( const char* msg, const char* prefix ) {
	if( !Cfg::get().value<bool>( Cfg::LOGGING ) ) { return; }
	std::cout << "\033[36m" << prefix << msg << "\033[0m" << std::endl;
}


/**
 * Log messages of level "debug".
 * @param {std::string} msg    Message to log.
 * @param {const char*} prefix Prefix for the line.
 */
void Logger::logDebug( std::string msg, const char* prefix ) {
	Logger::logDebug( msg.c_str(), prefix );
}


/**
 * Log messages of level "error".
 * @param {const char*} msg    Message to log.
 * @param {const char*} prefix Prefix for the line.
 */
void Logger::logError( const char* msg, const char* prefix ) {
	if( !Cfg::get().value<bool>( Cfg::LOGGING ) ) { return; }
	std::cerr << "\033[31;1m" << prefix << msg << "\033[0m" << std::endl;
}


/**
 * Log messages of level "error".
 * @param {std::string} msg    Message to log.
 * @param {const char*} prefix Prefix for the line.
 */
void Logger::logError( std::string msg, const char* prefix ) {
	Logger::logError( msg.c_str(), prefix );
}


/**
 * Log messages of level "info".
 * @param {const char*} msg    Message to log.
 * @param {const char*} prefix Prefix for the line.
 */
void Logger::logInfo( const char* msg, const char* prefix ) {
	if( !Cfg::get().value<bool>( Cfg::LOGGING ) ) { return; }
	std::cout << prefix << msg << std::endl;
}


/**
 * Log messages of level "info".
 * @param {std::string} msg    Message to log.
 * @param {const char*} prefix Prefix for the line.
 */
void Logger::logInfo( std::string msg, const char* prefix ) {
	Logger::logInfo( msg.c_str(), prefix );
}


/**
 * Log messages of level "warnning".
 * @param {const char*} msg    Message to log.
 * @param {const char*} prefix Prefix for the line.
 */
void Logger::logWarning( const char* msg, const char* prefix ) {
	if( !Cfg::get().value<bool>( Cfg::LOGGING ) ) { return; }
	std::cout << "\033[33;1m" << prefix << msg << "\033[0m" << std::endl;
}


/**
 * Log messages of level "warning".
 * @param {std::string} msg    Message to log.
 * @param {const char*} prefix Prefix for the line.
 */
void Logger::logWarning( std::string msg, const char* prefix ) {
	Logger::logWarning( msg.c_str(), prefix );
}
