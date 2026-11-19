.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

.. _compositing-color-management:

Color management with OpenColorIO
=================================

Images come from many sources: camera files in a log encoding, sRGB stills
from the web, linear EXR renders from a 3D package. To combine them correctly,
Natron converts all of them to one common **working space** when they are
read, does all the compositing in that space, and converts the result to the
colorspace of each output file when it is written. The conversions are done
by `OpenColorIO <https://opencolorio.org>`_ (OCIO), the color management
library used by most visual effects software, which reads the list of
available colorspaces from a **configuration file**.

This page explains the working space, how the OpenColorIO configuration is
chosen, the colorspace settings of the Read and Write nodes, how the Viewer
displays images, and the OCIO nodes.


The linear working space
########################

Most image files store **display-referred**, non-linear values: an 8-bit sRGB
JPEG or PNG, or a Rec.709 video, encodes brightness with a curve that matches
the response of a monitor and uses the available code values efficiently.
Camera files often use a **log** encoding (Cineon, ARRI LogC, Sony S-Log,
Panasonic V-Log...), which keeps a wide range of brightness in 10 or 12
bits.

Neither is suited to compositing. Blurs, merges, motion blur, defocus and
color corrections such as **gain** and **exposure** only behave like real light
when pixel values are proportional to the amount of light, that is, in a
**scene-linear** space. In a linear space, 0.18 is middle grey, 1.0 is the
white of a diffuse surface, and highlights can go far above 1.0: Natron
processes images in 32-bit floating point, so values above 1 and below 0 are
kept.

So the normal workflow is:

1. Each **Read** node converts its file from the file's colorspace to the
   working space (*linearization*).
2. All the nodes in the tree work on linear images.
3. The **Viewer** converts the linear image to the colorspace of your monitor,
   for display only.
4. Each **Write** node converts the linear image to the colorspace expected by
   the output file.

In the OpenColorIO configuration, the working space is the colorspace with the
``scene_linear`` role, for example ``Linear`` in the *blender* configuration or
``linear`` in *nuke-default*.


Choosing the OpenColorIO configuration
######################################

An OpenColorIO configuration (a ``config.ocio`` file, plus the LUT files it
uses) defines the colorspaces, their names, the roles (such as
``scene_linear``), the displays and views, and the looks. The same
configuration is used by every node in the project.

Configurations bundled with Natron
----------------------------------

Natron ships with a set of configurations from the
`NatronGitHub/OpenColorIO-Configs <https://github.com/NatronGitHub/OpenColorIO-Configs>`_
repository, installed in the ``Resources/OpenColorIO-Configs`` folder of the
Natron installation:

- **blender** (the default): the configuration of older Blender versions,
  with ``Linear`` as the working space; sRGB, Rec.709, Cineon and other log
  (``lg10``, ``lgf``) colorspaces; camera log curves (AlexaV3LogC, SLog3,
  V-Log, REDLog, CLog...); Filmic colorspaces; the ``sRGB``,
  ``DCI-P3``, ``Rec709`` and ``XYZ`` displays; and the Filmic and film
  emulation looks.
- **natron**: a small configuration with ``Linear sRGB / REC.709 D65`` as
  the working space (``scene_linear``); sRGB, AdobeRGB, Rec.709, Rec.1886
  and Rec.2020 curves, Cineon and Josh Pines log curves, V-Log/V-Gamut and
  XYZ; the ``sRGB / REC.709 Reference`` and ``DCI-P3 Reference`` displays;
  and no looks.
- **nuke-default**: the colorspaces of Nuke's default configuration
  (``linear``, ``sRGB``, ``rec709``, ``Cineon``, gamma curves and many camera
  log curves: AlexaV3LogC, SLog3, REDLog, CLog...), and no looks. Use it to
  exchange projects and files with a facility that uses Nuke's defaults.

No ACES configuration is bundled. To work in ACES, download an ACES
configuration and use it as a custom configuration (see below).

Changing the configuration
--------------------------

Open **Edit > Preferences** and go to the **Color Management** page:

**OpenColorIO configuration**
    Choose one of the bundled configurations, or **Custom config** to use your
    own file.

**Custom OpenColorIO configuration file**
    The ``config.ocio`` file to use when **Custom config** is selected.

**Warn on OpenColorIO config change**
    Show a reminder that Natron must be restarted after a change.

**Warn on startup if OpenColorIO config is not the default**
    At startup, offer to go back to the default *blender* configuration when
    another bundled configuration is selected, or when **Custom config** is
    selected but its file is not set or does not exist.

.. important::

   **Restart Natron after changing the configuration.** The colorspace
   menus of the Read, Write and OCIO nodes are built when Natron starts, and
   nodes that already exist keep the configuration they were created with.

The ``OCIO`` environment variable
---------------------------------

If the ``OCIO`` environment variable is set when Natron starts, it overrides
both preferences: Natron uses the file it points to. This is the usual way
for a studio to give every application (Natron, NatronRenderer, Blender, Nuke,
and so on) the same configuration. For example, on Linux or macOS:

