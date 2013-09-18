#include "Logger.h"


/**
 * Log messages of level "debug".
 * @param {const char*} msg Message to log.
 */
void Logger::logDebug( const char* msg ) {
	if( !DO_LOG ) { return; }
	std::cout << "* " << msg << std::endl;
}


/**
 * Log messages of level "debug".
 * @param {std::string} msg Message to log.
 */
void Logger::logDebug( std::string msg ) {
	Logger::logDebug( msg.c_str() );
}


/**
 * Log messages of level "error".
 * @param {const char*} msg Message to log.
 */
void Logger::logError( const char* msg ) {
	if( !DO_LOG ) { return; }
	std::cerr << "\033[31;1m" << "* " << msg << "\033[0m" << std::endl;
}


/**
 * Log messages of level "error".
 * @param {std::string} msg Message to log.
 */
void Logger::logError( std::string msg ) {
	Logger::logError( msg.c_str() );
}


/**
 * Log messages of level "info".
 * @param {const char*} msg Message to log.
 */
void Logger::logInfo( const char* msg ) {
	if( !DO_LOG ) { return; }
	std::cout << "* " << msg << std::endl;
}


/**
 * Log messages of level "info".
 * @param {std::string} msg Message to log.
 */
void Logger::logInfo( std::string msg ) {
	Logger::logInfo( msg.c_str() );
}


/**
 * Log messages of level "warnning".
 * @param {const char*} msg Message to log.
 */
void Logger::logWarning( const char* msg ) {
	if( !DO_LOG ) { return; }
	std::cout << "\033[33;1m" << "* " << msg << "\033[0m" << std::endl;
}


/**
 * Log messages of level "warning".
 * @param {std::string} msg Message to log.
 */
void Logger::logWarning( std::string msg ) {
	Logger::logWarning( msg.c_str() );
}
