#include "LightParser.h"

using std::string;
using std::vector;


/**
 * Get a light with some default values, meant to be overwritten.
 * @return {light_t} Default light.
 */
light_t LightParser::getEmptyLight() {
	cl_float4 white = { 1.0f, 1.0f, 1.0f, 0.0f };

	light_t light;
	light.lightName = "";
	light.pos = white;
	light.radius = 0.0f;
	light.rgb = white;
	light.type = 0;

	return light;
}


/**
 * Get the loaded lights.
 * @return {std::vector<light_t>} The light.
 */
vector<light_t> LightParser::getLights() {
	return mLights;
}


/**
 * Load the materials from the file.
 * @param {std::string} file File path and name of the MTL file.
 */
void LightParser::load( string file ) {
	mLights.clear();

	std::ifstream fileIn( file.c_str() );
	light_t light;
	int numLightsFound = 0;

	if( !fileIn ) {
		char msg[256];
		snprintf( msg, 256, "[LightParser] Could not open file \"%s\". No lights loaded.", file.c_str() );
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

		// Beginning of a new light
		if( parts[0] == "newlight" ) {
			if( parts.size() < 2 ) {
				Logger::logWarning( "[LightParser] No name for <newlight>. Ignoring entry." );
				continue;
			}
			if( numLightsFound > 0 ) {
				mLights.push_back( light );
			}
			numLightsFound++;

			light = this->getEmptyLight();
			light.lightName = parts[1];
		}
		// Light type
		else if( parts[0] == "type" ) {
			if( parts.size() < 2 ) {
				Logger::logWarning( "[LightParser] Not enough parameters for <type>. Ignoring attribute." );
				continue;
			}
			light.type = atol( parts[1].c_str() );
		}
		// Color
		else if( parts[0] == "rgb" ) {
			if( parts.size() < 4 ) {
				Logger::logWarning( "[LightParser] Not enough parameters for <rgb>. Ignoring attribute." );
				continue;
			}
			light.rgb.x = atof( parts[1].c_str() );
			light.rgb.y = atof( parts[2].c_str() );
			light.rgb.z = atof( parts[3].c_str() );
		}
		// Position
		else if( parts[0] == "pos" ) {
			if( parts.size() < 4 ) {
				Logger::logWarning( "[LightParser] Not enough parameters for <pos>. Ignoring attribute." );
				continue;
			}
			light.pos.x = atof( parts[1].c_str() );
			light.pos.y = atof( parts[2].c_str() );
			light.pos.z = atof( parts[3].c_str() );
		}
		// Radius, if orb
		else if( parts[0] == "radius" ) {
			if( parts.size() < 2 ) {
				Logger::logWarning( "[LightParser] Not enoug parameters for <radius>. Ignoring attribute." );
				continue;
			}
			light.radius = atof( parts[1].c_str() );
		}
	}

	if( numLightsFound > 0 ) {
		mLights.push_back( light );
	}
	else {
		Cfg::get().value( Cfg::RENDER_SHADOWRAYS, 0 );
	}

	fileIn.close();

	char msg[64];
	snprintf( msg, 64, "[LightParser] Loaded %lu light(s).", mLights.size() );
	Logger::logInfo( msg );
}
