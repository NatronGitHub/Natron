.. _fr.inria.EdgeBlur:

EdgeBlur node
=============

*This documentation is for version 1.0 of EdgeBlur.*

Description
-----------

Blur the image where there are edges in the alpha/matte channel.

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

+-----------------------------------------+-----------+------------+--------------------------------------------------------------------------------------------------------------------------------------------------+
| Parameter / script name                 | Type      | Default    | Function                                                                                                                                         |
+=========================================+===========+============+==================================================================================================================================================+
| Convert to Group / ``convertToGroup``   | Button    |            | Converts this node to a Group: the internal node-graph and the user parameters will become editable                                              |
+-----------------------------------------+-----------+------------+--------------------------------------------------------------------------------------------------------------------------------------------------+
| Size / ``size``                         | Double    | 3          |                                                                                                                                                  |
+-----------------------------------------+-----------+------------+--------------------------------------------------------------------------------------------------------------------------------------------------+
| Filter / ``filter``                     | Choice    | Gaussian   | |                                                                                                                                                |
|                                         |           |            | | **Simple**: Gradient is estimated by centered finite differences.                                                                              |
|                                         |           |            | | **Sobel**: Compute gradient using the Sobel 3x3 filter.                                                                                        |
|                                         |           |            | | **Rotation Invariant**: Compute gradient using a 3x3 rotation-invariant filter.                                                                |
|                                         |           |            | | **Quasi-Gaussian**: Quasi-Gaussian filter (0-order recursive Deriche filter, faster) - IIR (infinite support / impulsional response).          |
|                                         |           |            | | **Gaussian**: Gaussian filter (Van Vliet recursive Gaussian filter, more isotropic, slower) - IIR (infinite support / impulsional response).   |
|                                         |           |            | | **Box**: Box filter - FIR (finite support / impulsional response).                                                                             |
|                                         |           |            | | **Triangle**: Triangle/tent filter - FIR (finite support / impulsional response).                                                              |
|                                         |           |            | | **Quadratic**: Quadratic filter - FIR (finite support / impulsional response).                                                                 |
+-----------------------------------------+-----------+------------+--------------------------------------------------------------------------------------------------------------------------------------------------+
| Crop To Format / ``cropToFormat``       | Boolean   | On         |                                                                                                                                                  |
+-----------------------------------------+-----------+------------+--------------------------------------------------------------------------------------------------------------------------------------------------+
| Edge Mult / ``edgeMult``                | Double    | 2          | Sharpness of the borders of the blur area.                                                                                                       |
+-----------------------------------------+-----------+------------+--------------------------------------------------------------------------------------------------------------------------------------------------+
| Invert Mask / ``Merge1maskInvert``      | Boolean   | Off        |                                                                                                                                                  |
+-----------------------------------------+-----------+------------+--------------------------------------------------------------------------------------------------------------------------------------------------+
| Mix / ``Blur1mix``                      | Double    | 1          |                                                                                                                                                  |
+-----------------------------------------+-----------+------------+--------------------------------------------------------------------------------------------------------------------------------------------------+
