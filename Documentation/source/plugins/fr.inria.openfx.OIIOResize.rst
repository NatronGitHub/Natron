.. _fr.inria.openfx.OIIOResize:

Resize node
===========

|pluginIcon| 

*This documentation is for version 1.0 of Resize.*

Description
-----------

Resize input stream, using OpenImageIO.

Note that only full images can be rendered, so it may be slower for interactive editing than the Reformat plugin.

However, the rendering algorithms are different between Reformat and Resize: Resize applies 1-dimensional filters in the horizontal and vertical directins, whereas Reformat resamples the image, so in some cases this plugin may give more visually pleasant results than Reformat.

This plugin does not concatenate transforms (as opposed to Reformat).

Inputs
------

+----------+---------------+------------+
| Input    | Description   | Optional   |
+==========+===============+============+
| Source   |               | No         |
+----------+---------------+------------+

Controls
--------

.. tabularcolumns:: |>{\raggedright}p{0.2\columnwidth}|>{\raggedright}p{0.06\columnwidth}|>{\raggedright}p{0.07\columnwidth}|p{0.63\columnwidth}|

.. cssclass:: longtable

+----------------------------------+-----------+---------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Parameter / script name          | Type      | Default             | Function                                                                                                                                                                     |
+==================================+===========+=====================+==============================================================================================================================================================================+
| Type / ``type``                  | Choice    | Format              | | Format: Converts between formats, the image is resized to fit in the target format. Size: Scales to fit into a box of a given width and height. Scale: Scales the image.   |
|                                  |           |                     | | **Format (format)**                                                                                                                                                        |
|                                  |           |                     | | **Size (size)**                                                                                                                                                            |
|                                  |           |                     | | **Scale (scale)**                                                                                                                                                          |
+----------------------------------+-----------+---------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Format / ``format``              | Choice    | PC\_Video 640x480   | | The output format                                                                                                                                                          |
|                                  |           |                     | | **PC\_Video 640x480 (PC\_Video)**                                                                                                                                          |
|                                  |           |                     | | **NTSC 720x486 0.91 (NTSC)**                                                                                                                                               |
|                                  |           |                     | | **PAL 720x576 1.09 (PAL)**                                                                                                                                                 |
|                                  |           |                     | | **NTSC\_16:9 720x486 1.21 (NTSC\_16:9)**                                                                                                                                   |
|                                  |           |                     | | **PAL\_16:9 720x576 1.46 (PAL\_16:9)**                                                                                                                                     |
|                                  |           |                     | | **HD\_720 1280x1720 (HD\_720)**                                                                                                                                            |
|                                  |           |                     | | **HD 1920x1080 (HD)**                                                                                                                                                      |
|                                  |           |                     | | **UHD\_4K 3840x2160 (UHD\_4K)**                                                                                                                                            |
|                                  |           |                     | | **1K\_Super35(full-ap) 1024x778 (1K\_Super35(full-ap))**                                                                                                                   |
|                                  |           |                     | | **1K\_Cinemascope 914x778 2 (1K\_Cinemascope)**                                                                                                                            |
|                                  |           |                     | | **2K\_Super35(full-ap) 2048x1556 (2K\_Super35(full-ap))**                                                                                                                  |
|                                  |           |                     | | **2K\_Cinemascope 1828x1556 2 (2K\_Cinemascope)**                                                                                                                          |
|                                  |           |                     | | **2K\_DCP 2048x1080 (2K\_DCP)**                                                                                                                                            |
|                                  |           |                     | | **4K\_Super35(full-ap) 4096x3112 (4K\_Super35(full-ap))**                                                                                                                  |
|                                  |           |                     | | **4K\_Cinemascope 3656x3112 2 (4K\_Cinemascope)**                                                                                                                          |
|                                  |           |                     | | **4K\_DCP 4096x2160 (4K\_DCP)**                                                                                                                                            |
|                                  |           |                     | | **square\_256 256x256 (square\_256)**                                                                                                                                      |
|                                  |           |                     | | **square\_512 512x512 (square\_512)**                                                                                                                                      |
|                                  |           |                     | | **square\_1K 1024x1024 (square\_1K)**                                                                                                                                      |
|                                  |           |                     | | **square\_2K 2048x2048 (square\_2K)**                                                                                                                                      |
+----------------------------------+-----------+---------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Size / ``size``                  | Integer   | x: 200 y: 200       | The output size                                                                                                                                                              |
+----------------------------------+-----------+---------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Preserve PAR / ``preservePAR``   | Boolean   | On                  | Preserve Pixel Aspect Ratio (PAR). When checked, one direction will be clipped.                                                                                              |
+----------------------------------+-----------+---------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Scale / ``scale``                | Double    | x: 1 y: 1           | The scale factor to apply to the image.                                                                                                                                      |
+----------------------------------+-----------+---------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Filter / ``filter``              | Choice    | lanczos3            | | The filter used to resize. Lanczos3 is great for downscaling and blackman-harris is great for upscaling.                                                                   |
|                                  |           |                     | | **Impulse (no interpolation)**                                                                                                                                             |
|                                  |           |                     | | **box**                                                                                                                                                                    |
|                                  |           |                     | | **triangle**                                                                                                                                                               |
|                                  |           |                     | | **gaussian**                                                                                                                                                               |
|                                  |           |                     | | **sharp-gaussian**                                                                                                                                                         |
|                                  |           |                     | | **catmull-rom**                                                                                                                                                            |
|                                  |           |                     | | **blackman-harris**                                                                                                                                                        |
|                                  |           |                     | | **sinc**                                                                                                                                                                   |
|                                  |           |                     | | **lanczos3**                                                                                                                                                               |
|                                  |           |                     | | **radial-lanczos3**                                                                                                                                                        |
|                                  |           |                     | | **mitchell**                                                                                                                                                               |
|                                  |           |                     | | **bspline**                                                                                                                                                                |
|                                  |           |                     | | **disk**                                                                                                                                                                   |
|                                  |           |                     | | **cubic**                                                                                                                                                                  |
|                                  |           |                     | | **keys**                                                                                                                                                                   |
|                                  |           |                     | | **simon**                                                                                                                                                                  |
|                                  |           |                     | | **rifman**                                                                                                                                                                 |
+----------------------------------+-----------+---------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+

.. |pluginIcon| image:: fr.inria.openfx.OIIOResize.png
   :width: 10.0%
