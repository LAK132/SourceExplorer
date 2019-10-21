RMDIR /S /Q bin
RMDIR /S /Q release

MKDIR release

RMDIR /S /Q obj
call make release x64
RENAME bin x64
MOVE x64 release\x64

RMDIR /S /Q obj
call make release x86
RENAME bin x86
MOVE x86 release\x86
