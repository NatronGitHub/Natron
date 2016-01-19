#!/bin/sh
USER_DIR="${HOME}/.local/share/mime"
ROOT_DIR="/usr/share/mime"

if [ "$UID" = "0" ]; then
  MIME_DIR="$ROOT_DIR"
else
  MIME_DIR="$USER_DIR"
fi
if [ ! -d "$MIME_DIR/packages" ]; then
  mkdir -p "$MIME_DIR/packages"
fi

cat <<EOF > "$MIME_DIR/packages/x-natron.xml"
<?xml version="1.0" encoding="UTF-8"?>
<mime-info xmlns="http://www.freedesktop.org/standards/shared-mime-info">
    <mime-type type="application/x-natron">
        <comment>Natron Project File</comment>
        <icon name="natronIcon256_linux"/>
        <glob-deleteall/>
        <glob pattern="*.ntp"/>
    </mime-type>
</mime-info>
EOF

update-mime-database "$MIME_DIR"
