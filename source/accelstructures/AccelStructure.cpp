#include "AccelStructure.h"

using std::vector;


/**
 * Pack the list of single float values for the vertices as a list of glm::vec4 values.
 * @param  {std::vector<float>}     vertices The vertices to pack.
 * @return {std::vector<glm::vec4>}          List of packed vertices.
 */
vector<glm::vec4> AccelStructure::packFloatAsFloat4( const vector<float>* vertices ) {
	vector<glm::vec4> vertices4;

	for( unsigned long int i = 0; i < vertices->size(); i += 3 ) {
		glm::vec4 node = {
			(*vertices)[i],
			(*vertices)[i + 1],
			(*vertices)[i + 2],
			0.0f
		};
		vertices4.push_back( node );
	}

	return vertices4;
}
