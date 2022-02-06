#ifndef ACTIONHANDLER_H
#define ACTIONHANDLER_H

#include "Logger.h"
#include "PathTracer.h"

using std::string;


struct PathTracer;


class ActionHandler {


	public:
		void exit( PathTracer* pt );
		void loadModel( PathTracer* pt, const string& filepath, const string& filename );


};

#endif
