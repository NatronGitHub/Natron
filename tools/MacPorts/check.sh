#!/bin/sh
cd `dirname $0`
ports=/opt/local/var/macports/sources/rsync.macports.org/release/tarballs/ports
for p in `find . -name Portfile`; do
  obsolete=no
  if [ -f ${p}.orig ]; then
    if ! diff -q $ports/$p ${p}.orig &>/dev/null; then
      obsolete=yes
    fi
  elif [ $ports/$p -nt $p ]; then
    obsolete=yes
  fi
  if [ "$obsolete" = "yes" ]; then
#      ls -l $p $ports/$p |sed -e "s@$ports/@@"
      if cmp $p $ports/$p > /dev/null; then
	  touch $p
      else
	  echo "$p is obsolete:"
	  ls -l $p $ports/$p
	  if [ -f ${p}.patch ]; then
	      echo "\$ cp $ports/$p ${p}.orig; cp $ports/$p ${p}; (cd "`dirname $p`" && patch < Portfile.patch)"
	  fi
      fi
  fi
  d=`dirname $p`
  if [ -d "$ports/$d/files" ]; then
    for q in `find $ports/$d/files -type f`; do
	lq=`echo $q | sed -e s@$ports@.@`
	if [ ! -f $lq ]; then
            echo "$lq is missing:"
	    ls -l $q
	    echo "\$ mkdir "`dirname $lq`"; cp $q $lq"
	elif [ $q -nt $lq ]; then
            if cmp $lq $q > /dev/null; then
		touch $lq
	    else
		echo "$lq is obsolete:"
		ls -l $lq $q
		echo "\$ cp $q $lq"
	    fi
	fi
    done
  fi
done
