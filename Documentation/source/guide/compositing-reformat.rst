.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

.. _compositing-reformat:

Reformatting elements
=====================

A compositing script often mixes images of different sizes: a 4K plate, an HD
graphic, a square logo, an anamorphic background. This page explains how Natron
describes the size of an image, how each node decides the size of its output, and
how to change it with the Reformat node and its relatives.

Most of the words used here (pixel aspect ratio, region of definition, region
of interest, render scale, field order) come from the
`OpenFX standard <https://openeffects.org>`_, which defines how Natron talks to
its plug-ins. The **format** is not part of that standard: it is a Natron
extension to OpenFX, which plug-ins such as Reformat and Crop understand.


Format and region of definition
###############################

Every image that flows through the Node Graph carries two rectangles. They are
often the same, but they mean different things, and most reformatting questions
come from confusing them.

The format
~~~~~~~~~~

The **format** is the frame the image is meant to be seen in, for example
"HD 1920x1080" or "2K_DCP 2048x1080". It is made of:

- the **resolution**: the width and height of the frame, counted in pixels;
- the **pixel aspect ratio** (PAR): how much each pixel must be stretched
  horizontally to look right on a screen. Most images have square pixels
  (PAR = 1). Anamorphic film scans typically have a PAR of 2, so a
  1828x1556 image is displayed twice as wide as it is stored. Old PAL and NTSC
  video formats also have non-square pixels (1.09 and 0.91).

In the Viewer, the format is drawn as a thin solid rectangle, with its name
written at the bottom right corner.

The format is what you usually think of as "the size of the shot". It is
what a Write node renders when its **Format Type** is set to
**Input Format**, and what a Reformat node takes as its starting point.

The region of definition
~~~~~~~~~~~~~~~~~~~~~~~~

The **region of definition** (RoD, often called the *bounding box* or *bbox*
in compositing software) is the area where the image actually has pixels. It
does not have to match the format:

- It can be **smaller** than the format. A Rectangle of 200x100 pixels in an HD
  project only has pixels inside that rectangle. Everything else is
  transparent black and is never computed, which saves time.
- It can be **larger** than the format. A Blur spreads the image a few pixels
  outside its edges, and a Transform that moves or rotates the image can push
  it partly or completely outside the frame. Those pixels are kept, so that
  if you later move the image back, nothing is lost.

When the region of definition of the image it displays differs from its
format, the Viewer draws it as a dashed rectangle, with its coordinates at two
corners. The Viewer only shows the rectangles of the node it is connected to:
to see the region of definition of an element, view that element's node.

.. note::

   Both rectangles are listed in the **Info** tab of each node's settings
   panel, as **Format (pixels)** and **Region of Definition**.
   The format is given in pixels, while the region of definition is given in
   *canonical coordinates*, which means "pixels at full resolution (render
   scale 1, as if proxy mode were off), after correcting for the pixel aspect
   ratio". For square-pixel images the two units are the same.

The region of interest
~~~~~~~~~~~~~~~~~~~~~~

Natron works backwards from the Viewer or the Write node, and only computes the
pixels that are actually needed downstream. The part of an input that a node
needs is called its **region of interest** (RoI). You rarely have to think
about it, but it explains why zooming in the Viewer, or cropping early in the
tree, makes interactive work faster.


How Natron decides a node's output format
#########################################

Each node picks its output format automatically, following simple rules:

1. **Readers** use the format stored in the file they read (its size and pixel
   aspect ratio). A 4K EXR gives a 4K format, whatever the project settings
   say.
2. **Most other nodes** use the format of their first connected input. More
   precisely, Natron looks at the inputs in order and takes the format of the
   first connected input that is not optional. If no such input is connected,
   it takes the format of the first connected optional input. If nothing is
   connected at all, it uses the **project format**.

   For example, a Blur keeps the format of its source. A Merge takes the
   format of its **B** input, because B is its first input; if B is not
   connected, the format of **A** is used.
3. **Generators** such as Constant, CheckerBoard or Rectangle have an
   optional **Source** input. When nothing is connected to it, they follow
   the rule above and use the project format; when an image is connected, they
   use its format. Many of them also have an **Extent** parameter that lets you
   choose another format or a custom size. The Rectangle's **Extent** is
   **Size** by default, so its format comes from its Source, or from the
   project when the Source is not connected.
4. A few nodes are designed to **change the format**: Reformat, Crop (with its
   **Reformat** option), Resize, and NoOp (with **Set Format**). They are
   described below.

A node's format cannot be animated: it stays the same for every frame. If you
need a frame that changes size over time, animate a Crop or a Transform
instead, and keep a fixed format.

The project format
~~~~~~~~~~~~~~~~~~

