.. _net.sf.cimg.CImgErode:

ErodeCImg
=========

.. figure:: net.sf.cimg.CImgErode.png
   :alt: 

*This documentation is for version 2.1 of ErodeCImg.*

Erode (or dilate) input stream by a rectangular structuring element of specified size and Neumann boundary conditions (pixels out of the image get the value of the nearest pixel).

A negative size will perform a dilation instead of an erosion.

Different sizes can be given for the x and y axis.

Uses the 'erode' and 'dilate' functions from the CImg library.

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

+-------------------+---------------+-----------+-----------------+--------------------------------------------------------------------------------------------------------------------------------------+
| Label (UI Name)   | Script-Name   | Type      | Default-Value   | Function                                                                                                                             |
+===================+===============+===========+=================+======================================================================================================================================+
| size              | size          | Integer   | x: 1 y: 1       | Width/height of the rectangular structuring element is 2\*size+1, in pixel units (>=0).                                              |
+-------------------+---------------+-----------+-----------------+--------------------------------------------------------------------------------------------------------------------------------------+
| Expand RoD        | expandRoD     | Boolean   | On              | Expand the source region of definition by 2\*size pixels if size is negative                                                         |
+-------------------+---------------+-----------+-----------------+--------------------------------------------------------------------------------------------------------------------------------------+
| (Un)premult       | premult       | Boolean   | Off             | Divide the image by the alpha channel before processing, and re-multiply it afterwards. Use if the input images are premultiplied.   |
+-------------------+---------------+-----------+-----------------+--------------------------------------------------------------------------------------------------------------------------------------+
| Invert Mask       | maskInvert    | Boolean   | Off             | When checked, the effect is fully applied where the mask is 0.                                                                       |
+-------------------+---------------+-----------+-----------------+--------------------------------------------------------------------------------------------------------------------------------------+
| Mix               | mix           | Double    | 1               | Mix factor between the original and the transformed image.                                                                           |
+-------------------+---------------+-----------+-----------------+--------------------------------------------------------------------------------------------------------------------------------------+
