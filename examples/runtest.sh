#!/bin/sh
#
# Run dynamically linked test.
#

PROG="$1"
shift

LD_LIBRARY_PATH=../src/ ./$PROG "$@"
