.. _natronExec:

Command-line usage (Natron and NatronRenderer)
==============================================

Natron can be run from the command line, for example to render a project on a render farm,
to batch-process files with a Python script, or to type Python commands in an interpreter.
This page lists the command-line options and gives a few recipes.

The executables
---------------

Natron comes with two executables:

- **Natron** starts the graphical user interface. With the :option:`-b` option, it runs in
  background mode, without any window.
- **NatronRenderer** always runs in background mode. It doesn't need a display, and it doesn't
  run the startup scripts that build the user interface (:ref:`initGui.py<startupScripts>`).

NatronRenderer is installed next to the Natron executable: in the ``bin`` directory of the
Linux and Windows installations (``NatronRenderer.exe`` on Windows), and in
``Natron.app/Contents/MacOS`` on macOS.

Run ``NatronRenderer --help`` to print the options of your version.

Execution modes
---------------

The command line selects one of these modes:

- **GUI mode**: ``Natron [<project or script>]``, without :option:`-b` or :option:`-t`, opens the
  user interface. A project (``.ntp``) is loaded as if it were opened from the *File* menu. A Python script is run before the user
  interface is created, and nothing is rendered.
- **Background project**: ``NatronRenderer <project.ntp>`` (or ``Natron -b <project.ntp>``)
  loads the project and renders its Write nodes.
- **Background script**: ``NatronRenderer <script.py>`` runs a Python script that builds a
  node graph, then renders its Write nodes (see :ref:`below<cl-python-scripts>`).
- **Interpreter**: ``NatronRenderer -t [<script.py|project.ntp>]`` runs the optional script, or
  loads the optional project, then reads Python commands from the standard input.

In all modes, the Python variable ``app`` points to the application instance (the project).

General options
---------------

.. option:: -h, --help

   Print the usage and exit.

.. option:: -v, --version

   Print the Natron version, with the git commit and branch it was built from, and exit.

.. option:: -b, --background

   Run in background mode: no user interface is shown. NatronRenderer and :option:`-t` imply it.

.. option:: -t [<script.py|project.ntp>], --interpreter [<script.py|project.ntp>]

   Interpreter mode: run the optional Python script, or load the optional project, then read Python commands from the
   standard input and execute them. Write nodes are not rendered automatically: call
   :func:`app.render()<NatronEngine.App.render>` yourself. Only ``init.py`` is run, not
   ``initGui.py``, even with the Natron executable.

.. option:: -c <PythonCommand>, --cmd <PythonCommand>

   Run a line of Python code before the project is loaded or the script is run. This option can be
   given several times; the commands run in the order they are given. Use it to pass
   parameters to a script (see :ref:`the example below<cl-example-python>`).

.. option:: --setting <name>=<value>

   Set the Natron setting (preference) *name* to *value*. The value is a Python expression, so
   a string must be quoted, as in ``--setting enableOpenGLRendering='"enabled"'``; for a choice
   setting, the value may also be the index of the option. The names are the script names of the
   settings, as used by ``natron.getSettings().getParam(name)``. The option can be given several
   times. The settings are applied after the preferences are read and before the plug-ins are
   loaded, and when this option is given the settings are **not saved**: the preferences file is
   left unchanged.

.. option:: --no-settings

   Ignore the preferences file and use the default settings. The settings are not saved either.

.. option:: --opengl <enabled|disabled|foreground>

   Select whether OpenGL rendering is used: ``enabled`` (if available), ``disabled`` (even if a
   project enables GPU rendering), or ``foreground`` (only when not using NatronRenderer and when
   rendering runs in the foreground). This is a shortcut for the ``enableOpenGLRendering`` setting.

.. option:: --clear-cache

   Clear the image cache on startup.

.. option:: --clear-openfx-cache

   Clear the OpenFX plug-ins cache on startup, so that all OpenFX plug-ins are loaded again.

.. option:: --export-docs <directory>

   Write the reStructuredText documentation of the loaded nodes and of the preferences to
   *directory*, and exit. ``tools/genStaticDocs.sh`` uses it to generate the node pages of
   this documentation.

.. option:: --

   End of the options: what follows is the project or script file name, even if it starts with ``-``.

Rendering a project
-------------------

::

    NatronRenderer [options] [--] <project.ntp> [<frameRanges>]

.. option:: -w <WriteNode> [<filename>], --writer <WriteNode> [<filename>]

   Render only with the Write node whose script name is *WriteNode*. If there is no such node,
   Natron exits with an error. Without any :option:`-w` option, all the Write nodes of the
   project are rendered. The option can be given several times to render several Write nodes.

   The optional *filename* replaces the output file name of the Write node. The argument is only
   taken as a file name if it doesn't look like a frame range and doesn't end with ``.py`` or ``.ntp``.

.. option:: -i <ReadNode> <filename>, --reader <ReadNode> <filename>

   Set the file (or image sequence, or video) read by the Read node whose script name is
   *ReadNode*. If there is no such node, Natron exits with an error.

