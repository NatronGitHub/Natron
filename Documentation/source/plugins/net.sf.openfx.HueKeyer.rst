.. _net.sf.openfx.HueKeyer:

HueKeyer node
=============

*This documentation is for version 1.0 of HueKeyer.*

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

.. tabularcolumns:: |>{\raggedright}p{0.2\columnwidth}|>{\raggedright}p{0.06\columnwidth}|>{\raggedright}p{0.07\columnwidth}|p{0.63\columnwidth}|

.. cssclass:: longtable

+---------------------------+--------------+---------------------------+----------------------------------------------------------------------------------------------------+
| Parameter / script name   | Type         | Default                   | Function                                                                                           |
+===========================+==============+===========================+====================================================================================================+
| Hue Curves / ``hue``      | Parametric   | amount:   sat\_thrsh:     | | Hue-dependent alpha lookup curves:                                                               |
|                           |              |                           | | amount: transparency (1-alpha) amount for the given hue                                          |
|                           |              |                           | | sat\_thrsh: if source saturation is below this value, transparency is decreased progressively.   |
+---------------------------+--------------+---------------------------+----------------------------------------------------------------------------------------------------+
