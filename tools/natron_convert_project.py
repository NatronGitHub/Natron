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

"""Converts Natron projects between the binary, text and Python formats.

Natron installs this module in its Plugins directory, which is on the Python
path of Natron and NatronRenderer. Run it with NatronRenderer:

    NatronRenderer -t -c "import natron_convert_project as c; \\
        c.convert_and_quit('in.ntp', 'out.ntp', 'text')"

The exit status is 0 if the project was converted. Otherwise, the exception
is printed and the exit status is 1. In a script, call convert(), which
raises the same exceptions and does not quit.

The formats are "binary" and "text" (.ntp files), and "python" (a .py file
that rebuilds the project, see "Projects as Python scripts" in the Natron
documentation). The name of the converted project must have the extension of
its format. Loading a Python project runs it: convert only the files you
trust.

The project is loaded and saved, as with "File > Save As..." in Natron:
nothing is rendered, and the file is the project that this version of Natron
saves (a project saved by an older version is upgraded, and the parameters
that a plug-in no longer has are dropped). Its plug-ins and PyPlugs must be
installed: a project that loads with errors (e.g. a node that cannot be
created, or a PyPlug that cannot be found or imported, whose node would be
saved as a plain Group) is not converted. This includes the PyPlugs that
cannot be imported in NatronRenderer, e.g. those that import NatronGui or
PySide: convert those projects with "File > Save As..." in Natron.

The Python format does not save everything (see "Limitations" in "Projects
as Python scripts"): for example, the tracks of the Tracker nodes and the
paint strokes of the RotoPaint nodes, which Natron lists in a warning before
it converts the project without them. A Python project is converted by
running it and writing the project it builds, not by copying it: its
comments and its other code (e.g. helper functions) are not kept.

NatronRenderer does not read the GUI data of a project (node positions and
colours, viewer settings, window layout...), and saves projects without them.
A project saved by the Natron GUI has GUI data, so it is not converted, unless
discard_gui is True. In a Python project, the GUI data are the Viewers and
the calls that set the position, size or colour of a node, found in the file
as Natron writes them (see _PYTHON_GUI_DATA). Convert such a project in
Natron with "File > Save As...", which has a "File format" choice.
"""

import os
import re
import sys

import NatronEngine
from NatronEngine import natron

_FORMAT_ENUM = NatronEngine.Natron.ProjectFileFormatEnum
# The value and the file extension of each format
_FORMATS = {
    "text": (_FORMAT_ENUM.eProjectFileFormatText, ".ntp"),
    "binary": (_FORMAT_ENUM.eProjectFileFormatBinary, ".ntp"),
    "python": (_FORMAT_ENUM.eProjectFileFormatPython, ".py"),
}
# The GUI data of a Python project, as Natron writes them (see
# NodeCollection::exportGroupInternal() in Engine/NodeGroup.cpp): a Viewer
# (which only the GUI has), or a call that sets the position, size or colour
# of a node, which NatronRenderer ignores. In a project written by hand, it
# may miss some (e.g. app.getNode("A").setPosition(...)) or match calls that
# are not GUI data (e.g. the setColor() of a Roto shape): the error message
# gives the line, and discard_gui=True converts the project anyway.
_PYTHON_GUI_DATA = re.compile(
    rb"^[ \t]*(?:# Viewer |[\w.]+\.set(?:Position|Size|Color)\()",
    re.MULTILINE)


