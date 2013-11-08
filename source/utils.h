#ifndef UTILS_H
#define UTILS_H

#define PI 3.14159265359
#define BB_NUMDIM 3
#define BB_RIGHT 0
#define BB_LEFT 1
#define BB_MIDDLE 2

#include <cfloat>
#include <fstream>
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
		float* p_near, float* p_far
	) {
		float p_near_result = -FLT_MAX;
		float p_far_result = FLT_MAX;
		float p_near_comp, p_far_comp;

		float invDir[3] = {
			1.0f / dir[0],
			1.0f / dir[1],
			1.0f / dir[2]
		};

		for( int i = 0; i < 3; i++ ) {
			p_near_comp = ( bbMin[i] - origin[i] ) * invDir[i];
			p_far_comp = ( bbMax[i] - origin[i] ) * invDir[i];

			if( p_near_comp > p_far_comp ) {
				float temp = p_near_comp;
				p_near_comp = p_far_comp;
				p_far_comp = temp;
			}

			p_near_result = ( p_near_comp > p_near_result ) ? p_near_comp : p_near_result;
			p_far_result = ( p_far_comp < p_far_result ) ? p_far_comp : p_far_result;

			if( p_near_result > p_far_result ) {
				return false;
			}
		}

		*p_near = p_near_result;
		*p_far = p_far_result;

		return true;
	}


	/**
	 * Fast Ray-Box Intersection
	 * by Andrew Woo
	 * from "Graphics Gems", Academic Press, 1990
	 */
	inline bool hitBoundingBox_Woo( float* minB, float* maxB, float* origin, float* dir, float* coord ) {
		bool inside = true;
		int quadrant[BB_NUMDIM];
		int whichPlane;
		int i;
		float maxT[BB_NUMDIM];
		float candidatePlane[BB_NUMDIM];

		// Find candidate planes; this loop can be avoided if
		// rays cast all from the eye (assume perspective view)
		for( i = 0; i < BB_NUMDIM; i++ ) {
			quadrant[i] = BB_MIDDLE;

			if( origin[i] < minB[i] ) {
				quadrant[i] = BB_LEFT;
				candidatePlane[i] = minB[i];
				inside = false;
			}
			else if( origin[i] > maxB[i] ) {
				quadrant[i] = BB_RIGHT;
				candidatePlane[i] = maxB[i];
				inside = false;
			}
		}

		// Ray origin inside bounding box
		if( inside ) {
			coord = origin;
			return true;
		}


		// Calculate T distances to candidate planes
		for( i = 0; i < BB_NUMDIM; i++ ) {
			maxT[i] = ( quadrant[i] != BB_MIDDLE && dir[i] != 0.0f )
			        ? ( candidatePlane[i] - origin[i] ) / dir[i]
			        : -1.0f;
		}

		// Get largest of the maxT's for final choice of intersection
		whichPlane = 0;
		for( i = 1; i < BB_NUMDIM; i++ ) {
			whichPlane = ( maxT[whichPlane] < maxT[i] ) ? i : whichPlane;
		}

		// Check final candidate actually inside box
		if( maxT[whichPlane] < 0.0f ) {
			return false;
		}

		for( i = 0; i < BB_NUMDIM; i++ ) {
			coord[i] = ( whichPlane != i )
			         ? origin[i] + maxT[whichPlane] * dir[i]
			         : candidatePlane[i];

			if( coord[i] < minB[i] || coord[i] > maxB[i] ) {
				return false;
			}
		}

		return true;
	}


	/**
	 * Read the contents of a file as string.
	 * @param  {const char*} filename Path to and name of the file.
	 * @return {std::string}          File content as string.
	 */
	inline std::string loadFileAsString( const char* filename ) {
		std::ifstream ifile( filename );
		std::string content;

		while( ifile.good() ) {
			std::string line;
			std::getline( ifile, line );
			content.append( line + "\n" );
		}

		return content;
	}

}

#endif
