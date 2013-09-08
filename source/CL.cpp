#include <CL/cl.hpp>
#include <iostream>

#include "CL.h"


/**
 * Constructor.
 */
CL::CL() {
	mPlatform = this->getDefaultPlatform();
	mDevice = this->getDefaultDevice();
}


/**
 * Get the default device of the platform.
 * @return {cl_device_id} ID of the device.
 */
cl_device_id CL::getDefaultDevice() {
	char* value;
	size_t valueSize;
	cl_uint deviceCount;
	cl_device_id* devices;

	clGetDeviceIDs( mPlatform, CL_DEVICE_TYPE_ALL, 0, NULL, &deviceCount );

	if( deviceCount < 1 ) {
		std::cerr << "* [OpenCL] No devices found." << std::endl;
		exit( 1 );
	}

	devices = (cl_device_id*) malloc( sizeof( cl_device_id ) * deviceCount );
	clGetDeviceIDs( mPlatform, CL_DEVICE_TYPE_ALL, deviceCount, devices, NULL );

	clGetDeviceInfo( devices[0], CL_DEVICE_NAME, 0, NULL, &valueSize );
	value = (char*) malloc( valueSize );
	clGetDeviceInfo( devices[0], CL_DEVICE_NAME, valueSize, value, NULL );

	std::cout << "* [OpenCL] Using device " << value << std::endl;

	free( value );

	return devices[0];
}


/**
 * Get the default platform of the system.
 * @return {cl_platform_id} ID of the platform.
 */
cl_platform_id CL::getDefaultPlatform() {
	char* value;
	size_t valueSize;
	cl_uint platformCount;
	cl_platform_id* platforms;

	clGetPlatformIDs( 0, NULL, &platformCount );

	if( platformCount < 0 ) {
		std::cerr << "* [OpenCL] No platforms found." << std::endl;
		exit( 1 );
	}

	platforms = (cl_platform_id*) malloc( sizeof( cl_platform_id ) * platformCount );
	clGetPlatformIDs( platformCount, platforms, NULL );

	clGetPlatformInfo( platforms[0], CL_PLATFORM_NAME, 0, NULL, &valueSize );
	value = (char*) malloc( valueSize );
	clGetPlatformInfo( platforms[0], CL_PLATFORM_NAME, valueSize, value, NULL );

	std::cout << "* [OpenCL] Using platform " << value << std::endl;

	free( value );

	return platforms[0];
}