The **Project Format** (called **Output Format** in older versions) is set in
the :ref:`project settings <compositing-projects-setup>`. Despite its name, it
is *not* the format of every node. It is only a default, used:

- as the default workspace size of the Viewers when nothing is connected;
- as the format of nodes that have no input connected, such as generators;
- as the default output format of Write nodes, whose **Format Type** is
  **Project Format** unless you change it to **Input Format** or
  **Fixed Format**.

In OpenFX terms, the project format gives the *project size* that plug-ins
see. Plug-ins use it, for example, to place their default controls in the
middle of the frame.

By default, the first Read node created in a project sets the project format
to the format of the image it reads. This is controlled by the
**First image read set project format** option, in the **Project Setup**
section of the Preferences (on by default). Otherwise, set the project format
at the start of a project, normally to the resolution of your main plate.
Changing it later is possible, but it can move or resize the output of
generators and Write nodes that follow the project format.

.. tip::

   Natron is a compositor, not a video editor: sources of different sizes can
   live side by side in the same script, each with its own format. When a
   script or expression needs "the size of the image", it should ask the node
   it works on, not the project (see `Getting a node's format from Python`_
   below).


The Reformat node
#################

The Reformat node (**Transform > Reformat**) changes the format of an image and
resizes its content to match. Use it to:

- bring a plate of a different resolution to the working resolution of the
  shot, before merging it with other elements;
- create a lower-resolution version of a shot, for review or for the web;
- convert anamorphic footage to square pixels, or the reverse;
- fit a portrait photo or a logo into a landscape frame;
- mirror or rotate an image by 90 degrees.

All the parameters are listed in the :ref:`Reformat node reference
<net.sf.openfx.Reformat>`. The main ones are described here.

Type: choosing the output format
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The **Type** parameter says how the new format is chosen:

- **To Project Format** (the default): use the project format. This is the
  quickest way to conform every source to the resolution of the shot.
- **To Format**: pick a format from the **Format** menu, for example
  "HD_720 1280x720" or "2K_DCP 2048x1080". You can also add your own format
  from this menu.
- **To Box**: type a width and height in **Size**, and a
  **Pixel Aspect Ratio**. If **Force This Shape** is checked, the output
  format is exactly that box. If it is left unchecked (the default), the image
  is scaled (following **Resize Type**) to fit in the box, but the output
  format keeps the proportions of the resized image, so nothing is cut.
- **Scale**: multiply the size by a factor, for example 0.5 for half size or 2
  for double size. **Scale** shows a single value, used for both the width and
  the height; to use different factors, click the small **2** button next to
  it to expand it into separate X and Y values. The size is rounded to a whole number of
  pixels. Note that with this type, the size that is scaled is the size of the
  source's region of definition, not of its format: a Blur or a Transform
  before the Reformat changes the result.

The input size used by the other types is the source's *format*. Check
**Use Source RoD** to use its *region of definition* instead, for example to
resize exactly the area that has pixels, after a Crop that did not change the
format.

Resize Type: how the image fits in the new format
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When the source and the new format have different proportions, the image
cannot fill the frame exactly without being distorted. **Resize Type**
decides what to do:

- **None**: do not scale the image at all. Only the frame changes, so the image
  is padded with black or cut at the edges. Use it to extend the canvas, for
  example to add room around an element before tracking or stabilizing.
- **Width** (the default): scale the image so its width matches the new
  width. The height follows, keeping the proportions, and may be cut or
  leave black bands at the top and bottom.
- **Height**: the same, but matching the height. The sides may be cut or leave
  black bands.
- **Fit**: scale the image so that the *whole* image fits inside the new
  format, with black bands where the proportions differ (letterbox or
  pillarbox). Use it when nothing must be lost, for example a 9:16 phone video
  in a 16:9 frame.
- **Fill**: scale the image so that it *fills* the whole frame, cutting what
  sticks out. Use it when the frame must be covered and losing a bit of the
  edges is acceptable.
- **Distort**: stretch the image to match both the width and height. The
  proportions are not preserved, so circles become ovals. Use it only when the
  source was stored squeezed, or for a deliberate effect.

**Center** (on by default) places the center of the image at the center of the
new frame. When it is unchecked, the bottom left corners of the image and
the frame stay together, which is useful when an element must stay aligned
to a corner.

Flip, Flop and Turn
~~~~~~~~~~~~~~~~~~~

- **Flip** mirrors the image upside down.
- **Flop** mirrors the image left to right.
- **Turn** rotates the image 90 degrees counter-clockwise. The image is then
  fitted into the output format like any other source, following
  **Resize Type**. The output format itself only swaps its width and height
  when **Type** is **Scale**, or **To Box** with **Force This Shape**
  unchecked. With **To Project Format** (the default) or **To Format**, the
  output keeps the chosen format, and only the image inside it is rotated.

