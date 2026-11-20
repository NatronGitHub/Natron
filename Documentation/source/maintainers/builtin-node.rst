.. _maint-builtin-node:

Adding a Built-in Node
======================

Most Natron nodes are OpenFX plug-ins that live in other repositories
(``openfx-misc``, ``openfx-io``, ``openfx-arena``, ``openfx-gmic``). **Write a
new node as an OpenFX plug-in** whenever you can: it works in other hosts, it
does not need a Natron release, and the OpenFX API is stable. The
:ref:`developers-guide` explains how.

A built-in node, written in C++ inside ``Engine``, is only justified when the
node needs the host's internals: an internal node graph (``NodeGroup``,
``WriteNode``, ``TrackerNode``), the viewer (``ViewerInstance``), the project's
views (``OneViewNode``, ``JoinViewsNode``), or no rendering at all (``Dot``,
``Backdrop``). This chapter lists the steps, using the existing built-ins as
examples.

1. Choose the base class
------------------------

Derive from the class that matches what the node does (see
:ref:`maint-architecture` for the full hierarchy):

- ``EffectInstance`` for a node that renders images (``JoinViewsNode``,
  ``OneViewNode``, ``PrecompNode``);
- ``NoOpBase`` for a node that passes its input through (``Dot``,
  ``Backdrop``);
- ``NodeGroup`` for a node that is implemented as an internal graph of other
  nodes (``TrackerNode``, ``WriteNode``);
- ``OutputEffectInstance`` for a node that drives renders.

2. Write the class
------------------

Put it in ``Engine/Foo.h`` and ``Engine/Foo.cpp``, inside
``NATRON_NAMESPACE_ENTER`` / ``NATRON_NAMESPACE_EXIT``, with the Python block
first in the ``.cpp`` (see :ref:`maint-design`). ``Engine/Dot.h`` is the
smallest complete example, and ``Engine/JoinViewsNode.h`` a small one that
renders. The class needs:

- a static factory, used to register the plug-in::

      static EffectInstance* BuildEffect(NodePtr n)
      {
          return new Foo(n);
      }

- a constructor that takes the ``NodePtr`` and passes it to the base class;
- the plug-in description: ``getPluginID()`` (add a ``PLUGINID_NATRON_FOO``
  next to the others in ``Engine/EffectInstance.h``; the existing ones are
  ``fr.inria.built-in.*``), ``getPluginLabel()``,
  ``getPluginDescription()``, ``getPluginGrouping()``,
  ``getMajorVersion()`` / ``getMinorVersion()``;
- the inputs: ``getNInputs()``, ``getInputLabel()``, ``isInputOptional()``;
- what it supports: ``addAcceptedComponents()``, ``addSupportedBitDepth()``,
  ``renderThreadSafety()``, ``supportsTiles()``,
  ``supportsMultiResolution()``, ``isViewAware()`` and so on;
- its actions, overriding only those it needs: ``getRegionOfDefinition()``,
  ``getRegionsOfInterest()``, ``getFramesNeeded()``, ``isIdentity()``,
  ``render()`` (the virtuals in ``Engine/EffectInstance.h``; see
  :ref:`maint-rendering`).

3. Add its parameters
---------------------

Override ``initializeKnobs()`` and create the knobs there with
``AppManager::createKnob<KnobType>(this, label)``; give each one a script name
with ``setName()`` and add it to a page (``KnobPage``). Override
``knobChanged()`` to react to changes. See :ref:`maint-knobs`.

The script names of the knobs are part of the project format and of the
Python API: once a node is released, don't rename or remove a knob. Bump the
plug-in's version instead, and handle the old knobs when an old project is
loaded (see :ref:`maint-serialization`).

4. Register it
--------------

Add one line to ``AppManager::loadBuiltinNodePlugins()``
(``Engine/AppManager.cpp``)::

    registerBuiltInPlugin<Foo>(QString::fromUtf8(NATRON_IMAGES_PATH "foo_icon.png"), false, false);

The arguments are the icon (a Qt resource path: add the image to
``Gui/Resources/Images`` and to ``Gui/GuiResources.qrc``), whether the plug-in
is deprecated, and whether it is for internal use only (hidden from the
menus, like ``RotoSmear``). ``registerBuiltInPlugin`` builds one instance with
``BuildEffect`` to read its description, and stores ``BuildEffect`` so that
``Node::load`` can create the effect of each new node.

5. Add the files to both builds
-------------------------------

CMake picks up every ``Engine/*.cpp`` and ``Engine/*.h`` (``file(GLOB …)`` in
``Engine/CMakeLists.txt``): re-run CMake after adding the files. The qmake build
lists them one by one: add them to ``SOURCES`` and ``HEADERS`` in
``Engine/Engine.pro``. CI runs ``tools/utils/checkQmakeSources.py``, which fails
if the two builds disagree.

6. Document and test it
-----------------------

The node's page in the Reference Guide (``Documentation/source/plugins/``) is
generated from ``getPluginDescription()`` and the knobs' hints by
``NatronRenderer --export-docs``, through ``tools/genStaticDocs.sh``:
regenerate the pages rather than writing one by hand. Built-in nodes need no
plug-in to load, so they are easy to use in unit tests (``Tests/``, for
example ``Tests/KnobSlave_Test.cpp`` uses ``Dot`` nodes); register a new test
file in both ``Tests/CMakeLists.txt`` and ``Tests/Tests.pro``.
