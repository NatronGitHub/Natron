.. _pythonProjects:

Projects as Python scripts
==========================

Natron 2.6 can save a project as a Python script, and open it again as a project. Unlike a *.ntp*
file, such a script is easy to read, to compare with ``diff``, to keep in a version control system,
and above all to **generate**: any program, in any language, can write a Natron project by writing
a short Python file, then render it with NatronRenderer.

.. contents::
    :local:

The format
----------

A Python project is a *.py* file that rebuilds the project when Natron runs it::

    # -*- coding: utf-8 -*-
    # Natron Project
    # Saved by Natron 2.6.0 (Python project format version 1).

    import NatronEngine


    def createProject(app):
        group = app
        # the project settings, then the nodes, their parameters and their connections
        ...


    if __name__ == "__main__":
        createProject(app)

- The ``# Natron Project`` line, the first or the second one of the file, tells Natron that the file is
  a project. Without it, a *.py* file is a script, which Natron runs as before (see :ref:`natronExec`).
- ``createProject(app)`` builds the project in ``app``, an empty project (:doc:`App<PythonReference/NatronEngine/App>`).
- The ``__main__`` guard calls it when Natron loads the file. The file can also be imported by another
  script, which then calls ``createProject()`` itself.

When Natron opens such a file, it starts a new project, sets ``[Project]`` to the folder of the file
(and ``[OCIO]`` to the OCIO configuration in use), runs the script, and then treats the project as
loaded from a *.ntp* file: the ``afterProjectLoad`` callback runs (see :ref:`callbacks`), and
:meth:`App.projectLoadedWithErrors()<NatronEngine.App.projectLoadedWithErrors>` tells whether
it failed. The script stops at the first Python error: the load then fails.

Loading a file runs it: open only the files you trust, as with any Python script.

Saving and opening
------------------

In the GUI
    *File > Save Project As...* has a *File format* menu: *Binary*, *Text*, or *Python script*. A
    project opened from a *.py* file is saved again as Python by *File > Save*. *File > Open Project...*
    opens *.ntp* and Python project files, as does dropping the file on the node graph.

From the command line
    ``Natron project.py`` opens it in the GUI; ``NatronRenderer project.py`` loads it and renders its
    Write nodes, as for a *.ntp* file (see :ref:`natronExec`).

From Python
    :meth:`App.saveProjectAs()<NatronEngine.App.saveProjectAs>`, :meth:`App.saveProject()<NatronEngine.App.saveProject>`
    and :meth:`App.saveProjectCopy()<NatronEngine.App.saveProjectCopy>` save in the Python format
    when the file name ends in *.py*, or with
    ``NatronEngine.Natron.ProjectFileFormatEnum.eProjectFileFormatPython`` as their *format*.
    :meth:`App.saveProjectAsPython()<NatronEngine.App.saveProjectAsPython>` saves a copy.
    :meth:`App.loadProject()<NatronEngine.App.loadProject>` opens a Python project as a project.

The auto-saves of a Python project are binary, as for every project; a restored auto-save is saved
as Python again.

Generating a project from another program
-----------------------------------------

The script below is a standalone program, run with any Python 3 (not with Natron). It writes a
project that reads an image sequence, grades it, and writes the result as PNG files, then
NatronRenderer renders it::

    #!/usr/bin/env python3
    """Writes grade.py, a Natron project that grades an image sequence."""

    PROJECT = '''# -*- coding: utf-8 -*-
    # Natron Project

    import NatronEngine


    def createProject(app):
        source = app.createReader({source!r})
        source.setScriptName("Source")

        grade = app.createNode("net.sf.openfx.GradePlugin")
        grade.setScriptName("Grade")
        for channel in range(3):
            grade.getParam("multiply").setValue({gain}, channel)
        grade.connectInput(0, source)

        output = app.createWriter({output!r})
        output.setScriptName("Output")
        output.connectInput(0, grade)
        output.getParam("frameRange").setValue(2)  # manual
        output.getParam("firstFrame").setValue({first})
        output.getParam("lastFrame").setValue({last})

        # after the Read, which extends it when it reads its sequence
        frames = app.getProjectParam("frameRange")
        frames.setValue({first}, 0)
        frames.setValue({last}, 1)


    if __name__ == "__main__":
        createProject(app)
    '''

    with open("grade.py", "w", encoding="utf-8") as f:
        f.write(PROJECT.format(first=1, last=10, gain=1.2,
                               source="[Project]/plates/plate.####.exr",
                               output="[Project]/graded/graded.####.png"))

Then::

    NatronRenderer grade.py

renders frames 1 to 10 to *graded/* (create that folder first), next to *grade.py*. ``[Project]`` is the folder of the project
file, so the project can be moved with its images. Open *grade.py* in Natron to see the node graph,
and *File > Save* it: Natron writes it again in its own, longer form, with every parameter that
differs from its default.

Writing a project by hand:

- Create the nodes with :meth:`App.createNode()<NatronEngine.App.createNode>`, and the Read and Write
  nodes with :meth:`App.createReader()<NatronEngine.App.createReader>` and
  :meth:`App.createWriter()<NatronEngine.App.createWriter>`, which give them their file when they are
  created. While the script runs, the nodes don't react to parameter changes, as when a PyPlug creates
  its nodes: a Read whose file is set later reads it once the script has run.
- Set the project frame range (the ``frameRange`` project parameter) at the end of the script: the
  project keeps the frame range that the script leaves, as a *.ntp* file keeps the saved one, and the
  Reads don't extend it after the script.
- Use the script-names of the nodes and of their parameters (see :ref:`nodeScriptName` and
  :ref:`paramScriptName`), and connect the
  nodes with :meth:`Effect.connectInput()<NatronEngine.Effect.connectInput>`.
- Files saved by Natron are a good reference for the parameters of a node: save a small project
  as Python and read it.

Limitations
-----------

- The expressions and links of the parameters of Roto shapes, paint strokes and tracks (those of the
  node parameters are saved).
- The GUI data: the window layout, the viewer settings and the node graph zoom
  (see :meth:`App.projectFileHasGuiData()<NatronEngine.App.projectFileHasGuiData>`). A project saved
  by NatronRenderer has no node positions, as in a *.ntp* file.
- The nodes added or removed from Python inside a PyPlug instance that was never edited: the instance
  is saved as a reference to its PyPlug, which must be installed, with its parameters and the
  parameters of its nodes (editing a PyPlug instance converts it to a Group, which is saved with its
  nodes).
- The project information that every save changes (*Last Saved On*, *Last Author*, *Saved With*), so
  that saving the same project twice writes the same file.
- A keyframe whose tangents Natron would compute differently (edited, or computed by an older Natron)
  gets them explicitly, and reloads as a free keyframe: Natron no longer recomputes them when its
  neighbours change.
- A missing plug-in or PyPlug stops the load, where a *.ntp* file would load the rest of the project.
- Saving a reloaded project again writes the same file, except that the tangents of animated Roto shapes
  may change in their last digit (Natron stores them relative to their point).

Compatibility
-------------

- *.ntp* files (text and binary) are saved and loaded as before.
- A Python project is not a PyPlug: a PyPlug starts with ``# Natron PyPlug`` and defines
  ``createInstance(app, group)`` (see :ref:`pyplugWriting`), and the PyPlug scan ignores Python
  projects, also in a PyPlug search path. Natron never takes one for the other.
- Python projects need Natron 2.6 or later. An older Natron runs the file as a script, which may build
  the project, but doesn't treat it as one.
