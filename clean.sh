#!/bin/bash

echo ""

make clean

rm moc_*.cpp
rm -r CMakeFiles
rm CMakeCache.txt
rm "cmake_install.cmake"

echo "  Cleaned it all."
echo ""
