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
 * Extract the faces data from an object3D.
 * @param {object3D}                object
 * @param {std::vector<cl_uint4>*}  faces
 * @param {cl_int}                  offset
 */
void ModelLoader::getFacesOfObject(
	object3D object, vector<cl_uint4>* faces, cl_int offset
) {
	cl_uint a, b, c;

	for( cl_uint i = 0; i < object.facesV.size(); i += 3 ) {
		a = object.facesV[i + 0];
		b = object.facesV[i + 1];
		c = object.facesV[i + 2];

		cl_uint4 f = { a, b, c, offset + faces->size() };
		faces->push_back( f );
	}
}


void ModelLoader::getFaceNormalsOfObject(
	object3D object, vector<cl_uint4>* faceNormals, cl_int offset
) {
	cl_uint a, b, c;

	for( cl_uint i = 0; i < object.facesVN.size(); i += 3 ) {
		a = object.facesVN[i + 0];
		b = object.facesVN[i + 1];
		c = object.facesVN[i + 2];

		cl_uint4 fn = { a, b, c, offset + faceNormals->size() };
		faceNormals->push_back( fn );
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

	if( Cfg::get().value<bool>( Cfg::USE_SPECTRAL ) ) {
		mSpecParser->load( filepath, filename );
	}

	Logger::indent( 0 );

	vector<cl_uint> facesV = mObjParser->getFacesV();
	vector<cl_uint> facesVN = mObjParser->getFacesVN();
	vector<cl_float> vertices = mObjParser->getVertices();

	snprintf(
		msg, 256, "[ModelLoader] ... Done. %lu vertices, %lu normals, and %lu faces.",
		vertices.size() / 3, facesVN.size() / 3, facesV.size() / 3
	);
	Logger::logInfo( msg );
}
