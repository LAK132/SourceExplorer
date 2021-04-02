set BINDIR=bin
set OBJDIR=obj

set INCDIRS=include include\SDL include\imgui include\imgui\misc\cpp include\lisk include\lak\inc include\lak2 include\lisk\inc include\glm
set LIBDIR=lib
set LIBS=SDL2main.lib SDL2.lib

set SOURCE=src\all.cpp
set BINARY=srcexp.exe

@REM set CXX=cl /nologo /std:c++17 /MT /EHsc /Zc:__cplusplus /Zc:rvalueCast /Zc:wchar_t /Zc:ternary Gdi32.lib Opengl32.lib /DLAK_USE_WINAPI /D_CRT_SECURE_NO_WARNINGS /DNOMINMAX /DUNICODE
set CXX=cl /nologo /std:c++17 /MT /EHsc /Zc:__cplusplus /Zc:rvalueCast /Zc:wchar_t /Zc:ternary /DLAK_USE_SDL /D_CRT_SECURE_NO_WARNINGS /DNOMINMAX /DUNICODE

if "%mode%"=="debug" goto debug
if "%mode%"=="release" goto release
if "%mode%"=="nolog" goto nolog
goto :eof

:debug
set COMPFLAGS=/Zi /bigobj /Od
set LINKFLAGS=/SUBSYSTEM:CONSOLE /DEBUG
goto :eof

:release
set COMPFLAGS=/DNDEBUG /bigobj /O2
set LINKFLAGS=/SUBSYSTEM:CONSOLE
goto :eof

:nolog
set COMPFLAGS=/DNOLOG /DNDEBUG /bigobj /O2
set LINKFLAGS=/SUBSYSTEM:CONSOLE
