@echo off
SetLocal EnableDelayedExpansion

call msvc.bat

cd build || exit 1
meson compile || exit 1
