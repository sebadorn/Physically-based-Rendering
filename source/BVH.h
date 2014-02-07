#ifndef BVH_H
#define BVH_H

#include <CL/cl.hpp>
#include <vector>

#include "Cfg.h"
#include "KdTree.h"
#include "Logger.h"
#include "ModelLoader.h"

using std::vector;


class BVH {

	public:
		BVH(
			vector<object3D> objects,
			vector<cl_float> vertices, vector<cl_float> normals
		);

	protected:


	private:


};

#endif
