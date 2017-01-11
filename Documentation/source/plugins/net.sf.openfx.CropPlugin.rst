.. _net.sf.openfx.CropPlugin:

Crop node
=========

|pluginIcon| 

*This documentation is for version 1.0 of Crop.*

Removes everything outside the defined rectangle and optionally adds black edges so everything outside is black.

If the 'Extent' parameter is set to 'Format', and 'Reformat' is checked, the output pixel aspect ratio is also set to this of the format.

This plugin does not concatenate transforms.

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

+----------------------------------------+-----------+-----------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Parameter / script name                | Type      | Default         | Function                                                                                                                                                     |
+========================================+===========+=================+==============================================================================================================================================================+
| Extent / ``extent``                    | Choice    | Size            | | Extent (size and offset) of the output.                                                                                                                    |
|                                        |           |                 | | **Format**: Use a pre-defined image format.                                                                                                                |
|                                        |           |                 | | **Size**: Use a specific extent (size and offset).                                                                                                         |
|                                        |           |                 | | **Project**: Use the project extent (size and offset).                                                                                                     |
|                                        |           |                 | | **Default**: Use the default extent (e.g. the source clip extent, if connected).                                                                           |
+----------------------------------------+-----------+-----------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Center / ``recenter``                  | Button    |                 | Centers the region of definition to the input region of definition. If there is no input, then the region of definition is centered to the project window.   |
+----------------------------------------+-----------+-----------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Format / ``NatronParamFormatChoice``   | Choice    | HD 1920x1080    | The output format                                                                                                                                            |
+----------------------------------------+-----------+-----------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Bottom Left / ``bottomLeft``           | Double    | x: 0 y: 0       | Coordinates of the bottom left corner of the size rectangle.                                                                                                 |
+----------------------------------------+-----------+-----------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Size / ``size``                        | Double    | w: 1 h: 1       | Width and height of the size rectangle.                                                                                                                      |
+----------------------------------------+-----------+-----------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Interactive Update / ``interactive``   | Boolean   | Off             | If checked, update the parameter values during interaction with the image viewer, else update the values when pen is released.                               |
+----------------------------------------+-----------+-----------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Frame Range / ``frameRange``           | Integer   | min: 1 max: 1   | Time domain.                                                                                                                                                 |
+----------------------------------------+-----------+-----------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Softness / ``softness``                | Double    | 0               | Size of the fade to black around edges to apply.                                                                                                             |
+----------------------------------------+-----------+-----------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Reformat / ``reformat``                | Boolean   | Off             | Translates the bottom left corner of the crop rectangle to be in (0,0).                                                                                      |
+----------------------------------------+-----------+-----------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Intersect / ``intersect``              | Boolean   | Off             | Intersects the crop rectangle with the input region of definition instead of extending it.                                                                   |
+----------------------------------------+-----------+-----------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Black Outside / ``blackOutside``       | Boolean   | Off             | Add 1 black and transparent pixel to the region of definition so that all the area outside the crop rectangle is black.                                      |
+----------------------------------------+-----------+-----------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------+

.. |pluginIcon| image:: net.sf.openfx.CropPlugin.png
   :width: 10.0%
