#ifndef MODELLOADER_H
#define MODELLOADER_H

#include <boost/date_time/posix_time/posix_time.hpp>
#include <string>
#include <vector>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "parsers/ObjParser.h"
#include "utils.h"

using std::string;
using std::vector;


class ModelLoader {

	public:
		ModelLoader();
		~ModelLoader();
		ObjParser* getObjParser();
		void loadModel( string filepath, string filename );

		static void getFaceNormalsOfObject( object3D object, vector<glm::uvec4>* faceNormals, int offset );
		static void getFacesOfObject( object3D object, vector<glm::uvec4>* faces, int offset );

	private:
		ObjParser* mObjParser;

};

#endif
