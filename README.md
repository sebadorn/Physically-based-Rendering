## ToDo

* `Improve` PT: color
* `Improve` PT: shadows

* `Improve` Use framebuffer
* `Improve` kd-tree or octree for intersection test
* `Improve` Keep work group sizes down (64-256), even for bigger textures (GPU limit at 1024)
* `Improve` Flexible width and height
* `Improve` Dynamic lights (for now: number and position)
* `Improve` Use CPU if no GPU available for OpenCL (or read from config)


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
