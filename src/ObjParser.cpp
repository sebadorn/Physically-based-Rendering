#include "ObjParser.h"

using std::map;
using std::string;
using std::vector;


/**
 * Constructor.
 */
ObjParser::ObjParser() {
	mMtlParser = new MtlParser();
	mLightParser = new LightParser();
}


/**
 * Destructor.
 */
ObjParser::~ObjParser() {
	delete mMtlParser;
	delete mLightParser;
}


/**
 * Get the material of each face.
 * @return {std::vector<int>} The material index of each face.
 */
vector<int> ObjParser::getFacesMtl() {
	return mFacesMtl;
}


/**
 * Get the loaded faces.
 * @return {std::vector<unsigned long int>} The vertex indices of the faces.
 */
vector<unsigned long int> ObjParser::getFacesV() {
	return mFacesV;
}


/**
 * Get the loaded association of vertices to normals for each face.
 * @return {std::vector<unsigned long int>} The vertex normal indices of the faces.
 */
vector<unsigned long int> ObjParser::getFacesVN() {
	return mFacesVN;
}


/**
 * Get the loaded association of vertices to texture coordinates for each face.
 * @return {std::vector<unsigned long int>} The vertex texture indices of the faces.
 */
vector<unsigned long int> ObjParser::getFacesVT() {
	return mFacesVT;
}


/**
 * Get the loaded lights.
 * @return {std::vector<light_t>} The lights from the LIGHT file.
 */
vector<light_t> ObjParser::getLights() {
	return mLightParser->getLights();
}


/**
 * Get the loaded materials.
 * @return {std::vector<material_t>} The materials from the MTL file.
 */
vector<material_t> ObjParser::getMaterials() {
	return mMtlParser->getMaterials();
}


/**
 * Get the loaded vertex normals.
 * @return {std::vector<float>} The vertex normals.
 */
vector<float> ObjParser::getNormals() {
	return mNormals;
}


/**
 * Get the loaded 3D objects (groups of faces that describe an object in the scene).
 * @return {std::vector<object3D>} The 3D objects.
 */
vector<object3D> ObjParser::getObjects() {
	return mObjects;
}


/**
 * Get the loaded vertex texture coordinates.
 * @return {std::vector<float>} The texture coordinates.
 */
vector<float> ObjParser::getTextureCoordinates() {
	return mTextures;
}


/**
 * Get the loaded vertices.
 * @return {std::vector<float>} The vertices.
 */
vector<float> ObjParser::getVertices() {
	return mVertices;
}


/**
 * Load an OBJ file.
 * @param {std::string} filepath Path to the file.
 * @param {std::string} filename Name of the file.
 */
void ObjParser::load( string filepath, string filename ) {
	mObjects.clear();
	mFacesMtl.clear();
	mFacesV.clear();
	mFacesVN.clear();
	mFacesVT.clear();
	mNormals.clear();
	mTextures.clear();
	mVertices.clear();

	std::ifstream fileIn( filepath.append( filename ).c_str() );

	if( Cfg::get().value<int>( Cfg::RENDER_SHADOWRAYS ) > 0 ) {
		this->loadLights( filepath );
	}

	this->loadMtl( filepath );
	vector<material_t> materials = mMtlParser->getMaterials();
	vector<string> materialNames;
	int currentMtl = -1;

	for( int i = 0; i < materials.size(); i++ ) {
		materialNames.push_back( materials[i].mtlName );
	}

	boost::posix_time::ptime timerStart = boost::posix_time::microsec_clock::local_time();

	while( fileIn.good() ) {
		string line;
		getline( fileIn, line );
		boost::algorithm::trim( line );

		// Ignore comment lines
		if( line[0] == '#' ) {
			continue;
		}

		// 3D object
		if( line[0] == 'o' ) {
			object3D o;
			vector<string> parts;
			boost::split( parts, line, boost::is_any_of( " \t" ) );
			o.oName = parts[1];

			mObjects.push_back( o );
		}
		// Vertex data of some form
		else if( line[0] == 'v' ) {
			// vertex
			if( line[1] == ' ' ) {
				this->parseVertex( line, &mVertices );
			}
			// vertex normal
			else if( line[1] == 'n' && line[2] == ' ' ) {
				this->parseVertexNormal( line, &mNormals );
			}
			// vertex texture
			else if( line[1] == 't' && line[2] == ' ' ) {
				this->parseVertexTexture( line, &mTextures );
			}
		}
		// Faces
		else if( line[0] == 'f' ) {
			if( line[1] == ' ' ) {
				vector<unsigned long int> lineFacesV, lineFacesVN, lineFacesVT;
				this->parseFace( line, &lineFacesV, &lineFacesVN, &lineFacesVT );

				mFacesV.insert( mFacesV.end(), lineFacesV.begin(), lineFacesV.end() );
				mFacesVN.insert( mFacesVN.end(), lineFacesVN.begin(), lineFacesVN.end() );
				mFacesVT.insert( mFacesVT.end(), lineFacesVT.begin(), lineFacesVT.end() );

				mFacesMtl.push_back( currentMtl );

				if( mObjects.size() > 0 ) {
					object3D* op = &( mObjects[mObjects.size() - 1] );
					op->facesV.insert( op->facesV.end(), lineFacesV.begin(), lineFacesV.end() );
					op->facesVN.insert( op->facesVN.end(), lineFacesVN.begin(), lineFacesVN.end() );
				}
			}
		}
		// Use material
		else if( line.find( "usemtl" ) != string::npos ) {
			vector<string> parts;
			boost::split( parts, line, boost::is_any_of( " \t" ) );
			vector<string>::iterator it = std::find( materialNames.begin(), materialNames.end(), parts[1] );
			currentMtl = ( it != materialNames.end() ) ? it - materialNames.begin() : -1;
		}
	}

	fileIn.close();

	boost::posix_time::ptime timerEnd = boost::posix_time::microsec_clock::local_time();
	float timeDiff = ( timerEnd - timerStart ).total_milliseconds() / 1000.0f;

	char msg[256];
	snprintf(
		msg, 256, "[ObjParser] Loaded %lu vertices, %lu normals, and %lu faces in %g s.",
		mVertices.size() / 3, mFacesVN.size() / 3, mFacesV.size() / 3, timeDiff
	);
	Logger::logInfo( msg );
}


