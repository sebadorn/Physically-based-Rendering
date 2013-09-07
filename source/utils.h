#ifndef UTILS_H
#define UTILS_H

#define PI 3.14159265359


namespace utils {

	inline float degToRad( float deg ) {
		return ( deg * PI / 180.0f );
	}

}

#endif
