#!/usr/bin/env bash

cd $(dirname "$0")

# --gen-suppressions=all
valgrind --tool=memcheck --leak-check=full --show-reachable=yes --suppressions=valgrind.supp --log-file=valgrind.log ../PBR $@
