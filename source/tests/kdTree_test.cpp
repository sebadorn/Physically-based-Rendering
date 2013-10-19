#include "../KdTree.h"
#include "../ModelLoader.h"

#define TEST_LOG_PREFIX "* [Test/KdTree] "

int main( int argc, char* argv[] ) {
	setlocale( LC_ALL, "C" );
	Cfg::get().loadConfigFile( "../config.json" );


	ModelLoader* ml = new ModelLoader();
	ml->loadModel( "../resources/models/cornell-box/", "cornell_original.obj" );

	std::vector<GLfloat> vertices = ml->mVertices;
	std::vector<GLuint> indices = ml->mIndices;
	std::vector<GLfloat> bb = ml->mBoundingBox;

	Logger::logInfo( "BoundingBox:" );
	printf( "    Min: %g  %g  %g\n", bb[0], bb[1], bb[2] );
	printf( "    Max: %g  %g  %g\n", bb[3], bb[4], bb[5] );

	char msg[20];
	snprintf( msg, 20, "KdNodes: %lu", indices.size() / 3 );
	Logger::logInfo( msg, TEST_LOG_PREFIX );

	Logger::logInfo( "Starting.", TEST_LOG_PREFIX );
	KdTree* tree = new KdTree( vertices, indices );
	Logger::logInfo( "Created tree.", TEST_LOG_PREFIX );

	tree->print();

	delete tree;
	Logger::logInfo( "Deleted tree.", TEST_LOG_PREFIX );

	return EXIT_SUCCESS;
}
