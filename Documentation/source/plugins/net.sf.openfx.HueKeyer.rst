.. _net.sf.openfx.HueKeyer:

HueKeyerOFX
===========

*This documentation is for version 1.0 of HueKeyerOFX.*

Compute a key depending on hue value.

Hue and saturation are computed from the the source RGB values. Depending on the hue value, the various adjustment values are computed, and then applied:

amount: output transparency for the given hue (amount=1 means alpha=0).

sat\_thrsh: if source saturation is below this value, the output transparency is gradually decreased.

Inputs
------

+----------+---------------+------------+
| Input    | Description   | Optional   |
+==========+===============+============+
| Source   |               | No         |
+----------+---------------+------------+

Controls
--------

+-------------------+---------------+--------------+-----------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Label (UI Name)   | Script-Name   | Type         | Default-Value   | Function                                                                                                                                                                                  |
+===================+===============+==============+=================+===========================================================================================================================================================================================+
| Hue Curves        | hue           | Parametric   | N/A             | Hue-dependent alpha lookup curves:amount: transparency (1-alpha) amount for the given huesat\_thrsh: if source saturation is below this value, transparency is decreased progressively.   |
+-------------------+---------------+--------------+-----------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
