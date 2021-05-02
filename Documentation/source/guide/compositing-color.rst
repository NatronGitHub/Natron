.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf
   
Working with color
==================
How to use Natron color correction nodes and tools to adjust the appearance of the images in your composites.
When starting out, this information provides a good overview of Natron scopes and color-correction nodes,
however not all options are covered here.



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

**ColorSpace**

Engineers think of color as existing in things called spaces which are mathematical, 3D models the purpose of which is to organise them. Different color spaces serve different purposes: some are meant for printing, some are meant for screen-based work, some are meant for TV. The ColorSpace can move an image from one color space to another. The neat thing about this is that it makes it possible to use channels from exotic color spaces as masks to simple RGB operations or to perform adjustments on images to produce results that would have been impossible in ordinary RGB space. The workflow is: convert from Natron default Linear color space into the color space of your choice, perform your funky magic, then convert back to linear.
The saturation channel of HSL can, for example, have a contrast adjustment applied to it which could desaturate the less saturated parts of an image and super saturate the remainder. Woot! Try that in Photoshop!
Lab color space is another useful fellow. It is the quantum physics of the color world and I shall but kiss the shadow of it's vast and complex form. The fascinating thing about this space is that it separates the lightness values of an image (the L channel) from it's hue and saturation (the a and b channel combined). HSL also does this but not nearly so well. You can take the a and b and move them into those of another image.
The effect of this is similar to image editors hue blend mode. I have found it useful to augment the colorfulness of a dull sky by using the blurred color values from a vivid sunset.
Try also blurring the a and b channels. This will blur only the hue and saturation components of an image and leave its lightness values alone.
A novel use for the ColorSpace node can be found in the Assets page (see the Double Rainbows asset).

**ColorCorrect***

Artists have for thousands of years been separating the lightness values of their paintings into three bands: shadows, midtones and highlights. The ColorCorrect node is a collection of operations that not only can effect the entire image but can address separately these three ranges 

**Gamma**

This raises or lowers the middle-ish point of the color curve. The default value is one, with smaller numbers darkening the lower registers and higher numbers lightening them. There is a Gamma node but I find that stand-alone gamma adjustments are best done using ColorLookup so as to give you flexibility over where the ‘grab point‘ of your curve is. Both ColorCorrect and Grade have built in Gamma value sliders.

**Grade***

This node is a collection of operations that combine to work upon the lightness and hue values of an image. It is mostly a fixer: used for correcting and matching, though of course it can also be used for more aesthetically lavish purposes.

**Histogram**

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

