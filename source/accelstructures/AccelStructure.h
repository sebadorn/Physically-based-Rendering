#ifndef ACCELSTRUCT_H
#define ACCELSTRUCT_H

#define ACCELSTRUCT_BVH 0
#define ACCELSTRUCT_KDTREE 1

#include <CL/cl.hpp>
#include <glm/glm.hpp>
#include <vector>

using std::vector;


struct Tri {
	cl_uint4 face;
	glm::vec3 bbMin;
	glm::vec3 bbMax;
};


class AccelStructure {

	public:
		static vector<cl_float4> packFloatAsFloat4( const vector<cl_float>* vertices );
		virtual void visualize( vector<cl_float>* vertices, vector<cl_uint>* indices ) = 0;

};

#endif
