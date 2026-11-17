.. _maint-openfx:

Natron as an OpenFX Host
========================

`OpenFX (OFX) <http://openeffects.org/>`_ is the industry-standard C API for
image-effect plug-ins, also used by Nuke, DaVinci Resolve, Fusion and others.
Natron implements the **host** side of OFX v1.4 (with many extensions), which is
why Natron can load the same plug-ins as those commercial applications, and why
its own bundled effects (``openfx-io``, ``openfx-misc``, ``openfx-arena``,
``openfx-gmic``) are just OFX plug-ins living in separate repositories.

Understanding this is essential: **most nodes are OFX plug-in instances**, and a
large fraction of the Engine exists to present Natron's internals to plug-ins in
the shape the OFX contract expects.

Where the host code lives
-------------------------

- ``libs/OpenFX`` (a git submodule) — the official OpenFX headers, the C++
  plug-in *support* library, and the generic C++ host classes
  (``libs/OpenFX/HostSupport``, namespace ``OFX::Host``): plug-in cache and
  loading, property sets, image-effect descriptors and instances, clip and
  parameter descriptors, interact descriptors. This layer is
  application-agnostic. ``libs/OpenFX_extensions`` adds the extension suites
  Natron supports.
- ``HostSupport/`` — only the build files that compile
  ``libs/OpenFX/HostSupport`` into the ``HostSupport`` library.
- ``Engine/Ofx*`` — the **Natron-specific glue** that connects the generic host
  to Natron's node/knob/image model.

The glue classes
----------------

``OfxHost`` (``OfxHost.cpp``)
    Natron's implementation of the OFX host object. Advertises host capabilities
    to plug-ins, discovers and caches plug-ins at startup, and creates effect
    instances. Reached via the ``AppManager``.

``OfxEffectInstance`` (``OfxEffectInstance.cpp``), which derives from ``AbstractOfxEffectInstance``
    The bridge that makes an OFX plug-in look like a Natron ``EffectInstance``.
    It translates Natron's actions (``render``, ``getRegionOfDefinition``,
    ``getRegionsOfInterest``, ``getFramesNeeded``, ``isIdentity``, metadata,
    ``knobChanged``) into OFX actions, and vice versa.
    ``AbstractOfxEffectInstance`` (in ``OfxEffectInstance.h``) derives from
    ``OutputEffectInstance``, so every OFX effect is also an output effect
    that can drive a render (see :ref:`maint-architecture`).

``OfxImageEffectInstance`` (``OfxImageEffectInstance.cpp``), which derives from ``OFX::Host::ImageEffect::Instance``
    The concrete host-side image-effect instance; it owns the plug-in's clips
    and parameters and answers the plug-in's host callbacks.

``OfxClipInstance`` (``OfxClipInstance.cpp``)
    Presents a Natron input (and the images flowing through it) to the plug-in
    as an OFX *clip*. Wraps Natron ``Image`` objects as ``OfxImage`` so a plug-in
    can fetch and address pixels.

``OfxParamInstance`` (``OfxParamInstance.cpp``) and ``OfxStringInstance``
    Present Natron **knobs** to the plug-in as OFX *parameters*, keeping the two
    in sync in both directions. ``OfxParamToKnob`` maps an OFX parameter type to
    the matching Natron ``Knob`` subclass.

``OfxOverlayInteract`` / ``OfxParamOverlayInteract``
    Route the plug-in's viewport overlay drawing and mouse/key interaction
    through Natron's viewer (via the ``OverlaySupport`` interface).

``OfxMemory`` / ``PluginMemory``
    Implement the OFX memory-suite so plug-ins can allocate scratch buffers
    that participate in Natron's memory accounting.

How a plug-in becomes a node
----------------------------

Loading the plug-ins happens once, at startup, in ``OfxHost::loadOFXPlugins``
(``Engine/OfxHost.cpp``), called by ``AppManager::loadAllPlugins`` (see
:ref:`maint-startup`):

1. It gets the HostSupport plug-in cache (``OFX::Host::PluginCache``) and sets
   its version label (``NatronOFXCachev2``: changing it forces all plug-ins to
   be described again).
2. It builds the search path. The ``PluginCache`` constructor (in
   ``libs/OpenFX/HostSupport``) has already added the directories of the
   ``OFX_PLUGIN_PATH`` environment variable. ``loadOFXPlugins`` then adds the
   standard OFX directories (``setPluginHostPath("Natron")`` and ``("Nuke")``,
   unless the preferences say not to), the extra directories from the
   preferences, and the ``Plugins/OFX/Natron`` directory bundled with Natron
   (prepended or appended, depending on the preferences).
3. It reads the cache file from the previous run (``OFXLoadCache`` in the
   user's cache directory), then scans the directories
   (``scanPluginFiles``). A plug-in binary that is not in the cache, or that
   changed, is loaded and described (``kOfxActionLoad`` and
   ``kOfxActionDescribe``) during the scan; the others are not even loaded. If
   anything changed, the cache file is written again (``writeOFXCache``).
4. For each plug-in (``getPluginsByIDMajor``), it reads the label, grouping,
   icon, version and supported file formats, and registers it with
   ``AppManager::registerPlugin``. That is what makes it appear in the
   node-creation menu. Readers and writers are also entered in the tables that
   map a file extension to the best reader or writer.

When the user creates such a node, ``AppInstance::createNodeInternal``
(``Engine/AppInstance.cpp``) first makes sure the plug-in is described in a
context: on first use, ``OfxHost::getPluginContextAndDescribe`` picks the
context (filter, general, reader, writer, …) and calls
``kOfxImageEffectActionDescribeInContext``. It then creates the ``Node``, and
``Node::load`` (``Engine/NodeMain.cpp``) calls ``OfxHost::createOfxEffect``,
which creates the ``OfxEffectInstance`` and its ``OfxImageEffectInstance``
(``OfxEffectInstance::createOfxImageEffectInstance``). ``populate()`` creates
the OFX clip and parameter instances, and with them the Natron knobs
(``OfxParamToKnob`` maps each OFX parameter type to a knob class); then the
saved values, if any, are loaded and ``kOfxActionCreateInstance``
is called. The plug-in's inputs become the node's input arrows.

Readers and writers are a special case: the ``ReadNode`` and ``WriteNode``
built-ins are thin dispatchers that, based on the file extension, embed the
appropriate OFX reader/writer plug-in (from ``openfx-io``) and expose its
parameters.

Practical guidance
------------------

- When a bug reproduces with *all* plug-ins, suspect the host glue
  (``Engine/Ofx*`` or ``HostSupport``). When it reproduces with only one
  plug-in, the bug is probably in that plug-in's own repository, not here.
- Changing host behavior can affect every third-party plug-in, including
  commercial ones. Treat the host↔plug-in contract as a public API: prefer
  additive, capability-flagged changes and test against the four bundled plug-in
  sets.
