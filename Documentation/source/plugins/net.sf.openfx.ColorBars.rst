.. _net.sf.openfx.ColorBars:

ColorBars node
==============

|pluginIcon| 

*This documentation is for version 1.0 of ColorBars.*

Description
-----------

Generate an image with SMPTE RP 219:2002 color bars.

The output of this plugin is broadcast-safe of "Output IRE" is unchecked. Be careful that colorbars are defined in a nonlinear colorspace. In order to get linear RGB, this plug-in should be combined with a transformation from the video space to linear.

See also: http://opticalenquiry.com/nuke/index.php?title=Constant,\_CheckerBoard,\_ColorBars,\_ColorWheel

Inputs
------

+----------+---------------+------------+
| Input    | Description   | Optional   |
+==========+===============+============+
| Source   |               | Yes        |
+----------+---------------+------------+

Controls
--------

.. tabularcolumns:: |>{\raggedright}p{0.2\columnwidth}|>{\raggedright}p{0.06\columnwidth}|>{\raggedright}p{0.07\columnwidth}|p{0.63\columnwidth}|

.. cssclass:: longtable

+--------------------------------------------+-----------+-----------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Parameter / script name                    | Type      | Default         | Function                                                                                                                                                        |
+============================================+===========+=================+=================================================================================================================================================================+
| Extent / ``extent``                        | Choice    | Default         | | Extent (size and offset) of the output.                                                                                                                       |
|                                            |           |                 | | **Format (format)**: Use a pre-defined image format.                                                                                                          |
|                                            |           |                 | | **Size (size)**: Use a specific extent (size and offset).                                                                                                     |
|                                            |           |                 | | **Project (project)**: Use the project extent (size and offset).                                                                                              |
|                                            |           |                 | | **Default (default)**: Use the default extent (e.g. the source clip extent, if connected).                                                                    |
+--------------------------------------------+-----------+-----------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Center / ``recenter``                      | Button    |                 | Centers the region of definition to the input region of definition. If there is no input, then the region of definition is centered to the project window.      |
+--------------------------------------------+-----------+-----------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Reformat / ``reformat``                    | Boolean   | Off             | Set the output format to the given extent, except if the Bottom Left or Size parameters is animated.                                                            |
+--------------------------------------------+-----------+-----------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Format / ``NatronParamFormatChoice``       | Choice    | HD 1920x1080    | | The output format                                                                                                                                             |
|                                            |           |                 | | **PC\_Video 640x480 (PC\_Video)**                                                                                                                             |
|                                            |           |                 | | **NTSC 720x486 0.91 (NTSC)**                                                                                                                                  |
|                                            |           |                 | | **PAL 720x576 1.09 (PAL)**                                                                                                                                    |
|                                            |           |                 | | **NTSC\_16:9 720x486 1.21 (NTSC\_16:9)**                                                                                                                      |
|                                            |           |                 | | **PAL\_16:9 720x576 1.46 (PAL\_16:9)**                                                                                                                        |
|                                            |           |                 | | **HD\_720 1280x720 (HD\_720)**                                                                                                                                |
|                                            |           |                 | | **HD 1920x1080 (HD)**                                                                                                                                         |
|                                            |           |                 | | **UHD\_4K 3840x2160 (UHD\_4K)**                                                                                                                               |
|                                            |           |                 | | **1K\_Super\_35(full-ap) 1024x778 (1K\_Super\_35(full-ap))**                                                                                                  |
|                                            |           |                 | | **1K\_Cinemascope 914x778 2.00 (1K\_Cinemascope)**                                                                                                            |
|                                            |           |                 | | **2K\_Super\_35(full-ap) 2048x1556 (2K\_Super\_35(full-ap))**                                                                                                 |
|                                            |           |                 | | **2K\_Cinemascope 1828x1556 2.00 (2K\_Cinemascope)**                                                                                                          |
|                                            |           |                 | | **2K\_DCP 2048x1080 (2K\_DCP)**                                                                                                                               |
|                                            |           |                 | | **4K\_Super\_35(full-ap) 4096x3112 (4K\_Super\_35(full-ap))**                                                                                                 |
|                                            |           |                 | | **4K\_Cinemascope 3656x3112 2.00 (4K\_Cinemascope)**                                                                                                          |
|                                            |           |                 | | **4K\_DCP 4096x2160 (4K\_DCP)**                                                                                                                               |
|                                            |           |                 | | **square\_256 256x256 (square\_256)**                                                                                                                         |
|                                            |           |                 | | **square\_512 512x512 (square\_512)**                                                                                                                         |
|                                            |           |                 | | **square\_1K 1024x1024 (square\_1K)**                                                                                                                         |
|                                            |           |                 | | **square\_2K 2048x2048 (square\_2K)**                                                                                                                         |
+--------------------------------------------+-----------+-----------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Bottom Left / ``bottomLeft``               | Double    | x: 0 y: 0       | Coordinates of the bottom left corner of the size rectangle.                                                                                                    |
+--------------------------------------------+-----------+-----------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Size / ``size``                            | Double    | w: 1 h: 1       | Width and height of the size rectangle.                                                                                                                         |
+--------------------------------------------+-----------+-----------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Interactive Update / ``interactive``       | Boolean   | Off             | If checked, update the parameter values during interaction with the image viewer, else update the values when pen is released.                                  |
+--------------------------------------------+-----------+-----------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Frame Range / ``frameRange``               | Integer   | min: 1 max: 1   | Time domain.                                                                                                                                                    |
+--------------------------------------------+-----------+-----------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Output Components / ``outputComponents``   | Choice    | RGBA            | | Components in the output                                                                                                                                      |
|                                            |           |                 | | **RGBA**                                                                                                                                                      |
|                                            |           |                 | | **RGB**                                                                                                                                                       |
+--------------------------------------------+-----------+-----------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Bar Intensity / ``barIntensity``           | Double    | 75              | Bar Intensity, in IRE unit.                                                                                                                                     |
+--------------------------------------------+-----------+-----------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Output IRE / ``outputIRE``                 | Boolean   | Off             | When checked, the output is scaled so that 0 is black, the max value is white, and the superblack (under the middle of the magenta bar) has a negative value.   |
+--------------------------------------------+-----------+-----------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------------+

.. |pluginIcon| image:: net.sf.openfx.ColorBars.png
   :width: 10.0%
