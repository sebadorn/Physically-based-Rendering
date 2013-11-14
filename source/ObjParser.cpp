#include "ObjParser.h"

using std::string;
using std::vector;


/**
 * Get the loaded faces.
 * @return {std::vector<GLuint>} The vertex indices of the faces.
 */
vector<GLuint> ObjParser::getFacesV() {
	return mFacesV;
}


/**
 * Get the loaded association of vertices to normals for each face.
 * @return {std::vector<GLuint>} The vertex normal indices of the faces.
 */
vector<GLuint> ObjParser::getFacesVN() {
	return mFacesVN;
}


/**
 * Get the loaded association of vertices to texture coordinates for each face.
 * @return {std::vector<GLuint>} The vertex texture indices of the faces.
 */
vector<GLuint> ObjParser::getFacesVT() {
	return mFacesVT;
}


/**
 * Get the loaded vertex normals.
 * @return {std::vector<GLfloat>} The vertex normals.
 */
vector<GLfloat> ObjParser::getNormals() {
	return mNormals;
}


/**
 * Get the loaded vertex texture coordinates.
 * @return {std::vector<GLfloat>} The texture coordinates.
 */
vector<GLfloat> ObjParser::getTextureCoordinates() {
	return mTextures;
}


/**
 * Get the loaded vertices.
 * @return {std::vector<GLfloat>} The vertices.
 */
vector<GLfloat> ObjParser::getVertices() {
	return mVertices;
}


/**
 * Load an OBJ file.
 * @param {std::string} filepath Path to the file.
 * @param {std::string} filename Name of the file.
 */
void ObjParser::load( string filepath, string filename ) {
	mFacesV.clear();
	mFacesVN.clear();
	mFacesVT.clear();
	mNormals.clear();
	mTextures.clear();
	mVertices.clear();

	std::ifstream fileIn( filepath.append( filename ).c_str() );

	while( fileIn.good() ) {
		string line;
		getline( fileIn, line );
		boost::algorithm::trim( line );

		// Lines with less characters then this and comments
		// can't contain anything interesting
		if( line.length() < 6 || line[0] == '#' ) {
			continue;
		}

		// Vertex data of some form
		if( line[0] == 'v' ) {
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
				this->parseFace( line, &mFacesV, &mFacesVN, &mFacesVT );
			}
		}
	}
}


/**
 * Parse lines like "f 1 4 3" (f v0 v1 v2)
 * or "f 1/2/3 4/7/4 3/11/2" (f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3).
 * or "f 1/2 4/7 3/11" (f v1/vt1 v2/vt2 v3/vt3)
 * or "f 1//3 4//4 3//2" (f v1//vn1 v2//vn2 v3//vn3)
 * Only works for triangular faces.
 * Subtracts 1 from the vertex index, so it can directly be used as array index.
 * @param {std::string}          line  The line to parse.
 * @param {std::vector<GLuint>*} faces The vector to store the face in.
 */
void ObjParser::parseFace( string line, vector<GLuint>* facesV, vector<GLuint>* facesVN, vector<GLuint>* facesVT ) {
	GLuint numFaces = facesV->size();
	vector<string> parts;
	boost::split( parts, line, boost::is_any_of( " \t" ) );

	for( int i = 1; i < parts.size(); i++ ) {
		GLuint a;
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
 * @param {std::vector<GLfloat>*} faces The vector to store the vertex in.
 */
void ObjParser::parseVertex( string line, vector<GLfloat>* vertices ) {
	vector<string> parts;
	boost::split( parts, line, boost::is_any_of( " \t" ) );

	vertices->push_back( atof( parts[1].c_str() ) );
	vertices->push_back( atof( parts[2].c_str() ) );
	vertices->push_back( atof( parts[3].c_str() ) );
}


/**
 * Parse lines like "vn 1.0000 -0.3000 -14.0068" (vn x y z).
 * @param {std::string}           line  The line to parse.
 * @param {std::vector<GLfloat>*} faces The vector to store the vertex normal in.
 */
void ObjParser::parseVertexNormal( string line, vector<GLfloat>* normals ) {
	vector<string> parts;
	boost::split( parts, line, boost::is_any_of( " \t" ) );

	normals->push_back( atof( parts[1].c_str() ) );
	normals->push_back( atof( parts[2].c_str() ) );
	normals->push_back( atof( parts[3].c_str() ) );
}


/**
 * Parse lines like "vt 0.500 1" or "vt 0.500 1 0" (vt u v [w]).
 * @param {std::string}          line      The line to parse.
 * @param {std::vector<GLfloat>} texCoords The vector to store the vertex texture coordinates in.
 */
void ObjParser::parseVertexTexture( string line, vector<GLfloat>* texCoords ) {
	vector<string> parts;
	boost::split( parts, line, boost::is_any_of( " \t" ) );

	float weight = ( parts.size() >= 4 ) ? atof( parts[3].c_str() ) : 0.0f;

	texCoords->push_back( atof( parts[1].c_str() ) );
	texCoords->push_back( atof( parts[2].c_str() ) );
	texCoords->push_back( weight );
}
