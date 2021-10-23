@echo off
SetLocal EnableDelayedExpansion

call msvc.bat %~1

rmdir /s /q build
@REM meson setup build --buildtype release || exit 1
meson setup build --buildtype debug || exit 1
@REM meson setup build --buildtype debugoptimized || exit 1
