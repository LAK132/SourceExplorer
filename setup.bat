@echo off
SetLocal EnableDelayedExpansion

call msvc.bat

rmdir /s /q build
meson setup build --buildtype release || exit 1
@REM meson setup build --buildtype debug || exit 1
