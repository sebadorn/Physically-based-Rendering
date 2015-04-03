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
 * @param {glm::vec3*}             bbMin
 * @param {glm::vec3*}             bbMax
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
 * Calculate the bounding box from the given list of bounding boxes.
 * @param {std::vector<glm::vec3>} bbMins
 * @param {std::vector<glm::vec3>} bbMaxs
 * @param {glm::vec3*}             bbMin
 * @param {glm::vec3*}             bbMax
 */
void MathHelp::getAABB(
	vector<glm::vec3> bbMins, vector<glm::vec3> bbMaxs, glm::vec3* bbMin, glm::vec3* bbMax
) {
	(*bbMin)[0] = bbMins[0][0];
	(*bbMin)[1] = bbMins[0][1];
	(*bbMin)[2] = bbMins[0][2];

	(*bbMax)[0] = bbMaxs[0][0];
	(*bbMax)[1] = bbMaxs[0][1];
	(*bbMax)[2] = bbMaxs[0][2];

	for( cl_uint i = 1; i < bbMins.size(); i++ ) {
		(*bbMin)[0] = glm::min( bbMins[i][0], (*bbMin)[0] );
		(*bbMin)[1] = glm::min( bbMins[i][1], (*bbMin)[1] );
		(*bbMin)[2] = glm::min( bbMins[i][2], (*bbMin)[2] );

		(*bbMax)[0] = glm::max( bbMaxs[i][0], (*bbMax)[0] );
		(*bbMax)[1] = glm::max( bbMaxs[i][1], (*bbMax)[1] );
		(*bbMax)[2] = glm::max( bbMaxs[i][2], (*bbMax)[2] );
	}
}


/**
 * Get the surface are of a bounding box.
 * @param  {glm::vec3} bbMin
 * @param  {glm::vec3} bbMax
 * @return {cl_float}        Surface area.
 */
cl_float MathHelp::getSurfaceArea( glm::vec3 bbMin, glm::vec3 bbMax ) {
	cl_float xy = 2.0f * fabs( bbMax[0] - bbMin[0] ) * fabs( bbMax[1] - bbMin[1] );
	cl_float zy = 2.0f * fabs( bbMax[2] - bbMin[2] ) * fabs( bbMax[1] - bbMin[1] );
	cl_float xz = 2.0f * fabs( bbMax[0] - bbMin[0] ) * fabs( bbMax[2] - bbMin[2] );

	return xy + zy + xz;
}


/**
 * Get the bounding box a face (triangle).
 * @param {cl_uint4}               face
 * @param {std::vector<cl_float4>} vertices
 * @param {glm::vec3*}             bbMin
 * @param {glm::vec3*}             bbMax
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
 * Find the intersection of a line with a plane (3D).
 * @param  {glm::vec3} p          A point p on the line.
 * @param  {glm::vec3} q          A point q on the line.
 * @param  {glm::vec3} x          A point x on the plane.
 * @param  {glm::vec3} nl         Normal of the plane.
 * @param  {bool*}     isParallel Flag, that will be set to signalise if the line is parallel to the plane (= no intersection).
 * @return {glm::vec3}            The point of intersection.
 */
glm::vec3 MathHelp::intersectLinePlane( glm::vec3 p, glm::vec3 q, glm::vec3 x, glm::vec3 nl, bool* isParallel ) {
	glm::vec3 hit;
	glm::vec3 u = q - p;
	glm::vec3 w = p - x;
	cl_float d = glm::dot( nl, u );

	if( glm::abs( d ) < 0.000001f ) {
		*isParallel = true;
	}
	else {
		cl_float t = -glm::dot( nl, w ) / d;
		hit = p + u * t;
		*isParallel = false;
	}

	return hit;
}


/**
 * Get the index of the longest axis.
 * @param  {glm::vec3} bbMin
 * @param  {glm::vec3} bbMax
 * @return {short}           Index of the longest axis (X: 0, Y: 1, Z: 2).
 */
short MathHelp::longestAxis( glm::vec3 bbMin, glm::vec3 bbMax ) {
	glm::vec3 sides = bbMax - bbMin;

	if( sides[0] > sides[1] ) {
		return ( sides[0] > sides[2] ) ? 0 : 2;
	}
	else { // sides[1] > sides[0]
		return ( sides[1] > sides[2] ) ? 1 : 2;
	}
}


