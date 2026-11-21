.. _maint-openfx:

Natron as an OpenFX Host
========================

`OpenFX (OFX) <http://openeffects.org/>`_ is the industry-standard C API for
image-effect plug-ins, also used by Nuke, DaVinci Resolve, Fusion and others.
Natron implements the **host** side of OFX v1.5 (with many extensions), which is
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
- CMake builds ``libs/OpenFX/HostSupport`` (plus ``libs/OpenFX_extensions``)
  as openfx's ``OfxHost`` target; ``HostSupport/`` only holds the qmake
  project.
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
    Overlays may draw with OpenGL (``kOfxImageEffectPluginPropOverlayInteractV1``)
    or with the OFX 1.5 DrawSuite (``kOfxImageEffectPluginPropOverlayInteractV2``).
    If a plug-in has both, the V1 overlay is used
    (``OfxHost::preferOverlayInteractV1()``). ``OfxDrawSuiteGL``
    (``OfxDrawSuiteGL.cpp``) draws the DrawSuite calls with OpenGL, in the same
    canonical view and GL context as the OpenGL overlays. It is a thin adapter
    over HostSupport's ``OFX::Host::Interact::OpenGLDrawSuite``
    (``ofxhDrawSuiteOpenGL.h``), which draws with the openfx OpenGL DrawSuite core
    (``ofxDrawSuiteOpenGL.h``), the code that the Support library also uses to
    emulate the DrawSuite in OpenGL overlays. Natron provides the OpenGL
    functions (from glad, once OpenGL is loaded), its overlay colours, the
    viewport's font for the text (``OverlaySupport::canRenderText()``; otherwise
    the core's built-in font), and the shadows: the "Draw shadows on plug-in
    overlays drawn without OpenGL" preference (``drawSuiteShadows``, on by
    default) answers ``drawSuiteShadows()``, which HostSupport passes to every
    draw as ``kNatronOfxInteractPropDrawSuiteShadows``. So it also applies to the
    V1 (OpenGL) overlays of plug-ins built with the openfx Support library that
    draw through its DrawSuite emulation (``OFX::DrawArgs::getDrawContext()``):
    that emulation reads the property and draws the same shadows.

``OfxMemory`` / ``PluginMemory``
    Implement the OFX memory-suite so plug-ins can allocate scratch buffers
    that participate in Natron's memory accounting.

Conformance
-----------

How closely Natron follows the host side of the OpenFX 1.5.1 specification is
recorded in :ref:`ofx-host-conformance`: one table row per MUST, SHOULD or MAY
statement addressed to hosts, with what Natron and HostSupport do, a verdict,
and a link to the issue that tracks each deviation. Check it before changing
host behaviour, and update it when you fix one of those issues.

How a plug-in becomes a node
----------------------------

.. mermaid::
   :alt: At startup, OfxHost::loadOFXPlugins sets the cache version and the
         search path of the HostSupport PluginCache, reads the cache file and
         scans the plug-in files, which loads and describes only the new or
         changed binaries, writes the cache if it changed, and registers each
         plug-in with AppManager. When the user creates a node, AppInstance
         asks OfxHost to describe the plug-in in a context on first use,
         creates the Node, whose load calls OfxHost::createOfxEffect, which
         creates the OfxEffectInstance; populate creates the clips, parameters
         and knobs, and the create-instance action is called.
   :caption: Loading the OpenFX plug-ins at startup, and creating an OpenFX
             node.

   sequenceDiagram
       participant AM as AppManager
       participant OH as OfxHost
       participant PC as OFX::Host::PluginCache
       participant P as OFX plug-in
       participant AI as AppInstance
       participant N as Node
       participant OE as OfxEffectInstance
       Note over AM,P: At startup
       AM->>OH: loadOFXPlugins()
       OH->>PC: setCacheVersion(getPluginCacheVersion())<br/>"NatronOFXCachev3-" + hash of the host name
       OH->>PC: setPluginHostPath("Natron"), ("Nuke"), addFileToPath(...)
       Note right of PC: OFX_PLUGIN_PATH was added by the PluginCache constructor
       OH->>PC: readCache(), scanPluginFiles()
       PC->>P: new or changed binaries only: kOfxActionLoad, kOfxActionDescribe
       OH->>OH: writeOFXCache() if the cache changed
       loop each plug-in of getPluginsByIDMajor()
           OH->>AM: registerPlugin()
       end
       Note over AM,OE: When the user creates a node
       AI->>OH: getPluginContextAndDescribe() (first use only)
       OH->>P: kOfxImageEffectActionDescribeInContext
       AI->>N: new Node, Node::load()
       N->>OH: createOfxEffect()
       OH->>OE: new OfxEffectInstance, createOfxImageEffectInstance()
       OE->>OE: populate(): OFX clips and parameters, and the Natron knobs (OfxParamToKnob)
       OE->>P: kOfxActionCreateInstance

