.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf
   
Working with color
==================
How to use Natron color correction nodes and tools to adjust the appearance of the images in your composites.
When starting out, this information provides a good overview of Natron scopes and color-correction nodes,
however not all options are covered here.

All these nodes work on the linear images produced by the Read nodes. How
files are converted to and from that linear working space, how the Viewer
displays it, and how to choose the OpenColorIO configuration is explained in
:doc:`compositing-color-management`.



The nodes
---------
The most important nodes are marked with an asterisk:



**Add***

Adds its **Value** to every pixel, so dark and bright areas move up (or down,
with a negative value) by the same amount. On its own it is rarely the right
correction, but a small positive value is a quick way to raise the blacks,
for example to make a distant object look hazier.


**Multiply***

Multiplies every pixel by its **Value**: values above 1 brighten the image,
values below 1 darken it, and different values per channel correct a color
cast. Since zero times anything is zero, pure black stays black. This is the
same operation as the *gain* of other color nodes.

**Clamp**

Limits the values to a **Minimum** and a **Maximum**. Besides its normal use
(for example, keeping a matte between 0 and 1), it helps to see where an
image goes out of range: with **Enable MinClampTo** and **Enable
MaxClampTo** checked, the clamped pixels are replaced by the **MinClampTo**
and **MaxClampTo** colors, which can be set to a bright color. The
:ref:`ClipTest <net.sf.openfx.ClipTestPlugin>` node shows the same areas with
zebra stripes, without changing other pixels.


**ColorLookup***

The equivalent of the *Curves* tool of image editors: each channel (and a
master curve for all three colors) is remapped through a curve that you
edit. Most of the other color operations can be built with it, and it is the
most flexible way to shape contrast. The price is that a curve is harder to
read, and to set precisely, than a few sliders.

**Colorspace conversion nodes**

A color model describes colors with three numbers other than red, green and
blue: hue, saturation and lightness for HSL, or lightness and two color
opponents for Lab. Each model makes some adjustments easier. Nodes such as :ref:`RGBToHSL <net.sf.openfx.RGBToHSL>`, :ref:`RGBToHSV <net.sf.openfx.RGBToHSV>` or :ref:`RGB709ToLab <net.sf.openfx.RGB709ToLab>` (and their inverses, such as :ref:`HSLToRGB <net.sf.openfx.HSLToRGB>` and :ref:`LabToRGB709 <net.sf.openfx.LabToRGB709>`) move an image from RGB to another color model. This makes it possible to use the channels of that model as masks for simple RGB operations, or to make adjustments that would be hard to do in RGB. The workflow is: convert from the linear RGB working space to the model of your choice, make the adjustment, then convert back to RGB.
For example, a contrast adjustment on the saturation channel of HSL desaturates the less saturated parts of an image and increases the saturation of the rest.
The Lab model separates the lightness of an image (the L channel) from its hue and saturation (the a and b channels), better than HSL does. The a and b channels of one image can be copied into another one, with an effect similar to the hue blend mode of image editors: for example, the blurred colors of a vivid sunset can make a dull sky more colorful.
Blurring only the a and b channels blurs the hue and saturation of an image and keeps its lightness.
To convert between colorspaces of the OpenColorIO configuration (log, sRGB, Rec.709...), use :ref:`OCIOColorSpace <fr.inria.openfx.OCIOColorSpace>` instead (see :doc:`compositing-color-management`).

**ColorCorrect***

Has the same set of controls (**Saturation**, **Contrast**, **Gamma**,
**Gain** and **Offset**) four times: once for the whole image (*Master*), and
once each for the *Shadows*, *Midtones* and *Highlights*. This lets you, for
example, warm up the highlights while keeping neutral shadows. The curves
in its **Ranges** tab define which values count as shadows, midtones and
highlights.

**Gamma**

Bends the middle of the tone curve while leaving 0 and 1 in place. With the
default **Value** of 1 nothing changes; lower values darken the midtones and
higher values brighten them. Grade and ColorCorrect also have a gamma
control, and ColorLookup can do the same with more control over which part
of the curve moves.

**Grade***

The usual node to fix and match images. **Black Point** and **White Point**
pick the darkest and brightest values of the input, which are mapped to
**Lift** and **Gain**; **Multiply**, **Offset** and **Gamma** are then applied.
Setting the black and white points of one plate from the darkest and
brightest areas of another is a quick way to match their levels.

**Histogram**

The histogram is a scope, not a node: open it from the menu at the top left of any pane with **New histogram**. It shows the distribution of the values of the image displayed in a Viewer; right-click it to choose the Viewer, the input (A or B), the display mode and the smoothing.

**HueCorrect***

HueCorrect changes the colors of an image depending on their hue. It
computes the hue and saturation of each pixel, and reads the adjustment to
apply from a set of curves, where the horizontal axis is the hue. It has ten
curves:

hue
    Hue shift.

sat
    Saturation gain, applied last.

lum
    Luminance gain.

red, green, blue
    Gains on the red, green and blue channels.

r_sup, g_sup, b_sup
    Suppression of a channel toward the other two: for example, where r is
    larger than min(g, b), r_sup scales the difference, so that 0 brings r
    down to min(g, b). This is a common way to remove a color spill.

sat_thrsh
    Saturation threshold: the lum, red, green and blue gains are not applied
    to pixels whose saturation is below this value, and are applied
    progressively above it.

The **Mix Luminance** parameter restores part or all of the original
luminance. See :ref:`net.sf.openfx.HueCorrect` for all the parameters.


**HSVTool**

The HSVTool has three functions: Color replacement, Color adjust, Hue keyer


**Saturation***

A pixel with equal red, green and blue values is gray: it has no saturation.
The Saturation node mixes each pixel with its luminance, computed with the
**Luminance Math** formula (Rec. 709 by default). More localized control over
saturation is offered by the HueCorrect node.


Further reading
---------------

The archived Optical Enquiry Nuke wiki pages on `the Color nodes
<https://web.archive.org/web/20220523235532/http://www.opticalenquiry.com/nuke/index.php?title=The_Color_Nodes>`__
and `Color Operations Visualised
<https://web.archive.org/web/20220627030639/http://www.opticalenquiry.com/nuke/index.php?title=Color_Operations_Visualised>`__
show what the basic color operations do to a gradient. They use Nuke node
names, but these operations are the same in Natron.


.. toctree::
   :maxdepth: 2

   compositing-color-management

See also
--------

- :doc:`tutorials-hsvtool`: secondary color correction and color replacement
  with HSVTool.
- :doc:`tutorials-keying-huekeyer`: combining HueKeyer and HueCorrect for
  color effects.
- :ref:`evaluating-script-structure`: advice on organizing color grading in a
  project.