/**
 * Phong tessellate a given point.
 * @param  {const glm::vec3} p1    Point 1 of the triangle.
 * @param  {const glm::vec3} p2    Point 2 of the triangle.
 * @param  {const glm::vec3} p3    Point 3 of the triangle.
 * @param  {const glm::vec3} n1    Normal at point 1.
 * @param  {const glm::vec3} n2    Normal at point 2.
 * @param  {const glm::vec3} n3    Normal at point 3.
 * @param  {const float}     alpha Phong Tessellation strength [0,1].
 * @param  {const float}     u     Value for barycentric coordinate.
 * @param  {const float}     v     Value for barycentric coordinate.
 * @return {glm::vec3}       Phong tessellated point.
 */
glm::vec3 MathHelp::phongTessellate(
	const glm::vec3 p1, const glm::vec3 p2, const glm::vec3 p3,
	const glm::vec3 n1, const glm::vec3 n2, const glm::vec3 n3,
	const float alpha, const float u, const float v
) {
	float w = 1.0f - u - v;
	glm::vec3 pBary = p1 * u + p2 * v + p3 * w;
	glm::vec3 pTessellated =
		u * MathHelp::projectOnPlane( pBary, p1, n1 ) +
		v * MathHelp::projectOnPlane( pBary, p2, n2 ) +
		w * MathHelp::projectOnPlane( pBary, p3, n3 );

	return ( 1.0f - alpha ) * pBary + alpha * pTessellated;
}


glm::vec3 MathHelp::projectOnPlane( glm::vec3 q, glm::vec3 p, glm::vec3 n ) {
	return q - glm::dot( q - p, n ) * n;
}


/**
 * Convert an angle from radians to degree.
 * @param  {cl_float} rad Angle in radians.
 * @return {cl_float}     Angle in degree.
 */
cl_float MathHelp::radToDeg( cl_float rad ) {
	return ( rad * 180.0f / MH_PI );
}


/**
 * Calculate and set the AABB for a Tri (face).
 * @param {Tri*}                          tri      The face/triangle.
 * @param {const std::vector<cl_float4>*} vertices All vertices.
 * @param {const std::vector<cl_float4>*} normals  All normals.
 */
void MathHelp::triCalcAABB(
	Tri* tri, const vector<cl_float4>* vertices, const vector<cl_float4>* normals
) {
	vector<cl_float4> v;
	v.push_back( (*vertices)[tri->face.x] );
	v.push_back( (*vertices)[tri->face.y] );
	v.push_back( (*vertices)[tri->face.z] );

	glm::vec3 bbMin, bbMax;
	MathHelp::getAABB( v, &bbMin, &bbMax );
	tri->bbMin = bbMin;
	tri->bbMax = bbMax;

	// ALPHA <= 0.0, no Phong Tessellation
	if( Cfg::get().value<float>( Cfg::RENDER_PHONGTESS ) <= 0.0f ) {
		return;
	}

	glm::vec3 p1 = glm::vec3( v[0].x, v[0].y, v[0].z );
	glm::vec3 p2 = glm::vec3( v[1].x, v[1].y, v[1].z );
	glm::vec3 p3 = glm::vec3( v[2].x, v[2].y, v[2].z );

	cl_float4 fn1 = (*normals)[tri->normals.x];
	cl_float4 fn2 = (*normals)[tri->normals.y];
	cl_float4 fn3 = (*normals)[tri->normals.z];

	glm::vec3 n1 = glm::vec3( fn1.x, fn1.y, fn1.z );
	glm::vec3 n2 = glm::vec3( fn2.x, fn2.y, fn2.z );
	glm::vec3 n3 = glm::vec3( fn3.x, fn3.y, fn3.z );

	// Normals are the same, which means no Phong Tessellation possible
	glm::vec3 test = ( n1 - n2 ) + ( n2 - n3 );
	if(
		fabs( test.x ) <= 0.000001f &&
		fabs( test.y ) <= 0.000001f &&
		fabs( test.z ) <= 0.000001f
	) {
		return;
	}


	float thickness;
	glm::vec3 sidedropMin, sidedropMax;
	MathHelp::triThicknessAndSidedrop( p1, p2, p3, n1, n2, n3, &thickness, &sidedropMin, &sidedropMax );

	// Grow bigger according to thickness and sidedrop
	glm::vec3 e12 = p2 - p1;
	glm::vec3 e13 = p3 - p1;
	glm::vec3 e23 = p3 - p2;
	glm::vec3 e31 = p1 - p3;
	glm::vec3 ng = glm::normalize( glm::cross( e12, e13 ) );

	glm::vec3 p1thick = p1 + thickness * ng;
	glm::vec3 p2thick = p2 + thickness * ng;
	glm::vec3 p3thick = p3 + thickness * ng;

	tri->bbMin = glm::min( glm::min( tri->bbMin, p1thick ), glm::min( p2thick, p3thick ) );
	tri->bbMax = glm::max( glm::max( tri->bbMax, p1thick ), glm::max( p2thick, p3thick ) );
	tri->bbMin = glm::min( tri->bbMin, sidedropMin );
	tri->bbMax = glm::max( tri->bbMax, sidedropMax );
}


