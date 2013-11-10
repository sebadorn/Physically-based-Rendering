#ifndef OBJPARSER_H
#define OBJPARSER_H

#include <boost/algorithm/string.hpp>
#include <GL/gl.h>
#include <fstream>
#include <string>
#include <vector>

#include "Logger.h"

using namespace std;


class ObjParser {

	public:
		void load( string filepath, string filename );
		vector<GLuint> getFaces();
		vector<GLfloat> getNormals();
		vector<GLfloat> getVertices();

	protected:
		void parseFace( string line, vector<GLuint>* faces );
		void parseVertex( string line, vector<GLfloat>* vertices );
		void parseVertexNormal( string line, vector<GLfloat>* normals );

	private:
		vector<GLuint> mFaces;
		vector<GLfloat> mNormals;
		vector<GLfloat> mVertices;

};

#endif
