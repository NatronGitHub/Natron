.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

.. _evaluating-script-structure:

Evaluating Script Structure
===========================

Natron rarely stops you from doing things in an unusual way, so it is easy to
build a project that works but is hard to read, fix or hand over. This page
lists common bad practices. These are guidelines, not rules: break them when
you have a good reason.

Project and files
-----------------

**Set the format and frame rate before you start**

Open the :ref:`project settings <compositing-projects-setup>` (press :kbd:`S`
in the node graph) and set the **Project Format** and **Frame Rate** before you
add nodes. Generators such as Constant use the project format by default. If
it is wrong, many nodes end up with a format different from the one you work
in, which is tedious to fix later.

**Keep files organized, and use names that are consistent and meaningful.**

**Use paths relative to the project**

An absolute path, such as ``/Users/me/school/lesson_one/asset.jpg``, breaks
as soon as the project folder is moved to another computer or another disk,
and every file path then has to be fixed by hand. A path relative to the
project keeps working.

In Natron, relative paths use the ``[Project]`` project path, which is the
folder that contains the saved ``.ntp`` file (see **Project Paths** in the
project settings, where you can also add your own path variables). For
example, with this folder layout:

.. code-block:: none

   comp/
       comp-out/
       _setup/
           01_01_qua_v1.ntp
           sources/
               paving.jpg

- a file in a folder below the project: ``[Project]/sources/paving.jpg``;
- a file in a folder above the project:
  ``[Project]/../comp-out/01_01_qua_v1.jpg``. Add one ``../`` for each level
  up.

The ``[Project]`` path exists only once the project is saved, so save the
project before you add Read or Write nodes. The file dialog then has a
**Relative to** menu: choose **Project** to get a relative path. To convert an
existing path, right-click on the file name field of a Read or Write node and
choose **Make relative to project** (or **Make absolute**).

.. note::

   Nuke users may know the Tcl expression
   ``[file dirname [value root.name]]``. Natron does not understand Tcl in
   file paths: use ``[Project]`` instead.

**Read image sequences rather than movies**

Movie files, such as QuickTime or MP4, are slower to seek in and can cause
problems with time edits. Convert them to image sequences before you read them
into Natron (see :ref:`tutorials-imagesequence`).

Node graph housekeeping
-----------------------

**Consider masking still images in GIMP or Krita**

For a still image with a complex shape, painting the mask in GIMP or Krita is
often faster and better than drawing a Roto shape. Save the result as TIFF or
PNG with an alpha channel.

**Keep the B input as the main stream**

In a Merge node, B is the background and A is composited over it. Keep the
main branch of the comp on the B inputs, so that it reads as a straight line
down the node graph, and bring each new element in through A.

**Use separate Merge nodes rather than many A inputs**

A Merge node accepts many inputs: A is merged with B first, then A2 with the
result, then A3, and so on. But all the A inputs share the same operator, mix
and channel settings. Separate Merge nodes stacked on the B stream are easier
to adjust and to read.

**Use the Mask input for effects, and alpha for transparency**

The Mask input of a node limits where its effect is applied. To control the
transparency of an element in a Merge, set the alpha of the A input instead,
for example by copying a matte into its alpha channel with a
:ref:`Shuffle <net.sf.openfx.ShufflePlugin>` node.

**Use as few points as you can in Roto shapes**

Shapes with many points are hard to edit and to animate.

**Don't reuse one mask for a chain of nodes**

If several consecutive nodes are masked by the same matte, the edges of the
mask are processed several times. Apply the effects without masks, then mix
the result with the original once, with a
:ref:`KeyMix <net.sf.openfx.KeyMix>` node and that matte.

Color grading
-------------

**Think in hue, saturation and lightness**

A color can be described by three coordinates. Images are processed in red,
green and blue (RGB), but artists usually think in terms of hue, saturation
and lightness (HSL), which are easier to judge by eye.

Hue
   The name of the color: blue, green, pink, and so on.

Saturation
   The intensity, or purity, of the color. Black, white and gray have zero
   saturation. Hue and saturation together make up the chroma of the color.

Lightness
   What is left when the saturation is set to zero. To see it, hover the
   viewer and press :kbd:`Y` (**Display Luminance**); press :kbd:`Y` again to
   go back to RGB.

Lightness and saturation are bounded: zero or maximum lightness (black or
white) means zero saturation. Hue is usually shown on a color wheel.

When grading, adjust lightness first, then hue, then saturation. Changing one
of them often changes how the others look.

**Know what each channel carries**

On typical photographic footage:

Red
   Often carries the fine detail of the image.

Green
   Contributes most to the lightness of the image. When you adjust the hue,
   try to leave the green channel alone, as changing it also changes the
   lightness.

Blue
   Carries the large masses of the image, and is usually the noisiest
   channel.

**Grade in order**

Color grading is usually done in three stages, in this order:

#. Color correction
#. Color matching
#. Color stylization

**Split up compound color edits**

Don't adjust the lightness and the hue in the same node. Smaller, separate
adjustments are easier to edit and to troubleshoot.

**Try simple color tools first**

For much color correction work, a simple node such as Multiply or Saturation
is enough. Simple nodes are faster, and the node graph is easier to read.

**Don't leave "fiddle" values in the parameters**

A ColorCorrect with a multiply of 1.0003 has no visible effect, but whoever
opens the project has to wonder why it is there. Either change a value on
purpose or leave it at its default.

Merging and premultiplication
-----------------------------

**Don't color correct premultiplied images**

Unpremultiply the image first, either with an Unpremult node before the color
correction and a Premult node after it, or with the **(Un)premult** checkbox
that many color nodes have.

**Composite premultiplied images**

The Merge operators expect premultiplied inputs: premultiply the foreground
before you merge it.

**Don't premultiply twice**

Premultiplying an image that is already premultiplied darkens the edges of
the alpha.

**Don't use the composite output of a keyer**

Keyers can output the foreground already composited over the background.
Don't use that composite: it leaves no room to color correct the foreground.
Use the keyer's premultiplied or unpremultiplied output, or its alpha, and do
the Merge yourself further down the node graph.

See also
--------

- :ref:`transition-from-nuke`: the differences between Nuke and Natron.
- :doc:`getstarted-environment-nodegraph`: arranging and labeling the nodes of
  the Node Graph.
- :doc:`compositing-color`: the color correction nodes.
- :doc:`compositing-channels`: alpha, mattes and premultiplication.
