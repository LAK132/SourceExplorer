@echo off
for /f %%f in ('git rev-parse --short HEAD') do (set "git_hash=%%f")
for /f %%f in ('git describe --tags --abbrev^=0') do (set "git_tag=%%f")
(
echo #define GIT_HASH "%git_hash%"
echo #define GIT_TAG "%git_tag%"
)>%1 || exit 1
