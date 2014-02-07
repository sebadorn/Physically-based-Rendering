#include "ModelLoader.h"

using namespace std;
using namespace boost::property_tree;


/**
 * Constructor.
 */
ModelLoader::ModelLoader() {
	mObjParser = new ObjParser();
	mSpecParser = new SpecParser();
}


/**
 * Deconstructor.
 */
ModelLoader::~ModelLoader() {
	delete mObjParser;
	delete mSpecParser;
}


/**
 * Get the axis-aligned bounding box.
 * @return {std::vector<cl_float>} Axis-aligned bounding box.
 */
vector<cl_float> ModelLoader::getBoundingBox() {
	return mBoundingBox;
}


/**
 * Get the material of each face.
 * @return {std::vector<cl_int>} The material index of each face.
 */
vector<cl_int> ModelLoader::getFacesMtl() {
	return mObjParser->getFacesMtl();
}


/**
 * Get the faces.
 * @return {std::vector<cl_uint>} Faces.
 */
vector<cl_uint> ModelLoader::getFacesV() {
	return mObjParser->getFacesV();
}


/**
 * Get the loaded association of vertices to normals for each face.
 * @return {std::vector<cl_uint>} The vertex normal indices of the faces.
 */
vector<cl_uint> ModelLoader::getFacesVN() {
	return mObjParser->getFacesVN();
}


/**
 * Get the loaded association of vertices to texture coordinates for each face.
 * @return {std::vector<cl_uint>} The vertex texture indices of the faces.
 */
vector<cl_uint> ModelLoader::getFacesVT() {
	return mObjParser->getFacesVT();
}


/**
 * Get the loaded materials of the MTL file.
 * @return {std::vector<material_t>} The materials.
 */
vector<material_t> ModelLoader::getMaterials() {
	return mObjParser->getMaterials();
}


/**
 * Get the association of material names to spectral power distribution.
 * @return {std::map<std::string, std::string>}
 */
map<string, string> ModelLoader::getMaterialToSPD() {
	return mSpecParser->getMaterialToSPD();
}


/**
 * Get the vertex normals.
 * @return {std::vector<cl_float>} Normals.
 */
vector<cl_float> ModelLoader::getNormals() {
	return mObjParser->getNormals();
}


/**
 * Get the loaded 3D objects (groups of faces that describe an object in the scene).
 * @return {std::vector<object3D>} The 3D objects.
 */
vector<object3D> ModelLoader::getObjects() {
	return mObjParser->getObjects();
}


/**
 * Get the spectral power distributions.
 * @return {std::map<std::string, std::vector<cl_float>>}
 */
map<string, vector<cl_float> > ModelLoader::getSpectralPowerDistributions() {
	return mSpecParser->getSpectralPowerDistributions();
}


/**
 * Get the loaded vertex texture coordinates.
 * @return {std::vector<cl_float>} The texture coordinates.
 */
vector<cl_float> ModelLoader::getTextureCoordinates() {
	return mObjParser->getTextureCoordinates();
}


/**
 * Get the vertices.
 * @return {std::vector<cl_float>} Vertices.
 */
vector<cl_float> ModelLoader::getVertices() {
	return mObjParser->getVertices();
}


/**
 * Load 3D model.
 * @param {std::string} filepath Path to the file, without file name.
 * @param {std::string} filename Name of the file.
 */
void ModelLoader::loadModel( string filepath, string filename ) {
	mObjParser->load( filepath, filename );
	mSpecParser->load( filepath, filename );

	vector<cl_uint> facesV = mObjParser->getFacesV();
	vector<cl_float> vertices = mObjParser->getVertices();
	mBoundingBox = utils::computeBoundingBox( vertices );

	char msg[256];
	snprintf(
		msg, 256, "[ModelLoader] Imported model \"%s\". %lu vertices and %lu faces.",
		filename.c_str(), vertices.size() / 3, facesV.size() / 3
	);
	Logger::logInfo( msg );
}