.. option:: -l <script.py>, --onload <script.py>

   Run a Python script after the project is created or loaded, and after the
   *After project created* and *After project loaded* callbacks. This also works in GUI mode.
   The rules for :ref:`Python scripts<cl-python-scripts>` apply to this script.

.. option:: -s, --render-stats

   Write render statistics for each frame, in a text file next to each rendered image, with the
   same name and a ``-stats.txt`` suffix. It lists, for each node, the render time and other
   information, which helps to check or debug a render. It doesn't work when writing video files.

The :option:`-w`, :option:`-i` and :option:`-o` options are only accepted in background mode, and
:option:`-w` and :option:`-i` are also refused in interpreter mode.
Node names are script names (see :ref:`autoVar`), so they can't contain spaces or
other characters that are not allowed in Python names.

*frameRanges* is a positional argument: one or more frame ranges, separated by commas.
Each range is one of:

- ``<frame>``: a single frame, e.g. ``57``;
- ``<firstFrame>-<lastFrame>``: e.g. ``10-40``;
- ``<firstFrame>-<lastFrame>:<frameStep>``: e.g. ``1-10:2`` renders the frames 1, 3, 5, 7 and 9.

For example ``1-10:1,20-30:2,40-50`` or ``1329,2450,123,1-10:2``. The same frame ranges are used for
all the Write nodes.

.. note::

    Put the frame ranges after the project file name. Right after ``-w <WriteNode>``, a frame
    range with a step (``1-10:2``) would be taken as an output file name.

Use absolute file paths for the project, the script and the file names given to :option:`-w`
and :option:`-i`. On Linux and macOS, Natron also expands a leading ``~`` itself, e.g. when the
file name is quoted. In some shells ``#`` or ``(`` are special characters: quote file names
such as ``'/FastDisk/Pictures/sequence###.exr'``.

Examples::

    Natron /Users/Me/MyNatronProjects/MyProject.ntp

    Natron -b -w MyWriter /Users/Me/MyNatronProjects/MyProject.ntp

    NatronRenderer -w MyWriter /Users/Me/MyNatronProjects/MyProject.ntp

    NatronRenderer -w MyWriter /FastDisk/Pictures/sequence###.exr /Users/Me/MyNatronProjects/MyProject.ntp 1-100

    NatronRenderer -w MyWriter -w MySecondWriter /Users/Me/MyNatronProjects/MyProject.ntp 1-10

    NatronRenderer -w MyWriter -l /Users/Me/Scripts/onProjectLoaded.py /Users/Me/MyNatronProjects/MyProject.ntp 1-10

An example of a script passed to :option:`--onload`, which creates a Write node in every project
that is created or loaded (:func:`createWriter()<NatronEngine.App.createWriter>` picks the
Write plug-in from the file extension)::

    writer = app.createWriter("/FastDisk/Pictures/output###.exr")

.. _cl-python-scripts:

Running a Python script
-----------------------

::

    NatronRenderer [options] [--] <script.py> [<frameRanges>]

This doesn't apply to the interpreter mode (:option:`-t`).

The script can be a PyPlug (a Group exported from the user interface), an exported
project, or a script written by hand. Natron first looks for a function with the
following signature in the script::

    def createInstance(app,group):

If the function is found, the script is imported as a module and the function is called.
The script then only sees the variables passed to *createInstance* and the modules it imports.
Otherwise, the whole script is run as if it were given to Python, and it can use the
variables declared by Natron (``app``, ``app1``, the nodes...).

In both cases, ``app`` is defined and points to the application instance.
In GUI mode, Natron runs the script before creating the user interface, and doesn't render.
In background mode, Natron then renders the Write nodes given with :option:`-w`, or the
Output nodes given with :option:`-o`; without these options, it renders all the Write nodes
of the graph.

.. option:: -o <filename>, --output <filename>

   Replace the node named *Output1* in the script with a Write node that writes *filename*, as
   when a Write node is created in the user interface, and render it. A frame range must be given.

.. option:: -o<N> <filename>, --output<N> <filename>

   The same, for the node named *Output<N>*: ``-o1`` uses *Output1*, ``-o2`` uses *Output2*, etc.
   Use it to render several Output nodes.

Examples::

    Natron /Users/Me/MyNatronScripts/MyScript.py

    Natron -b -w MyWriter /Users/Me/MyNatronScripts/MyScript.py

    NatronRenderer -w MyWriter /Users/Me/MyNatronScripts/MyScript.py

    NatronRenderer -o /FastDisk/Pictures/sequence###.exr /Users/Me/MyNatronScripts/MyScript.py 1-100

    NatronRenderer -o1 /FastDisk/Pictures/sequence###.exr -o2 /FastDisk/Pictures/test###.exr /Users/Me/MyNatronScripts/MyScript.py 1-100

    NatronRenderer -w MyWriter -o /FastDisk/Pictures/sequence###.exr /Users/Me/MyNatronScripts/MyScript.py 1-100

    NatronRenderer -c 'print("Now executing MyScript.py...")' -w MyWriter /FastDisk/Pictures/sequence.mov /Users/Me/MyNatronScripts/MyScript.py 1-100

