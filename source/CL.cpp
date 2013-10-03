#include "CL.h"

using namespace std;


/**
 * Constructor.
 */
CL::CL() {
	this->getDefaultPlatform();
	this->getDefaultDevice();
	this->initCommandQueue();

	this->loadProgram( Cfg::get().value<string>( Cfg::OPENCL_PROGRAM ) );
}


/**
 * Deconstructor.
 */
CL::~CL() {
	if( mKernel ) { clReleaseKernel( mKernel ); }
	if( mProgram ) { clReleaseProgram( mProgram ); }
	if( mCommandQueue ) { clReleaseCommandQueue( mCommandQueue ); }
	if( mContext ) { clReleaseContext( mContext ); }
}


/**
 * Check for an CL related error and log it.
 * @param  {cl_int}      err          Error code.
 * @param  {const char*} functionName Name of the function it occured in.
 * @return {bool}                     True, if no error occured, false otherwise.
 */
bool CL::checkError( cl_int err, const char* functionName ) {
	if( err != CL_SUCCESS ) {
		char msg[60];
		snprintf( msg, 60, "[OpenCL] %s error code %d", functionName, (int) err );
		Logger::logError( msg );

		return false;
	}

	return true;
}


/**
 * Get the default device of the platform.
 */
void CL::getDefaultDevice() {
	char* value;
	size_t valueSize;
	cl_uint deviceCount;
	cl_device_id* devices;

	clGetDeviceIDs( mPlatform, CL_DEVICE_TYPE_ALL, 0, NULL, &deviceCount );

	if( deviceCount < 1 ) {
		Logger::logError( "[OpenCL] No devices found." );
		exit( 1 );
	}

	devices = new cl_device_id[deviceCount];
	clGetDeviceIDs( mPlatform, CL_DEVICE_TYPE_ALL, deviceCount, devices, NULL );

	clGetDeviceInfo( devices[0], CL_DEVICE_NAME, 0, NULL, &valueSize );
	value = (char*) malloc( valueSize );
	clGetDeviceInfo( devices[0], CL_DEVICE_NAME, valueSize, value, NULL );

	Logger::logInfo( string( "[OpenCL] Using device " ).append( value ) );
	free( value );


	this->initContext( devices );

	mDevice = devices[0];

	delete devices;
}


/**
 * Get the default platform of the system.
 */
void CL::getDefaultPlatform() {
	char* value;
	size_t valueSize;
	cl_uint platformCount;
	cl_platform_id* platforms;

	clGetPlatformIDs( 0, NULL, &platformCount );

	if( platformCount < 0 ) {
		Logger::logError( "[OpenCL] No platforms found." );
		exit( 1 );
	}

	platforms = new cl_platform_id[platformCount];
	clGetPlatformIDs( platformCount, platforms, NULL );

	clGetPlatformInfo( platforms[0], CL_PLATFORM_NAME, 0, NULL, &valueSize );
	value = (char*) malloc( valueSize );
	clGetPlatformInfo( platforms[0], CL_PLATFORM_NAME, valueSize, value, NULL );

	Logger::logInfo( string( "[OpenCL] Using platform " ).append( value ) );
	free( value );

	mPlatform = platforms[0];

	delete platforms;
}


/**
 * Initialise the OpenCL context.
 * @param {cl_device_id*} devices List of all available devices.
 */
void CL::initContext( cl_device_id* devices ) {
	cl_int err;
	mContext = clCreateContext( 0, 1, devices, NULL, NULL, &err );

	this->checkError( err, "clCreateContext" );
}


/**
 * Initialise the OpenCL command queue.
 */
void CL::initCommandQueue() {
	cl_int err;
	mCommandQueue = clCreateCommandQueue( mContext, mDevice, 0, &err );

	this->checkError( err, "clCreateCommandQueue" );
}


/**
 * Load a program.
 * @param {string} filepath Path to the CL code file.
 */
void CL::loadProgram( string filepath ) {
	string clProgramString = utils::loadFileAsString( filepath.c_str() );
	const char* clProgramChar = clProgramString.c_str();
	const size_t clProgramLength = clProgramString.size();

	cl_int err;
	mProgram = clCreateProgramWithSource( mContext, 1, &clProgramChar, &clProgramLength, &err );

	if( this->checkError( err, "clCreateProgramWithSource" ) ) {
		Logger::logInfo( string( "[OpenCL] Loaded program " ).append( filepath ) );
	}
}
