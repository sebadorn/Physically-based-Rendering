#ifndef CL_H
#define CL_H

#include <CL/cl.hpp>
#include <GL/gl.h>
#include <iostream>
#include <QGLWidget>
#include <string>
#include <vector>

#include "Cfg.h"
#include "Logger.h"
#include "utils.h"


class CL {

	public:
		CL();
		~CL();

		template<typename T> cl_mem createBuffer( T* object, size_t objectSize ) {
			cl_int err;
			cl_mem buffer = clCreateBuffer( mContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, objectSize, object, &err );
			this->checkError( err, "clCreateBuffer" );
			mMemObjects.push_back( buffer );

			return buffer;
		}

		template<typename T> cl_mem createBuffer( std::vector<T> object, size_t objectSize ) {
			cl_int err;
			cl_mem buffer = clCreateBuffer( mContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, objectSize, &object[0], &err );
			this->checkError( err, "clCreateBuffer" );
			mMemObjects.push_back( buffer );

			return buffer;
		}

		cl_mem createEmptyBuffer( size_t size, cl_mem_flags flags );
		cl_mem createImageReadOnly( size_t width, size_t height, float* data );
		cl_mem createImageWriteOnly( size_t width, size_t height );
		cl_kernel createKernel( const char* functionName );
		void execute( cl_kernel kernel );
		void finish();
		void loadProgram( std::string filepath );
		void readImageOutput( size_t width, size_t height, float* outputTarget );
		void setKernelArg( cl_kernel kernel, uint index, size_t size, void* data );
		cl_mem updateBuffer( cl_mem buffer, size_t size, void* data );
		cl_mem updateImageReadOnly( size_t width, size_t height, float* data );

	protected:
		void buildProgram();
		bool checkError( cl_int err, const char* functionName );
		const char* errorCodeToName( cl_int errorCode );
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
		cl_mem mReadImage;
		cl_mem mWriteImage;

		std::vector<cl_kernel> mKernels;
		std::vector<cl_event> mEvents;
		std::vector<cl_mem> mMemObjects;

};

#endif
