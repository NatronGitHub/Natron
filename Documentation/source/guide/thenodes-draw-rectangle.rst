.. for help on writing/extending this file, see the reStructuredText cheatsheet
    http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

Rectangle Node
==============

The Rectangle node draws a rectangle, with optional rounded corners
(**Corner Radius**) and soft edges (**Softness**). It is useful as a mask for a
color correction or another effect.

It is faster to set up and to render than a Roto or RotoPaint node.

Usage
--------

While the node's properties are open, drag the rectangle handle in the Viewer
to change the shape.
For an exact square, give the same width and height to **Size**: the button
next to it switches between one value for both dimensions and one value per
dimension.
To center the rectangle in the image, click **Center**.

**Softness** is 0 by default, which gives hard edges.

**Color 0** is the color outside the rectangle and **Color 1** the color
inside.

To fill the image with the effect set "Extent" to "Project".

.. toctree::
    :maxdepth: 2


