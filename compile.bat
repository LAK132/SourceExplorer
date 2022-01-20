@echo off
meson compile -C build %* || exit 1
