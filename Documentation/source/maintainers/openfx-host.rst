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

- ``libs/OpenFX`` — the official OpenFX headers and the C++ *support* library
  (the reference host/plug-in support code). ``libs/OpenFX_extensions`` adds the
  extension suites Natron supports.
- ``HostSupport/`` — the generic C++ host classes (``OFX::Host::…``): plug-in
  cache and loading, property sets, image-effect descriptors and instances,
  clip and parameter descriptors, interact descriptors. This layer is
  application-agnostic.
- ``Engine/Ofx*`` — the **Natron-specific glue** that connects the generic host
  to Natron's node/knob/image model.

The glue classes
----------------

``OfxHost`` (``OfxHost.cpp``)
    Natron's implementation of the OFX host object. Advertises host capabilities
    to plug-ins, discovers and caches plug-ins at startup, and creates effect
    instances. Reached via the ``AppManager``.

``OfxEffectInstance`` (``OfxEffectInstance.cpp``), which derives from ``EffectInstance``
    The bridge that makes an OFX plug-in look like a Natron ``EffectInstance``.
    It translates Natron's render actions (``renderRoI`` → OFX render action,
    ``getRegionOfDefinition``, ``getRegionsOfInterest``, ``getFramesNeeded``,
    ``isIdentity``, metadata) into OFX suite calls, and vice versa.
    ``AbstractOfxEffectInstance`` is the interface it satisfies.

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

At startup ``OfxHost`` scans the OFX plug-in paths, loads each bundle and reads
its *descriptors* (what parameters and clips it declares) into the plug-in
cache. Those descriptors become entries in the ``AppManager`` plug-in registry
and appear in the node-creation menu. When the user creates such a node, Natron
builds a ``Node`` whose ``EffectInstance`` is an ``OfxEffectInstance`` wrapping a
freshly instantiated plug-in; the plug-in's parameters materialize as Natron
knobs and its inputs as Natron input arcs.

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
