#ifndef UTILS_H
#define UTILS_H

#define PI 3.14159265359

#include <fstream>
#include <math.h>
#include <string>


namespace utils {

	/**
	 * Convert an angle from degree to radians.
	 * @param  {float} deg Angle in degree.
	 * @return {float}     Angle in radians.
	 */
	inline float degToRad( float deg ) {
		return ( deg * PI / 180.0f );
	}


	/**
	 * Read the contents of a file as string.
	 * @param  {const char*} filename Path to and name of the file.
	 * @return {std::string}          File content as string.
	 */
	inline std::string loadFileAsString( const char* filename ) {
		std::ifstream fileIn( filename );
		std::string content;

		while( fileIn.good() ) {
			std::string line;
			std::getline( fileIn, line );
			content.append( line + "\n" );
		}

		return content;
	}

}

#endif
