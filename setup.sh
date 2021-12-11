#! /bin/sh
rm -rf build
case $1 in
  clang)
    shift
    CC=clang CXX=clang++ meson setup build $@
  ;;

  gcc)
    shift
    CC=gcc CXX=g++ meson setup build $@
  ;;

  msvc)
    shift
    meson setup build --vsenv $@
  ;;

  *)
    echo "./setup.sh [compiler] <setup args>"
    echo "examples:"
    echo "./setup.sh msvc"
    echo "./setup.sh msvc --buildtype release"
    echo "./setup.sh gcc --buildtype debug"
    echo "./setup.sh clang --buildtype debugoptimised"
  ;;
esac