.. code-block:: bash

   export OCIO=/path/to/studio-config/config.ocio
   natron

If neither the variable nor the custom file is set, Natron sets ``OCIO`` to
the bundled configuration chosen in the preferences, so that the plug-ins
use it. See :doc:`../_environment` for the other environment variables.

Natron also adds an ``OCIO`` entry to the **Project Paths** of the project
settings, pointing to the folder that contains the configuration, so that file
paths in the project can refer to the configuration's files as ``[OCIO]/...``
(for example a LUT in ``[OCIO]/luts/``).

.. note::

   Each Read, Write and OCIO node also has an **OCIO Config File** parameter,
   which shows the configuration that the node uses. Changing it reloads that
   configuration for this node only and rebuilds its colorspace menus, but it
   is best left alone: the node then no longer uses the same colorspaces as
   the rest of the project, and the project depends on a file path that may
   not exist on another machine. Change the global configuration instead.


Reading files: File Colorspace and Output Colorspace
####################################################

Each Read node has two colorspace parameters:

**File Colorspace**
    The colorspace the pixel values are stored in, in the file.

**Output Colorspace**
    The colorspace the Read node converts the image to. It defaults to the
    ``scene_linear`` role of the configuration. Leave it at the working space
    unless you have a reason to do otherwise.

When you choose a file, Natron sets **File Colorspace** automatically:

1. If the name of a colorspace of the configuration comes right before the
   last ``.`` of the file name, after a ``_``, ``-``, ``.`` or space (for
   example ``plate_lg10.dpx`` with a colorspace named ``lg10``), that
   colorspace is used. In a numbered sequence such as
   ``plate_lg10.0001.dpx``, the frame number comes between the colorspace
   name and the extension, so the name is not detected: set **File
   Colorspace** by hand.
2. Otherwise, the colorspace stored in the file's metadata is used, if there
   is one and the configuration has a matching colorspace.
3. Otherwise, it is guessed from the bit depth and the format: floating-point
   files (EXR, HDR, float TIFF...) are taken as linear, 8-bit files (JPEG,
   most PNG and TIFF files) as sRGB, 10- to 16-bit DPX and Cineon files as
   Cineon log, and other 16-bit integer files as Rec.709. Video files read by
   the FFmpeg reader are taken as Rec.709.

Once you set **File Colorspace** yourself, Natron keeps your choice when the
file name changes. Always check this parameter for camera footage: a log file
read as linear (or the opposite) looks washed out or far too contrasted, and
every color correction done afterwards is wrong.

The Read node also has a **File Premult** parameter. Colorspace conversions
must be applied to unpremultiplied color values, so the reader divides the
color by the alpha before the conversion and multiplies it back afterwards
when the file is premultiplied. Natron sets this parameter from the file, but
you can fix it if the file's metadata is wrong. See
:doc:`compositing-channels` for premultiplication.

Camera raw files (read by ReadOIIO through LibRaw) are first developed to
the colorspace chosen in the raw **Output Colorspace** option (sRGB by
default), and **File Colorspace** must then match that choice. See the :ref:`Read node reference <fr.inria.built-in.Read>`
and :ref:`ReadOIIO <fr.inria.openfx.ReadOIIO>` for all the parameters.


Writing files: Input Colorspace and File Colorspace
###################################################

The Write node does the opposite conversion:

**Input Colorspace**
    The colorspace of the images coming from the node tree. It defaults to the
    working space.

**File Colorspace**
    The colorspace to store in the file. As for the Read node, Natron first
    looks for a colorspace name right before the last ``.`` of the file name
    (which usually rules out numbered sequences). Otherwise it is
    set from the file format and bit depth: linear for floating-point formats
    such as EXR, Cineon log for 10-, 12- and 16-bit DPX and Cineon files,
    Rec.709 for other 10-, 12- and 16-bit integer formats, and sRGB for 8-bit
    formats.

Check **File Colorspace** before rendering, in particular when the file is
going to someone else: an EXR for another compositor should stay linear, a
DPX for a grading suite is usually expected in log, and a PNG or an H.264 video
for the web in sRGB or Rec.709. The **Input Premult** parameter tells the
writer whether its input is premultiplied, so that it can unpremultiply before
a non-linear conversion. See the
:ref:`Write node reference <fr.inria.built-in.Write>` and
:ref:`WriteOIIO <fr.inria.openfx.WriteOIIO>`.


Viewing images
##############

The Viewer displays the linear image through a simple **viewer process**,
chosen in the menu of the Viewer's top toolbar (next to the gain and gamma
controls):

- **Linear(None)**: no conversion. The linear values are sent to the screen
  as they are, which looks too dark and too contrasted.
- **sRGB** (the default): the sRGB curve, correct for most computer monitors.
- **Rec.709** and **BT1886**: for HD video monitors.

These are fixed curves built into Natron. They do **not** use the OpenColorIO
configuration, and they only change what you see: the images computed by the
nodes and written to files are not affected. The Viewer's **gain** and
**gamma** controls are display-only too: the gain is applied before this
conversion, and the gamma after it.

