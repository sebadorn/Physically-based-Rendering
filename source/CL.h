#ifndef CL_H
#define CL_H

#include "cl.hpp"
#include <GL/gl.h>
#include <iostream>
#include <QGLWidget>
#include <string>
#include <vector>

#include "Cfg.h"
#include "Logger.h"
#include "utils.h"

using std::map;
using std::string;
using std::vector;


class CL {

	public:
		CL( const bool silent = false );
		~CL();

		template<typename T> cl_mem createBuffer( vector<T> object, size_t objectSize ) {
			cl_int err;
			cl_mem buffer = clCreateBuffer( mContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, objectSize, &object[0], &err );
			this->checkError( err, "clCreateBuffer" );
			mMemObjects.push_back( buffer );

			return buffer;
		}

		cl_mem createEmptyBuffer( size_t size, cl_mem_flags flags );
		cl_mem createImage2DReadOnly( size_t width, size_t height, cl_float* data );
		cl_mem createImage2DWriteOnly( size_t width, size_t height );
		cl_kernel createKernel( const char* functionName );
		void execute( cl_kernel kernel );
		void finish();
		void freeBuffers();
		map<cl_kernel, string> getKernelNames();
		map<cl_kernel, double> getKernelTimes();
		void loadProgram( string filepath );
		void readImageOutput( cl_mem image, size_t width, size_t height, cl_float* outputTarget );
		void setKernelArg( cl_kernel kernel, cl_uint index, size_t size, void* data );
		void setReplacement( string before, string after );
		cl_mem updateBuffer( cl_mem buffer, size_t size, void* data );
		cl_mem updateImageReadOnly( cl_mem image, size_t width, size_t height, cl_float* data );

	protected:
		void buildProgram();
		bool checkError( cl_int err, const char* functionName );
		string combineParts( string filepath );
		const char* errorCodeToName( cl_int errorCode );
		void getDefaultDevice( const bool silent = false );
		void getDefaultPlatform( const bool silent = false );
		double getKernelExecutionTime( cl_event kernelEvent );
		void initCommandQueue();
		void initContext( cl_device_id* devices );
		string setValues( string clProgramString );

	private:
		bool mDoCheckErrors;
		cl_uint mWorkHeight;
		cl_uint mWorkWidth;

		cl_command_queue mCommandQueue;
		cl_context mContext;
		cl_device_id mDevice;
		cl_kernel mKernel;
		cl_platform_id mPlatform;
		cl_program mProgram;

		vector<cl_kernel> mKernels;
		vector<cl_event> mEvents;
		vector<cl_mem> mMemObjects;

		map<cl_kernel, string> mKernelNames;
		map<cl_kernel, double> mKernelTime;
		map<string, string> mReplaceString;

};

#endif
