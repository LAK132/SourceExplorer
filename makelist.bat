set BINDIR=bin
set OBJDIR=obj

set INCDIRS=include include\SDL include\imgui include\imgui\misc\cpp
set LIBDIR=lib
set LIBS=SDL2main.lib SDL2.lib

set SOURCE=src\main.cpp
set BINARY=explorer.exe

set CXX=cl /nologo /std:c++17 /D_CRT_SECURE_NO_WARNINGS /MD /EHsc

if "%mode%"=="release" goto release
if "%mode%"=="debug" goto debug
goto :eof

:debug
set COMPFLAGS=/Zi /bigobj /O2
set LINKFLAGS=/SUBSYSTEM:CONSOLE /DEBUG
goto :eof

:release
set COMPCOM=/DNDEBUG /bigobj /O2
set LINKCOM=/SUBSYSTEM:CONSOLE
