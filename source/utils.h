#ifndef UTILS_H
#define UTILS_H

#define PI 3.14159265359

#include <cfloat>
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


	inline bool hitBoundingBox(
		float* bbMin, float* bbMax, float* origin, float* dir,
		float* tNear, float* tFar
	) {
		float invDir[3] = {
			1.0f / dir[0],
			1.0f / dir[1],
			1.0f / dir[2]
		};
		float bounds[2][3] = {
			bbMin[0], bbMin[1], bbMin[2],
			bbMax[0], bbMax[1], bbMax[2]
		};
		float tmin, tmax, tymin, tymax, tzmin, tzmax;
		bool signX = invDir[0] < 0.0f;
		bool signY = invDir[1] < 0.0f;
		bool signZ = invDir[2] < 0.0f;

		// X
		tmin = ( bounds[signX][0] - origin[0] ) * invDir[0];
		tmax = ( bounds[1 - signX][0] - origin[0] ) * invDir[0];
		// Y
		tymin = ( bounds[signY][1] - origin[1] ) * invDir[1];
		tymax = ( bounds[1 - signY][1] - origin[1] ) * invDir[1];

		if( tmin > tymax || tymin > tmax ) {
			return false;
		}

		// X vs. Y
		tmin = ( tymin > tmin ) ? tymin : tmin;
		tmax = ( tymax < tmax ) ? tymax : tmax;
		// Z
		tzmin = ( bounds[signZ][2] - origin[2] ) * invDir[2];
		tzmax = ( bounds[1 - signZ][2] - origin[2] ) * invDir[2];

		if( tmin > tzmax || tzmin > tmax ) {
			return false;
		}

		// Z vs. previous winner
		tmin = ( tzmin > tmin ) ? tzmin : tmin;
		tmax = ( tzmax < tmax ) ? tzmax : tmax;
		// Result
		*tNear = tmin;
		*tFar = tmax;

		return true;
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
