#ifndef ACCELSTRUCT_H
#define ACCELSTRUCT_H

#define ACCELSTRUCT_BVH 0
#define ACCELSTRUCT_KDTREE 1

#include <CL/cl.hpp>
#include <vector>

using std::vector;


class AccelStructure {

	public:
		virtual void visualize( vector<cl_float>* vertices, vector<cl_uint>* indices ) = 0;

};

#endif
