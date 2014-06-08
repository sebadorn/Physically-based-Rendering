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
 * # Material:
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
 *
 * # Custom additions:
 * light   - Is this a light [0, 1].
 *
 * # BRDF: Schlick
 * p       - Isotropy/anisotropy factor [0.0, 1.0] with 1.0 having perfect isotropy.
 * rough   - Roughness factor [0.0, 1.0]. 0 – perfectly specular; 1 - perfectly diffuse.
 *
 * # BRDF: Shirley-Ashikhmin
 * nu - Phong-like exponent to control the shape of the specular lobe.
 * nv - Phong-like exponent to control the shape of the specular lobe.
 * Rs - Specular color reflectance.
 * Rd - Diffuse color reflectance.
 */
struct material_t {
	string mtlName;
	cl_float4 Ka;
	cl_float4 Kd;
	cl_float4 Ks;
	cl_float d;
	cl_float Ni;
	cl_float Ns;
	cl_char illum;
	// Light source yes/no
	cl_char light;
	// BRDF: Schlick
	cl_float rough;
	cl_float p;
	// BRDF: Shirley-Ashikhmin
	cl_float nu;
	cl_float nv;
	cl_float Rs;
	cl_float Rd;
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