Loading the plug-ins happens once, at startup, in ``OfxHost::loadOFXPlugins``
(``Engine/OfxHost.cpp``), called by ``AppManager::loadAllPlugins`` (see
:ref:`maint-startup`):

1. It gets the HostSupport plug-in cache (``OFX::Host::PluginCache``) and sets
   its version label (``getPluginCacheVersion()``: ``NatronOFXCachev3-``
   followed by a SHA-1 hash of the host name set in the preferences; changing
   it forces all plug-ins to be described again).
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
   A cache file that ``readCache`` cannot read (an XML error, an unknown
   element, a bad property index) throws: ``loadOFXPlugins`` logs the error,
   nothing from that file is kept, and the scan loads and describes every
   plug-in, then rewrites the cache.
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

Host behaviour plug-in authors can notice
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

These rules changed in Natron 2.6 (`#1099 <https://github.com/NatronGitHub/Natron/pull/1099>`_, `#1108 <https://github.com/NatronGitHub/Natron/pull/1108>`_, `#1150 <https://github.com/NatronGitHub/Natron/pull/1150>`_ and the matching
``libs/OpenFX`` updates). They are not always obvious from the OFX
specification, so check them first when a plug-in that worked in Natron 2.5
doesn't load or behaves differently.

Which binary is loaded from a bundle
    A bundle ``NAME.ofx.bundle/Contents/ARCHITECTURE/NAME.ofx`` may hold
    binaries for several platforms. The plug-in cache
    (``PluginCache::scanDirectory`` in
    ``libs/OpenFX/HostSupport/src/ofxhPluginCache.cpp``) checks the
    ``ARCHITECTURE`` folders in the order given by ``archDirsFor()``
    (``libs/OpenFX/HostSupport/include/ofxhArchDirs.h``):

    - macOS: ``MacOS``, then, on an x86-64 host only, the deprecated
      ``MacOS-x86-64`` folder.
    - Windows x64: ``Win64``, then ``win64`` (for case-sensitive folders). An
      x64 host running under emulation on Arm64 checks ``Win-arm64ec`` first. A
      32-bit host uses ``Win32``, then ``win32``.
    - Linux: ``Linux-x86-64`` or ``Linux-x86`` on x86. On other CPUs,
      ``Linux-$(uname -m)`` (e.g. ``Linux-aarch64``), then the name of the
      architecture the host was compiled for if it differs, then the legacy
      ``Linux-x86-64`` (64-bit host) or ``Linux-x86`` (32-bit host) folder,
      where older build systems put every binary.

    The host loads the binary from the first of these folders whose file
    matches the host process (word size and CPU, read from the ELF, Mach-O or
    PE header), so a folder earlier in the list with a binary the host can't
    load is skipped. If no binary matches, the first one found is used, as
    before (openfx `#90 <https://github.com/NatronGitHub/openfx/pull/90>`_, `#103 <https://github.com/NatronGitHub/openfx/pull/103>`_, `#105 <https://github.com/NatronGitHub/openfx/pull/105>`_, `#106 <https://github.com/NatronGitHub/openfx/pull/106>`_, `#112 <https://github.com/NatronGitHub/openfx/pull/112>`_).

