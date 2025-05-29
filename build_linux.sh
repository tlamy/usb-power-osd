#!/usr/bin/env sh
ARCH=$(arch)
BUILDDIR="$(pwd)/build-linux-$ARCH"
cmake -B $BUILDDIR -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build $BUILDDIR --config "Unix Makefiles" -j 8
