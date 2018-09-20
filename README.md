# Physically-based Rendering

Rendering 3D scenes physically-based through Path Tracing. Interaction of light with surfaces is evaluated through bi-directional reflection distribution functions (BRDF). *— Master Thesis 2013/14*


## Vulkan branch

**Not in a working state.** All it shows currently is a ImGui window, but no model rendering is done.


## Acceleration structure: BVH

* Stackless traversal.
* 1 or 2 faces per leaf node.


## Requirements

* **OS:** Linux  
*Though it should be possible to build and run it on other platforms as well. I just never tried it.*
* **Hardware:** GPU with Vulkan capabilities  
*Tested only with NVIDIA.*


### Libraries/headers

* GLFW: https://github.com/glfw/glfw
* imgui: https://github.com/ocornut/imgui
* libboost-dev
* libdevil-dev
* libglm-dev
* libxi-dev
* libxmu-dev
* LunarG Vulkan SDK: https://vulkan.lunarg.com/sdk/home


## Build and execute

    cmake -DCMAKE_BUILD_TYPE=Release CMakeLists.txt
    make
    ./PBR
