#ifndef MODELLOADER_H
#define MODELLOADER_H

#include <algorithm>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <GL/glew.h>
#include <GL/glut.h>
#include <iostream>
#include <IL/il.h>
#include <map>
#include <string>
#include <vector>

#include "ObjParser.h"

using std::map;
using std::string;
using std::vector;


typedef struct {
	float position[4];
	float diffuse[4],
	      specular[4];
	float constantAttenuation,
	      linearAttenuation,
	      quadraticAttenuation;
	float spotCutoff,
	      spotExponent,
	      spotDirection[3];
} light_t;


class ModelLoader {

	public:
		ModelLoader();
		~ModelLoader();
		vector<light_t> getLights();
		void loadModel( string filepath, string filename );

		vector<GLfloat> mVertices;
		vector<GLuint> mFaces;
		vector<GLfloat> mNormals;
		vector<GLfloat> mBoundingBox;

	protected:
		vector<GLfloat> computeBoundingBox( vector<GLfloat> vertices );
		void loadLights( string filepath, string filename );
		GLuint loadTexture( string filepath );

	private:
		vector<GLuint> mNumIndices;
		map<GLuint, GLuint> mTextureIDs;
		map<string, GLuint> mFileToTextureID;
		vector<light_t> mLights;
		ObjParser* mObjParser;

};

#endif
