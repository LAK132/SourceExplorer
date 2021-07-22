@echo off

reg Query "HKLM\Hardware\Description\System\CentralProcessor\0" | find /i "x86" > NUL && set target=x86 || set target=x64

if exist "vcvarsall.bat" (
    call "vcvarsall.bat" %target%
    goto :eof
)
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" %target%
    goto :eof
)
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" %target%
    goto :eof
)
set /p VCVARSALLBAT=vcvarsall.bat directory:
call "%VCVARSALLBAT%" %target%

echo error: cannot find vcvarsall.bat
exit 1
