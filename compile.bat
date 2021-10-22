@echo off
SetLocal EnableDelayedExpansion

call msvc.bat %~1

cd build || exit 1
meson compile || exit 1
