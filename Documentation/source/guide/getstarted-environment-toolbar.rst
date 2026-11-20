.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf
   
Using the toolbar
=================

The toolbar is the column of icons on the left side of the Natron window.
Each icon opens a menu with one category of nodes. Click an entry of the menu
to create that node in the Node Graph.

.. note::  If a node is selected in the Node Graph, the new node is usually
   placed below it and connected to its output, so it processes the output of
   the selected node. If the selected node is an output, such as a Write or a
   Viewer, or if the new node has no input, such as a Read, the new node is
   placed above the selected node and connected to its input instead.

You can also create nodes with the :kbd:`Tab` key in the Node Graph, or with
their shortcuts (see :doc:`getstarted-environment-nodes`).

The categories below are listed in the order of the toolbar. A category only
appears if at least one installed plug-in belongs to it. Each one links to the
reference pages of its nodes.

Image
   Nodes that read and write images and videos, and generate simple images
   such as a constant color or a checkerboard. See :doc:`thenodes-image` and
   :doc:`/_groupImage`.

Draw
   Nodes that draw shapes, gradients, noise and text, including Roto and
   RotoPaint. See :doc:`thenodes-draw` and :doc:`/_groupDraw`.

Time
   Nodes that change the timing of clips: retiming, frame holds, time
   offsets, and so on. See :doc:`/_groupTime`.

Channel
   Nodes that copy, shuffle and combine channels and layers (the basic
   channels are R, G, B and A, but images can have more). See
   :doc:`/_groupChannel`.

Color
   Color correction and color management nodes. See :doc:`/_groupColor`.

Filter
   Nodes that change the texture of the image, for example blur, sharpen,
   denoise or edge detection. See :doc:`/_groupFilter`.

Keyer
   Nodes that pull a matte from an image, for example from a green screen,
   and remove the spill. See :doc:`compositing-keying` and
   :doc:`/_groupKeyer`.

Merge
   Nodes with several inputs that combine images into one. See
   :doc:`thenodes-merge` and :doc:`/_groupMerge`.

Transform
   Nodes that change the geometry of the image: move, rotate, scale, crop,
   distort, track and stabilize. See :doc:`/_groupTransform`.

Views
   Nodes for multi-view images, such as the left and right views of a
   stereo image. See :doc:`/_groupViews`.

Other
   Utility nodes, mostly to keep the Node Graph clean and readable: Dot,
   Backdrop, Group, NoOp, and so on. See :doc:`/_groupOther`.

Plug-ins that use their own category get their own icon after these, for
example **GMIC** for the G'MIC plug-ins (see :doc:`/_groupGMIC`) and
**Extra** for the openfx-arena plug-ins (see :doc:`/_groupExtra`). Plug-ins
that declare no category are in **Misc**. Community PyPlugs and other
third-party plug-ins can also add icons to the toolbar (see
:doc:`getstarted-installation`).
