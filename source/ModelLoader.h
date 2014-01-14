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
#include "SpecParser.h"

using std::map;
using std::string;
using std::vector;


class ModelLoader {

	public:
		ModelLoader();
		~ModelLoader();
		vector<GLfloat> getBoundingBox();
		vector<GLint> getFacesMtl();
		vector<GLuint> getFacesV();
		vector<GLuint> getFacesVN();
		vector<GLuint> getFacesVT();
		vector<material_t> getMaterials();
		map<string, string> getMaterialToSPD();
		vector<GLfloat> getNormals();
		map<string, vector<cl_float> > getSpectralPowerDistributions();
		vector<GLfloat> getTextureCoordinates();
		vector<GLfloat> getVertices();
		void loadModel( string filepath, string filename );

	protected:
		vector<GLfloat> computeBoundingBox( vector<GLfloat> vertices );
		GLuint loadTexture( string filepath );

	private:
		ObjParser* mObjParser;
		SpecParser* mSpecParser;

		map<string, GLuint> mFileToTextureID;
		map<GLuint, GLuint> mTextureIDs;

		vector<GLfloat> mBoundingBox;

};

#endif
