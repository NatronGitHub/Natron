# This script has to be run after uncrustify, to fix various glitches.
# Usage:
# - run uncrustify first
#  uncrustify -c tools/uncrustify.cfg --replace */*.cpp */*.h
# - fix the output of uncrustify using this script
#  sed -f tools/uncrustify-post.sed -i .bak */*.cpp */*.h

# arguments to CLANG_DIAG_ON, CLANG_DIAG_OFF, GCC_DIAG_ON, GCC_DIAG_OFF should have no spaces around "-"
/CLANG_DIAG_ON/ s/ - /-/g
/CLANG_DIAG_OFF/ s/ - /-/g
/GCC_DIAG_ON/ s/ - /-/g
/GCC_DIAG_OFF/ s/ - /-/g
