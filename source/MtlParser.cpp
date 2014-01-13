#include "MtlParser.h"

using std::string;
using std::vector;


/**
 * Get a material with some default values, meant to be overwritten.
 * @return {material_t} Default material.
 */
material_t MtlParser::getEmptyMaterial() {
	cl_float4 zeros = { 0.0f, 0.0f, 0.0f, 0.0f };

	material_t mtl;
	mtl.mtlName = "";
	mtl.Ka = zeros;
	mtl.Kd = zeros;
	mtl.Ks = zeros;
	mtl.Ns = 100.0f;
	mtl.Ni = 1.0f;
	mtl.d = 1.0f;
	mtl.gloss = 0.0f;
	mtl.illum = 2;

	return mtl;
}


/**
 * Get the loaded materials.
 * @return {std::vector<material_t>} The materials.
 */
vector<material_t> MtlParser::getMaterials() {
	return mMaterials;
}


/**
 * Load the materials from the file.
 * @param {std::string} file File path and name of the MTL file.
 */
void MtlParser::load( string file ) {
	mMaterials.clear();

	std::ifstream fileIn( file.c_str() );
	material_t mtl;
	int numMtlFound = 0;
	bool isSetTransparency = false;

	if( !fileIn ) {
		char msg[256];
		snprintf( msg, 256, "[MtlParser] Could not open file \"%s\". No materials loaded.", file.c_str() );
		Logger::logWarning( msg );
		return;
	}

	while( fileIn.good() ) {
		string line;
		getline( fileIn, line );
		boost::algorithm::trim( line );

		if( line.length() < 3 || line[0] == '#' ) {
			continue;
		}

		vector<string> parts;
		boost::split( parts, line, boost::is_any_of( " \t" ) );

		// Beginning of a new material
		if( parts[0] == "newmtl" ) {
			if( parts.size() < 2 ) {
				Logger::logWarning( "[MtlParser] No name for <newmtl>. Ignoring entry." );
				continue;
			}
			if( numMtlFound > 0 ) {
				mMaterials.push_back( mtl );
			}
			numMtlFound++;

			mtl = this->getEmptyMaterial();
			mtl.mtlName = parts[1];
		}
		// Transparency (dissolve)
		else if( parts[0] == "d" ) {
			if( parts.size() < 2 ) {
				Logger::logWarning( "[MtlParser] Not enough parameters for <d>. Ignoring attribute." );
				continue;
			}
			mtl.d = atof( parts[1].c_str() );
			isSetTransparency = true;
		}
		// Transparency (dissolve)
		else if( parts[0] == "Tr" && !isSetTransparency ) {
			if( parts.size() < 2 ) {
				Logger::logWarning( "[MtlParser] Not enough parameters for <Tr>. Ignoring attribute." );
				continue;
			}
			mtl.d = 1.0f - atof( parts[1].c_str() );
		}
		// Illumination model
		else if( parts[0] == "illum" ) {
			if( parts.size() < 2 ) {
				Logger::logWarning( "[MtlParser] Not enough parameters for <illum>. Ignoring attribute." );
				continue;
			}
			mtl.illum = atol( parts[1].c_str() );

			if( mtl.illum < 0 || mtl.illum > 10 ) {
				Logger::logWarning( "[MtlParser] Invalid value for <illum>. Has to be between 0 and 10. Ignoring attribute." );
				mtl.illum = 2;
				continue;
			}
		}
		// Ambient color
		else if( parts[0] == "Ka" ) {
			if( parts.size() < 4 ) {
				Logger::logWarning( "[MtlParser] Not enough parameters for <Ka>. Ignoring attribute." );
				continue;
			}
			mtl.Ka.x = atof( parts[1].c_str() );
			mtl.Ka.y = atof( parts[2].c_str() );
			mtl.Ka.z = atof( parts[3].c_str() );
		}
		// Diffuse color
		else if( parts[0] == "Kd" ) {
			if( parts.size() < 4 ) {
				Logger::logWarning( "[MtlParser] Not enough parameters for <Kd>. Ignoring attribute." );
				continue;
			}
			mtl.Kd.x = atof( parts[1].c_str() );
			mtl.Kd.y = atof( parts[2].c_str() );
			mtl.Kd.z = atof( parts[3].c_str() );
		}
		// Specular color
		else if( parts[0] == "Ks" ) {
			if( parts.size() < 4 ) {
				Logger::logWarning( "[MtlParser] Not enough parameters for <Ks>. Ignoring attribute." );
				continue;
			}
			mtl.Ks.x = atof( parts[1].c_str() );
			mtl.Ks.y = atof( parts[2].c_str() );
			mtl.Ks.z = atof( parts[3].c_str() );
		}
		// Optical density
		else if( parts[0] == "Ni" ) {
			if( parts.size() < 2 ) {
				Logger::logWarning( "[MtlParser] Not enough parameters for <Ni>. Ignoring attribute." );
				continue;
			}
			mtl.Ni = atof( parts[1].c_str() );
		}
		// Specular exponent
		else if( parts[0] == "Ns" ) {
			if( parts.size() < 2 ) {
				Logger::logWarning( "[MtlParser] Not enough parameters for <Ns>. Ignoring attribute." );
				continue;
			}
			mtl.Ns = atof( parts[1].c_str() );
		}
		// Glossiness
		else if( parts[0] == "gloss" ) {
			if( parts.size() < 2 ) {
				Logger::logWarning( "[MtlParser] Not enough paramters for <gloss>. Ignoring attribute." );
				continue;
			}
			mtl.gloss = atof( parts[1].c_str() );
		}
	}

	if( numMtlFound > 0 ) {
		mMaterials.push_back( mtl );
	}

	fileIn.close();

	char msg[64];
	snprintf( msg, 64, "[MtlParser] Loaded %lu materials.", mMaterials.size() );
	Logger::logDebug( msg );
}
