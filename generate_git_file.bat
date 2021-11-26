@echo off
for /f %%f in ('git rev-parse --short HEAD') do (set "git_hash=%%f")
for /f %%f in ('git describe --tags --always --abbrev^=0') do (set "git_tag=%%f")
(
echo #ifndef GIT_HASH
echo #define GIT_HASH "%git_hash%"
echo #endif
echo #ifndef GIT_TAG
echo #define GIT_TAG "%git_tag%"
echo #endif
)>%1 || exit 1
