.. _net.sf.openfx.DifferencePlugin:

DifferenceOFX
=============

.. figure:: net.sf.openfx.DifferencePlugin.png
   :alt: 

*This documentation is for version 1.0 of DifferenceOFX.*

Produce a rough matte from the difference of two input images.

A is the background without the subject (clean plate). B is the subject with the background. RGB is copied from B, the difference is output to alpha, after applying offset and gain.

See also: http://opticalenquiry.com/nuke/index.php?title=The\_Keyer\_Nodes#Difference and http://opticalenquiry.com/nuke/index.php?title=Keying\_Tips

Inputs
------

+---------+-------------------------------------------------------+------------+
| Input   | Description                                           | Optional   |
+=========+=======================================================+============+
| B       | The subject with the background.                      | No         |
+---------+-------------------------------------------------------+------------+
| A       | The background without the subject (a clean plate).   | No         |
+---------+-------------------------------------------------------+------------+

Controls
--------

+-------------------+---------------+----------+-----------------+---------------------------------------------------+
| Label (UI Name)   | Script-Name   | Type     | Default-Value   | Function                                          |
+===================+===============+==========+=================+===================================================+
| Offset            | offset        | Double   | 0               | Value subtracted to each pixel of the output      |
+-------------------+---------------+----------+-----------------+---------------------------------------------------+
| Gain              | gain          | Double   | 1               | Multiply each pixel of the output by this value   |
+-------------------+---------------+----------+-----------------+---------------------------------------------------+
