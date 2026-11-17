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

OpenFX version
    Natron implements OpenFX 1.4, and declares it in ``kOfxPropAPIVersion``
    (its own version is in ``kOfxPropVersion``). The reference below describes
    OpenFX 1.5.1: the features added in 1.5, such as GPU rendering (CUDA,
    Metal, OpenCL), the Draw suite and colour management, are **not available
    in Natron**. Check the host properties before using them, and keep a
    fallback.

Where Natron looks for plug-ins
    An OpenFX plug-in is a ``.ofx.bundle`` directory. Natron looks for them,
    in this order, in:

    #. the directories of the ``OFX_PLUGIN_PATH`` environment variable (see
       :doc:`/_environment`; the preferences can't change it);
    #. the standard OpenFX location: ``/usr/OFX/Plugins`` on Linux,
       ``/Library/OFX/Plugins`` on macOS, ``%CommonProgramFiles%\OFX\Plugins``
       (usually ``C:\Program Files\Common Files\OFX\Plugins``) on Windows;
    #. the host-specific locations, in the same parent directory:
       ``OFX/Natron``, then ``OFX/Nuke`` (most Nuke plug-ins work in Natron),
       e.g. ``/usr/OFX/Natron`` and ``/usr/OFX/Nuke`` on Linux;
    #. the directories of the **OpenFX plug-ins search path** preference;
    #. the plug-ins bundled with Natron (``Plugins/OFX/Natron`` in the
       installation), which come first instead if **Prefer bundled plug-ins
       over system-wide plug-ins** is checked.

    The standard and host-specific locations are used only if **Enable
    default OpenFX plugins location** is checked, and the bundled plug-ins only
    if **Use bundled plug-ins** is checked (**Plug-ins** tab of the
    :doc:`/_prefs`, which also lets you disable individual plug-ins). When the
    same plug-in identifier is found several times, the highest version wins;
    for equal versions, the first one found in this order.

Rebuilding a plug-in
    Restart Natron after rebuilding. The plug-in cache is keyed on the
    binary's time and size, so a rebuilt plug-in is described again; clear
    it (**Cache > Clear Plug-ins Load Cache**) only if a change isn't picked
    up.

Detecting Natron
    Natron sets ``kNatronOfxHostIsNatron`` to 1 on its host descriptor: test
    this property, not the host name. The host name (``kOfxPropName``) can be
    changed by the user (**Appear to plug-ins as** in the preferences), and
    Natron reports another host's name to some plug-ins known to accept only
    that host.

Natron extensions
    ``ofxNatron.h`` defines Natron's extensions to OpenFX: multi-plane images
    and channel selectors, transform and distortion concatenation, host
    masking and mixing, in-viewer parameters and toolbars, dynamic choice
    menus, Markdown descriptions, and others. As for every extension, check
    that the host supports it (through the host property that the header
    documents) and keep a fallback for other hosts. The
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
