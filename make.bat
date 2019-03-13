@echo off
SetLocal EnableDelayedExpansion

set mode=%1
set target=%2

if not "%mode%"=="debug" if not "%mode%"=="release" if not "%mode%"=="clean" goto useage
if not "%target%"=="x86" if not "%target%"=="x64" if not "%mode%"=="clean" goto useage

call makelist.bat %target% %mode%

if "%mode%"=="clean" goto clean

echo Compiling in %mode% mode for %target%

title Compiler

REM some windows functions are pedantic about \
set OBJDIR=!OBJDIR!\%mode%\%target%
set LIBDIR=!LIBDIR!\%target%

if not exist %OBJDIR% mkdir %OBJDIR%
if not exist %BINDIR% mkdir %BINDIR%

if "%mode%"=="debug" goto run
if "%mode%"=="release" goto run

:useage
echo compile: "make [debug/release] [x86/x64]"
echo clean: "make clean"
goto :eof

:clean
for /f %%F in ('dir /b %OBJDIR%') do (
    if "%%~xF"==".obj" del %OBJDIR%\%%F
)
goto :eof

:run
set _LIBS_=
for %%L in (%LIBS%) do (set _LIBS_=!_LIBS_! %LIBDIR%\%%L)

set _INC_=
for %%I in (%INCDIRS%) do (set _INC_=!_INC_! /I%%I)

call %CXX% %COMPFLAGS% /Fo:%OBJDIR%\ /Fe:%BINDIR%\%BINARY% %SOURCE% !_LIBS_! !_INC_! /link %LINKFLAGS%

for /f %%F in ('dir /b %LIBDIR%') do (if "%%~xF"==".dll" echo f | xcopy /y %LIBDIR%\%%F %BINDIR%\%%F)

EndLocal