Using the interpreter
---------------------

::

    NatronRenderer -t [--] [<script.py|project.ntp>]

Natron runs the script, or loads the project, given as argument, if any, and then reads
Python commands from the standard input. Examples::

    Natron -t

    NatronRenderer -t

    NatronRenderer -t /Users/Me/MyNatronScripts/MyScript.py

Order of execution
------------------

In background mode, Natron does the following, in this order:

1. read the preferences (unless :option:`--no-settings` is given), then apply the
   :option:`--setting` and :option:`--opengl` options;
2. load the plug-ins, and run the :ref:`startup scripts<startupScripts>` (``init.py``) and
   load the PyPlugs;
3. run the :option:`-c` commands;
4. load the project, or run the Python script;
5. run the :option:`--onload` script;
6. apply the :option:`-w` and :option:`-o` options, then the :option:`-i` file names;
7. render (in interpreter mode, read commands from the standard input instead).

Exit status
-----------

NatronRenderer exits with status 0 when it succeeds (and after :option:`-h` or :option:`-v`),
and with status 1 when the command line is wrong, when the project can't be loaded
(for example when a node given to :option:`-w` or :option:`-i` doesn't exist), or when the
Python script, or the :option:`--onload` script, raises an exception: Natron prints the error
and renders nothing. A script can also call ``sys.exit(status)`` to exit with its own status.
Up to Natron 2.5, a Python error in the script did not set status 1, and the Write nodes
created before the error were still rendered
(`issue #1276 <https://github.com/NatronGitHub/Natron/issues/1276>`_).

In interpreter mode (:option:`-t`), a Python error in the script is printed, and the
interpreter still starts, so that the error can be inspected.

Check the output for errors that happen while frames are rendered.

Example: converting an image sequence
-------------------------------------

A typical example is converting an image sequence to another format. This can be done with a
project file or with a Python script.

With a Natron project (.ntp) file
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Let's suppose the user already set up the project in the user interface, with two nodes::

    MyReader ---> MyWriter

The render can then be launched from the command line::

    NatronRenderer -w MyWriter /path/to/myProject.ntp 10-20

The Read and Write file names can be changed from the command line::

    NatronRenderer -i MyReader /Users/Toto/Sequences/AnotherSequence__####.exr -w MyWriter /Users/Toto/Sequences/mySequence.mov /path/to/myProject.ntp 10-20

.. _cl-example-python:

With a Python script file
^^^^^^^^^^^^^^^^^^^^^^^^^

We write a Python script that we pass to the command line::

    #This is the content of myStartupScript.py

    reader = app.createReader("/Users/Toto/Sequences/Sequence__####.exr")
    writer = app.createWriter("/Users/Toto/Sequences/Sequence.mov")

    #The node will be accessible via app.MyWriter after this call
    #We do this so that we can reference it from the command-line arguments
    writer.setScriptName("MyWriter")

    #The node will be accessible via app.MyReader after this call
    reader.setScriptName("MyReader")

    #Set the format type parameter of the Write node to Input Stream Format so that the video
    #is written to the size of the input images and not to the size of the project
    formatType = writer.getParam("formatType")
    formatType.setValue(0)

    #Connect the Writer to the Reader
    writer.connectInput(0,reader)

    #When using Natron (Gui) then the render must explicitly be requested.
    #Otherwise if using NatronRenderer or Natron -b the render will be automatically started
    #using the command-line arguments

    #To use with Natron (Gui) to start render
    #app.render(writer, 10, 20)

To run this script in the background::

    NatronRenderer -w MyWriter /path/to/myStartupScript.py 10-20

The input sequence and the output file name are fixed in the script. The command-line options
can change them::

    NatronRenderer -i MyReader /Users/Toto/Sequences/AnotherSequence__####.exr -w MyWriter /Users/Toto/Sequences/mySequence.mov /path/to/myStartupScript.py 10-20

Now let's suppose that the frame rate of the output video must also change for each sequence.
No command-line option sets it, but the :option:`-c` option runs Python code before the
script, so it can define variables that the script uses::

    NatronRenderer -c "fpsValue=60" -w MyWriter /path/to/myStartupScript.py 10-20

with this line added at the end of the script::

    writer.getParam("fps").set(fpsValue)

The Read and Write file names can be passed the same way::

    NatronRenderer -c "fpsValue=60" -c "readFileName='/Users/Toto/Sequences/AnotherSequence__####.exr'" -c "writeFileName='/Users/Toto/Sequences/mySequence.mov'" /path/to/myStartupScript.py 10-20

with the following changes to the script::

    ...
    reader = app.createReader(readFileName)
    writer = app.createWriter(writeFileName)
    ...
    writer.getParam("fps").set(fpsValue)

:option:`-w` is not needed here: without it, all the Write nodes of the graph are rendered.
