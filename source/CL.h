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
		CL( QGLWidget* parent );
		~CL();
		cl_mem createBuffer( float object );
		cl_mem createBuffer( float* object, size_t objectSize );
		template<typename T> cl_mem createBuffer( std::vector<T> object, size_t objectSize ) {
			cl_int err;
			cl_mem clBuffer = clCreateBuffer( mContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, objectSize, &object[0], &err );
			this->checkError( err, "clCreateBuffer" );

			return clBuffer;
		}
		// cl_mem createImageGL( GLuint textureID );
		cl_mem createImage( size_t width, size_t height, float* data, cl_mem_flags flags );
		void createKernel( const char* functionName );
		void execute();
		void loadProgram( std::string filepath );
		void setKernelArgs( std::vector<cl_mem> buffers );

	protected:
		void buildProgram();
		bool checkError( cl_int err, const char* functionName );
		const char* errorCodeToName( cl_int errorCode );
		void getDefaultDevice();
		void getDefaultPlatform();
		void initCommandQueue();
		void initContext( cl_device_id* devices );

	private:
		QGLWidget* mParent;
		std::vector<cl_mem> mImages;
		cl_command_queue mCommandQueue;
		cl_context mContext;
		cl_device_id mDevice;
		cl_kernel mKernel;
		cl_platform_id mPlatform;
		cl_program mProgram;

};

#endif
