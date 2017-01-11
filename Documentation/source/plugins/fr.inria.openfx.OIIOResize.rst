.. _fr.inria.openfx.OIIOResize:

Resize node
===========

|pluginIcon| 

*This documentation is for version 1.0 of Resize.*

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

+----------------------------------+-----------+---------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Parameter / script name          | Type      | Default             | Function                                                                                                                                                                   |
+==================================+===========+=====================+============================================================================================================================================================================+
| Type / ``type``                  | Choice    | Format              | Format: Converts between formats, the image is resized to fit in the target format. Size: Scales to fit into a box of a given width and height. Scale: Scales the image.   |
+----------------------------------+-----------+---------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Format / ``format``              | Choice    | PC\_Video 640x480   | The output format                                                                                                                                                          |
+----------------------------------+-----------+---------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Size / ``size``                  | Integer   | x: 200 y: 200       | The output size                                                                                                                                                            |
+----------------------------------+-----------+---------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Preserve PAR / ``preservePAR``   | Boolean   | On                  | Preserve Pixel Aspect Ratio (PAR). When checked, one direction will be clipped.                                                                                            |
+----------------------------------+-----------+---------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Scale / ``scale``                | Double    | x: 1 y: 1           | The scale factor to apply to the image.                                                                                                                                    |
+----------------------------------+-----------+---------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Filter / ``filter``              | Choice    | lanczos3            | The filter used to resize. Lanczos3 is great for downscaling and blackman-harris is great for upscaling.                                                                   |
+----------------------------------+-----------+---------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------+

.. |pluginIcon| image:: fr.inria.openfx.OIIOResize.png
   :width: 10.0%
