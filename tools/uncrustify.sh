#!/bin/sh -x

TOOLSDIR=`dirname $0`

if [ $# -ge 1 ]; then
    FILES="$@"
else
    FILES=`ls */*.cpp */*.h  |fgrep -v /moc_ |fgrep -v -v /qrc_ | fgrep -v /Macros.h`
fi

if [ -d Engine -a ! -x "$TOOLSDIR"/normalize/normalize ]; then
    echo "Error: please compile $TOOLSDIR/normalize/normalize first"
    exit 1
fi

for f in $FILES; do
    uncrustify -c "$TOOLSDIR"/uncrustify.cfg --replace $f
    sed -f "$TOOLSDIR"/uncrustify-post.sed -i .bak $f
done

for d in Gui Engine App Global Renderer Tests; do
    if [ -d $d ]; then
	"$TOOLSDIR"/normalize/normalize --modify $d
    fi
done

rm */*.bak */*~