/**
 * Calculate thickness and sidedrops of the tessellated face.
 * @param {const glm::vec3}  p1           Point 1 of the triangle.
 * @param {const glm::vec3}  p2           Point 2 of the triangle.
 * @param {const glm::vec3}  p3           Point 3 of the triangle.
 * @param {const glm::vec3}  n1           Normal at point 1.
 * @param {const glm::vec3}  n2           Normal at point 2.
 * @param {const glm::vec3}  n3           Normal at point 3.
 * @param {float*}           thickness    Thickness.
 * @param {glm::vec3*}       sidedropMin  Sidedrops of all sides, minimum coordinates.
 * @param {glm::vec3*}       sidedropMax  Sidedrops of all sides, maximum coordinates.
 */
void MathHelp::triThicknessAndSidedrop(
	const glm::vec3 p1, const glm::vec3 p2, const glm::vec3 p3,
	const glm::vec3 n1, const glm::vec3 n2, const glm::vec3 n3,
	float* thickness, glm::vec3* sidedropMin, glm::vec3* sidedropMax
) {
	float alpha = Cfg::get().value<float>( Cfg::RENDER_PHONGTESS );

	glm::vec3 e12 = p2 - p1;
	glm::vec3 e13 = p3 - p1;
	glm::vec3 e23 = p3 - p2;
	glm::vec3 e31 = p1 - p3;
	glm::vec3 c12 = alpha * ( glm::dot( n2, e12 ) * n2 - glm::dot( n1, e12 ) * n1 );
	glm::vec3 c23 = alpha * ( glm::dot( n3, e23 ) * n3 - glm::dot( n2, e23 ) * n2 );
	glm::vec3 c31 = alpha * ( glm::dot( n1, e31 ) * n1 - glm::dot( n3, e31 ) * n3 );
	glm::vec3 ng = glm::normalize( glm::cross( e12, e13 ) );

	float k_tmp = glm::dot( ng, c12 - c23 - c31 );
	float k = 1.0f / ( 4.0f * glm::dot( ng, c23 ) * glm::dot( ng, c31 ) - k_tmp * k_tmp );

	float u = k * (
		2.0f * glm::dot( ng, c23 ) * glm::dot( ng, c31 + e31 ) +
		glm::dot( ng, c23 - e23 ) * glm::dot( ng, c12 - c23 - c31 )
	);
	float v = k * (
		2.0f * glm::dot( ng, c31 ) * glm::dot( ng, c23 - e23 ) +
		glm::dot( ng, c31 + e31 ) * glm::dot( ng, c12 - c23 - c31 )
	);

	u = ( u < 0.0f || u > 1.0f ) ? 0.0f : u;
	v = ( v < 0.0f || v > 1.0f ) ? 0.0f : v;

	glm::vec3 pt = MathHelp::phongTessellate( p1, p2, p3, n1, n2, n3, alpha, u, v );
	*thickness = glm::dot( ng, pt - p1 );

	glm::vec3 ptsd[9] = {
		MathHelp::phongTessellate( p1, p2, p3, n1, n2, n3, alpha, 0.0f, 0.5f ),
		MathHelp::phongTessellate( p1, p2, p3, n1, n2, n3, alpha, 0.5f, 0.0f ),
		MathHelp::phongTessellate( p1, p2, p3, n1, n2, n3, alpha, 0.5f, 0.5f ),
		MathHelp::phongTessellate( p1, p2, p3, n1, n2, n3, alpha, 0.25f, 0.75f ),
		MathHelp::phongTessellate( p1, p2, p3, n1, n2, n3, alpha, 0.75f, 0.25f ),
		MathHelp::phongTessellate( p1, p2, p3, n1, n2, n3, alpha, 0.25f, 0.0f ),
		MathHelp::phongTessellate( p1, p2, p3, n1, n2, n3, alpha, 0.75f, 0.0f ),
		MathHelp::phongTessellate( p1, p2, p3, n1, n2, n3, alpha, 0.0f, 0.25f ),
		MathHelp::phongTessellate( p1, p2, p3, n1, n2, n3, alpha, 0.0f, 0.75f )
	};

	*sidedropMin = ptsd[0];
	*sidedropMax = ptsd[0];

	for( cl_uint i = 1; i < 9; i++ ) {
		*sidedropMin = glm::min( *sidedropMin, ptsd[i] );
		*sidedropMax = glm::max( *sidedropMax, ptsd[i] );
	}
}