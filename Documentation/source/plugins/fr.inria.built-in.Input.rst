.. _fr.inria.built-in.Input:

Input
=====

*This documentation is for version 1.0 of Input.*

This node can only be used within a Group. It adds an input arrow to the group.

Inputs
------

+---------+---------------+------------+
| Input   | Description   | Optional   |
+=========+===============+============+
+---------+---------------+------------+

Controls
--------

+-------------------+---------------+-----------+-----------------+------------------------------------------------------------------------------------------------------------------------------------+
| Label (UI Name)   | Script-Name   | Type      | Default-Value   | Function                                                                                                                           |
+===================+===============+===========+=================+====================================================================================================================================+
| Optional          | optional      | Boolean   | Off             | When checked, this input of the group will be optional, i.e it will not be required that it is connected for the render to work.   |
+-------------------+---------------+-----------+-----------------+------------------------------------------------------------------------------------------------------------------------------------+
| Mask              | isMask        | Boolean   | Off             | When checked, this input of the group will be considered as a mask. A mask is always optional.                                     |
+-------------------+---------------+-----------+-----------------+------------------------------------------------------------------------------------------------------------------------------------+
