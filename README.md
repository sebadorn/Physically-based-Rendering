## ToDo

* `Feature` Anti-Aliasing


## Libraries/headers

* freeglut3
* libassimp-dev
* libboost-dev
* libdevil-dev
* libqt4-dev
* libglm-dev
* opencl-headers


## Build and execute

    cmake -DCMAKE_BUILD_TYPE=Release CMakeLists.txt
    make
    ./PBR


## Annoyances

* NVIDIA only supports OpenCL 1.1. OpenCL 1.2 support seems unlikely at the moment.
