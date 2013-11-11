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
		std::vector<light_t> getLights();
		void loadModel( std::string filepath, std::string filename );

		std::vector<GLfloat> mVertices;
		std::vector<GLuint> mFaces;
		std::vector<GLfloat> mNormals;
		std::vector<GLfloat> mBoundingBox;

	protected:
		std::vector<GLfloat> computeBoundingBox( std::vector<GLfloat> vertices );
		void loadLights( std::string filepath, std::string filename );
		GLuint loadTexture( std::string filepath );

	private:
		std::vector<GLuint> mNumIndices;
		std::map<GLuint, GLuint> mTextureIDs;
		std::map<std::string, GLuint> mFileToTextureID;
		std::vector<light_t> mLights;
		ObjParser* mObjParser;

};

#endif
