#ifndef ACCELSTRUCT_H
#define ACCELSTRUCT_H

#define ACCELSTRUCT_BVH 0

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include <vector>

using std::vector;


struct Tri {
	glm::uvec4 face;
	glm::uvec4 normals;
	glm::vec3 bbMin;
	glm::vec3 bbMax;
};


class AccelStructure {

	public:
		static vector<glm::vec4> packFloatAsFloat4( const vector<float>* vertices );
		virtual void visualize( vector<float>* vertices, vector<unsigned long int>* indices ) = 0;

};

#endif
