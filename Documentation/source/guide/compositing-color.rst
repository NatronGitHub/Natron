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
Really important nodes are marked with an asterix:



**Add***

This node affects all values within the image in the same way: literally adding to them.
Positive values will brighten all parts of the image. As a stand alone color manipulation it is of limited use, though it is ideal for lightening the blacks of distant objects.


**Multiply***

It can be used to brighten or darken an image, or to fix a color cast. In maths, Multiply has no effect upon zero (black). Hence, the Multiply node will have no effect upon the blacks of an image.

**Clamp**

The Clamp node, like the ColorLookup node, is not explicitly designed to provide feedback on images, however it can easily be used to do so.
It functions in much the same way as the ClipTest node: it flattens out the user-defined lower and upper lightness range of the image and,
if you tick MinClampTo and MaxClampTo, can replace these values with user defined colors.
Properly used it can provide clearer information than the ClipTest node.


**ColorLookup***

A color lookup is what image editors Curves tool does.
It is a very powerful tool, capable of replicating the function of many other color nodes.
Its disadvantage is that it requires more processing than many of those nodes.
The curve of a color lookup can also be a bit more difficult to read than nodes featuring sliders.

**Colorspace conversion nodes**

Engineers think of color as existing in things called spaces which are mathematical, 3D models the purpose of which is to organise them. Different color spaces serve different purposes: some are meant for printing, some are meant for screen-based work, some are meant for TV. Nodes such as :ref:`RGBToHSL <net.sf.openfx.RGBToHSL>`, :ref:`RGBToHSV <net.sf.openfx.RGBToHSV>` or :ref:`RGB709ToLab <net.sf.openfx.RGB709ToLab>` (and their inverses, such as :ref:`HSLToRGB <net.sf.openfx.HSLToRGB>` and :ref:`LabToRGB709 <net.sf.openfx.LabToRGB709>`) move an image from RGB to another color model. This makes it possible to use the channels of that model as masks for simple RGB operations, or to make adjustments that would be hard to do in RGB. The workflow is: convert from the linear RGB working space to the model of your choice, make the adjustment, then convert back to RGB.
For example, a contrast adjustment on the saturation channel of HSL desaturates the less saturated parts of an image and increases the saturation of the rest.
The Lab model separates the lightness of an image (the L channel) from its hue and saturation (the a and b channels), better than HSL does. The a and b channels of one image can be copied into another one, with an effect similar to the hue blend mode of image editors: for example, the blurred colors of a vivid sunset can make a dull sky more colorful.
Blurring only the a and b channels blurs the hue and saturation of an image and keeps its lightness.
To convert between colorspaces of the OpenColorIO configuration (log, sRGB, Rec.709...), use :ref:`OCIOColorSpace <fr.inria.openfx.OCIOColorSpace>` instead (see :doc:`compositing-color-management`).

**ColorCorrect***

Artists have for thousands of years been separating the lightness values of their paintings into three bands: shadows, midtones and highlights. The ColorCorrect node is a collection of operations that not only can effect the entire image but can address separately these three ranges 

**Gamma**

This raises or lowers the middle-ish point of the color curve. The default value is one, with smaller numbers darkening the lower registers and higher numbers lightening them. Stand-alone gamma adjustments can also be done with ColorLookup, which gives more control over where the curve is bent. Both ColorCorrect and Grade have built in Gamma value sliders.

**Grade***

This node is a collection of operations that combine to work upon the lightness and hue values of an image. It is mostly a fixer: used for correcting and matching, though of course it can also be used for more aesthetically lavish purposes.

**Histogram**

The histogram is a scope, not a node: open it from the menu at the top left of any pane with **New histogram**. It shows the distribution of the values of the image displayed in a Viewer; right-click it to choose the Viewer, the input (A or B), the display mode and the smoothing.

**HueCorrect***

This can be a very tricky node to get to know.
We can conclude that the perception of the amount of hue within a color (its chroma) is linked to two things: saturation and lightness.
When adjusting color it is important to have separate control over these values, which is something that the HueCorrect node offers.
This effect it masks by two further values: hue and saturation. Its interface offers control over nine values:

Saturation (sat)
This can change the saturation value of an image, with respect to particular regions of hue.

Luminance (lum)
This can change the luminosity (i.e. brightness) value of an image, with respect to particular regions of hue.

Luminance components (red, blue, green)
This can change the r, g and b channels of an image, with respect to particular regions of hue.

Suppression (r_sup, g_sup, b_sup)
This is similar to adjusting the luminance components, but instead of nullifying them (replacing them with black), replaces them with white.

Saturation threshold (sat_thrsh)
This only effects the image if first the 'Luminance' or 'Luminance components' have been adjusted. Adjustments to this value will act as 'per hue' saturation level mask to the effect.


**HSVTool**

The HSVTool has three functions: Color replacement, Color adjust, Hue keyer


**Saturation***

A color becomes a grey if its RGB values are all identical.
The Saturation node desaturates an image by averaging its RGB channels.
More localised control over saturation is offered by the HueCorrect node.


.. toctree::
   :maxdepth: 2

   compositing-color-management

