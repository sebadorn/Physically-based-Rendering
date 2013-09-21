#include "CL.h"

using namespace std;


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
		Logger::logError( "[OpenCL] No devices found." );
		exit( 1 );
	}

	devices = (cl_device_id*) malloc( sizeof( cl_device_id ) * deviceCount );
	clGetDeviceIDs( mPlatform, CL_DEVICE_TYPE_ALL, deviceCount, devices, NULL );

	clGetDeviceInfo( devices[0], CL_DEVICE_NAME, 0, NULL, &valueSize );
	value = (char*) malloc( valueSize );
	clGetDeviceInfo( devices[0], CL_DEVICE_NAME, valueSize, value, NULL );

	Logger::logInfo( string( "[OpenCL] Using device " ).append( value ) );
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
		Logger::logError( "[OpenCL] No platforms found." );
		exit( 1 );
	}

	platforms = (cl_platform_id*) malloc( sizeof( cl_platform_id ) * platformCount );
	clGetPlatformIDs( platformCount, platforms, NULL );

	clGetPlatformInfo( platforms[0], CL_PLATFORM_NAME, 0, NULL, &valueSize );
	value = (char*) malloc( valueSize );
	clGetPlatformInfo( platforms[0], CL_PLATFORM_NAME, valueSize, value, NULL );

	Logger::logInfo( string( "[OpenCL] Using platform " ).append( value ) );
	free( value );

	return platforms[0];
}
