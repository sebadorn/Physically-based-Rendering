#ifndef CL_H
#define CL_H

#include <CL/cl.hpp>
#include <iostream>
#include <string>

#include "Cfg.h"
#include "Logger.h"
#include "utils.h"


class CL {

	public:
		CL();
		~CL();
		void loadProgram( std::string filepath );

	protected:
		void buildProgram();
		bool checkError( cl_int err, const char* functionName );
		void getDefaultDevice();
		void getDefaultPlatform();
		void initCommandQueue();
		void initContext( cl_device_id* devices );

	private:
		cl_command_queue mCommandQueue;
		cl_context mContext;
		cl_device_id mDevice;
		cl_kernel mKernel;
		cl_platform_id mPlatform;
		cl_program mProgram;

};

#endif
