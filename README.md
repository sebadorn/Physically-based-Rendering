## ToDo

* `Improve` Normals are only supported per face, but not per vertex
* `Feature` PT: colors
* `Feature` PT: shadows
* `Feature` Dynamic lights (for now: number and position)
* `Improve` Surface fitting for kd-tree
* `Improve` Can currently only handle single mesh models
* `Improve` Keep work group sizes down (64-256), even for bigger textures (GPU limit at 1024)
* `Improve` Flexible width and height
* `Improve` Use CPU if no GPU available for OpenCL (or read from config)


## Optimization notes and ideas

* Trade space for speed
* Trade accuracy/error rate for speed
* OpenCL code is just a string that can be changed and recompiled
* OpenGL shaders are just strings that can be changed and recompiled
* Split OpenCL into multiply kernels for each (greater) task, like ray generation, ray traversal, intersection test, and so on
* Are some tasks in OpenCL better done on CPU than GPU?
* Is clFinish() always necessary where I used it?


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


## Annoyances

* NVIDIA only supports OpenCL 1.1. OpenCL 1.2 support seems unlikely at the moment.
* The OpenCL code in this project will probably need some extra adjustments to make it work with Intel's OpenCL SDK.
* Assimp is a feature-rich import library, however it doesn't seem possible to disable its habit of duplicating vertices on import.
