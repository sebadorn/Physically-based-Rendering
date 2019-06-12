#ifndef OBJPARSER_H
#define OBJPARSER_H

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "Logger.h"
#include "MtlParser.h"
#include "LightParser.h"

using std::map;
using std::string;
using std::vector;


struct object3D {
	string oName;
	vector<unsigned long int> facesV;
	vector<unsigned long int> facesVN;
};


class ObjParser {

	public:
		ObjParser();
		~ObjParser();
		void load( string filepath, string filename );
		vector<int> getFacesMtl();
		vector<unsigned long int> getFacesV();
		vector<unsigned long int> getFacesVN();
		vector<unsigned long int> getFacesVT();
		vector<light_t> getLights();
		vector<material_t> getMaterials();
		vector<float> getNormals();
		vector<object3D> getObjects();
		vector<float> getTextureCoordinates();
		vector<float> getVertices();

	protected:
		void loadLights( string file );
		void loadMtl( string file );
		void parseFace(
			string line, vector<unsigned long int>* facesV,
			vector<unsigned long int>* facesVN, vector<unsigned long int>* facesVT
		);
		void parseVertex( string line, vector<float>* vertices );
		void parseVertexNormal( string line, vector<float>* normals );
		void parseVertexTexture( string line, vector<float>* textures );

	private:
		LightParser* mLightParser;
		MtlParser* mMtlParser;

		vector<object3D> mObjects;
		vector<int> mFacesMtl;
		vector<unsigned long int> mFacesV;
		vector<unsigned long int> mFacesVN;
		vector<unsigned long int> mFacesVT;
		vector<float> mNormals;
		vector<float> mTextures;
		vector<float> mVertices;

};

#endif
