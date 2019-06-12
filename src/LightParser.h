#ifndef LIGHTPARSER_H
#define LIGHTPARSER_H

#include <boost/algorithm/string.hpp>
#include <fstream>
#include <GL/gl.h>
#include <string>
#include <vector>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "Logger.h"

using std::string;
using std::vector;


// Light types:
// 1: Point light
// 2: Orb light
struct light_t {
	string lightName;
	unsigned int type;
	glm::vec4 pos;
	glm::vec4 rgb;
	float radius;
};


class LightParser {

	public:
		vector<light_t> getLights();
		void load( string file );

	protected:
		light_t getEmptyLight();

	private:
		vector<light_t> mLights;

};

#endif
