.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

Channels, alpha and premultiplication
=====================================

An image in Natron is a set of *channels*: grayscale images of the same size,
each holding one value per pixel. Red, green and blue hold the color, and
the alpha channel usually holds the transparency. Values are floating-point
numbers: 0 is black (or fully transparent), 1 is white (or fully opaque), and
values below 0 or above 1 are kept.

Channels are grouped in *layers* (also called *planes*). The main one is
``Color.RGBA``. A multi-layer OpenEXR file from a 3D renderer can contain many
more, for example diffuse, specular, depth or motion vectors, and the Read
node gives access to all of them.

Viewing channels
----------------

The Viewer shows one layer at a time, and can display only one of its
channels. With the mouse over the Viewer, press :kbd:`R`, :kbd:`G`, :kbd:`B`
or :kbd:`A` to see the red, green, blue or alpha channel as a grayscale image,
:kbd:`Y` for the luminance, and the same key again to go back to RGB.
:kbd:`M` shows the image with its alpha as a red overlay, which is a quick way
to check a matte. The **Layer** menu of the Viewer selects the layer to
display. See :doc:`getstarted-environment-viewer` for all the controls.

Check the alpha channel often. Many problems in a composite, such as dark or
light edges, holes in a matte, or a background that shows through a
foreground, are easier to see in the alpha than in the color channels.

Choosing which channels a node processes
----------------------------------------

Most nodes that process colors have **R**, **G**, **B** and **A** checkboxes
at the top of their properties, which select the channels they modify. The
other channels are copied unchanged. These checkboxes are added by Natron to
every node that supports them, so the node reference pages do not list them.

In most color correction nodes (Grade, ColorCorrect, Multiply, Add, Gamma,
Saturation, ColorLookup, HueCorrect, and others), **A** is unchecked by
default: a color correction normally changes the color of an element, not its
shape, so it should leave the alpha alone. Some nodes check **A** by default,
because they are often used on mattes: Invert, ColorMatrix, Clamp, Threshold,
KeyMix, the Draw nodes (Rectangle, Radial, Ramp, Roto), FrameBlend and
Distortion, for example. Check these checkboxes when you add a node.

The Merge node has the same kind of checkboxes for each input and for its
output. For example, to combine only the alphas of two images, uncheck **R**,
**G** and **B** in the output row, and connect the image whose colors you want
to keep to the **B** input.

Moving channels with Shuffle
----------------------------

The :ref:`Shuffle node <net.sf.openfx.ShufflePlugin>` (**Channel** menu)
builds an output layer from the channels of its two inputs, **A** and **B**.
For each output channel (**R**, **G**, **B** and **A**), choose an input
channel, such as ``A.Color.A``, or the constant ``0`` or ``1``. Typical uses:

- copy the matte produced by a keyer or a Roto node into the alpha of
  another image: connect the image to **B**, the matte to **A**, and set the
  output **A** to ``A.Color.A``;
- extract one layer of a multi-layer file, such as a render pass, into
  ``Color.RGBA`` so that the other nodes can process it;
- remove the alpha (set it to ``1``), or copy one channel to R, G and B to
  look at it.

Shuffle moves values without changing them: it does not convert colorspaces
and does not premultiply. Its **Output Premult** parameter only sets the
premultiplication *state* that is passed to the nodes below it (see below).

Alpha, matte and mask
---------------------

These words are often used for the same thing, but they mean slightly
different things:

**Alpha**
   The fourth channel of an RGBA image. It can hold anything, but it usually
   holds the transparency of the image.

**Matte**
   The grayscale image that defines which part of a foreground is kept when it
   is placed over a background. It is often the alpha of the foreground, but
   it may also come from a keyer, a Roto node, or another image.

**Mask**
   A grayscale image that limits where a node has an effect. Most nodes
   have an optional **Mask** input (the arrow on the side of the node in the
   Node Graph): where the mask is 1 the node is fully applied, where it is 0
   the input passes through unchanged. **Invert Mask** reverses this, and
   **Mix** scales the effect everywhere.

In a matte, the white area that keeps the foreground is sometimes called the
*holdout* or *core*, and the black area that removes it the *garbage* or
*junk* area.

Premultiplication
-----------------

Natron, like most compositing applications, works with *premultiplied*
images (also called *associated alpha*): the red, green and blue values of
each pixel have already been multiplied by its alpha. Where the alpha is 0,
the color is black; where the alpha is 0.5, the color is half as bright as in
the original image.

Premultiplied images make compositing simple. The **over** operation of the
Merge node computes A + B × (1 - a), where *a* is the alpha of A: the
background is darkened where the foreground is opaque, then the
premultiplied foreground is added to it. This formula is only correct if A is
premultiplied. Filters such as Blur, and the filtering done by Transform,
also give correct edges only on premultiplied images: on unpremultiplied
images, the color of the transparent pixels leaks into the edges.

