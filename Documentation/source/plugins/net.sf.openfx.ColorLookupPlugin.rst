.. _net.sf.openfx.ColorLookupPlugin:

ColorLookup node
================

|pluginIcon| 

*This documentation is for version 1.0 of ColorLookup.*

Description
-----------

Apply a parametric lookup curve to each channel separately.

The master curve is combined with the red, green and blue curves, but not with the alpha curve.

Computation is faster for values that are within the given range, so it is recommended to set the Range parameter if the input range goes beyond [0,1].

Note that you can easily do color remapping by setting Source and Target colors and clicking "Set RGB" or "Set RGBA" below.

This will add control points on the curve to match the target from the source. You can add as many point as you like.

This is very useful for matching color of one shot to another, or adding custom colors to a black and white ramp.

See also: http://opticalenquiry.com/nuke/index.php?title=ColorLookup

Inputs
------

+----------+---------------+------------+
| Input    | Description   | Optional   |
+==========+===============+============+
| Source   |               | No         |
+----------+---------------+------------+
| Mask     |               | Yes        |
+----------+---------------+------------+

Controls
--------

.. tabularcolumns:: |>{\raggedright}p{0.2\columnwidth}|>{\raggedright}p{0.06\columnwidth}|>{\raggedright}p{0.07\columnwidth}|p{0.63\columnwidth}|

.. cssclass:: longtable

+--------------------------------------+--------------+----------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------+
| Parameter / script name              | Type         | Default                                      | Function                                                                                                                                          |
+======================================+==============+==============================================+===================================================================================================================================================+
| Range / ``range``                    | Double       | min: 0 max: 1                                | Expected range for input values. Within this range, a lookup table is used for faster computation.                                                |
+--------------------------------------+--------------+----------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------+
| Lookup Table / ``lookupTable``       | Parametric   | master:   red:   green:   blue:   alpha:     | Colour lookup table. The master curve is combined with the red, green and blue curves, but not with the alpha curve.                              |
+--------------------------------------+--------------+----------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------+
| Display Color Ramp / ``showRamp``    | Boolean      | On                                           | Display the color ramp under the curves.                                                                                                          |
+--------------------------------------+--------------+----------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------+
| Source / ``source``                  | Color        | r: 0 g: 0 b: 0 a: 0                          | Source color for newly added points (x coordinate on the curve).                                                                                  |
+--------------------------------------+--------------+----------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------+
| Target / ``target``                  | Color        | r: 0 g: 0 b: 0 a: 0                          | Target color for newly added points (y coordinate on the curve).                                                                                  |
+--------------------------------------+--------------+----------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------+
| Set Master / ``setMaster``           | Button       |                                              | Add a new control point mapping source to target to the master curve (the relative luminance is computed using the 'Luminance Math' parameter).   |
+--------------------------------------+--------------+----------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------+
| Set RGB / ``setRGB``                 | Button       |                                              | Add a new control point mapping source to target to the red, green, and blue curves.                                                              |
+--------------------------------------+--------------+----------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------+
| Set RGBA / ``setRGBA``               | Button       |                                              | Add a new control point mapping source to target to the red, green, blue and alpha curves.                                                        |
+--------------------------------------+--------------+----------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------+
| Set A / ``setA``                     | Button       |                                              | Add a new control point mapping source to target to the alpha curve                                                                               |
+--------------------------------------+--------------+----------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------+
| Luminance Math / ``luminanceMath``   | Choice       | Rec. 709                                     | | Formula used to compute luminance from RGB values (only used by 'Set Master').                                                                  |
|                                      |              |                                              | | **Rec. 709**: Use Rec. 709 (0.2126r + 0.7152g + 0.0722b).                                                                                       |
|                                      |              |                                              | | **Rec. 2020**: Use Rec. 2020 (0.2627r + 0.6780g + 0.0593b).                                                                                     |
|                                      |              |                                              | | **ACES AP0**: Use ACES AP0 (0.3439664498r + 0.7281660966g + -0.0721325464b).                                                                    |
|                                      |              |                                              | | **ACES AP1**: Use ACES AP1 (0.2722287168r + 0.6740817658g + 0.0536895174b).                                                                     |
|                                      |              |                                              | | **CCIR 601**: Use CCIR 601 (0.2989r + 0.5866g + 0.1145b).                                                                                       |
|                                      |              |                                              | | **Average**: Use average of r, g, b.                                                                                                            |
|                                      |              |                                              | | **Max**: Use max or r, g, b.                                                                                                                    |
+--------------------------------------+--------------+----------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------+
| Clamp Black / ``clampBlack``         | Boolean      | Off                                          | All colors below 0 on output are set to 0.                                                                                                        |
+--------------------------------------+--------------+----------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------+
| Clamp White / ``clampWhite``         | Boolean      | Off                                          | All colors above 1 on output are set to 1.                                                                                                        |
+--------------------------------------+--------------+----------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------+
| (Un)premult / ``premult``            | Boolean      | Off                                          | Divide the image by the alpha channel before processing, and re-multiply it afterwards. Use if the input images are premultiplied.                |
+--------------------------------------+--------------+----------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------+
| Invert Mask / ``maskInvert``         | Boolean      | Off                                          | When checked, the effect is fully applied where the mask is 0.                                                                                    |
+--------------------------------------+--------------+----------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------+
| Mix / ``mix``                        | Double       | 1                                            | Mix factor between the original and the transformed image.                                                                                        |
+--------------------------------------+--------------+----------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------+

.. |pluginIcon| image:: net.sf.openfx.ColorLookupPlugin.png
   :width: 10.0%
