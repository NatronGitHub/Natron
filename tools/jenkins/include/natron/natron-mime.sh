#!/bin/sh
#
# Add this copy of Natron to the desktop: applications menu entry, icons, and
# the Natron project file type (application/x-natron, *.ntp).
#
# Usage: natron-mime.sh [--uninstall]
#
# Run as a normal user, it installs for that user only, in
# ${XDG_DATA_HOME:-~/.local/share}. Run as root, it installs for all users,
# in /usr/local/share.
#
# This script must stay in the bin folder of the portable archive: it finds
# the Natron launcher and the icons relative to its own location.
#
# --uninstall removes the files that this script installs, then updates the
# caches.

set -u

usage() {
    echo "Usage: $0 [--uninstall]"
    echo "Add Natron to the desktop (menu entry, icons, .ntp file type)."
    echo "Run as a normal user to install for that user, or as root to"
    echo "install for all users. --uninstall removes what it installed."
}

UNINSTALL=0
case "${1:-}" in
    "") ;;
    --uninstall) UNINSTALL=1 ;;
    -h|--help) usage; exit 0 ;;
    *) usage >&2; exit 1 ;;
esac

# $UID is not set by POSIX shells such as dash: use id -u.
if [ "$(id -u)" = "0" ]; then
    DATA_DIR="/usr/local/share"
else
    DATA_DIR="${XDG_DATA_HOME:-$HOME/.local/share}"
fi
MIME_DIR="$DATA_DIR/mime"
APPS_DIR="$DATA_DIR/applications"
ICON_THEME_DIR="$DATA_DIR/icons/hicolor"

# The files that this script installs, and that --uninstall removes.
# The bundle ships 256x256 icons only.
DESKTOP_FILE="$APPS_DIR/natron-portable.desktop"
MIME_FILE="$MIME_DIR/packages/x-natron.xml"
APP_ICON="$ICON_THEME_DIR/256x256/apps/natronIcon256_linux.png"
PROJECT_ICON="$ICON_THEME_DIR/256x256/mimetypes/natronProjectIcon_linux.png"

update_caches() {
    if command -v update-mime-database >/dev/null 2>&1 && [ -d "$MIME_DIR" ]; then
        update-mime-database "$MIME_DIR"
    fi
    if command -v update-desktop-database >/dev/null 2>&1 && [ -d "$APPS_DIR" ]; then
        update-desktop-database "$APPS_DIR"
    fi
    if [ -d "$ICON_THEME_DIR" ]; then
        # Desktops notice new icons when the theme folder changes.
        touch "$ICON_THEME_DIR"
        # Update the icon cache only if there is one: a stale cache would
        # hide the icons that other applications add later.
        if [ -f "$ICON_THEME_DIR/icon-theme.cache" ] && command -v gtk-update-icon-cache >/dev/null 2>&1; then
            gtk-update-icon-cache -f -t -q "$ICON_THEME_DIR"
        fi
    fi
}

if [ "$UNINSTALL" = "1" ]; then
    for f in "$DESKTOP_FILE" "$MIME_FILE" "$APP_ICON" "$PROJECT_ICON"; do
        if [ -e "$f" ]; then
            rm -f "$f" && echo "Removed $f"
        fi
    done
    update_caches
    exit 0
fi

# The folder of this copy of Natron (the parent of bin/).
NATRON_DIR="$(cd "$(dirname "$0")/.." && pwd -P)" || exit 1
LAUNCHER="$NATRON_DIR/Natron"
PIXMAPS_DIR="$NATRON_DIR/Resources/pixmaps"

case "$NATRON_DIR" in
    *"
"*) echo "Error: the Natron folder name must not contain a newline." >&2; exit 1 ;;
esac
for f in "$LAUNCHER" "$PIXMAPS_DIR/natronIcon256_linux.png" "$PIXMAPS_DIR/natronProjectIcon_linux.png"; do
    if [ ! -f "$f" ]; then
        echo "Error: $f not found. Run this script from the bin folder of Natron." >&2
        exit 1
    fi
done

# Quote the launcher path for the Exec key of the .desktop file: inside double
# quotes, escape \ " ` $, then escape \ again (string value), and % as %%.
EXEC_PATH="$(printf '%s\n' "$LAUNCHER" | sed -e 's/[\\"`$]/\\&/g' -e 's/\\/\\\\/g' -e 's/%/%%/g')"
# TryExec takes a plain string value: only \ is escaped.
TRYEXEC_PATH="$(printf '%s\n' "$LAUNCHER" | sed -e 's/\\/\\\\/g')"

mkdir -p "$MIME_DIR/packages" "$APPS_DIR" "$(dirname "$APP_ICON")" "$(dirname "$PROJECT_ICON")" || exit 1

cp "$PIXMAPS_DIR/natronIcon256_linux.png" "$APP_ICON" || exit 1
cp "$PIXMAPS_DIR/natronProjectIcon_linux.png" "$PROJECT_ICON" || exit 1
chmod 644 "$APP_ICON" "$PROJECT_ICON"

cat <<EOF > "$MIME_FILE" || exit 1
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

cat <<EOF > "$DESKTOP_FILE" || exit 1
[Desktop Entry]
Version=1.0
Type=Application
Terminal=false
Name=Natron
GenericName=Compositing software
Comment=Node-graph based compositing software
TryExec=$TRYEXEC_PATH
Exec="$EXEC_PATH" %f
Icon=natronIcon256_linux
MimeType=application/x-natron;
Categories=Graphics;2DGraphics;RasterGraphics;
EOF
chmod 644 "$MIME_FILE" "$DESKTOP_FILE"

echo "Installed $DESKTOP_FILE"
echo "Installed $MIME_FILE"
echo "Installed $APP_ICON"
echo "Installed $PROJECT_ICON"

update_caches

echo "Natron ($LAUNCHER) is now in the applications menu, and opens .ntp files."
