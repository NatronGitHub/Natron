.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

.. _transition-from-nuke:

Nuke to Natron transition guide
===============================

Natron and Nuke work in a similar way: both are node-based compositors with
a B-stream Merge, multi-layer images, expressions, groups and Python
scripting. This page lists the main differences. It is not complete.

Natron can't open Nuke scripts (``.nk``) or run gizmos: the node graph has to
be rebuilt in Natron.

Node names
----------

Most nodes have the same name in Natron and in Nuke: Merge, Grade,
ColorCorrect, Transform, CornerPin, Reformat, Crop, Blur, KeyMix, Roto,
RotoPaint, Tracker, LensDistortion, STMap, IDistort, Retime, TimeOffset,
FrameHold, FrameRange, AppendClip, Switch, Dissolve, Premult, Unpremult,
Dot, Backdrop, Precomp, Read, Write, and the OCIO nodes. These have a
different name:

.. list-table::
   :header-rows: 1

   * - Nuke
     - Natron
   * - CurveTool
     - :ref:`ImageStatistics <net.sf.openfx.ImageStatistics>`
   * - Copy, ShuffleCopy
     - :ref:`Shuffle <net.sf.openfx.ShufflePlugin>` (it has an A and a B input)
   * - Noise
     - :ref:`SeNoise <net.sf.openfx.SeNoise>`
   * - Grain
     - :ref:`SeGrain <net.sf.openfx.SeGrain>`
   * - Expression
     - :ref:`SeExprSimple <fr.inria.openfx.SeExprSimple>`, :ref:`SeExpr <fr.inria.openfx.SeExpr>`
   * - BlinkScript
     - :ref:`Shadertoy <net.sf.openfx.Shadertoy>` (GLSL shaders)
   * - IBKColour, IBKGizmo
     - :ref:`PIKColor <fr.inria.PIKColor>`, :ref:`PIK <net.sf.openfx.PIK>`
   * - Denoise
     - :ref:`DenoiseSharpen <net.sf.openfx.DenoiseSharpen>`
   * - Gizmo
     - PyPlug (a Group saved as a Python script, see the :ref:`developers-guide`)

Natron features that work differently
-------------------------------------

- **Cloning:** any node except a Viewer can be cloned, including Groups and
  PyPlugs (right-click in the node graph, **Edit > Clone Nodes**, or
  :kbd:`Alt+K`). A cloned Group follows the parameter changes of the
  original, so you can apply the same complex process to several images. The
  nodes and connections inside the group are not kept in sync, only the
  parameters.
- **Hiding unmodified parameters:** the **Show/Hide all parameters without
  modifications** button in the header of the properties panel (the fourth
  button from the right) hides every parameter that is at its default value.
- **Relative file paths** use the ``[Project]`` path variable instead of a
  Tcl expression (see :ref:`evaluating-script-structure`).
- **Expressions** are written in Python, not Tcl. For example, use
  ``frame`` instead of ``[frame]`` (see :ref:`paramExpr`).

What's not in Natron
--------------------

- **3D:** there is no 3D scene, camera or ScanlineRender. The
  :ref:`Card3D <net.sf.openfx.Card3D>` node places one image on a card seen by
  its own camera (see :doc:`tutorials-hugin` for an example). Natron is good
  at compositing 3D renders from other software, for example multi-layer EXR
  files from Blender.
- **Optical flow:** there is no optical-flow retiming or motion vector
  generation like Kronos or OFlow. Retime and FrameBlend only blend frames.
- **Deep compositing** is not supported.
- **Keylight and Primatte** are not available. Use
  :ref:`ChromaKeyer <net.sf.openfx.ChromaKeyerPlugin>`,
  :ref:`PIK <net.sf.openfx.PIK>`, :ref:`Keyer <net.sf.openfx.KeyerPlugin>` or
  :ref:`HueKeyer <net.sf.openfx.HueKeyer>` (see :doc:`tutorials-keying`).

Some missing features can be added with OpenFX plug-ins from other vendors
(check with the vendor that Natron is supported), or with community PyPlugs
from the `natron-plugins <https://github.com/NatronGitHub/natron-plugins>`_
collection. For example, the **PM_Card3D** and **PM_Camera** PyPlugs bring
basic 3D cards and cameras.

Python scripting
----------------

+----------------------------------+----------------------------------------------+
| Nuke                             | Natron                                       |
+==================================+==============================================+
| ``nuke.toNode("Blur1")``         | ``app.getNode("Blur1")``, or ``app.Blur1``   |
+----------------------------------+----------------------------------------------+
| ``nuke.selectedNodes()``         | ``app.getSelectedNodes()`` (GUI only)        |
+----------------------------------+----------------------------------------------+
| ``node["size"].value()``         | ``node.getParam("size").getValue()``         |
+----------------------------------+----------------------------------------------+
| ``nuke.createNode("Blur")``      | ``app.createNode("net.sf.cimg.CImgBlur")``   |
|                                  | (nodes are created by plug-in ID)            |
+----------------------------------+----------------------------------------------+

To read the value of a pixel, use an ImageStatistics node with **Restrict to
Rectangle** checked and a 1×1 rectangle at that pixel, click **Analyze Frame**
(``analyzeFrame.trigger()`` in Python), then read the **Mean** parameter
(``statMean``). For example, in the Script Editor:

.. code-block:: python

   stats = app.createNode("net.sf.openfx.ImageStatistics")
   stats.connectInput(0, app.getNode("Read1"))
   stats.getParam("bottomLeft").set(100, 50)
   stats.getParam("size").set(1, 1)
   stats.getParam("analyzeFrame").trigger()
   mean = stats.getParam("statMean")
   print([mean.getValue(i) for i in range(4)])
