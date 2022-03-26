#! /bin/sh
rm -rf build
case $1 in
  clang)
    shift
    CC=clang CXX=clang++ meson setup build $@ || exit 1
  ;;

  gcc)
    shift
    CC=gcc CXX=g++ meson setup build $@ || exit 1
  ;;

  msvc)
    shift
    meson setup build --vsenv $@ || exit 1
  ;;

  *)
    echo "./setup.sh [compiler] <setup args>"
    echo "examples:"
    echo "./setup.sh msvc"
    echo "./setup.sh msvc --buildtype release"
    echo "./setup.sh gcc --buildtype debug"
    echo "./setup.sh clang --buildtype debugoptimized"
    exit 1
  ;;
esac
