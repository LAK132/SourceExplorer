set BINDIR=bin
set OBJDIR=obj

set INCDIRS=include include\SDL include\imgui include\imgui\misc\cpp
set LIBDIR=lib
set LIBS=SDL2main.lib SDL2.lib

set SOURCE=src\main.cpp
set BINARY=app.exe

set CXX=cl /nologo /std:c++17 /D_CRT_SECURE_NO_WARNINGS /MD /EHsc

if "%mode%"=="clean" goto :eof

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" %1 /vcvars_ver=14.20

if "%mode%"=="release" goto release
if "%mode%"=="debug" goto debug

:debug
set COMPFLAGS=/Zi /bigobj
set LINKFLAGS=/SUBSYSTEM:CONSOLE /DEBUG
goto :eof

:release
set COMPCOM=/DNDEBUG /bigobj
set LINKCOM=/SUBSYSTEM:CONSOLE
