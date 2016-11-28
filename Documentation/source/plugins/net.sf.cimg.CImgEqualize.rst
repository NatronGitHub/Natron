.. _net.sf.cimg.CImgEqualize:

EqualizeCImg
============

.. figure:: net.sf.cimg.CImgEqualize.png
   :alt: 

*This documentation is for version 2.0 of EqualizeCImg.*

Equalize histogram of pixel values.

To equalize image brightness only, use the HistEQCImg plugin.

Uses the 'equalize' function from the CImg library.

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
| NbLevels          | nb\_levels    | Integer   | 4096            | Number of histogram levels used for the equalization.                                                                                |
+-------------------+---------------+-----------+-----------------+--------------------------------------------------------------------------------------------------------------------------------------+
| Min Value         | min\_value    | Double    | 0               | Minimum pixel value considered for the histogram computation. All pixel values lower than min\_value will not be counted.            |
+-------------------+---------------+-----------+-----------------+--------------------------------------------------------------------------------------------------------------------------------------+
| Max Value         | max\_value    | Double    | 1               | Maximum pixel value considered for the histogram computation. All pixel values higher than max\_value will not be counted.           |
+-------------------+---------------+-----------+-----------------+--------------------------------------------------------------------------------------------------------------------------------------+
| (Un)premult       | premult       | Boolean   | Off             | Divide the image by the alpha channel before processing, and re-multiply it afterwards. Use if the input images are premultiplied.   |
+-------------------+---------------+-----------+-----------------+--------------------------------------------------------------------------------------------------------------------------------------+
| Invert Mask       | maskInvert    | Boolean   | Off             | When checked, the effect is fully applied where the mask is 0.                                                                       |
+-------------------+---------------+-----------+-----------------+--------------------------------------------------------------------------------------------------------------------------------------+
| Mix               | mix           | Double    | 1               | Mix factor between the original and the transformed image.                                                                           |
+-------------------+---------------+-----------+-----------------+--------------------------------------------------------------------------------------------------------------------------------------+
