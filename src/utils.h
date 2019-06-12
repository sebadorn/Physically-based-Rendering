#ifndef UTILS_H
#define UTILS_H

#include <fstream>
#include <string>

using std::string;


namespace utils {


	/**
	 * Format a value of bytes into more readable units.
	 * @param {size_t}  bytes
	 * @param {float*}  bytesFloat
	 * @param {string*} unit
	 */
	inline void formatBytes( size_t bytes, float* bytesFloat, string* unit ) {
		*unit = string( "bytes" );
		*bytesFloat = (float) bytes;

		if( *bytesFloat >= 1024.0f ) {
			*bytesFloat /= 1024.0f;
			*unit = string( "KB" );
		}
		if( *bytesFloat >= 1024.0f ) {
			*bytesFloat /= 1024.0f;
			*unit = string( "MB" );
		}
		if( *bytesFloat >= 1024.0f ) {
			*bytesFloat /= 1024.0f;
			*unit = string( "GB" );
		}
	}


	/**
	 * Read the contents of a file as string.
	 * @param  {const char*} filename Path to and name of the file.
	 * @return {std::string}          File content as string.
	 */
	inline string loadFileAsString( const char* filename ) {
		std::ifstream fileIn( filename );
		string content;

		while( fileIn.good() ) {
			string line;
			std::getline( fileIn, line );
			content.append( line + "\n" );
		}
		fileIn.close();

		return content;
	}

}

#endif