Use **Turn**, for example, for footage that was shot with the camera held
vertically.

These options move whole pixels. Combined with **Resize Type** set to
**None**, they give an exact result, without any softening, as long as the
image is not shifted by half a pixel. This happens when **Center** is checked
and the difference between the source and output widths (or heights) is an
odd number of pixels; uncheck **Center**, or use sizes that differ by an even
number of pixels.

Preserve BBox
~~~~~~~~~~~~~

By default, Reformat cuts everything outside the new format: the region of
definition of the output is limited to the frame.

Check **Preserve BBox** to keep the pixels that fall outside. The output then
still has the new format, but its region of definition can be bigger. This is
useful when a later Transform moves the image, and you want the parts
outside the frame to come back into view instead of being black.

Preserve BBox also lets Reformat combine (*concatenate*) with the transform
nodes that follow it, such as Transform or CornerPin, so that the image is
resampled only once for the whole chain, which keeps it sharper. Without
this option, Reformat only concatenates with the transform nodes placed
before it.

Filtering
~~~~~~~~~

Scaling an image means computing new pixels from the old ones. The
**Filter** parameter chooses how:

- **Impulse** copies the nearest pixel. It is fast and keeps hard edges, but
  looks blocky when enlarging. Use it for mattes or ID passes, where values
  must not be mixed.
- **Box** averages all the source pixels that fall inside each output pixel.
  It is a good choice when shrinking an image a lot.
- **Bilinear** gives a soft result, and is fast.
- **Cubic** (the default) is a good general-purpose choice.
- **Keys**, **Simon** and **Rifman** add more and more sharpening. They keep
  the image crisp, but they can create a dark or bright halo along strong
  edges.
- **Mitchell**, **Parzen** and **Notch** are softer. They help hide aliasing,
  pixelation or moiré patterns.

Some filters can create values slightly below 0 or above 1 near sharp edges.
Check **Clamp** to prevent this, for example when reformatting a matte.

Outside the region of definition, the image is always transparent black.
**Black outside** only changes what happens at the edges of the image. When
it is unchecked (the default), the filter repeats the edge pixels, so the
edges stay sharp. When it is checked, the edges fade to transparent black
over one pixel, which gives an antialiased border; the region of definition
then grows by one pixel. Check it when the scaled image does not cover the
whole frame and will be placed over another image.

Pixel aspect ratio and anamorphic footage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Reformat takes the pixel aspect ratio into account. For example, to turn an
anamorphic 2K plate (1828x1556 pixels, PAR 2) into a square-pixel image of the
same shape:

1. Add a Reformat node after the Read node.
2. Set **Type** to **To Box**.
3. Set **Size** to 3656 x 1556 and **Pixel Aspect Ratio** to 1.
4. Optionally, set **Resize Type** to **Distort**. Here it only "unsqueezes"
   the image, because the displayed shape stays the same. Since the box has
   exactly the displayed shape of the source, the default **Width** gives the
   same result.

The image is now stored with square pixels, and looks the same in the Viewer.


Workflow: conforming a plate to the shot resolution
###################################################

A typical example: the shot is delivered in HD (1920x1080), but one of the
background plates is a 4K (3840x2160) image and a logo is a 1000x1000 PNG.

1. Open the project settings (press **S** in the Node Graph) and check (or
   set) that the **Project Format** is "HD 1920x1080". If the first image you
   read was not HD, the project format was set to its size, and you have to
   change it here.
2. Select the 4K Read node, and add a Reformat node (**Transform > Reformat**).
   Leave **Type** on **To Project Format** and **Resize Type** on **Width**.
   The plate is scaled down to 1920x1080. Because both images are 16:9,
   nothing is cut.
3. Select the logo's Read node, and add a second Reformat node. Set **Type** to
   **To Box**, **Size** to 400 x 400, leave **Force This Shape** unchecked,
   and set **Resize Type** to **Fit**. The logo is now 400x400 pixels.
