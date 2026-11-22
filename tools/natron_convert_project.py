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

"""Converts Natron projects (.ntp) between the binary and text formats.

Natron installs this module with its bundled PyPlugs, which are on the
Python path of Natron and NatronRenderer while the "Use bundled plug-ins"
preference is on. Run it with NatronRenderer:

    NatronRenderer -t -c "import natron_convert_project as c; \\
        c.convert_and_quit('in.ntp', 'out.ntp', 'text')"

The exit status is 0 if the project was converted. Otherwise, the exception
is printed and the exit status is 1. In a script, call convert(), which
raises the same exceptions and does not quit.

The project is loaded and saved, as with "File > Save As..." in Natron:
nothing is rendered, and the file is the project that this version of Natron
saves (a project saved by an older version is upgraded, and the parameters
that a plug-in no longer has are dropped). Its plug-ins and PyPlugs must be
installed: a project that loads with errors (e.g. a node that cannot be
created, or a PyPlug that cannot be found or imported, whose node would be
saved as a plain Group) is not converted. This includes the PyPlugs that
cannot be imported in NatronRenderer, e.g. those that import NatronGui or
PySide: convert those projects with "File > Save As..." in Natron.

NatronRenderer does not read the GUI data of a project (node positions and
colours, viewer settings, window layout...), and saves projects without them.
A project saved by the Natron GUI has GUI data, so it is not converted, unless
discard_gui is True. Convert such a project in Natron with "File > Save As...",
which has a "File format" choice.
"""

import os
import sys

import NatronEngine
from NatronEngine import natron

_FORMAT_ENUM = NatronEngine.Natron.ProjectFileFormatEnum
_FORMATS = {
    "text": _FORMAT_ENUM.eProjectFileFormatText,
    "binary": _FORMAT_ENUM.eProjectFileFormatBinary,
}


def convert(src: str, dst: str | None = None, to: str = "text",
            discard_gui: bool = False) -> None:
    """Converts the project src to the format to, and saves it as dst.

    Replaces the project of the current (background) application: run it in
    NatronRenderer (or Natron -b), not in a project that is being edited.

    Args:
        src: Path of the project to convert. "~" is expanded.
        dst: Path of the converted project. None (the default) replaces src;
            the project is first written to a temporary file, as with every
            save in Natron.
        to: "text" (XML, readable by Natron 2.5 and earlier) or "binary".
        discard_gui: If False (the default), a project that has GUI data
            is not converted, because the converted project would not have
            them. If True, it is converted without them, with a warning on
            sys.stderr.

    Raises:
        ValueError: to is not "text" or "binary".
        FileNotFoundError: src is not a file.
        IsADirectoryError: dst is a directory.
        RuntimeError: Natron is not in background mode; the project could
            not be loaded, or loaded with errors; it has GUI data and
            discard_gui is False; or it could not be saved.
    """
    if not natron.isBackground():
        raise RuntimeError('run the conversion in NatronRenderer; in Natron, '
                           'use "File > Save As..."')
    if to not in _FORMATS:
        raise ValueError('to must be "text" or "binary", not %r' % (to,))
    src = os.path.abspath(os.path.expanduser(src))
    if not os.path.isfile(src):
        raise FileNotFoundError("%s: no such file" % src)
    dst = src if dst is None else os.path.abspath(os.path.expanduser(dst))
    if os.path.isdir(dst):
        # Natron would rename the directory as a backup, and save in its place
        raise IsADirectoryError("%s is a directory" % dst)

    app = natron.getInstance(0).loadProject(src)
    if app is None:
        raise RuntimeError("%s could not be loaded (see the messages above; "
                           "are its plug-ins installed?)" % src)
    try:
        _check_and_save(app, src, dst, _FORMATS[to], discard_gui)
    except BaseException:
        # Close the project, which removes its lock file (src.lock)
        app.resetProject()
        raise


def _check_and_save(app, src, dst, file_format, discard_gui):
    """Saves the project loaded from src as dst, unless it would lose data.

    Raises:
        RuntimeError: See convert().
    """
    if app.projectLoadedWithErrors():
        # e.g. nodes whose plug-in is missing: they would not be saved
        raise RuntimeError("there were errors while loading %s (see the "
                           "messages above; are all its plug-ins and PyPlugs "
                           "installed?): it is not converted" % src)
    if app.projectFileHasGuiData():
        if not discard_gui:
            raise RuntimeError(
                "%s has GUI data (node positions, viewer settings, window "
                "layout...), which NatronRenderer does not save: convert it "
                'in Natron with "File > Save As...", or pass '
                "discard_gui=True to convert it without them" % src)
        sys.stderr.write("natron_convert_project: warning: the GUI data of "
                         "%s are not converted\n" % src)
        sys.stderr.flush()
    if not app.saveProjectAs(dst, file_format):
        raise RuntimeError("%s could not be saved" % dst)


def convert_and_quit(src: str, dst: str | None = None, to: str = "text",
                     discard_gui: bool = False) -> None:
    """Calls convert(), then quits Natron with the exit status 0.

    For the -c option of NatronRenderer -t: -t runs the command without
    rendering, and without natron.quit() it would then read Python commands
    from the standard input. An exception raised by convert() is not caught:
    NatronRenderer prints it, and exits with the status 1.

    Args:
        src: See convert().
        dst: See convert().
        to: See convert().
        discard_gui: See convert().

    Raises:
        ValueError, FileNotFoundError, IsADirectoryError, RuntimeError: See
            convert().
    """
    convert(src, dst, to, discard_gui)
    natron.quit(0)
