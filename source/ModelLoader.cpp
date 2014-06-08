#include "ModelLoader.h"

using namespace std;


/**
 * Constructor.
 */
ModelLoader::ModelLoader() {
	mObjParser = new ObjParser();
	mSpecParser = new SpecParser();
}


/**
 * Destructor.
 */
ModelLoader::~ModelLoader() {
	delete mObjParser;
	delete mSpecParser;
}


/**
 * Extract the faces and vertices data from an object3D.
 * @param {object3D}                object
 * @param {std::vector<cl_float>}   allVertices
 * @param {std::vector<cl_uint4>*}  faces
 * @param {std::vector<cl_float4>*} vertices
 */
void ModelLoader::getFacesAndVertices(
	object3D object, vector<cl_float> allVertices,
	vector<cl_uint4>* faces, vector<cl_float4>* vertices, cl_int offset
) {
	cl_uint a, b, c;

	for( cl_uint i = 0; i < allVertices.size(); i += 3 ) {
		cl_float4 v = {
			allVertices[i + 0],
			allVertices[i + 1],
			allVertices[i + 2],
			0.0f
		};
		vertices->push_back( v );
	}

	for( cl_uint i = 0; i < object.facesV.size(); i += 3 ) {
		a = object.facesV[i + 0];
		b = object.facesV[i + 1];
		c = object.facesV[i + 2];

		cl_uint4 f = { a, b, c, offset + faces->size() };
		faces->push_back( f );
	}
}


/**
 * Get the used instance of the ObjParser.
 * @return {ObjParser*}
 */
ObjParser* ModelLoader::getObjParser() {
	return mObjParser;
}


/**
 * Get the used instance of the SpecParser.
 * @return {SpecParser*}
 */
SpecParser* ModelLoader::getSpecParser() {
	return mSpecParser;
}


/**
 * Load 3D model.
 * @param {std::string} filepath Path to the file, without file name.
 * @param {std::string} filename Name of the file.
 */
void ModelLoader::loadModel( string filepath, string filename ) {
	char msg[256];
	snprintf( msg, 256, "[ModelLoader] Importing model \"%s\" ...", filename.c_str() );
	Logger::logInfo( msg );

	Logger::indent( LOG_INDENT );
	mObjParser->load( filepath, filename );
	mSpecParser->load( filepath, filename );
	Logger::indent( 0 );

	vector<cl_uint> facesV = mObjParser->getFacesV();
	vector<cl_float> vertices = mObjParser->getVertices();

	snprintf(
		msg, 256, "[ModelLoader] ... Done. %lu vertices and %lu faces.",
		vertices.size() / 3, facesV.size() / 3
	);
	Logger::logInfo( msg );
}
