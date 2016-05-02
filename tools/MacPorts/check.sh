#!/bin/sh
cd `dirname $0`
ports=/opt/local/var/macports/sources/rsync.macports.org/release/tarballs/ports
for p in `find . -name Portfile`; do
  if [ $ports/$p -nt $p ]; then
#      ls -l $p $ports/$p |sed -e "s@$ports/@@"
      if cmp $p $ports/$p > /dev/null; then
	  touch $p
      else
	  echo "$p is obsolete:"
	  ls -l $p $ports/$p
      fi
  fi
  d=`dirname $p`
  if [ -d "$ports/$d/files" ]; then
    for q in `find $ports/$d/files -type f`; do
	lq=`echo $q | sed -e s@$ports@.@`
	if [ ! -f $lq ]; then
            echo "$lq is missing:"
	    ls -l $q
	elif [ $q -nt $lq ]; then
            if cmp $lq $q > /dev/null; then
		touch $lq
	    else
		echo "$lq is obsolete:"
		ls -l $lq $q
	    fi
	fi
    done
  fi
done
