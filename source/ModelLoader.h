#ifndef MODELLOADER_H
#define MODELLOADER_H

#include <algorithm>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <CL/cl.hpp>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "ObjParser.h"
#include "SpecParser.h"
#include "utils.h"

using std::map;
using std::string;
using std::vector;


class ModelLoader {

	public:
		ModelLoader();
		~ModelLoader();
		vector<cl_float> getBoundingBox();
		vector<cl_int> getFacesMtl();
		vector<cl_uint> getFacesV();
		vector<cl_uint> getFacesVN();
		vector<cl_uint> getFacesVT();
		vector<material_t> getMaterials();
		map<string, string> getMaterialToSPD();
		vector<cl_float> getNormals();
		vector<object3D> getObjects();
		map<string, vector<cl_float> > getSpectralPowerDistributions();
		vector<cl_float> getTextureCoordinates();
		vector<cl_float> getVertices();
		void loadModel( string filepath, string filename );

		static void getFacesAndVertices(
			object3D object, vector<cl_float> allVertices,
			vector<cl_uint4>* faces, vector<cl_float4>* vertices
		);

	private:
		ObjParser* mObjParser;
		SpecParser* mSpecParser;

		vector<cl_float> mBoundingBox;

};

#endif
