## ToDo

* `Feature` Specular highlights
* `Feature` Dynamic lights (for now: number)
* `Improve` Normals are only supported per face, but not per vertex
* `Improve` Surface Area Heuristic (SAH) for kd-tree
* `Improve` Can currently only handle single mesh models
* `Improve` Change width and height while application is running


## Optimization notes and ideas

* Trade space for speed
* Trade accuracy/error rate for speed
* OpenCL code is just a string that can be changed and recompiled
* OpenGL shaders are just strings that can be changed and recompiled
* Are some tasks in OpenCL better done on CPU than GPU?
* Is clFinish() always necessary where I used it?
* Access array elements in order
* OpenCL: local memory is much much faster than global memory
* OpenCL: texture memory is much faster than global memory
* OpenCL: constant memory is faster than global memory, but also much smaller


## Libraries/headers

* freeglut3-dev
* libboost-dev
* libdevil-dev
* libglew-dev
* libglm-dev
* libqt4-dev
* libxi-dev
* libxmu-dev
* opencl-headers


## Build and execute

    cmake -DCMAKE_BUILD_TYPE=Release CMakeLists.txt
    make
    ./PBR


## Observations

* For a 512x512 image a workgroupsize of 256 is infinitesimal better than 512, while a workgroupsize of 128 is worse than 512.


## Annoyances

* NVIDIA only supports OpenCL 1.1. OpenCL 1.2 support seems unlikely at the moment.
* The OpenCL code in this project will probably need some extra adjustments to make it work with Intel's OpenCL SDK.
* Assimp is a feature-rich import library, however it doesn't seem possible to disable its habit of duplicating vertices on import.
