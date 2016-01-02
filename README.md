# Physically-based Rendering

Rendering 3D scenes physically-based through Path Tracing. Interaction of light with surfaces is evaluated through bi-directional reflection distribution functions (BRDF). *— Master Thesis 2013/14*


## Acceleration structure: BVH

* Stackless traversal.
* 1 or 2 faces per leaf node.
* Code for use of spatial splits in build process exists, but seems faulty. Should not be used.


## Requirements

* **OS:** Linux  
*Though it should be possible to built and run it on other platforms as well. I just never tried it.*
* **Hardware:** GPU with OpenCL 1.1 and OpenGL 3.1 capabilities  
*Tested only with NVIDIA.*


### Libraries/headers

* freeglut3-dev
* libboost-dev
* libglew-dev
* libglm-dev
* libqt4-dev
* libxi-dev
* libxmu-dev
* opencl-headers
* libdevil-dev


### NVIDIA and OpenCL

If there are problems to get NVIDIA hardware to work with OpenCL (which is very likely in my experience), here is some trouble shooting:

* Check if you have this file: `/etc/OpenCL/vendors/nvidia.icd`  
The name doesn't really matter, but there has to be a file with the name or path of the NVIDIA OpenCL lib, e.g.: `/usr/lib/x86_64-linux-gnu/libnvidia-opencl.so.346.82` or just `libnvidia-opencl.so.1`  
Check which library files you have with `locate libnvidia-opencl.so`.
* I have the following installed and it works:

        libcg
        libcggl
        libcuda1-346-updates
        nvidia-cg-dev
        nvidia-cg-toolkit
        nvidia-346-updates
        nvidia-346-updates-uvm
        nvidia-common
        nvidia-modprobe
        nvidia-prime
        nvidia-opencl-icd-346-updates
        nvidia-settings
        ocl-icd-libopencl1
        opencl-headers


## Build and execute

    cmake -DCMAKE_BUILD_TYPE=Release CMakeLists.txt
    make
    ./PBR


## Notes

* NVIDIA only supports OpenCL 1.1. OpenCL 1.2 support seems unlikely at the moment.