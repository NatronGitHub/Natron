.. _net.sf.openfx.Premult:

Premult node
============

|pluginIcon| 

*This documentation is for version 2.0 of Premult.*

Multiply the selected channels by alpha (or another channel).

If no channel is selected, or the premultChannel is set to None, the image data is left untouched, but its premultiplication state is set to PreMultiplied.

See also: http://opticalenquiry.com/nuke/index.php?title=Premultiplication

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

+-------------------------------+----------+-----------+-----------------------------------------+
| Parameter / script name       | Type     | Default   | Function                                |
+===============================+==========+===========+=========================================+
| By / ``premultChannel``       | Choice   | A         | | The channel to use for (un)premult.   |
|                               |          |           | | **None**: Don't multiply/divide       |
|                               |          |           | | **R**: R channel from input           |
|                               |          |           | | **G**: G channel from input           |
|                               |          |           | | **B**: B channel from input           |
|                               |          |           | | **A**: A channel from input           |
+-------------------------------+----------+-----------+-----------------------------------------+
| Clip Info... / ``clipInfo``   | Button   |           | Display information about the inputs    |
+-------------------------------+----------+-----------+-----------------------------------------+

.. |pluginIcon| image:: net.sf.openfx.Premult.png
   :width: 10.0%
