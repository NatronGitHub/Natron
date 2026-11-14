#!/bin/bash
set -e # Exit immediately if a command exits with a non-zero status
set -u # Treat unset variables as an error when substituting.
cd "$(dirname "$0")"
# upstream ports tree: the one of the [default] source in sources.conf
# (e.g. rsync://rsync.macports.org/macports/release/tarballs/ports.tar.gz or
# https://distfiles.macports.org/ports.tar.gz), else the first one found
ports=
default_src=$(awk '/\[default\]/ {print $1; exit}' /opt/local/etc/macports/sources.conf 2>/dev/null || true)
if [ -n "$default_src" ]; then
    d="/opt/local/var/macports/sources/$(echo "$default_src" | sed -e 's@^[a-z]*://@@' -e 's@\.tar\.gz$@@')"
    [ -d "$d" ] && ports="$d"
fi
if [ -z "$ports" ]; then
    for d in /opt/local/var/macports/sources/rsync.macports.org/macports/release/tarballs/ports /opt/local/var/macports/sources/rsync.macports.org/release/tarballs/ports /opt/local/var/macports/sources/distfiles.macports.org/ports; do
        if [ -d "$d" ]; then ports="$d"; break; fi
    done
fi
if [ -z "$ports" ]; then
    echo "Error: upstream MacPorts ports tree not found (run 'sudo port selfupdate' or 'sudo port sync')" >&2
    exit 1
fi

find . -name Portfile | \
while read -r p; do
  obsolete=no
  if [ -f "${p}.orig" ]; then
    if ! diff -q "$ports/$p" "${p}.orig" &>/dev/null; then
      obsolete=yes
    fi
  elif [ "$ports/$p" -nt "$p" ]; then
    obsolete=yes
  fi
  if [ "$obsolete" = "yes" ]; then
#      ls -l $p $ports/$p |sed -e "s@$ports/@@"
      if cmp "$p" "$ports/$p" > /dev/null; then
	  touch "$p"
      else
	  echo "$p is obsolete:"
	  ls -l "$p" "$ports/$p"
	  if [ -f "${p}.patch" ]; then
	      echo "\$ cp $ports/$p ${p}.orig; cp $ports/$p ${p}; (cd $(dirname "$p") && patch < Portfile.patch)"
	  fi
      fi
  fi
  d=$(dirname "$p")
  if [ -d "$ports/$d/files" ]; then
      find "$ports/$d/files" -type f | \
      while read -r q; do
	lq="$(echo $q | sed -e s@$ports@.@)"
	if [ ! -f "$lq" ]; then
            echo "$lq is missing:"
	    ls -l "$q"
	    echo "\$ mkdir $(dirname "$lq"); cp $q $lq"
	elif [ "$q" -nt "$lq" ]; then
            if cmp "$lq" "$q" > /dev/null; then
		touch "$lq"
	    else
		echo "$lq is obsolete:"
		ls -l "$lq" "$q"
		echo "\$ cp $q $lq"
	    fi
	fi
    done
  fi
done
