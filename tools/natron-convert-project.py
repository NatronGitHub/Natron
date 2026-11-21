# -*- coding: utf-8 -*-
# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <https://natrongithub.github.io/>,
# (C) 2018-2026 The Natron developers
#
# Natron is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Natron is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
# ***** END LICENSE BLOCK *****

"""Converts a Natron project (.ntp) between the binary and the text (XML) formats.

Run it with NatronRenderer; the arguments are given in environment variables, because
NatronRenderer does not pass command-line arguments to the script:

    NATRON_CONVERT_TO=text NATRON_CONVERT_INPUT=in.ntp NATRON_CONVERT_OUTPUT=out.ntp \\
        NatronRenderer -t natron-convert-project.py

NATRON_CONVERT_TO       "text" (readable by Natron 2.5 and earlier) or "binary"
NATRON_CONVERT_INPUT    the project to convert
NATRON_CONVERT_OUTPUT   the converted project (default: NATRON_CONVERT_INPUT, which is replaced)
NATRON_CONVERT_DISCARD_GUI
                        "1": convert a project that has GUI data, without it (see below)

The project is loaded and saved, like "File > Save As..." in Natron: nothing is rendered, and
the project is the one Natron would have saved (a project saved by an older version is saved
in the current version, parameters that a plug-in no longer has are dropped). Its plug-ins
and PyPlugs must be installed: a project that loads with errors (e.g. nodes that cannot be
created) is not converted.

NatronRenderer does not read the GUI data of a project (node positions and colours, viewer
settings, window layout...), and saves projects without them. A project saved by the Natron
GUI has GUI data, so it is not converted, unless NATRON_CONVERT_DISCARD_GUI=1: convert such a
project in Natron with "File > Save As...", which has a "File format" choice.

The exit status is 0 on success, 1 on failure.
"""

import os
import sys

import NatronEngine
from NatronEngine import natron

FORMATS = {
    "text": NatronEngine.Natron.ProjectFileFormatEnum.eProjectFileFormatText,
    "binary": NatronEngine.Natron.ProjectFileFormatEnum.eProjectFileFormatBinary,
}


def _error(message):
    sys.stderr.write("natron-convert-project: error: %s\n" % message)
    sys.stderr.flush()


def _convert():
    to = os.environ.get("NATRON_CONVERT_TO", "").strip().lower()
    if to not in FORMATS:
        _error("NATRON_CONVERT_TO must be \"text\" or \"binary\", not %r" % os.environ.get("NATRON_CONVERT_TO"))
        return False
    src = os.environ.get("NATRON_CONVERT_INPUT", "")
    if not src:
        _error("NATRON_CONVERT_INPUT (the project to convert) is not set")
        return False
    src = os.path.abspath(os.path.expanduser(src))
    if not os.path.isfile(src):
        _error("%s: no such file" % src)
        return False
    dst = os.environ.get("NATRON_CONVERT_OUTPUT", "") or src
    dst = os.path.abspath(os.path.expanduser(dst))
    discardGui = os.environ.get("NATRON_CONVERT_DISCARD_GUI", "") == "1"

    if not natron.isBackground():
        _error("run this script with NatronRenderer; in Natron, use \"File > Save As...\"")
        return False
    app = natron.getInstance(0)
    loaded = app.loadProject(src)
    if loaded is None:
        _error("%s could not be loaded (see the messages above; are its plug-ins installed?)" % src)
        return False
    if loaded.projectLoadedWithErrors():
        # e.g. nodes whose plug-in is missing: they would not be in the converted project
        _error("there were errors while loading %s (see the messages above; are all its plug-ins and PyPlugs "
               "installed?): it is not converted" % src)
        return False
    if loaded.projectFileHasGuiData():
        if not discardGui:
            _error("%s has GUI data (node positions, viewer settings, window layout...), which NatronRenderer "
                   "does not save. Convert it in Natron with \"File > Save As...\", or set "
                   "NATRON_CONVERT_DISCARD_GUI=1 to convert it without its GUI data." % src)
            return False
        sys.stdout.write("natron-convert-project: warning: the GUI data of %s are not converted\n" % src)
    if not loaded.saveProjectAs(dst, FORMATS[to]):
        _error("%s could not be saved" % dst)
        return False
    sys.stdout.write("natron-convert-project: %s -> %s (%s)\n" % (src, dst, to))
    sys.stdout.flush()
    return True


_status = 1
try:
    if _convert():
        _status = 0
except Exception:
    import traceback
    traceback.print_exc()
# natron.quit() also stops NatronRenderer from rendering after the script (when run without -t)
natron.quit(_status)
