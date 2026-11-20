.. _developers-guide:

Developers Guide
================

This guide is for people who automate or extend Natron without changing its C++ code:

- Python script authors, who automate tasks, set up projects, or add menu commands and panels;
- PyPlug authors, who package node graphs as reusable nodes;
- pipeline TDs, who render projects from the command line or on a render farm;
- OpenFX plug-in authors, who write new nodes in C or C++ as OpenFX plug-ins.

To change Natron itself, read the :ref:`maintainers-guide`. To use Natron, read the :ref:`user-guide`.

Natron 2.6 embeds Python 3: the version comes with your Natron build, and you can check it with
``import sys; print(sys.version)`` in the *Script Editor*. The Qt bindings are PySide2 in Qt 5 builds
of Natron and PySide6 in Qt 6 builds. Write your Qt code against
`qtpy <https://github.com/spyder-ide/qtpy>`_ (``from qtpy import QtCore, QtGui, QtWidgets``), which
Natron bundles and loads at startup, so that it works with both.

Where to start:

- :ref:`natronObjects` explains the objects that Natron gives to your scripts (``natron``, ``app1``,
  nodes, parameters), then :ref:`creatingNode` shows how to build a node graph.
- :ref:`natronExec` explains how to render from the command line with NatronRenderer.
- :ref:`guiTesting` explains how to drive and test the user interface from scripts.
- :ref:`groups` explains how to turn a Group into a PyPlug, and :ref:`pyplugWriting` how to write one.
- :ref:`debuggingPython` explains where the output and the errors of your scripts go.
- :ref:`pythonEnvironment` explains which Python Natron uses and how to add modules such as numpy.
- :ref:`devel-openfx` is for authors of OpenFX plug-ins (C or C++), with the OpenFX Programming Guide
  and Reference.
- The :ref:`apiReference` lists all the classes and functions.

.. toctree::
    :maxdepth: 2

    introduction
    gettingstarted
    pyplugs
    python-gui
    commandline
    guitesting
    openfx
    devel-faq
    tutorials
    APIReference