To see the image through a view transform of the configuration (a film
emulation, *Filmic*, an ACES output transform, a P3 display...), insert an
:ref:`OCIODisplay <fr.inria.openfx.OCIODisplay>` node between the last node
of your tree and the Viewer, choose its **Display Device** and **View
Transform**, and set the Viewer's process to **Linear(None)** so that the
image is not converted twice. Remember that this node is for viewing only:
connect the Write node above it, not below it, unless you really want the view
transform baked into the output.

Color pickers in the Viewer return linear values by default (**Edit >
Preferences > User Interface > Linear color pickers**), so the values you pick can be
typed directly into color parameters. See :doc:`getstarted-environment-color`.


The OCIO nodes
##############

The OCIO nodes are in the **Color > OCIO** menu of the
:doc:`toolbar <getstarted-environment-toolbar>`. All of
them use the global configuration.

:ref:`OCIOColorSpace <fr.inria.openfx.OCIOColorSpace>`
    Converts from one colorspace of the configuration to another. Use it in the
    middle of a tree, for example to do a key or a grain match in log and then
    go back to linear, or to convert an element whose colorspace was not set
    correctly in the Read node. Check **(Un)premult** if its input is
    premultiplied and one of the colorspaces is not linear.

:ref:`OCIODisplay <fr.inria.openfx.OCIODisplay>`
    Applies a display and view transform of the configuration (see above).

:ref:`OCIOLookTransform <fr.inria.openfx.OCIOLookTransform>`
    Applies one or several *looks* defined in the configuration, for example a
    show LUT or a per-shot grade that the configuration references.

:ref:`OCIOFileTransform <fr.inria.openfx.OCIOFileTransform>`
    Applies a LUT or color transform file (``.cube``, ``.3dl``, ``.csp``,
    ``.cc``...) directly, without colorspace handling: the image must already
    be in the space the LUT expects.

:ref:`OCIOCDLTransform <fr.inria.openfx.OCIOCDLTransform>`
    Applies an ASC CDL grade (slope, offset, power and saturation), typed in or
    read from a ``.cc`` or ``.ccc`` file, and can export it. CDLs are the usual
    way to exchange simple grades with editorial and grading.

:ref:`OCIOLogConvert <fr.inria.openfx.OCIOLogConvert>`
    Converts between the ``scene_linear`` and ``compositing_log`` roles of the
    configuration. Of the bundled configurations, only *blender* and
    *nuke-default* define ``compositing_log``: with *natron* this node cannot
    be used.

Most of these nodes have an **OCIO config help...** button that lists the
colorspaces, displays or looks of the current configuration.

Natron also has color conversion nodes that do not use OpenColorIO, such as
:ref:`RGBToHSL <net.sf.openfx.RGBToHSL>` or
:ref:`RGB709ToLab <net.sf.openfx.RGB709ToLab>`: they change the color model
(HSV, HSL, Lab, YCbCr...), not the transfer curve, and are described in
:doc:`compositing-color`.


Typical setups
##############

sRGB stills and graphics
    JPEG, PNG and 8-bit TIFF files are read as sRGB and converted to linear.
    When the result is written to an 8-bit file in sRGB, the conversion is
    undone, so graphics that are only moved or merged keep their original
    colors.

Log camera footage
    Set **File Colorspace** to the log curve of the camera (for example
    ``AlexaV3LogC`` or ``SLog3`` in *blender* or *nuke-default*, or
    ``V-Log - V-Gamut`` in *blender* or *natron*). If the configuration does not have the camera's curve, use a
    configuration that does, or convert with a LUT from the camera vendor in an
    OCIOFileTransform node.

CG renders
    Multi-layer EXR renders from Blender, Houdini or another renderer are
    usually linear already: keep **File Colorspace** linear. If the renderer
    used a different set of primaries (for example ACEScg), use a configuration
    that has that colorspace, so that the renders and the plates are converted
    to the same working space.

ACES
    Download an ACES configuration (for example from the
    `OpenColorIO-Config-ACES <https://github.com/AcademySoftwareFoundation/OpenColorIO-Config-ACES>`_
    project), select it as a custom configuration or with the ``OCIO``
    variable, and restart Natron. Each ACES configuration needs a minimum
    OpenColorIO version, given in its file name (``..._ocio-v2.1.ocio``) and
    in its release notes. Natron 2.6 uses OpenColorIO 2.6 on macOS and
    Windows, and OpenColorIO 2.2 on Linux: on Linux, use a configuration made
    for OCIO 2.2 or earlier, such as the ``ocio-v2.1`` ones of release 1.0.0.
    Read each input with its ACES input
    colorspace (the camera's log colorspace, or the sRGB texture colorspace for
    sRGB images), keep the working space (ACEScg), view through an OCIODisplay
    node with the ACES output transform for your monitor, and write EXR files
    in ACES2065-1 or ACEScg for delivery.

Exchanging files with other applications
    Use the same configuration everywhere: point the ``OCIO`` variable of all
    the applications to the same file. For single files, putting the
    colorspace name right before the extension lets every application pick the
    right colorspace.
