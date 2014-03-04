#include "MathHelp.h"


/**
 * Convert an angle from degree to radians.
 * @param  {cl_float} deg Angle in degree.
 * @return {cl_float}     Angle in radians.
 */
cl_float MathHelp::degToRad( cl_float deg ) {
	return ( deg * MH_PI / 180.0f );
}


/**
 * Calculate the bounding box from the given vertices.
 * @param {std::vector<cl_float4>} vertices
 * @param {glm::vec3}              bbMin
 * @param {glm::vec3}              bbMax
 */
void MathHelp::getAABB( vector<cl_float4> vertices, glm::vec3* bbMin, glm::vec3* bbMax ) {
	*bbMin = glm::vec3( vertices[0].x, vertices[0].y, vertices[0].z );
	*bbMax = glm::vec3( vertices[0].x, vertices[0].y, vertices[0].z );

	for( cl_uint i = 1; i < vertices.size(); i++ ) {
		cl_float4 v = vertices[i];

		(*bbMin)[0] = ( (*bbMin)[0] < v.x ) ? (*bbMin)[0] : v.x;
		(*bbMin)[1] = ( (*bbMin)[1] < v.y ) ? (*bbMin)[1] : v.y;
		(*bbMin)[2] = ( (*bbMin)[2] < v.z ) ? (*bbMin)[2] : v.z;

		(*bbMax)[0] = ( (*bbMax)[0] > v.x ) ? (*bbMax)[0] : v.x;
		(*bbMax)[1] = ( (*bbMax)[1] > v.y ) ? (*bbMax)[1] : v.y;
		(*bbMax)[2] = ( (*bbMax)[2] > v.z ) ? (*bbMax)[2] : v.z;
	}
}


/**
 * Get the surface are of a bounding box.
 * @param  {glm::vec3} bbMin
 * @param  {glm::vec3} bbMax
 * @return {cl_float}        Surface area.
 */
cl_float MathHelp::getSurfaceArea( glm::vec3 bbMin, glm::vec3 bbMax ) {
	cl_float xy = 2.0f * ( bbMax[0] - bbMin[0] ) * ( bbMax[1] - bbMin[1] );
	cl_float zy = 2.0f * ( bbMax[2] - bbMin[2] ) * ( bbMax[1] - bbMin[1] );
	cl_float xz = 2.0f * ( bbMax[0] - bbMin[0] ) * ( bbMax[2] - bbMin[2] );

	return xy + zy + xz;
}


/**
 * Get the bounding box a face (triangle).
 * @param {cl_uint4}               face
 * @param {std::vector<cl_float4>} vertices
 * @param {glm::vec3}              bbMin
 * @param {glm::vec3}              bbMax
 */
void MathHelp::getTriangleAABB( cl_float4 v0, cl_float4 v1, cl_float4 v2, glm::vec3* bbMin, glm::vec3* bbMax ) {
	vector<cl_float4> vertices;

	vertices.push_back( v0 );
	vertices.push_back( v1 );
	vertices.push_back( v2 );

	MathHelp::getAABB( vertices, bbMin, bbMax );
}


/**
 * Get the center point of the bounding box of a triangle.
 * @param  {cl_float4} v0 Vertex of the triangle.
 * @param  {cl_float4} v1 Vertex of the triangle.
 * @param  {cl_float4} v2 Vertex of the triangle.
 * @return {glm::vec3}    Center point.
 */
glm::vec3 MathHelp::getTriangleCenter( cl_float4 v0, cl_float4 v1, cl_float4 v2 ) {
	glm::vec3 bbMin;
	glm::vec3 bbMax;
	MathHelp::getTriangleAABB( v0, v1, v2, &bbMin, &bbMax );

	return ( bbMax - bbMin ) / 2.0f;
}


/**
 * Get the centroid of a triangle.
 * @param  {cl_float4} v0 Vertex of the triangle.
 * @param  {cl_float4} v1 Vertex of the triangle.
 * @param  {cl_float4} v2 Vertex of the triangle.
 * @return {glm::vec3}    Centroid.
 */
glm::vec3 MathHelp::getTriangleCentroid( cl_float4 v0, cl_float4 v1, cl_float4 v2 ) {
	glm::vec3 a( v0.x, v0.y, v0.z );
	glm::vec3 b( v1.x, v1.y, v1.z );
	glm::vec3 c( v2.x, v2.y, v2.z );

	return ( a + b + c ) / 3.0f;
}


/**
 * Convert an angle from radians to degree.
 * @param  {cl_float} rad Angle in radians.
 * @return {cl_float}     Angle in degree.
 */
cl_float MathHelp::radToDeg( cl_float rad ) {
	return ( rad * 180.0f / MH_PI );
}
