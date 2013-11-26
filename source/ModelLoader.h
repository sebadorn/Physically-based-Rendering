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


struct light_t {
	float position[4];
	float diffuse[4],
	      specular[4];
	float constantAttenuation,
	      linearAttenuation,
	      quadraticAttenuation;
	float spotCutoff,
	      spotExponent,
	      spotDirection[3];
};


class ModelLoader {

	public:
		ModelLoader();
		~ModelLoader();
		vector<GLfloat> getBoundingBox();
		vector<GLint> getFacesMtl();
		vector<GLuint> getFacesV();
		vector<GLuint> getFacesVN();
		vector<GLuint> getFacesVT();
		vector<light_t> getLights();
		vector<material_t> getMaterials();
		vector<GLfloat> getNormals();
		vector<GLfloat> getTextureCoordinates();
		vector<GLfloat> getVertices();
		void loadModel( string filepath, string filename );

	protected:
		vector<GLfloat> computeBoundingBox( vector<GLfloat> vertices );
		void loadLights( string filepath, string filename );
		GLuint loadTexture( string filepath );

	private:
		ObjParser* mObjParser;

		map<string, GLuint> mFileToTextureID;
		map<GLuint, GLuint> mTextureIDs;

		vector<GLfloat> mBoundingBox;
		vector<light_t> mLights;

};

#endif
