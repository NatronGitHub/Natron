.. _pyplugs:

PyPlugs and toolsets
====================

A PyPlug is a Group node saved as a Python script, which Natron then shows as a node of its own.
A toolset is a script that adds a set of nodes to the node graph. This section explains how to
create, write and distribute them. A PyPlug's button can also render and read the result back, or
make its edits one undo step: see :ref:`pyplugWriting-renderReadBack`.

.. toctree::
    :maxdepth: 1

    groups.rst
    pyplug-writing.rst

Start with :ref:`the steps of Writing a PyPlug<pyplugWriting-path>`, which go from a first PyPlug
to its distribution, and end with :ref:`the most frequent mistakes<pyplugWriting-pitfalls>`.
The other pages about PyPlugs are:

- :ref:`callbacks`: the functions that Natron calls when a parameter, an input or the project
  changes;
- :ref:`natronPath`: where Natron looks for PyPlugs, and ``NATRON_PLUGIN_PATH``;
- :ref:`startupScripts`: ``init.py`` and ``initGui.py``, which run before the PyPlugs are loaded;
- :ref:`debuggingPython`: where the errors of PyPlugs and callbacks are shown.
