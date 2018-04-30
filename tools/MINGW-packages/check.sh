#!/bin/sh
set -e
cd `dirname $0`
ports=../../../MINGW-packages
for p in `find . -name PKGBUILD`; do
    d=`dirname $p`
    origd="$ports/$d"
    origd="${origd/%-lgpl/}"
    origd="${origd/%-gpl2/}"
    origd="${origd/%-gpl3/}"
    orig="$origd/PKGBUILD"
    #echo "$p -> $orig"
    if [ ! -f "$orig" ]; then
	continue
    fi
    obsolete=no
    if [ -f "${p}.orig" ]; then
	if ! diff -q "$orig" "${p}.orig" &>/dev/null; then
	    obsolete=yes
	fi
    elif [ "$orig" -nt "$p" ]; then
	obsolete=yes
    fi
    if [ "$obsolete" = "yes" ]; then
	#      ls -l $p $ports/$p |sed -e "s@$ports/@@"
	if cmp "$p" "$orig" > /dev/null; then
	    touch "$p"
	else
	    echo "$p is obsolete:"
	    ls -l "$p" "$orig"
	    if [ -f "${p}.patch" ]; then
		echo "\$ cp $orig ${p}.orig; cp $orig ${p}; (cd "`dirname $p`" && patch < PKGBUILD.patch)"
	    fi
	fi
    fi
    #if [ -d "$ports/$d/files" ]; then
    for q in `find $origd -type f -not -name PKGBUILD`; do
	lq=`echo $q | sed -e s@$origd@$d@`
	if [ ! -f "$lq" ]; then
            echo "$lq is missing:"
	    ls -l "$q"
	    echo "\$ mkdir "`dirname $lq`"; cp $q $lq"
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
    #fi
done
