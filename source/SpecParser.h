#ifndef SPECPARSER_H
#define SPECPARSER_H

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <CL/cl.hpp>
#include <map>
#include <string>
#include <vector>

#include "Logger.h"

using std::map;
using std::string;
using std::vector;


class SpecParser {

	public:
		map<string, string> getMaterialToSPD();
		string getSkySPDName();
		map<string, vector<cl_float> > getSpectralPowerDistributions();
		void load( string filepath, string filename );

	protected:
		void loadMaterialToSPD( boost::property_tree::ptree propTree );
		void loadSpectralPowerDistributions( boost::property_tree::ptree propTree );

	private:
		map<string, string> mMaterialToSPD;
		map<string, vector<cl_float> > mSPDs;
		string mSky;

};

#endif
