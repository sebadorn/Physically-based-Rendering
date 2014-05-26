#ifndef MATHHELP_H
#define MATHHELP_H

#define GLM_FORCE_RADIANS

#include <CL/cl.hpp>
#include <glm/glm.hpp>
#include <vector>

using std::vector;

#define MH_PI 3.14159265359
#define FLOAT4_TO_VEC3( v ) ( glm::vec3( ( v ).x, ( v ).y, ( v ).z ) )


class MathHelp {

	public:
		static cl_float degToRad( cl_float deg );
		static void getAABB(
			vector<cl_float4> vertices, glm::vec3* bbMin, glm::vec3* bbMax
		);
		static void getAABB(
			vector<glm::vec3> bbMins, vector<glm::vec3> bbMaxs,
			glm::vec3* bbMin, glm::vec3* bbMax
		);
		static cl_float getSurfaceArea( glm::vec3 bbMin, glm::vec3 bbMax );
		static void getTriangleAABB(
			cl_float4 v0, cl_float4 v1, cl_float4 v2, glm::vec3* bbMin, glm::vec3* bbMax
		);
		static glm::vec3 getTriangleCenter(
			cl_float4 v0, cl_float4 v1, cl_float4 v2
		);
		static glm::vec3 getTriangleCentroid(
			cl_float4 v0, cl_float4 v1, cl_float4 v2
		);
		static glm::vec3 intersectLinePlane(
			glm::vec3 p, glm::vec3 q, glm::vec3 x, glm::vec3 nl, bool* isParallel
		);
		static cl_float radToDeg( cl_float rad );

};

#endif