def convert(src: str, dst: str | None = None, to: str = "text",
            discard_gui: bool = False) -> None:
    """Converts the project src to the format to, and saves it as dst.

    Replaces the project of the current (background) application: run it in
    NatronRenderer (or Natron -b), not in a project that is being edited.

    Args:
        src: Path of the project to convert. "~" is expanded.
        dst: Path of the converted project, with the extension of the
            format to (.ntp for "text" and "binary", .py for "python",
            case-insensitively). None (the default) is src with that
            extension: src itself if it has it (the project is first written
            to a temporary file, as with every save in Natron), else a file
            next to it, which must not exist (pass dst to replace a file).
        to: "text" (XML, readable by Natron 2.5 and earlier), "binary", or
            "python" (a Python script, readable by Natron 2.6 and later).
        discard_gui: If False (the default), a project that has GUI data
            is not converted, because the converted project would not have
            them. If True, it is converted without them, with a warning on
            sys.stderr.

    Raises:
        ValueError: to is not "text", "binary" or "python", or the extension
            of dst is not the one of its format.
        FileNotFoundError: src is not a file.
        FileExistsError: dst is None, and the file next to src that it
            stands for exists (a file or a directory).
        IsADirectoryError: dst is a directory.
        RuntimeError: Natron is not in background mode; the project could
            not be loaded, or loaded with errors; it has GUI data and
            discard_gui is False; or it could not be saved.
    """
    if not natron.isBackground():
        raise RuntimeError('run the conversion in NatronRenderer; in Natron, '
                           'use "File > Save As..."')
    if to not in _FORMATS:
        raise ValueError('to must be "text", "binary" or "python", not %r'
                         % (to,))
    file_format, ext = _FORMATS[to]
    src = os.path.abspath(os.path.expanduser(src))
    if not os.path.isfile(src):
        raise FileNotFoundError("%s: no such file" % src)
    if dst is None:
        dst = _with_extension(src, ext)
        if dst != src and os.path.lexists(dst):
            # e.g. a script next to the project, or the current project next
            # to a backup: replace it only if asked
            raise FileExistsError("%s exists: pass it as dst to replace it"
                                  % dst)
    else:
        dst = os.path.abspath(os.path.expanduser(dst))
        if not _has_extension(dst, ext):
            # Natron would save the format to in a file of another type
            raise ValueError("%s: a %s project must have the %s extension"
                             % (dst, to, ext))
    if os.path.isdir(dst):
        # Natron would rename the directory as a backup, and save in its place
        raise IsADirectoryError("%s is a directory" % dst)

    app = natron.getInstance(0).loadProject(src)
    if app is None:
        raise RuntimeError("%s could not be loaded (see the messages above; "
                           "are its plug-ins installed?)" % src)
    try:
        _check_and_save(app, src, dst, file_format, discard_gui)
    except BaseException:
        # Close the project, which removes its lock file (src.lock)
        app.resetProject()
        raise


def _extension(path: str) -> str:
    """Returns the extension of path in lower case, with its dot, or "".

    As QFileInfo.suffix() in Natron: the text after the last dot of the file
    name, also for a name that starts with it (".py").
    """
    name = os.path.basename(path)
    dot = name.rfind(".")
    return name[dot:].lower() if dot != -1 else ""


def _has_extension(path: str, ext: str) -> bool:
    """Returns whether path ends with ext, case-insensitively."""
    return _extension(path) == ext


def _with_extension(path: str, ext: str) -> str:
    """Returns path if it ends with ext, else path with its extension ext."""
    if _has_extension(path, ext):
        return path
    return path[:len(path) - len(_extension(path))] + ext


def _python_gui_data_line(path: str) -> int | None:
    """Returns the first line of GUI data of the Python project path, or None.

    See _PYTHON_GUI_DATA.
    """
    with open(path, "rb") as f:
        data = f.read()
    match = _PYTHON_GUI_DATA.search(data)
    return None if match is None else data.count(b"\n", 0, match.start()) + 1


def _gui_data(app, src: str) -> str | None:
    """Returns the GUI data of the project loaded from src, or None.

    App.projectFileHasGuiData() is False for a Python project: its GUI data
    are calls that NatronRenderer ignores, found in the file itself.

    Returns:
        A description of the GUI data, for the messages, or None if the
        project has none.
    """
    if app.getProjectFormat() == _FORMAT_ENUM.eProjectFileFormatPython:
        line = _python_gui_data_line(src)
        if line is None:
            return None
        return ("a Viewer, or node positions, sizes or colours, from line %d"
                % line)
    if app.projectFileHasGuiData():
        return "node positions, viewer settings, window layout..."
    return None


def _check_and_save(app, src: str, dst: str, file_format,
                    discard_gui: bool) -> None:
    """Saves the project loaded from src as dst, after the checks of convert().

    A project that loaded with errors is not saved, nor one with GUI data
    unless discard_gui. The data that the format of dst does not save (see
    the module docstring) are dropped.

    Raises:
        RuntimeError: See convert().
    """
    if app.projectLoadedWithErrors():
        # e.g. nodes whose plug-in is missing: they would not be saved
        raise RuntimeError("there were errors while loading %s (see the "
                           "messages above; are all its plug-ins and PyPlugs "
                           "installed?): it is not converted" % src)
    gui_data = _gui_data(app, src)
    if gui_data is not None:
        if not discard_gui:
            raise RuntimeError(
                "%s has GUI data (%s), which NatronRenderer does not save: "
                'convert it in Natron with "File > Save As...", or pass '
                "discard_gui=True to convert it without them"
                % (src, gui_data))
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
        ValueError, FileNotFoundError, FileExistsError, IsADirectoryError,
            RuntimeError: See convert().
    """
    convert(src, dst, to, discard_gui)
    natron.quit(0)
