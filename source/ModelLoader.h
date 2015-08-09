#ifndef MODELLOADER_H
#define MODELLOADER_H

#include <CL/cl.hpp>
#include <string>
#include <vector>

#include "ObjParser.h"
#include "utils.h"

using std::string;
using std::vector;


class ModelLoader {

	public:
		ModelLoader();
		~ModelLoader();
		ObjParser* getObjParser();
		void loadModel( string filepath, string filename );

		static void getFaceNormalsOfObject( object3D object, vector<cl_uint4>* faceNormals, cl_int offset );
		static void getFacesOfObject( object3D object, vector<cl_uint4>* faces, cl_int offset );

	private:
		ObjParser* mObjParser;

};

#endif
