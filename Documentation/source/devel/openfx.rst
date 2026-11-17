.. _devel-openfx:

Writing OpenFX plug-ins
=======================

Most Natron nodes are `OpenFX <https://openeffects.org>`_ (OFX) plug-ins: C or
C++ shared libraries that implement the OpenFX image-effect API. Natron is an
OpenFX *host*, like Nuke, DaVinci Resolve or Vegas, so a plug-in written for
OpenFX can run in Natron, and the plug-ins bundled with Natron (openfx-io,
openfx-misc, openfx-arena, openfx-gmic) can run in other hosts.

This part of the Developers Guide is for plug-in authors. It includes the
OpenFX Programming Guide and the OpenFX Reference from
`NatronGitHub/openfx <https://github.com/NatronGitHub/openfx>`_ (the
``libs/OpenFX`` submodule of Natron, at commit |openfx_commit| for this build).
That repository follows the OpenFX standard and adds the headers of the
extensions that Natron and other hosts implement.
How Natron implements the host side is described in the Maintainers Guide
(:ref:`maint-openfx`).

Natron and OpenFX
-----------------

Where Natron looks for plug-ins
    An OpenFX plug-in is a ``.ofx.bundle`` directory. Natron loads the
    plug-ins it ships with (``Plugins/OFX/Natron`` in the installation), the
    plug-ins in the standard OpenFX location (``/usr/OFX/Plugins`` on Linux,
    ``/Library/OFX/Plugins`` on macOS, ``C:\Program Files\Common Files\OFX\Plugins``
    on Windows), the directories listed in the ``OFX_PLUGIN_PATH``
    environment variable, and the directories of the **OpenFX plug-ins search
    path** preference. All of these can be changed in the **Plug-ins** tab of
    the preferences, which also lets you disable individual plug-ins.

Reloading a plug-in
    Natron caches the descriptions of the plug-ins it has loaded. After you
    rebuild a plug-in and change its parameters or clips, use
    **Cache > Clear Plug-ins Load Cache** and restart Natron.

OpenFX version
    Natron declares OpenFX API version 1.4 (``kOfxPropAPIVersion``) and its
    own version in ``kOfxPropVersion``.

Natron extensions
    ``ofxNatron.h`` defines Natron's extensions to OpenFX: multi-plane images
    and channel selectors, transform and distortion concatenation, host
    masking and mixing, in-viewer parameters and toolbars, dynamic choice
    menus, Markdown descriptions, and others. The host sets
    ``kNatronOfxHostIsNatron`` to 1 on its descriptor. As for every
    extension, check that the host supports it (through the host property
    that the header documents) and keep a fallback for other hosts. The
    :doc:`openfx/Reference/ofxForkExtensions` page of the reference lists
    these extensions and those of the other hosts; the header itself
    documents each property.

Examples
    The plug-ins of `openfx-misc <https://github.com/NatronGitHub/openfx-misc>`_
    are complete, maintained examples. They use the C++ support library of
    ``libs/OpenFX/Support`` and the helpers of
    `openfx-supportext <https://github.com/NatronGitHub/openfx-supportext>`_.

The OpenFX documentation
------------------------

.. toctree::
    :maxdepth: 1

    openfx/Guide/index
    openfx/Reference/index
    openfx/ReleaseNotes/index
