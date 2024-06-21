#!/bin/bash

set -e

compiler=g++
executable="wika"

compiler_flags="-O0 -fPIC -Wall -Wextra"
linker_flags="-pthread -ldl -lm"
if [ "$1" == "release" ]
then
  echo "building Wika in release mode..."
else
  echo "building Wika in debug mode..."
  compiler_flags+=" -g -ggdb"
fi

working_path=$(pwd)
build_path="$working_path/build"
data_path="$working_path/data"
code_path="$working_path/code"
[ -d $build_path ] || mkdir -p $build_path
[ -d $data_path ] || mkdir -p $data_path

# build the executable
$compiler $compiler_flags $code_path/$executable.cpp -o $build_path/$executable $linker_flags
