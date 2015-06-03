#ifndef LIGHTPARSER_H
#define LIGHTPARSER_H

#include <boost/algorithm/string.hpp>
#include <CL/cl.hpp>
#include <fstream>
#include <GL/gl.h>
#include <string>
#include <vector>

#include "Logger.h"

using std::string;
using std::vector;


struct light_t {
	string lightName;
	cl_uint type;
	cl_float4 pos;
	cl_float4 rgb;
};


class LightParser {

	public:
		vector<light_t> getLights();
		void load( string file );

	protected:
		light_t getEmptyLight();

	private:
		vector<light_t> mLights;

};

#endif
