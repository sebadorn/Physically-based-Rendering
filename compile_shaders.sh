#!/bin/bash

cd $(dirname "$0")

SHADER_DIR='source/shaders'
VK_SDK_DIR='../VulkanSDK/1.0.30.0'

${VK_SDK_DIR}/x86_64/bin/glslangValidator -V ${SHADER_DIR}/vertex.vert -o ${SHADER_DIR}/vert.spv
${VK_SDK_DIR}/x86_64/bin/glslangValidator -V ${SHADER_DIR}/fragment.frag -o ${SHADER_DIR}/frag.spv
