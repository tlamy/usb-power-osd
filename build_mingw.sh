#!/usr/bin/env sh
BUILD_DIR="$(pwd)/build-windows-mingw"
BUILD_TYPE="Release"
cmake -B "$BUILD_DIR" \
    -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DCMAKE_SYSTEM_NAME=Windows \
    -DCMAKE_CROSSCOMPILING=TRUE \
    -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
    -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
    -DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres \

cmake --build "$BUILD_DIR" --config $BUILD_TYPE -j 12

