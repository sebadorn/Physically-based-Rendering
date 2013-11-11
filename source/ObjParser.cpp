#include "ObjParser.h"

using namespace std;


/**
 * Get the loaded faces.
 * @return {std::vector<GLuint>}
 */
vector<GLuint> ObjParser::getFaces() {
	return mFaces;
}


/**
 * Get the loaded vertex normals.
 * @return {std::vector<GLfloat>}
 */
vector<GLfloat> ObjParser::getNormals() {
	return mNormals;
}


/**
 * Get the loaded vertices.
 * @return {std::vector<GLfloat>}
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
	mFaces.clear();
	mNormals.clear();
	mVertices.clear();

	bool warnVertexTexture = true;
	ifstream fileIn( filepath.append( filename ).c_str() );

	while( fileIn.good() ) {
		string line;
		getline( fileIn, line );
		boost::algorithm::trim( line );

		// Lines with less characters then this and comments
		// can't contain anything interesting
		if( line.length() < 6 || line[0] == '#' ) {
			continue;
		}

		switch( line[0] ) {

			// Vertex data of some form
			case 'v':
				// vertex
				if( line[1] == ' ' ) {
					this->parseVertex( line, &mVertices );
				}
				// vertex normal
				else if( line[1] == 'n' && line[2] == ' ' ) {
					this->parseVertexNormal( line, &mNormals );
				}
				// vertex texture
				else if( line[1] == 't' && line[2] == ' ' && warnVertexTexture ) {
					// TODO
					Logger::logWarning( "[ObjParser] Import of vertex textures not supported." );
					warnVertexTexture = false; // Show this warning only once
				}
				break;

			// Faces
			case 'f':
				if( line[1] == ' ' ) {
					this->parseFace( line, &mFaces );
				}
				break;

		}
	}
}


/**
 * Parse lines like "f 1 4 3" or "f 1//3 4//4 3//2" or "f 1/2/3 4/7/4 3/11/2".
 * Only works for triangular faces.
 * Subtracts 1 from the vertex index, so it can directly be used as array index.
 * @param {std::string}          line  The line to parse.
 * @param {std::vector<GLuint>*} faces The vector to store the face in.
 */
void ObjParser::parseFace( string line, vector<GLuint>* faces ) {
	GLuint numFaces = faces->size();
	vector<string> parts;
	boost::split( parts, line, boost::is_any_of( " \t" ) );

	GLuint a = atol( parts[1].c_str() );
	GLuint b = atol( parts[2].c_str() );
	GLuint c = atol( parts[3].c_str() );

	a = ( a < 0 ) ? numFaces - a : a;
	b = ( b < 0 ) ? numFaces - b : b;
	c = ( c < 0 ) ? numFaces - c : c;

	faces->push_back( a - 1 );
	faces->push_back( b - 1 );
	faces->push_back( c - 1 );
}


/**
 * Parse lines like "v 1.0000 -0.3000 -14.0068".
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
 * Parse lines like "vn 1.0000 -0.3000 -14.0068".
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
