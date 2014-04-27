#ifndef MODELLOADER_H
#define MODELLOADER_H

#include <CL/cl.hpp>
#include <string>
#include <vector>

#include "ObjParser.h"
#include "SpecParser.h"
#include "utils.h"

using std::string;
using std::vector;


class ModelLoader {

	public:
		ModelLoader();
		~ModelLoader();
		ObjParser* getObjParser();
		SpecParser* getSpecParser();
		void loadModel( string filepath, string filename );

		static void getFacesAndVertices(
			object3D object, vector<cl_float> allVertices,
			vector<cl_uint4>* faces, vector<cl_float4>* vertices, cl_int offset
		);

	private:
		ObjParser* mObjParser;
		SpecParser* mSpecParser;

};

#endif
