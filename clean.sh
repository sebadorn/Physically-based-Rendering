#!/bin/bash

echo ''

make clean

if [ -f 'make.log' ]; then
	rm 'make.log'
fi

if [ -d 'CMakeFiles' ]; then
	rm -r 'CMakeFiles'
fi

if [ -f 'CMakeCache.txt' ]; then
	rm 'CMakeCache.txt'
fi

if [ -f 'cmake_install.cmake' ]; then
	rm 'cmake_install.cmake'
fi

echo '  Cleaned it all.'
echo ''
