#include "AccelStructure.h"

using std::vector;


/**
 * Pack the list of single float values for the vertices as a list of cl_float4 values.
 * @param  {std::vector<cl_float>}  vertices The vertices to pack.
 * @return {std::vector<cl_float4>}          List of packed vertices.
 */
vector<cl_float4> AccelStructure::packFloatAsFloat4( const vector<cl_float>* vertices ) {
	vector<cl_float4> vertices4;

	for( cl_uint i = 0; i < vertices->size(); i += 3 ) {
		cl_float4 node = {
			(*vertices)[i],
			(*vertices)[i + 1],
			(*vertices)[i + 2],
			0.0f
		};
		vertices4.push_back( node );
	}

	return vertices4;
}
