#! /bin/sh
git_hash="`git rev-parse --short HEAD`" || exit 1
git_tag="`git describe --tags --always --abbrev=0`" || exit 1
echo "#ifndef GIT_HASH" > $1
echo "#define GIT_HASH "\"$git_hash\" >> $1
echo "#endif" >> $1
echo "#ifndef GIT_TAG" >> $1
echo "#define GIT_TAG "\"$git_tag\" >> $1
echo "#endif" >> $1
