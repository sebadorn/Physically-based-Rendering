#ifndef MATHHELP_H
#define MATHHELP_H

#include <CL/cl.hpp>
#include <glm/glm.hpp>
#include <vector>

using std::vector;

#define MH_PI 3.14159265359


class MathHelp {

	public:
		static cl_float degToRad( cl_float deg );
		static void getAABB( vector<cl_float4> vertices, glm::vec3* bbMin, glm::vec3* bbMax );
		static cl_float getSurfaceArea( glm::vec3 bbMin, glm::vec3 bbMax );
		static void getTriangleAABB( cl_float4 v0, cl_float4 v1, cl_float4 v2, glm::vec3* bbMin, glm::vec3* bbMax );
		static glm::vec3 getTriangleCenter( cl_float4 v0, cl_float4 v1, cl_float4 v2 );
		static glm::vec3 getTriangleCentroid( cl_float4 v0, cl_float4 v1, cl_float4 v2 );
		static cl_float radToDeg( cl_float rad );

};

#endif
