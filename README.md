# SourceExplorer

Source Explorer allows you to walk through Clickteam exes and view object names, textures, etc.

[Download from releases](https://github.com/LAK132/SourceExplorer/releases)

## Requirements:

### Windows
git, [meson](https://mesonbuild.com/) and the latest visual studio (with build tools).

setup:
1. open command prompt
2. `cd` to the directory where you want to clone the repo (!!!! command prompt may default to System32, DO NOT CLONE HERE!!!! change the directory to something else!!!)
3. clone the repo using git `git clone https://github.com/LAK132/SourceExplorer SourceExplorer`
4. `cd` into the repo `cd SourceExplorer`
6. run `git submodule update --init --recursive`
7. run `setup.bat`

compiling:
1. run `compile.bat`
2. run `build\srcexp.exe`

### Linux
git, at least SDL 2.0.5 (and clang-13 if building from source)

setup:
1. `git clone https://github.com/LAK132/SourceExplorer SourceExplorer`
2. `cd SourceExplorer`
3. `git submodule update --init --recursive`
4. `./clang_setup.sh`

compiling:
1. `./compile.sh`
2. `./build/srcexp`
