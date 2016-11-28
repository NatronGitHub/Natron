.. _net.sf.openfx.VectorToColorPlugin:

VectorToColorOFX
================

.. figure:: net.sf.openfx.VectorToColorPlugin.png
   :alt: 

*This documentation is for version 1.0 of VectorToColorOFX.*

Convert x and y vector components to a color representation.

H (hue) gives the direction, S (saturation) is set to the amplitude/norm, and V is 1.The role of S and V can be switched.Output can be RGB or HSV, with H in degrees.

Inputs
------

+----------+---------------+------------+
| Input    | Description   | Optional   |
+==========+===============+============+
| Source   |               | No         |
+----------+---------------+------------+

Controls
--------

+-------------------+---------------+-----------+-----------------+----------------------------------------------------------------------------------------------------------------------------------------------------------+
| Label (UI Name)   | Script-Name   | Type      | Default-Value   | Function                                                                                                                                                 |
+===================+===============+===========+=================+==========================================================================================================================================================+
| X channel         | xChannel      | Choice    | r               | Selects the X component of vectors\ **r**: R channel from input\ **g**: G channel from input\ **b**: B channel from input\ **a**: A channel from input   |
+-------------------+---------------+-----------+-----------------+----------------------------------------------------------------------------------------------------------------------------------------------------------+
| Y channel         | yChannel      | Choice    | g               | Selects the Y component of vectors\ **r**: R channel from input\ **g**: G channel from input\ **b**: B channel from input\ **a**: A channel from input   |
+-------------------+---------------+-----------+-----------------+----------------------------------------------------------------------------------------------------------------------------------------------------------+
| Opposite          | opposite      | Boolean   | Off             | If checked, opposite of X and Y are used.                                                                                                                |
+-------------------+---------------+-----------+-----------------+----------------------------------------------------------------------------------------------------------------------------------------------------------+
| Inverse Y         | inverseY      | Boolean   | On              | If checked, opposite of Y is used (on by default, because most optical flow results are shown using a downward Y axis).                                  |
+-------------------+---------------+-----------+-----------------+----------------------------------------------------------------------------------------------------------------------------------------------------------+
| Modulate V        | modulateV     | Boolean   | Off             | If checked, modulate V using the vector amplitude, instead of S.                                                                                         |
+-------------------+---------------+-----------+-----------------+----------------------------------------------------------------------------------------------------------------------------------------------------------+
| HSV Output        | hsvOutput     | Boolean   | Off             | If checked, output is in the HSV color model.                                                                                                            |
+-------------------+---------------+-----------+-----------------+----------------------------------------------------------------------------------------------------------------------------------------------------------+
