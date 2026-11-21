#!/bin/sh
#
# Add this copy of Natron to the desktop: applications menu entry, icons, and
# the Natron project file type (application/x-natron, *.ntp).
#
# Usage: natron-mime.sh [--uninstall]
#
# Run as a normal user, it installs for that user only, in
# ${XDG_DATA_HOME:-~/.local/share}. Run as root, it installs for all users,
# in /usr/local/share: the Natron folder should then be in a place that only
# root can change, such as /opt.
#
# This script must stay in the bin folder of the portable archive: it finds
# the Natron launcher and the icons relative to its own location. Run it
# directly, not through a symbolic link.
#
# --uninstall removes the files that this script installed, if the menu entry
# starts this copy of Natron, then updates the caches.

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
IS_ROOT=0
if [ "$(id -u)" = "0" ]; then
    IS_ROOT=1
    DATA_DIR="/usr/local/share"
else
    # The XDG Base Directory spec says to ignore a relative XDG_DATA_HOME.
    case "${XDG_DATA_HOME:-}" in
        /*) DATA_DIR="$XDG_DATA_HOME" ;;
        *) DATA_DIR="$HOME/.local/share" ;;
    esac
fi
MIME_DIR="$DATA_DIR/mime"
APPS_DIR="$DATA_DIR/applications"
ICON_THEME_DIR="$DATA_DIR/icons/hicolor"

# The files that this script installs, and that --uninstall removes.
# The bundle ships 256x256 icons only. The MIME file is not x-natron.xml, which
# Natron 2.5's installer and source installs (in /usr/local) also use.
DESKTOP_FILE="$APPS_DIR/natron-portable.desktop"
MIME_FILE="$MIME_DIR/packages/natron-portable.xml"
APP_ICON="$ICON_THEME_DIR/256x256/apps/natronIcon256_linux.png"
PROJECT_ICON="$ICON_THEME_DIR/256x256/mimetypes/natronProjectIcon_linux.png"

update_caches() {
    if command -v update-mime-database >/dev/null 2>&1 && [ -d "$MIME_DIR/packages" ]; then
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

# Through a symbolic link, $0 is the link, and the Natron folder can't be found.
if [ -L "$0" ]; then
    echo "Error: run $0 directly from the bin folder of Natron, not through a symbolic link." >&2
    exit 1
fi

# The folder of this copy of Natron (the parent of bin/).
NATRON_DIR="$(cd "$(dirname "$0")/.." && pwd -P)" || exit 1
LAUNCHER="$NATRON_DIR/Natron"
PIXMAPS_DIR="$NATRON_DIR/Resources/pixmaps"

# TryExec takes a plain string value: only \ is escaped.
TRYEXEC_PATH="$(printf '%s\n' "$LAUNCHER" | sed -e 's/\\/\\\\/g')"

if [ "$UNINSTALL" = "1" ]; then
    # Remove the files only if the menu entry starts this copy: they may have
    # been installed by another copy of Natron, which still uses them.
    if [ -f "$DESKTOP_FILE" ]; then
        OWNER="$(sed -n -e 's/^TryExec=//p' "$DESKTOP_FILE" | head -n 1)"
        if [ "$OWNER" != "$TRYEXEC_PATH" ]; then
            echo "$DESKTOP_FILE starts another copy of Natron:"
            echo "    $OWNER"
            echo "Nothing was removed. Run natron-mime.sh --uninstall from the bin folder of that copy."
            exit 1
        fi
    fi
    # Without the menu entry, the other files belong to no copy of Natron.
    for f in "$DESKTOP_FILE" "$MIME_FILE" "$APP_ICON" "$PROJECT_ICON"; do
        if [ -e "$f" ]; then
            rm -f "$f" && echo "Removed $f"
        fi
    done
    update_caches
    exit 0
fi

# A newline can't be written in a .desktop file. GLib rejects %% in Exec,
# so a % can't be written either.
case "$NATRON_DIR" in
    *"
"*|*%*) echo "Error: the Natron folder path ($NATRON_DIR) must not contain a newline or a %." >&2; exit 1 ;;
esac
for f in "$LAUNCHER" "$PIXMAPS_DIR/natronIcon256_linux.png" "$PIXMAPS_DIR/natronProjectIcon_linux.png"; do
    if [ ! -f "$f" ]; then
        echo "Error: $f not found. Run this script from the bin folder of Natron." >&2
        exit 1
    fi
done

# For all users, the menu entry runs this folder's programs: anyone who can
# change them could run their own programs as the other users.
if [ "$IS_ROOT" = "1" ]; then
    NOT_ROOT_OWNED="$(find "$NATRON_DIR" \( ! -user 0 -o -perm -0002 \) -print 2>/dev/null | head -n 1)"
    if [ -n "$NOT_ROOT_OWNED" ]; then
        echo "Warning: $NATRON_DIR can be changed by other users than root"
        echo "(for example $NOT_ROOT_OWNED). To add Natron for all users, move"
        echo "it to a place that only root can change, such as /opt, and run"
        echo "this script again from there."
    fi
fi

# Quote the launcher path for the Exec key of the .desktop file: inside double
# quotes, escape \ " ` $, then escape \ again (string value).
EXEC_PATH="$(printf '%s\n' "$LAUNCHER" | sed -e 's/[\\"`$]/\\&/g' -e 's/\\/\\\\/g')"

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

# Name: "(portable)", so that it is not mistaken for the entry of a
# distribution or Flathub package. StartupWMClass: Qt's WM_CLASS is Natron.
cat <<EOF > "$DESKTOP_FILE" || exit 1
[Desktop Entry]
Version=1.0
Type=Application
Terminal=false
Name=Natron (portable)
GenericName=Compositing software
Comment=Node-graph based compositing software
Keywords=compositing;compositor;VFX;OpenFX;node;
TryExec=$TRYEXEC_PATH
Exec="$EXEC_PATH" %f
Icon=natronIcon256_linux
StartupWMClass=Natron
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
