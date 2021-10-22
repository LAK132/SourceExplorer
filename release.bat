@echo off
SetLocal EnableDelayedExpansion

rmdir /s /q release
mkdir release

SetLocal
rmdir /s /q build
call msvc.bat x86 || exit 1
meson setup build --buildtype release || exit 1
meson install -C build --destdir %cd%\release || exit 1
EndLocal

SetLocal
rmdir /s /q build
call msvc.bat x64 || exit 1
meson setup build --buildtype release || exit 1
meson install -C build --destdir %cd%\release || exit 1
EndLocal
