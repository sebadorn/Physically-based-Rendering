#ifndef MTLPARSER_H
#define MTLPARSER_H

#include <boost/algorithm/string.hpp>
#include <CL/cl.hpp>
#include <fstream>
#include <GL/gl.h>
#include <string>
#include <vector>

#include "Logger.h"

using std::string;
using std::vector;


/**
 * Material:
 * d     - Transparency (dissolve), sometimes the identifier is "Tr".
 * illum - Illumination model.
 * Ka    - Ambient color (rgb).
 * Kd    - Diffuse color (rgb).
 * Ks    - Specular color (rgb).
 * Ni    - Optical density between [0.001, 10.0].
 *         A value of 1.0 means the light doesn't bend as it passes through.
 *         Glass has a value of around 1.5.
 * Ns    - Specular exponent between [0.0, 1000.0].
 *         A value of 0.0 will be interpreted as disabled specular highlights. (Implementation dependent.)
 */
struct material_t {
	string mtlName;
	cl_float4 Ka;
	cl_float4 Kd;
	cl_float4 Ks;
	cl_float d;
	cl_float Ni;
	cl_float Ns;
	cl_int illum;
};


class MtlParser {

	public:
		vector<material_t> getMaterials();
		void load( string file );

	protected:
		material_t getEmptyMaterial();

	private:
		vector<material_t> mMaterials;

};

#endif
