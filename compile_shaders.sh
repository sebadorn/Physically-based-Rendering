#!/usr/bin/env bash

cd $(dirname "$0")

SHADER_DIR='src/shaders'

${VULKAN_SDK}/bin/glslangValidator -V ${SHADER_DIR}/vertex.vert -o ${SHADER_DIR}/vert.spv
${VULKAN_SDK}/bin/glslangValidator -V ${SHADER_DIR}/fragment.frag -o ${SHADER_DIR}/frag.spv
${VULKAN_SDK}/bin/glslangValidator -V ${SHADER_DIR}/compute.comp.glsl -o ${SHADER_DIR}/compute.spv