/**
 * Load the LIGHTS file to the OBJ.
 * @param {std::string} file File path and name of the OBJ. Assuming the LIGHTS file has the same name aside from the file extension.
 */
void ObjParser::loadLights( string file ) {
	size_t extensionIndex = file.rfind( ".obj" );
	file.replace( extensionIndex, 4, ".lights" );

	mLightParser->load( file );
}


/**
 * Load the MTL file to the OBJ.
 * @param {std::string} file File path and name of the OBJ. Assuming the MTL file has the same name aside from the file extension.
 */
void ObjParser::loadMtl( string file ) {
	size_t extensionIndex = file.rfind( ".obj" );
	file.replace( extensionIndex, 4, ".mtl" );

	mMtlParser->load( file );
}


/**
 * Parse lines like "f 1 4 3" (f v0 v1 v2)
 * or "f 1/2/3 4/7/4 3/11/2" (f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3).
 * or "f 1/2 4/7 3/11" (f v1/vt1 v2/vt2 v3/vt3)
 * or "f 1//3 4//4 3//2" (f v1//vn1 v2//vn2 v3//vn3)
 * Only works for triangular faces.
 * Subtracts 1 from the vertex index, so it can directly be used as array index.
 * @param {std::string}                     line  The line to parse.
 * @param {std::vector<unsigned long int>*} faces The vector to store the face in.
 */
void ObjParser::parseFace(
	string line,
	vector<unsigned long int>* facesV,
	vector<unsigned long int>* facesVN,
	vector<unsigned long int>* facesVT
) {
	unsigned long int numFaces = facesV->size();
	vector<string> parts;
	boost::split( parts, line, boost::is_any_of( " \t" ) );

	for( int i = 1; i < parts.size(); i++ ) {
		unsigned long int a;
		vector<string> e0, e1;
		boost::split( e0, parts[i], boost::is_any_of( "/" ) );
		boost::split( e1, parts[i], boost::is_any_of( "//" ) );

		// "v//vn"
		if( e1.size() == 2 ) {
			// v
			a = atol( e1[0].c_str() );
			a = ( a < 0 ) ? numFaces - a : a;
			facesV->push_back( a - 1 );

			// vn
			a = atol( e1[1].c_str() );
			a = ( a < 0 ) ? numFaces - a : a;
			facesVN->push_back( a - 1 );
		}
		else {
			// "v"
			a = atol( e0[0].c_str() );
			a = ( a < 0 ) ? numFaces - a : a;
			facesV->push_back( a - 1 );

			// "v/vt"
			if( e0.size() >= 2 ) {
				a = atol( e0[1].c_str() );
				a = ( a < 0 ) ? numFaces - a : a;
				facesVT->push_back( a - 1 );
			}
			// "v/vt/vn"
			if( e0.size() >= 3 ) {
				a = atol( e0[2].c_str() );
				a = ( a < 0 ) ? numFaces - a : a;
				facesVN->push_back( a - 1 );
			}
		}
	}
}


/**
 * Parse lines like "v 1.0000 -0.3000 -14.0068" (v x y z).
 * @param {std::string}           line  The line to parse.
 * @param {std::vector<float>*} faces The vector to store the vertex in.
 */
void ObjParser::parseVertex( string line, vector<float>* vertices ) {
	vector<string> parts;
	boost::split( parts, line, boost::is_any_of( " \t" ) );

	vertices->push_back( atof( parts[1].c_str() ) );
	vertices->push_back( atof( parts[2].c_str() ) );
	vertices->push_back( atof( parts[3].c_str() ) );
}


/**
 * Parse lines like "vn 1.0000 -0.3000 -14.0068" (vn x y z).
 * @param {std::string}           line  The line to parse.
 * @param {std::vector<float>*} faces The vector to store the vertex normal in.
 */
void ObjParser::parseVertexNormal( string line, vector<float>* normals ) {
	vector<string> parts;
	boost::split( parts, line, boost::is_any_of( " \t" ) );

	normals->push_back( atof( parts[1].c_str() ) );
	normals->push_back( atof( parts[2].c_str() ) );
	normals->push_back( atof( parts[3].c_str() ) );
}


/**
 * Parse lines like "vt 0.500 1" or "vt 0.500 1 0" (vt u v [w]).
 * @param {std::string}          line      The line to parse.
 * @param {std::vector<float>} texCoords The vector to store the vertex texture coordinates in.
 */
void ObjParser::parseVertexTexture( string line, vector<float>* texCoords ) {
	vector<string> parts;
	boost::split( parts, line, boost::is_any_of( " \t" ) );

	float weight = ( parts.size() >= 4 ) ? atof( parts[3].c_str() ) : 0.0f;

	texCoords->push_back( atof( parts[1].c_str() ) );
	texCoords->push_back( atof( parts[2].c_str() ) );
	texCoords->push_back( weight );
}
