#! /bin/sh
rm -rf build
CC=clang-13 CXX=clang++-13 meson setup build --buildtype release || exit 1
# CC=clang-13 CXX=clang++-13 meson setup build --buildtype debug || exit 1
# CC=clang-13 CXX=clang++-13 meson setup build --buildtype debugoptimised || exit 1