Clip depths in the clip-preferences action
    In ``kOfxImageEffectActionGetClipPreferences``, the
    ``OfxImageClipPropDepth_<clip>`` out-args are writable only if both the host
    and the plug-in support multiple clip depths
    (``kOfxImageEffectPropSupportsMultipleClipDepths``), as the OFX
    clip-preferences reference requires. Natron sets it to 1 on the host, so in
    practice this depends on the plug-in. Otherwise these properties are
    read-only for the plug-in
    (``OfxImageEffectInstance::setupClipPreferencesArgsFromMetadata`` in
    ``Engine/OfxImageEffectInstance.cpp``; openfx `#102 <https://github.com/NatronGitHub/openfx/pull/102>`_).

Writes to read-only properties
    HostSupport knows which properties are read-only for plug-ins, and by
    default refuses a plug-in's write to them with ``kOfxStatErrValue``
    (openfx `#91 <https://github.com/NatronGitHub/openfx/pull/91>`_). Natron relaxes this to ``ePluginReadOnlyWarn``
    (``OfxHost::OfxHost`` in ``Engine/OfxHost.cpp``) so that existing
    plug-ins keep working: the write is accepted, and the first write to each
    property name prints ``OFX: warning: a plug-in wrote the read-only property
    <name>`` on stderr. A plug-in that triggers this warning should be fixed.

Cursors
    Natron 2.6 no longer honours the Natron extension's secret
    ``kNatronOfxParamCursorName`` string parameter, and no longer sets the
    ``kNatronOfxImageEffectPropDefaultCursors`` host property (`#1108 <https://github.com/NatronGitHub/Natron/pull/1108>`_). Plug-ins
    should use the interact cursors instead: check
    ``kOfxInteractHostPropSupportedCursors`` on the host and set
    ``kOfxInteractPropCursor`` on the overlay interact (see "Interact cursors
    (proposed extension)" in ``ofxInteract.h``). None of the bundled plug-ins
    used the old mechanism.

Obsolete plug-ins
    A plug-in that sets ``kOfxImageEffectPluginPropObsolete`` to 1 (OFX, after
    1.5.1) on its descriptor is still loaded, so existing projects that use it
    open, and it can be created by ID (from Python, ``app.createNode()``).
    It is not offered for new nodes: it is hidden from the node menus, the
    toolbars, the Tab node-creation dialog and the documentation, and it is
    not registered as a reader or writer for its file extensions
    (``OfxHost::isPluginObsolete``, ``Plugin::getIsUserCreatable``). The
    spec uses the property for a plug-in "made obsolete by a different
    version of the same plugin contained in the same bundle", and requires
    at least one version that is not obsolete. Natron is more lenient: it
    hides any plug-in that sets it, even when no version of it is left offered
    (openfx-misc's JoinViews, OneView, Roto and TrackerPM, which are obsolete
    only in Natron, where none of their versions is left offered). Natron
    2.6 no longer reads the Natron extension
    ``kNatronOfxImageEffectPropDeprecated``, which had the same meaning (`#1491 <https://github.com/NatronGitHub/Natron/issues/1491>`_):
    plug-ins must set the standard property (``setIsObsolete(true)`` in the
    Support library, instead of ``setIsDeprecated(true)``).

Practical guidance
------------------

- When a bug reproduces with *all* plug-ins, suspect the host glue
  (``Engine/Ofx*`` or ``HostSupport``). When it reproduces with only one
  plug-in, the bug is probably in that plug-in's own repository, not here.
- Changing host behavior can affect every third-party plug-in, including
  commercial ones. Treat the host↔plug-in contract as a public API: prefer
  additive, capability-flagged changes and test against the four bundled plug-in
  sets.
