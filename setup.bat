@echo off
SetLocal EnableDelayedExpansion

if "%~1"=="msvc" (
  set meson_args=--vsenv
  goto run
)

if "%~1"=="clang" (
  set CC=clang
  set CXX=clang++
  goto run
)

if "%~1"=="gcc" (
  set CC=gcc
  set CXX=g++
  goto run
)

goto usage

:run
shift
if not "%~1"=="--reconfigure" (
  rmdir /s /q build
)

:arg-loop
if "%1"=="" goto end-arg-loop
set meson_args=!meson_args! %1
shift
goto arg-loop
:end-arg-loop

meson setup build !meson_args!
goto :eof

:usage
echo setup.bat [compiler] ^<setup args^>
echo examples:
echo setup.bat msvc
echo setup.bat msvc --buildtype release
echo setup.bat gcc --buildtype debug
echo setup.bat clang --buildtype debugoptimized
goto :eof
