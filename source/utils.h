#ifndef UTILS_H
#define UTILS_H

#define PI 3.14159265359

#include <fstream>
#include <math.h>
#include <string>

using std::string;
using std::vector;


namespace utils {

	/**
	 * Compute the bounding box of the object.
	 * @param  {std::vector<float>} vertices Vertices of the object.
	 * @return {std::vector<float>}          Vector with the minimum and maximum of the bounding box.
	 */
	inline vector<float> computeBoundingBox( vector<float> vertices ) {
		// Index 0 - 2: bbMin
		// Index 3 - 5: bbMax
		vector<float> bb = vector<float>( 6 );

		for( int i = 0; i < vertices.size(); i += 3 ) {
			if( i == 0 ) {
				bb[0] = bb[3] = vertices[i];
				bb[1] = bb[4] = vertices[i + 1];
				bb[2] = bb[5] = vertices[i + 2];
				continue;
			}

			bb[0] = ( vertices[i] < bb[0] ) ? vertices[i] : bb[0];
			bb[1] = ( vertices[i + 1] < bb[1] ) ? vertices[i + 1] : bb[1];
			bb[2] = ( vertices[i + 2] < bb[2] ) ? vertices[i + 2] : bb[2];

			bb[3] = ( vertices[i] > bb[3] ) ? vertices[i] : bb[3];
			bb[4] = ( vertices[i + 1] > bb[4] ) ? vertices[i + 1] : bb[4];
			bb[5] = ( vertices[i + 2] > bb[5] ) ? vertices[i + 2] : bb[5];
		}

		return bb;
	}


	/**
	 * Convert an angle from degree to radians.
	 * @param  {float} deg Angle in degree.
	 * @return {float}     Angle in radians.
	 */
	inline float degToRad( float deg ) {
		return ( deg * PI / 180.0f );
	}


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
