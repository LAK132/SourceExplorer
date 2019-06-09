set BINDIR=bin
set OBJDIR=obj

set INCDIRS=include include\SDL include\imgui include\imgui\misc\cpp
set LIBDIR=lib
set LIBS=SDL2main.lib SDL2.lib

set SOURCE=src\main.cpp
set BINARY=explorer.exe

set CXX=cl /nologo /std:c++17 /D_CRT_SECURE_NO_WARNINGS /MD /EHsc

if "%mode%"=="clean" goto :eof

if exist "vcvarsall.bat" (
    call "vcvarsall.bat" %1
    goto mode
)
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" %1
    goto mode
)
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Preview\VC\Auxiliary\Build\vscarsall.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Preview\VC\Auxiliary\Build\vscarsall.bat" %1
    goto mode
)
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" %1
    goto mode
)
set /p VCVARSALLBAT=vcvarsall.bat directory:
call "%VCVARSALLBAT%" %1


:mode
if "%mode%"=="release" goto release
if "%mode%"=="debug" goto debug

:debug
set COMPFLAGS=/Zi /bigobj /O2
set LINKFLAGS=/SUBSYSTEM:CONSOLE /DEBUG
goto :eof

:release
set COMPCOM=/DNDEBUG /bigobj /O2
set LINKCOM=/SUBSYSTEM:CONSOLE
