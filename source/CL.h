#ifndef CL_H
#define CL_H

#include <CL/cl.hpp>


class CL {

	public:
		CL();

	protected:
		cl_device_id getDefaultDevice();
		cl_platform_id getDefaultPlatform();

	private:
		cl_device_id mDevice;
		cl_platform_id mPlatform;

};

#endif
