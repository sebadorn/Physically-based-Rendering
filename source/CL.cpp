#include "CL.h"

using namespace std;


/**
 * Constructor.
 */
CL::CL() {
	mCommandQueue = NULL;
	mContext = NULL;
	mDevice = NULL;
	mKernel = NULL;
	mPlatform = NULL;
	mProgram = NULL;

	this->getDefaultPlatform();
	this->getDefaultDevice();
	this->initCommandQueue();

	this->loadProgram( Cfg::get().value<string>( Cfg::OPENCL_PROGRAM ) );

	// cl_int err;
	// int num = 10;
	// float* a = new float[num];
	// float* b = new float[num];

	// for( int i = 0; i < num; i++ ) {
	// 	a[i] = 1.0f * i;
	// 	b[i] = 1.0f * i;
	// }

	// cl_mem cl_a = clCreateBuffer( mContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof( float ) * num, a, &err );
	// cl_mem cl_b = clCreateBuffer( mContext, CL_MEM_READ_ONLY, sizeof( float) * num, NULL, &err );
	// cl_mem cl_c = clCreateBuffer( mContext, CL_MEM_WRITE_ONLY, sizeof( float ) * num, NULL, &err );

	// cl_event event;
	// err = clEnqueueWriteBuffer( mCommandQueue, cl_b, CL_TRUE, 0, sizeof( float ) * num, b, 0, NULL , &event );
	// clReleaseEvent( event );
	// this->checkError( err, "clEnqueueWriteBuffer" );

	// err = clSetKernelArg( mKernel, 0, sizeof( cl_mem ), (void*) &cl_a );
	// err = clSetKernelArg( mKernel, 1, sizeof( cl_mem ), (void*) &cl_b );
	// err = clSetKernelArg( mKernel, 2, sizeof( cl_mem ), (void*) &cl_c );

	// clFinish( mCommandQueue );

	// delete [] a;
	// delete [] b;


	// size_t workgroupSize[1] = { num };
	// err = clEnqueueNDRangeKernel( mCommandQueue, mKernel, 1, NULL, workgroupSize, NULL, 0, NULL, &event );
	// clReleaseEvent( event );
	// this->checkError( err, "clEnqueueNDRangeKernel" );
	// clFinish( mCommandQueue );


	// float c_done[num];
	// err = clEnqueueReadBuffer( mCommandQueue, cl_c, CL_TRUE, 0, sizeof( float ) * num, &c_done, 0, NULL, &event );
	// clReleaseEvent( event );
	// this->checkError( err, "clEnqueueReadBuffer" );

	// for( int i = 0; i < num; i++ ) {
	// 	printf( "c_done[%d] = %g\n", i, c_done[i] );
	// }
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
 * Build the CL program.
 */
void CL::buildProgram() {
	cl_int err;

	err = clBuildProgram( mProgram, 0, NULL, NULL, NULL, NULL );
	this->checkError( err, "clBuildProgram" );

	cl_build_status buildStatus;
	err = clGetProgramBuildInfo( mProgram, mDevice, CL_PROGRAM_BUILD_STATUS, sizeof( cl_build_status ), &buildStatus, NULL );
	this->checkError( err, "clGetProgramBuildInfo/BUILD_STATUS" );

	size_t logSize;
	err = clGetProgramBuildInfo( mProgram, mDevice, CL_PROGRAM_BUILD_LOG, 0, NULL, &logSize );
	this->checkError( err, "clGetProgramBuildInfo/BUILD_LOG/size" );

	if( logSize > 2 ) {
		char* buildLog;
		err = clGetProgramBuildInfo( mProgram, mDevice, CL_PROGRAM_BUILD_LOG, logSize, buildLog, NULL );
		this->checkError( err, "clGetProgramBuildInfo/BUILD_LOG/text" );
		Logger::logError( buildLog, "" );
		exit( EXIT_FAILURE );
	}
}


/**
 * Check for an CL related error and log it.
 * @param  {cl_int}	  err		  Error code.
 * @param  {const char*} functionName Name of the function it occured in.
 * @return {bool}					 True, if no error occured, false otherwise.
 */
bool CL::checkError( cl_int err, const char* functionName ) {
	if( err != CL_SUCCESS ) {
		char msg[120];
		snprintf( msg, 120, "[OpenCL] Error in function %s: %s (code %d)", functionName, this->errorCodeToName( err ), (int) err );
		Logger::logError( msg );

		return false;
	}

	return true;
}


/**
 * Create a kernel.
 * @param {const char*} functionName Name of the function to create kernel of.
 */
void CL::createKernel( const char* functionName ) {
	if( mKernel ) {
		clReleaseKernel( mKernel );
	}

	cl_int err;
	mKernel = clCreateKernel( mProgram, functionName, &err );

	if( !this->checkError( err, "clCreateKernel" ) ) {
		exit( EXIT_FAILURE );
	}
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
		exit( EXIT_FAILURE );
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
		exit( EXIT_FAILURE );
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

	if( !this->checkError( err, "clCreateContext" ) ) {
		exit( EXIT_FAILURE );
	}
}


/**
 * Initialise the OpenCL command queue.
 */
void CL::initCommandQueue() {
	cl_int err;
	mCommandQueue = clCreateCommandQueue( mContext, mDevice, 0, &err );

	if( !this->checkError( err, "clCreateCommandQueue" ) ) {
		exit( EXIT_FAILURE );
	}
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

	if( !this->checkError( err, "clCreateProgramWithSource" ) ) {
		exit( EXIT_FAILURE );
	}

	Logger::logInfo( string( "[OpenCL] Loaded program " ).append( filepath ) );

	this->buildProgram();
}


/**
 * Return a more readable name to a given error code number.
 * Source: https://github.com/enjalot/adventures_in_opencl/blob/master/part1/util.cpp
 * @param  {cl_int}      errorCode Error code.
 * @return {const char*}           Error name.
 */
const char* CL::errorCodeToName( cl_int errorCode ) {
	static const char* errorString[] = {
		"CL_SUCCESS",
		"CL_DEVICE_NOT_FOUND",
		"CL_DEVICE_NOT_AVAILABLE",
		"CL_COMPILER_NOT_AVAILABLE",
		"CL_MEM_OBJECT_ALLOCATION_FAILURE",
		"CL_OUT_OF_RESOURCES",
		"CL_OUT_OF_HOST_MEMORY",
		"CL_PROFILING_INFO_NOT_AVAILABLE",
		"CL_MEM_COPY_OVERLAP",
		"CL_IMAGE_FORMAT_MISMATCH",
		"CL_IMAGE_FORMAT_NOT_SUPPORTED",
		"CL_BUILD_PROGRAM_FAILURE",
		"CL_MAP_FAILURE",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"CL_INVALID_VALUE",
		"CL_INVALID_DEVICE_TYPE",
		"CL_INVALID_PLATFORM",
		"CL_INVALID_DEVICE",
		"CL_INVALID_CONTEXT",
		"CL_INVALID_QUEUE_PROPERTIES",
		"CL_INVALID_COMMAND_QUEUE",
		"CL_INVALID_HOST_PTR",
		"CL_INVALID_MEM_OBJECT",
		"CL_INVALID_IMAGE_FORMAT_DESCRIPTOR",
		"CL_INVALID_IMAGE_SIZE",
		"CL_INVALID_SAMPLER",
		"CL_INVALID_BINARY",
		"CL_INVALID_BUILD_OPTIONS",
		"CL_INVALID_PROGRAM",
		"CL_INVALID_PROGRAM_EXECUTABLE",
		"CL_INVALID_KERNEL_NAME",
		"CL_INVALID_KERNEL_DEFINITION",
		"CL_INVALID_KERNEL",
		"CL_INVALID_ARG_INDEX",
		"CL_INVALID_ARG_VALUE",
		"CL_INVALID_ARG_SIZE",
		"CL_INVALID_KERNEL_ARGS",
		"CL_INVALID_WORK_DIMENSION",
		"CL_INVALID_WORK_GROUP_SIZE",
		"CL_INVALID_WORK_ITEM_SIZE",
		"CL_INVALID_GLOBAL_OFFSET",
		"CL_INVALID_EVENT_WAIT_LIST",
		"CL_INVALID_EVENT",
		"CL_INVALID_OPERATION",
		"CL_INVALID_GL_OBJECT",
		"CL_INVALID_BUFFER_SIZE",
		"CL_INVALID_MIP_LEVEL",
		"CL_INVALID_GLOBAL_WORK_SIZE"
	};

	const int errorCount = sizeof( errorString ) / sizeof( errorString[0] );
	const int index = -errorCode;

	return ( index >= 0 && index < errorCount ) ? errorString[index] : "";
}
