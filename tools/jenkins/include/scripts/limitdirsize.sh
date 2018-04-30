#!/bin/bash
# http://stackoverflow.com/a/11981367/2607517
DELETEDIR="$1"
MAXSIZE="$2"
if [[ -z "$DELETEDIR" || -z "$MAXSIZE" || "$MAXSIZE" -lt 1 ]]; then
    echo "usage: $0 [directory] [maxsize in megabytes]" >&2
    exit 1
fi
FIND=find
SORT=sort
AWK=awk
TAC=tac
XARGS=xargs
# tac is in coreutils
TAC=tac
if [ "$(uname -s)" = "Darwin" ]; then
    # findutils
    FIND=gfind
    XARGS=gxargs
    # coreutils
    SORT=gsort
    TAC=gtac
    XARGS=gxargs
    # GNU awk
    AWK=gawk
fi

$FIND "$DELETEDIR" -type f -printf "%T@::%p::%s\n" \
| $SORT -rn \
| $AWK -v maxbytes="$((1024 * 1024 * $MAXSIZE))" -F "::" '
  BEGIN { curSize=0; }
  { 
  curSize += $3;
  if (curSize > maxbytes) { print $2; }
  }
  ' \
  | $TAC | $AWK '{printf "%s\0",$0}' | $XARGS -0 -r rm
# delete empty directories
$FIND "$DELETEDIR" -mindepth 1 -depth -type d -empty -exec rmdir "{}" \;
