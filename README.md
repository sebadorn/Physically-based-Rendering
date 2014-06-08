## Libraries/headers

* freeglut3-dev
* libboost-dev
* libglew-dev
* libglm-dev
* libqt4-dev
* libxi-dev
* libxmu-dev
* opencl-headers
* libdevil-dev (currently not needed, maybe in the future for texture support)


## Build and execute

    cmake -DCMAKE_BUILD_TYPE=Release CMakeLists.txt
    make
    ./PBR


## Annoyances

* NVIDIA only supports OpenCL 1.1. OpenCL 1.2 support seems unlikely at the moment.
* The OpenCL code in this project will probably need some extra adjustments to make it work with Intel's OpenCL SDK.
* Assimp is a feature-rich import library, however it doesn't seem possible to disable its habit of duplicating vertices on import.