A consequence: in Natron, a pixel with zero alpha and a non-zero color is not
transparent. Merged over a background with **over**, its color is *added* to
the background. Applications that use unpremultiplied images, such as
After Effects, behave differently. Such pixels usually come from a color
correction applied to a premultiplied image, which brightened its black
background.

Premult and Unpremult
~~~~~~~~~~~~~~~~~~~~~

The :ref:`Premult <net.sf.openfx.Premult>` node (**Merge** menu) multiplies the
color channels by the alpha, and the :ref:`Unpremult <net.sf.openfx.Unpremult>`
node divides them by the alpha (pixels with a zero alpha are left unchanged).
Their **By** parameter selects another channel to use instead of the alpha.

Each image also carries a *premultiplication state*, which tells the nodes
below whether it is premultiplied, unpremultiplied or opaque. Premult and
Unpremult set this state. The **Clip Info...** button of the Shuffle, Premult
and Unpremult nodes shows the state of their inputs and output.

Rules of thumb
~~~~~~~~~~~~~~

**Unpremultiply before changing colors.**
   A color correction applied to a premultiplied image also changes the
   colors of the semi-transparent edges and of the black transparent area:
   brightening, for example, turns the transparent area gray, which shows
   as a "box" around the element once it is merged. Divide by the alpha,
   correct, then multiply again. The color nodes do this for you when their
   **(Un)premult** checkbox is checked. In Grade, ColorCorrect, Add,
   ColorLookup, ColorMatrix, Gamma, HSVTool, HueCorrect, Invert and
   Multiply, this checkbox is set when you connect the **Source** input, as
   long as you have never changed it on that node: it is checked if the
   input is a premultiplied RGBA image, and unchecked otherwise (opaque,
   unpremultiplied, or not RGBA). It is not changed when a project is
   loaded. You can also use an Unpremult node, then the color nodes, then a
   Premult node.

**Premultiply once, before merging.**
   The **A** input of a Merge node must be premultiplied. Premultiplying
   twice darkens the soft edges, which shows as a dark fringe around the
   foreground. Check the **Output Mode** of the keyers: the default of the
   Keyer node, *Intermediate*, outputs the original colors, not
   premultiplied, with the key in the alpha, so it must be followed by a
   Premult node before merging. The *Premultiplied* mode does both at once.

**Filter and transform premultiplied images.**
   Blur, Transform, CornerPin and the other filters should process images in
   their premultiplied state, so that the colors of the transparent pixels
   do not bleed into the edges.

**Use the keyer for the matte only.**
   Most keyers can also output a composite over their **Bg** input. That
   leaves no room to correct the foreground (color, grain, edges) before it
   is merged, so it is usually better to use the keyer for its matte and to
   build the composite with Merge nodes downstream. See
   :doc:`compositing-keying`.

Reading and writing premultiplied images
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

By default, the Read node outputs premultiplied images (its **Output
Premult** parameter). If the file contains unpremultiplied colors, as PNG
files do, the Read node premultiplies them after converting their
colorspace. The **File Premult** parameter of the Read node tells it how the
file is stored. It is set from the file, but you can change it if the file
metadata is wrong, for example if the soft edges of an image look too bright
(it was premultiplied but is read as unpremultiplied) or too dark (the
opposite).

The Write node does the reverse: for file formats that store unpremultiplied
colors, it divides by the alpha before converting the colorspace. Its
**Input Premult** parameter is set from its input, and normally does not need
to be changed.

Further reading
---------------

- `Premultiplication
  <https://web.archive.org/web/20210413043727/http://opticalenquiry.com/nuke/index.php?title=Premultiplication>`__
  and `Alpha Channel
  <https://web.archive.org/web/20220627015005/http://www.opticalenquiry.com/nuke/index.php?title=Alpha_Channel>`__,
  on the (archived) Optical Enquiry Nuke wiki. The node names are those of
  Nuke, but the principles are the same in Natron.
- T. Porter and T. Duff, `Compositing Digital Images
  <http://keithp.com/~keithp/porterduff/p253-porter.pdf>`__, SIGGRAPH 1984:
  the paper that introduced premultiplied alpha and the compositing operators
  of the Merge node.

See also
--------

- :doc:`tutorials-keying`: keying tutorials, which build mattes and
  premultiply the foreground with them.
- :ref:`evaluating-script-structure`: common premultiplication mistakes, among
  other bad practices.
- :doc:`tutorials-svgworkflow`: using the layers of an SVG file as separate
  images.
