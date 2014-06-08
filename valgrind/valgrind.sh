#!/bin/bash

# --gen-suppressions=all

valgrind --tool=memcheck --leak-check=full --show-reachable=yes --suppressions=./valgrind/valgrind.supp --log-file=./valgrind/valgrind.log ./PBR $@
