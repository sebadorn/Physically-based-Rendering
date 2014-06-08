#include "SpecParser.h"

using std::map;
using std::string;
using std::vector;


/**
 * Get the associations of material names to spectrum.
 * @return {std::map<std::string, std::map<std::string, std::string>>}
 */
map<string, map<string, string> > SpecParser::getMaterialToSPD() {
	return mMaterialToSPD;
}


/**
 * Get the SPD name of the sky (environment).
 * @return {std::string}
 */
string SpecParser::getSkySPDName() {
	return mSky;
}


/**
 * Get the spectral power distributions.
 * @return {std::map<std::string, std::vector<cl_float>>}
 */
map<string, vector<cl_float> > SpecParser::getSpectralPowerDistributions() {
	return mSPDs;
}


/**
 * Load the SPEC file associated with the given OBJ model.
 * @param {std::string} filepath Path to the file.
 * @param {std::string} filename Name of the OBJ file inclusive extension.
 */
void SpecParser::load( string filepath, string filename ) {
	size_t extensionIndex = filename.rfind( ".obj" );
	filename.replace( extensionIndex, 4, ".spec" );

	boost::property_tree::ptree propTree;
	boost::property_tree::json_parser::read_json( filepath + filename, propTree );

	this->loadMaterialToSPD( propTree );
	this->loadSpectralPowerDistributions( propTree );

	char msg[128];
	snprintf( msg, 128, "[SpecParser] Loaded %lu spectral power distributions.", mSPDs.size() );
	Logger::logInfo( msg );
}


/**
 * Load the associations of material names to spectrum.
 * @param {boost::property_tree::ptree} propTree
 */
void SpecParser::loadMaterialToSPD( boost::property_tree::ptree propTree ) {
	boost::property_tree::ptree::const_iterator it;
	boost::property_tree::ptree mtlTree = propTree.get_child( "materials" );

	for( it = mtlTree.begin(); it != mtlTree.end(); ++it ) {
		map<string, string> spds;
		spds["diff"] = it->second.get<string>( "diff" );
		spds["spec"] = it->second.get<string>( "spec" );
		mMaterialToSPD[it->first] = spds;
	}
}


/**
 * Load the spectral power distributions.
 * @param {boost::property_tree::ptree} propTree
 */
void SpecParser::loadSpectralPowerDistributions( boost::property_tree::ptree propTree ) {
	boost::property_tree::ptree::const_iterator it, itSPD;
	boost::property_tree::ptree specTree = propTree.get_child( "spectra" );
	mSky = propTree.get<string>( "sky" );

	for( it = specTree.begin(); it != specTree.end(); ++it ) {
		vector<cl_float> spd;

		for( itSPD = it->second.begin(); itSPD != it->second.end(); ++itSPD ) {
			spd.push_back( it->second.get<cl_float>( itSPD->first ) );
		}

		mSPDs[it->first] = spd;
	}
}
