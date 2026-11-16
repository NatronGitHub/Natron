.. _maintainers-guide:

Maintainer Guide
================

This guide is for **developers who want to contribute to Natron's own source
code** — fixing bugs, adding features, porting to new platforms or toolkits,
and maintaining the code base over time.

It is different from the :ref:`developers-guide`, which explains how to *use*
Natron through Python scripting and how to write plug-ins. If you want to write
a PyPlug, an OpenFX plug-in, or automate Natron with Python, read the
Developers Guide instead. If you want to understand and change the C++ code that
*is* Natron, you are in the right place.

The guide assumes you are comfortable with modern C++, have a working knowledge
of `Qt <https://www.qt.io/>`_, and understand the basics of image processing and
OpenGL. You do **not** need prior knowledge of the OpenFX standard — it is
introduced where needed.

.. toctree::
    :maxdepth: 2

    overview
    building
    build-macos
    codebase-map
    architecture
    design-techniques
    engine
    gui
    rendering
    openfx-host
    python-bindings
    subsystems
    contributing
    qt6-migration
    todo
    issue-triage
