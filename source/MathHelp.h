#ifndef MATHHELP_H
#define MATHHELP_H

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include <vector>

#include "Cfg.h"
#include "accelstructures/AccelStructure.h"

using std::vector;

#define MH_PI 3.14159265359
#define FLOAT4_TO_VEC3( v ) ( glm::vec3( ( v ).x, ( v ).y, ( v ).z ) )


class MathHelp {

	public:
		static float degToRad( float deg );
		static void getAABB(
			vector<glm::vec4> vertices, glm::vec3* bbMin, glm::vec3* bbMax
		);
		static void getAABB(
			vector<glm::vec3> bbMins, vector<glm::vec3> bbMaxs,
			glm::vec3* bbMin, glm::vec3* bbMax
		);
		static float getOverlapSA( glm::vec3 bbA, glm::vec3 bbB );
		static float getSurfaceArea( glm::vec3 bbMin, glm::vec3 bbMax );
		static void getTriangleAABB(
			glm::vec4 v0, glm::vec4 v1, glm::vec4 v2, glm::vec3* bbMin, glm::vec3* bbMax
		);
		static glm::vec3 getTriangleCenter(
			glm::vec4 v0, glm::vec4 v1, glm::vec4 v2
		);
		static glm::vec3 getTriangleCentroid(
			glm::vec4 v0, glm::vec4 v1, glm::vec4 v2
		);
		static glm::vec3 intersectLinePlane(
			glm::vec3 p, glm::vec3 q, glm::vec3 x, glm::vec3 nl, bool* isParallel
		);
		static short longestAxis( glm::vec3 bbMin, glm::vec3 bbMax );
		static glm::vec3 phongTessellate(
			const glm::vec3 p1, const glm::vec3 p2, const glm::vec3 p3,
			const glm::vec3 n1, const glm::vec3 n2, const glm::vec3 n3,
			const float alpha, const float u, const float v
		);
		static glm::vec3 projectOnPlane( glm::vec3 q, glm::vec3 p, glm::vec3 n );
		static float radToDeg( float rad );
		static void triCalcAABB( Tri* tri, const vector<glm::vec4>* vertices, const vector<glm::vec4>* normals );
		static void triThicknessAndSidedrop(
			const glm::vec3 p1, const glm::vec3 p2, const glm::vec3 p3,
			const glm::vec3 n1, const glm::vec3 n2, const glm::vec3 n3,
			float* thickness, glm::vec3* sidedropMin, glm::vec3* sidedropMax
		);

};

#endif
