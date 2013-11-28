#include "CL.h"

using std::string;
using std::vector;


/**
 * Constructor.
 */
CL::CL() {
	mCommandQueue = NULL;
	mContext = NULL;
	mDevice = NULL;
	mPlatform = NULL;
	mProgram = NULL;

	this->getDefaultPlatform();
	this->getDefaultDevice();
	this->initCommandQueue();
}


/**
 * Deconstructor.
 */
CL::~CL() {
	cl_int err;

	for( uint i = 0; i < mMemObjects.size(); i++ ) {
		err = clReleaseMemObject( mMemObjects[i] );
		this->checkError( err, "clReleaseMemObject" );
	}

	for( uint i = 0; i < mKernels.size(); i++ ) {
		clReleaseKernel( mKernels[i] );
	}

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
 * @param  {cl_int}      err          Error code.
 * @param  {const char*} functionName Name of the function it occured in.
 * @return {bool}                     True, if no error occured, false otherwise.
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
 * Create an empty buffer that can be updated with data later.
 * @param  {size_t}       size  Size of the buffer.
 * @param  {cl_mem_flags} flags CL flags like CL_MEM_READ_ONLY.
 * @return {cl_mem}             Handle for the buffer.
 */
cl_mem CL::createEmptyBuffer( size_t size, cl_mem_flags flags ) {
	cl_int err;
	cl_mem buffer = clCreateBuffer( mContext, flags, size, NULL, &err );
	this->checkError( err, "clCreateBuffer" );
	mMemObjects.push_back( buffer );

	return buffer;
}


/**
 * Create a buffer for a 2D image that will be read-only in the OpenCL context, and fill it with data.
 * @param  {size_t}    width  Width of the image.
 * @param  {size_t}    height Height of the image.
 * @param  {cl_float*} data   Data of the image.
 * @return {cl_mem}           Handle for the buffer.
 */
cl_mem CL::createImageReadOnly( size_t width, size_t height, cl_float* data ) {
	cl_int err;
	cl_image_format format;

	format.image_channel_order = CL_RGBA;
	format.image_channel_data_type = CL_FLOAT;

	mReadImage = clCreateImage2D( mContext, CL_MEM_READ_ONLY, &format, width, height, 0, NULL, &err );
	this->checkError( err, "clCreateImage2D" );


	size_t origin[] = { 0, 0, 0 }; // Defines the offset in pixels in the image from where to write.
	size_t region[] = { width, height, 1 }; // Size of object to be transferred
	cl_event event;

	err = clEnqueueWriteImage( mCommandQueue, mReadImage, CL_TRUE, origin, region, 0, 0, data, mEvents.size(), &mEvents[0], &event );
	this->checkError( err, "clEnqueueWriteImage" );
	mEvents.push_back( event );

	return mReadImage;
}


/**
 * Create a buffer for a 2D image that will be write-only in the OpenCL context.
 * @param  {size_t} width  Width of the image.
 * @param  {size_t} height Height of the image.
 * @return {cl_mem}        Handle for the buffer.
 */
cl_mem CL::createImageWriteOnly( size_t width, size_t height ) {
	cl_int err;
	cl_image_format format;

	format.image_channel_order = CL_RGBA;
	format.image_channel_data_type = CL_FLOAT;

	mWriteImage = clCreateImage2D( mContext, CL_MEM_READ_WRITE, &format, width, height, 0, NULL, &err );
	this->checkError( err, "clCreateImage2D" );

	return mWriteImage;
}


/**
 * Create a kernel.
 * @param  {const char*} functionName Name of the function to create kernel of.
 * @return {cl_kernel}                The created kernel.
 */
cl_kernel CL::createKernel( const char* functionName ) {
	cl_int err;
	cl_kernel kernel = clCreateKernel( mProgram, functionName, &err );

	if( !this->checkError( err, "clCreateKernel" ) ) {
		exit( EXIT_FAILURE );
	}

	mKernels.push_back( kernel );
	mKernelNames[kernel] = string( functionName );

	return kernel;
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
		"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
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


/**
 * Execute a kernel.
 * @param {cl_kernel} kernel Handle of the kernel to execute.
 */
void CL::execute( cl_kernel kernel ) {
	cl_int err;
	cl_event event;

	cl_uint w = Cfg::get().value<cl_uint>( Cfg::WINDOW_WIDTH );
	cl_uint h = Cfg::get().value<cl_uint>( Cfg::WINDOW_HEIGHT );
	cl_uint wgs = Cfg::get().value<cl_uint>( Cfg::OPENCL_WORKGROUPSIZE );

	cl_uint offsetH;
	cl_uint offsetW = 0;

	double avgKernelTime = 0.0;
	int i = 0;

	while( offsetW < w ) {
		offsetH = 0;

		while( offsetH < h ) {
			this->setKernelArg( kernel, 0, sizeof( cl_uint ), &w );
			this->setKernelArg( kernel, 1, sizeof( cl_uint ), &h );
			this->setKernelArg( kernel, 2, sizeof( cl_uint ), &offsetW );
			this->setKernelArg( kernel, 3, sizeof( cl_uint ), &offsetH );

			size_t workSize[3] = {
				std::min( w - offsetW, wgs ),
				std::min( h - offsetH, wgs ),
				1
			};
			err = clEnqueueNDRangeKernel( mCommandQueue, kernel, 2, NULL, workSize, NULL, mEvents.size(), &mEvents[0], &event );
			this->checkError( err, "clEnqueueNDRangeKernel" );
			mEvents.push_back( event );

			avgKernelTime += this->getKernelExecutionTime( event );
			i++;

			offsetH += wgs;
		}

		offsetW += wgs;
	}

	mKernelTime[kernel] = avgKernelTime / i;
}


/**
 * Finish a kernel execution by flushing the command queue and clearing all events.
 */
void CL::finish() {
	clFlush( mCommandQueue );
	clFinish( mCommandQueue );
	mEvents.clear();
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

	mDevice = devices[0];


	// Get device name
	for( int i = deviceCount - 1; i >= 0; i-- ) {
		clGetDeviceInfo( devices[i], CL_DEVICE_NAME, 0, NULL, &valueSize );
		value = (char*) malloc( valueSize );
		clGetDeviceInfo( devices[i], CL_DEVICE_NAME, valueSize, value, NULL );

		if( i == 0 ) {
			Logger::logInfo( string( "[OpenCL] Using device " ).append( value ) );
		}
		else {
			Logger::logDebug( string( "[OpenCL] Found device " ).append( value ) );
		}
		free( value );
	}


	this->initContext( devices );
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

	for( int i = platformCount - 1; i >= 0; i-- ) {
		clGetPlatformInfo( platforms[i], CL_PLATFORM_NAME, 0, NULL, &valueSize );
		value = (char*) malloc( valueSize );
		clGetPlatformInfo( platforms[i], CL_PLATFORM_NAME, valueSize, value, NULL );

		if( i == 0 ) {
			Logger::logInfo( string( "[OpenCL] Using platform " ).append( value ) );
		}
		else {
			Logger::logDebug( string( "[OpenCL] Found platform " ).append( value ) );
		}
		free( value );
	}

	mPlatform = platforms[0];

	delete platforms;
}


/**
 * Returns the kernel execution time in milliseconds.
 * @return {double} Time it took to execute the kernel in milliseconds.
 */
double CL::getKernelExecutionTime( cl_event kernelEvent ) {
	cl_ulong timeEnd, timeStart;

	clWaitForEvents( 1, &kernelEvent );
	clGetEventProfilingInfo( kernelEvent, CL_PROFILING_COMMAND_START, sizeof( timeStart ), &timeStart, NULL );
	clGetEventProfilingInfo( kernelEvent, CL_PROFILING_COMMAND_END, sizeof( timeEnd ), &timeEnd, NULL );

	return (double) ( timeEnd - timeStart ) / 1000000.0f;
}


/**
 * The a map of kernel IDs to kernel names.
 * @return {std::map<cl_kernel, std::string>} Kernel IDs to names.
 */
map<cl_kernel, string> CL::getKernelNames() {
	return mKernelNames;
}


/**
 * Get the last profiled kernel execution times.
 * @return {std::map<cl_kernel, double>} Map of kernel ID to execution time [ms].
 */
map<cl_kernel, double> CL::getKernelTimes() {
	return mKernelTime;
}


/**
 * Initialise the OpenCL context.
 * @param {cl_device_id*} devices List of all available devices.
 */
void CL::initContext( cl_device_id* devices ) {
	cl_int err;
	cl_context_properties properties[] = { CL_CONTEXT_PLATFORM, (cl_context_properties) mPlatform, 0 };
	mContext = clCreateContext( properties, 1, devices, NULL, NULL, &err );

	if( !this->checkError( err, "clCreateContext" ) ) {
		exit( EXIT_FAILURE );
	}
}


/**
 * Initialise the OpenCL command queue.
 */
void CL::initCommandQueue() {
	cl_int err;
	mCommandQueue = clCreateCommandQueue( mContext, mDevice, CL_QUEUE_PROFILING_ENABLE, &err );

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
	clProgramString = this->setValues( clProgramString );

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
 * Read the content of an image buffer.
 * @param {cl_mem}    image        Handle to the image buffer.
 * @param {size_t}    width        Width of the image.
 * @param {size_t}    height       Height of the image.
 * @param {cl_float*} outputTarget Write target for the image data.
 */
void CL::readImageOutput( cl_mem image, size_t width, size_t height, cl_float* outputTarget ) {
	cl_int err;
	cl_event event;
	size_t origin[] = { 0, 0, 0 };
	size_t region[] = { width, height, 1 };

	err = clEnqueueReadImage( mCommandQueue, image, CL_TRUE, origin, region, 0, 0, outputTarget, mEvents.size(), &mEvents[0], &event );
	this->checkError( err, "clEnqueueReadImage" );
	mEvents.push_back( event );
}


/**
 * Set a kernel argument.
 * @param {cl_kernel} kernel Kernel handle to set the argument for.
 * @param {cl_uint}   index  Index of the argument.
 * @param {size_t}    size   Size of the data.
 * @param {void*}     data   A pointer to the data.
 */
void CL::setKernelArg( cl_kernel kernel, cl_uint index, size_t size, void* data ) {
	cl_int err = clSetKernelArg( kernel, index, size, data );
	this->checkError( err, "clSetKernelArg" );
}


/**
 * Replace placeholder text in the OpenCL (*.cl) file with the appropiate values
 * set or indicated in the config.
 * @param  {std::string} clProgramString The original file content.
 * @return {std::string}                 The adjusted file content.
 */
string CL::setValues( string clProgramString ) {
	// Placeholder names in the OpenCL file
	vector<string> boolReplace;
	boolReplace.push_back( "BACKFACE_CULLING" );
	boolReplace.push_back( "SHADOWS" );

	// Boolean values from the config
	vector<bool> configValue;
	configValue.push_back( Cfg::get().value<bool>( Cfg::RENDER_BACKFACECULLING ) );
	configValue.push_back( Cfg::get().value<bool>( Cfg::RENDER_SHADOWS ) );

	string search, value;
	size_t foundStrPos;

	for( int i = 0; i < boolReplace.size(); i++ ) {
		search = "#" + boolReplace[i] + "#";
		foundStrPos = clProgramString.find( search );

		if( foundStrPos != string::npos ) {
			value = configValue[i] ? "#define " + boolReplace[i] + " 1" : "";
			clProgramString.replace( foundStrPos, search.length(), value );
		}
	}

	return clProgramString;
}


/**
 * Update the data of a buffer.
 * @param  {cl_mem} buffer Handle of the buffer.
 * @param  {size_t} size   Size of the data to write into it.
 * @param  {void*}  data   Pointer to the data.
 * @return {cl_mem}        Handle of the buffer.
 */
cl_mem CL::updateBuffer( cl_mem buffer, size_t size, void* data ) {
	cl_int err = clEnqueueWriteBuffer( mCommandQueue, buffer, CL_TRUE, 0, size, data, 0, NULL, NULL );
	this->checkError( err, "clEnqueueWriteBuffer" );

	return buffer;
}


/**
 * Update the image data of a read-only buffer in OpenCL context.
 * @param  {cl_mem}    image  Handle of the buffer.
 * @param  {size_t}    width  Width of the image.
 * @param  {size_t}    height Height of the image.
 * @param  {cl_float*} data   Data to write into the image.
 * @return {cl_mem}           Handle of the buffer.
 */
cl_mem CL::updateImageReadOnly( cl_mem image, size_t width, size_t height, cl_float* data ) {
	size_t origin[] = { 0, 0, 0 };
	size_t region[] = { width, height, 1 };
	cl_int err;
	cl_event event;

	err = clEnqueueWriteImage( mCommandQueue, image, CL_TRUE, origin, region, 0, 0, data, mEvents.size(), &mEvents[0], &event );
	this->checkError( err, "clEnqueueReadImage" );
	mEvents.push_back( event );

	return image;
}
