.. _net.sf.openfx.timeOffset:

TimeOffsetOFX
=============

.. figure:: net.sf.openfx.timeOffset.png
   :alt: 

*This documentation is for version 1.0 of TimeOffsetOFX.*

Move the input clip forward or backward in time. This can also reverse the order of the input frames so that last one is first.

See also http://opticalenquiry.com/nuke/index.php?title=TimeOffset

Inputs
------

+----------+---------------+------------+
| Input    | Description   | Optional   |
+==========+===============+============+
| Source   |               | No         |
+----------+---------------+------------+

Controls
--------

+------------------------+--------------------+-----------+-----------------+-------------------------------------------------------------------+
| Label (UI Name)        | Script-Name        | Type      | Default-Value   | Function                                                          |
+========================+====================+===========+=================+===================================================================+
| Time Offset (Frames)   | timeOffset         | Integer   | 0               | Offset in frames (frame f from the input will be at f+offset)     |
+------------------------+--------------------+-----------+-----------------+-------------------------------------------------------------------+
| Reverse Input          | reverseInput       | Boolean   | Off             | Reverse the order of the input frames so that last one is first   |
+------------------------+--------------------+-----------+-----------------+-------------------------------------------------------------------+
| Clip to Input Range    | clipToInputRange   | Boolean   | Off             | Never ask for frames outside of the input frame range.            |
+------------------------+--------------------+-----------+-----------------+-------------------------------------------------------------------+
