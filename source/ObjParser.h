#ifndef OBJPARSER_H
#define OBJPARSER_H

#include <boost/algorithm/string.hpp>
#include <GL/gl.h>
#include <fstream>
#include <string>
#include <vector>

#include "Logger.h"

using std::string;
using std::vector;


class ObjParser {

	public:
		void load( string filepath, string filename );
		vector<GLuint> getFacesV();
		vector<GLuint> getFacesVN();
		vector<GLuint> getFacesVT();
		vector<GLfloat> getNormals();
		vector<GLfloat> getTextureCoordinates();
		vector<GLfloat> getVertices();

	protected:
		void parseFace(
			string line, vector<GLuint>* facesV,
			vector<GLuint>* facesVN, vector<GLuint>* facesVT
		);
		void parseVertex( string line, vector<GLfloat>* vertices );
		void parseVertexNormal( string line, vector<GLfloat>* normals );
		void parseVertexTexture( string line, vector<GLfloat>* textures );

	private:
		vector<GLuint> mFacesV;
		vector<GLuint> mFacesVN;
		vector<GLuint> mFacesVT;
		vector<GLfloat> mNormals;
		vector<GLfloat> mTextures;
		vector<GLfloat> mVertices;

};

#endif
