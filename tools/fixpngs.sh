#!/bin/sh
# see https://bbs.archlinux.org/viewtopic.php?pid=1331598#p1331598

TP=`mktemp -d tmp.XXXXXXXXXX`          # make a temp dir for the fixed files
find . -exec file {} \; |              # get the real type of any files inside the current directory (disregarding the extension)
  grep PNG |                           # filter those which are not PNG
  sed 's/: .*//' |                     # extract the filename from `file` output
#  while read pngf; do                  # for each PNG file
#    identify "$pngf" 2>&1 >&-          # check if the file is alright: discard stdout and keep stderr, which should contain the filename of the files to fix
#  done |
#  sed 's/.*`\(.*\)'"'.*/\1/" |         # extract the filename from the warning
  while read badf; do                  # for each file to fix
    echo "fixing $badf"
    mkdir -p "$TP/`dirname \"$badf\"`" # if necessary, create a similar subtree inside the temp dir
    convert -strip "$badf" -define png:exclude-chunks=date "$TP/$badf" 2>&-   # create a fixed copy of the file in the temp dir (discard stderr as it will give the same warning as `identify` above)
    mv "$TP/$badf" "$badf"             # overwrite the bad file with the fixed file
  done
rm -r "$TP"                            # delete the temp dir
