@echo off
meson install -C build %* || exit /b 1
