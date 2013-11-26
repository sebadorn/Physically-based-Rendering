#ifndef OBJPARSER_H
#define OBJPARSER_H

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <GL/gl.h>
#include <string>
#include <vector>

#include "Logger.h"
#include "MtlParser.h"

using std::string;
using std::vector;


class ObjParser {

	public:
		ObjParser();
		~ObjParser();
		void load( string filepath, string filename );
		vector<GLint> getFacesMtl();
		vector<GLuint> getFacesV();
		vector<GLuint> getFacesVN();
		vector<GLuint> getFacesVT();
		vector<material_t> getMaterials();
		vector<GLfloat> getNormals();
		vector<GLfloat> getTextureCoordinates();
		vector<GLfloat> getVertices();

	protected:
		void loadMtl( string file );
		void parseFace(
			string line, vector<GLuint>* facesV,
			vector<GLuint>* facesVN, vector<GLuint>* facesVT
		);
		void parseVertex( string line, vector<GLfloat>* vertices );
		void parseVertexNormal( string line, vector<GLfloat>* normals );
		void parseVertexTexture( string line, vector<GLfloat>* textures );

	private:
		MtlParser* mMtlParser;

		vector<GLint> mFacesMtl;
		vector<GLuint> mFacesV;
		vector<GLuint> mFacesVN;
		vector<GLuint> mFacesVT;
		vector<GLfloat> mNormals;
		vector<GLfloat> mTextures;
		vector<GLfloat> mVertices;

};

#endif
