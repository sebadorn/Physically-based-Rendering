#ifndef UTILS_H
#define UTILS_H

#define PI 3.14159265359


namespace utils {

	/**
	 * Convert an angle from degree to radians.
	 * @param  deg {float} Angle in degree.
	 * @return     {float} Angle in radians.
	 */
	inline float degToRad( float deg ) {
		return ( deg * PI / 180.0f );
	}

}

#endif