4. Add a Transform node after the logo's Reformat to place it in the frame.
   Connect the Viewer to this Transform, and check that the dashed rectangle
   (the logo's region of definition) is where you expect it.
5. Merge the logo (**A** input) over the plate (**B** input). The Merge takes
   the format of its **B** input, so the result is in HD. Connect the Viewer
   to the Merge, and check that the solid rectangle shows "HD".
6. In the Write node, leave **Format Type** on **Project Format**, or set it to
   **Input Format** to render exactly the format of the Merge.

.. tip::

   Reformat early: placing the Reformat right after the Read node means that
   all the following nodes work at the final resolution, which is faster when
   reducing a large plate. When you enlarge an image, do the opposite: keep it
   small for as long as possible, and scale it up near the end.


Related nodes
#############

Several other nodes change the format or the region of definition. Choose the
one that matches what you want to change:

- **Crop** (**Transform > Crop**, :ref:`reference <net.sf.openfx.CropPlugin>`)
  cuts the image to a rectangle, without scaling it. It changes the region of
  definition. To also change the format, check **Reformat**; this sets the
  format when **Extent** is **Format** or **Project**. Use Crop to remove
  unwanted edges, or to make later nodes faster by computing fewer pixels.
- **Resize** (**Transform > Resize**,
  :ref:`reference <fr.inria.openfx.OIIOResize>`) also changes the resolution
  and scales the image, using a different method (from OpenImageIO). It can
  give nicer results when shrinking an image a lot, but it is slower than
  Reformat for interactive work and it does not concatenate with transforms.
- **Transform** (**Transform > Transform**,
  :ref:`reference <net.sf.openfx.TransformPlugin>`) moves, rotates and scales
  the image *inside* the same format. Its output keeps the format of its
  input. See :doc:`compositing-transform`.
- **AdjustRoD** (**Transform > AdjustRoD**,
  :ref:`reference <net.sf.openfx.AdjustRoDPlugin>`) enlarges the region of
  definition by adding transparent pixels around the image, without changing
  the format. Use it before a filter that needs room around the image, such
  as a large blur or a glow.
- **NoOp** (**Other > NoOp**, :ref:`reference <net.sf.openfx.NoOpPlugin>`)
  passes the image unchanged, but with **Set Format** and
  **Set Pixel Aspect Ratio** it can relabel the format or the pixel aspect
  ratio without touching the pixels. Use it when a file reports a wrong format
  or pixel aspect ratio.
- **Project settings**: the **Project Format** sets the default for
  generators, Viewers and Write nodes, as explained above.


Other things that affect image size
###################################

Proxy mode and render scale
~~~~~~~~~~~~~~~~~~~~~~~~~~~

In :doc:`proxy mode <compositing-projects-proxy>`, Natron renders images at a
reduced *render scale*, for example half the width and half the height, to
make interactive work faster. The format, the region of definition and all
parameter values are still given at full resolution, so you never need to
change your settings when you switch proxy mode on or off. A Reformat to
1920x1080 still produces a 1920x1080 format in proxy mode; only the number of
pixels actually computed is smaller.

Field order and interlaced video
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Interlaced video stores each frame as two *fields*: the even lines and the odd
lines, recorded at slightly different times. The **field order** says which
field comes first (upper or lower). Natron treats every image as a full frame.
Scaling interlaced footage vertically mixes lines from both fields and creates
artifacts, so first remove the interlacing with the
:ref:`Deinterlace <net.sf.openfx.Deinterlace>` node (**Time > Deinterlace**),
and then reformat.


Getting a node's format from Python
###################################

In a Python script or an expression, use the node's own format rather than the
project format. Every node has these methods (see the
:ref:`Effect <Effect>` class in the Python reference):

- ``getOutputFormat()`` returns the output format as a
  :ref:`RectI <RectI>` rectangle, in pixels. Use ``width()`` and ``height()``
  to get its size, and ``left()`` and ``bottom()`` to get its origin.
- ``getPixelAspectRatio()`` returns the output pixel aspect ratio.
- ``getRegionOfDefinition(time, view)`` returns the region of definition at a
  given frame, as a ``RectD`` in canonical coordinates.

For example, in the **Script Editor**:

.. code-block:: python

    node = app1.Read1
    fmt = node.getOutputFormat()
    print(fmt.width(), fmt.height(), node.getPixelAspectRatio())

    rod = node.getRegionOfDefinition(app1.timelineGetTime(), 0)
    print(rod.left(), rod.bottom(), rod.right(), rod.top())

The same methods work in parameter
:ref:`expressions <paramExpr>`. For example, to make a progress bar with a
Rectangle node whose width grows from 0 to the full width of the Read1 image
over the first 100 frames, set this expression on the first dimension of the
Rectangle's **Size** parameter::

    thisGroup.Read1.getOutputFormat().width() * min(frame, 100) / 100.0

To place an element at the center of a node's frame, use
``thisNode.getInput(0).getOutputFormat()`` and take half its width and height.

The name of the project format can be read with
``app1.getProjectParam("outputFormat")``, which is a choice parameter: use
``get()`` to get the index of the selected entry, and ``getOption(index)``
to get its name. Remember that this is only the default format, not
necessarily the format of the image you are working on.

.. toctree::
   :maxdepth: 2
