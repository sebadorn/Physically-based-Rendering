#ifndef OBJPARSER_H
#define OBJPARSER_H

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <CL/cl.hpp>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "Logger.h"
#include "MtlParser.h"

using std::map;
using std::string;
using std::vector;


struct object3D {
	string oName;
	vector<cl_uint> facesV;
	vector<cl_uint> facesVN;
};


class ObjParser {

	public:
		ObjParser();
		~ObjParser();
		void load( string filepath, string filename );
		vector<cl_int> getFacesMtl();
		vector<cl_uint> getFacesV();
		vector<cl_uint> getFacesVN();
		vector<cl_uint> getFacesVT();
		vector<material_t> getMaterials();
		vector<cl_float> getNormals();
		vector<object3D> getObjects();
		vector<cl_float> getTextureCoordinates();
		vector<cl_float> getVertices();

	protected:
		void loadMtl( string file );
		void parseFace(
			string line, vector<cl_uint>* facesV,
			vector<cl_uint>* facesVN, vector<cl_uint>* facesVT
		);
		void parseVertex( string line, vector<cl_float>* vertices );
		void parseVertexNormal( string line, vector<cl_float>* normals );
		void parseVertexTexture( string line, vector<cl_float>* textures );

	private:
		MtlParser* mMtlParser;

		vector<object3D> mObjects;
		vector<cl_int> mFacesMtl;
		vector<cl_uint> mFacesV;
		vector<cl_uint> mFacesVN;
		vector<cl_uint> mFacesVT;
		vector<cl_float> mNormals;
		vector<cl_float> mTextures;
		vector<cl_float> mVertices;

};

#endif
