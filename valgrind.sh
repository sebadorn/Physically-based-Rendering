#!/bin/bash

# --gen-suppressions=all

valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --suppressions=./valgrind.supp --log-file=./valgrind.log ./PBR $@
