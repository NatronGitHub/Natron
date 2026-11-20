#!/bin/sh
# Build the package in the current directory for the environment of this MSYS2 shell (UCRT64).
MINGW_ARCH="$(echo "${MSYSTEM}" | tr "[:upper:]" "[:lower:]")" makepkg-mingw -LfC
