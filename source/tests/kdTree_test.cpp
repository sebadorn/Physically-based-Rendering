#include "../KdTree.h"


int main( int argc, char* argv[] ) {
	setlocale( LC_ALL, "C" );
	Cfg::get().loadConfigFile( "config.json" );

	// Test 1: makeTree
	std::vector<float> vertices;
	std::vector<uint> indices;

	KdTree* tree = new KdTree( vertices, indices );

	return EXIT_SUCCESS;
}
