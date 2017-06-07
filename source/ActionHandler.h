#ifndef ACTIONHANDLER_H
#define ACTIONHANDLER_H

#include "Logger.h"
#include "VulkanHandler.h"

using std::string;


struct VulkanHandler;


class ActionHandler {


	public:
		void exit( VulkanHandler* vh );
		void loadModel( const string& filepath, const string& filename );


	protected:
		//


	private:
		//


};

#endif
