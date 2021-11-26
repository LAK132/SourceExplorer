@echo off
SetLocal EnableDelayedExpansion

call msvc.bat %~1

meson compile -C build || exit 1
