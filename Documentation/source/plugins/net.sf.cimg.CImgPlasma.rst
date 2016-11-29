.. _net.sf.cimg.CImgPlasma:

PlasmaCImg
==========

.. figure:: net.sf.cimg.CImgPlasma.png
   :alt: 

*This documentation is for version 2.0 of PlasmaCImg.*

Draw a random plasma texture (using the mid-point algorithm).

Note that each render scale gives a different noise, but the image rendered at full scale always has the same noise at a given time. Noise can be modulated using the 'seed' parameter.

Uses the 'draw\_plasma' function from the CImg library.

CImg is a free, open-source library distributed under the CeCILL-C (close to the GNU LGPL) or CeCILL (compatible with the GNU GPL) licenses. It can be used in commercial applications (see http://cimg.eu).

Inputs
------

+----------+---------------+------------+
| Input    | Description   | Optional   |
+==========+===============+============+
| Source   |               | Yes        |
+----------+---------------+------------+
| Mask     |               | Yes        |
+----------+---------------+------------+

Controls
--------

+-------------------+---------------+-----------+-----------------+--------------------------------------------------------------------------------------------------------------------------------------+
| Label (UI Name)   | Script-Name   | Type      | Default-Value   | Function                                                                                                                             |
+===================+===============+===========+=================+======================================================================================================================================+
| Alpha             | alpha         | Double    | 0.002           | Alpha-parameter, in intensity units (>=0).                                                                                           |
+-------------------+---------------+-----------+-----------------+--------------------------------------------------------------------------------------------------------------------------------------+
| Beta              | beta          | Double    | 0               | Beta-parameter, in intensity units (>=0).                                                                                            |
+-------------------+---------------+-----------+-----------------+--------------------------------------------------------------------------------------------------------------------------------------+
| Scale             | scale         | Integer   | 8               | Noise scale, as a power of two (>=0).                                                                                                |
+-------------------+---------------+-----------+-----------------+--------------------------------------------------------------------------------------------------------------------------------------+
| Offset            | offset        | Double    | 0               | Offset to add to the plasma noise.                                                                                                   |
+-------------------+---------------+-----------+-----------------+--------------------------------------------------------------------------------------------------------------------------------------+
| Random Seed       | seed          | Integer   | 0               | Random seed used to generate the image. Time value is added to this seed, to get a time-varying effect.                              |
+-------------------+---------------+-----------+-----------------+--------------------------------------------------------------------------------------------------------------------------------------+
| (Un)premult       | premult       | Boolean   | Off             | Divide the image by the alpha channel before processing, and re-multiply it afterwards. Use if the input images are premultiplied.   |
+-------------------+---------------+-----------+-----------------+--------------------------------------------------------------------------------------------------------------------------------------+
| Invert Mask       | maskInvert    | Boolean   | Off             | When checked, the effect is fully applied where the mask is 0.                                                                       |
+-------------------+---------------+-----------+-----------------+--------------------------------------------------------------------------------------------------------------------------------------+
| Mix               | mix           | Double    | 1               | Mix factor between the original and the transformed image.                                                                           |
+-------------------+---------------+-----------+-----------------+--------------------------------------------------------------------------------------------------------------------------------------+
