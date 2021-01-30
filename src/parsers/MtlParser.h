#ifndef MTLPARSER_H
#define MTLPARSER_H

#include <boost/algorithm/string.hpp>
#include <fstream>
#include <string>
#include <vector>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "../Logger.h"

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
	glm::vec4 Ka;
	glm::vec4 Kd;
	glm::vec4 Ks;
	float d;
	float Ni;
	float Ns;
	char illum;
	// Light source yes/no
	char light;
	// BRDF: Schlick
	float rough;
	float p;
	// BRDF: Shirley-Ashikhmin
	float nu;
	float nv;
	float Rs;
	float Rd;
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
