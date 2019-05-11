#!/bin/sh
USER_MIME_DIR="${HOME}/.local/share/mime"
USER_DESK_DIR="${HOME}/.local/share/applications"
ROOT_MIME_DIR="/usr/share/mime"
ROOT_DESK_DIR="/usr/share/applications"

if [ "$UID" = "0" ]; then
  MIME_DIR="$ROOT_MIME_DIR"
  DESK_DIR="$ROOT_DESK_DIR"
else
  MIME_DIR="$USER_MIME_DIR"
  DESK_DIR="$USER_DESK_DIR"
fi
if [ ! -d "$MIME_DIR/packages" ]; then
  mkdir -p "$MIME_DIR/packages"
fi

cat <<EOF > "$MIME_DIR/packages/x-natron.xml"
<?xml version="1.0" encoding="UTF-8"?>
<mime-info xmlns="http://www.freedesktop.org/standards/shared-mime-info">
    <mime-type type="application/x-natron">
        <comment>Natron Project File</comment>
        <icon name="natronProjectIcon_linux"/>
        <glob-deleteall/>
        <glob pattern="*.ntp"/>
    </mime-type>
</mime-info>
EOF

update-mime-database "$MIME_DIR"
update-desktop-database "$DESK_DIR"
