.. _eu.cimg.Distance:

Distance node
=============

*This documentation is for version 1.0 of Distance.*

Description
-----------

Compute Euclidean distance function to pixels that have a value of zero. The distance is normalized with respect to the largest image dimension, so that it is always between 0 and 1.

Uses the 'distance' function from the CImg library.

CImg is a free, open-source library distributed under the CeCILL-C (close to the GNU LGPL) or CeCILL (compatible with the GNU GPL) licenses. It can be used in commercial applications (see http://cimg.eu).

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

+--------------------------------+-----------+-------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Parameter / script name        | Type      | Default     | Function                                                                                                                                                      |
+================================+===========+=============+===============================================================================================================================================================+
| Metric / ``metric``            | Choice    | Euclidean   | | Type of metric.                                                                                                                                             |
|                                |           |             | | **Chebyshev**: max(abs(x-xborder),abs(y-yborder))                                                                                                           |
|                                |           |             | | **Manhattan**: abs(x-xborder) + abs(y-yborder)                                                                                                              |
|                                |           |             | | **Euclidean**: sqrt(sqr(x-xborder) + sqr(y-yborder))                                                                                                        |
|                                |           |             | | **Squared Euclidean**: sqr(x-xborder) + sqr(y-yborder)                                                                                                      |
|                                |           |             | | **Spherical**: Compute the Euclidean distance, and draw a sphere at each point. Gives a round shape rather than a conical shape to the distance function.   |
+--------------------------------+-----------+-------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
| (Un)premult / ``premult``      | Boolean   | Off         | Divide the image by the alpha channel before processing, and re-multiply it afterwards. Use if the input images are premultiplied.                            |
+--------------------------------+-----------+-------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Invert Mask / ``maskInvert``   | Boolean   | Off         | When checked, the effect is fully applied where the mask is 0.                                                                                                |
+--------------------------------+-----------+-------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Mix / ``mix``                  | Double    | 1           | Mix factor between the original and the transformed image.                                                                                                    |
+--------------------------------+-----------+-------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------+
