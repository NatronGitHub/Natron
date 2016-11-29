.. _net.sf.openfx.NoTimeBlurPlugin:

NoTimeBlurOFX
=============

*This documentation is for version 1.0 of NoTimeBlurOFX.*

Rounds fractional frame numbers to integers. This can be used to avoid computing non-integer frame numbers, and to discretize motion (useful for animated objects). This plug-in is usually inserted upstream from TimeBlur.

Inputs
------

+----------+---------------+------------+
| Input    | Description   | Optional   |
+==========+===============+============+
| Source   |               | No         |
+----------+---------------+------------+

Controls
--------

+-------------------+---------------+----------+-----------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Label (UI Name)   | Script-Name   | Type     | Default-Value   | Function                                                                                                                                                                                                                                          |
+===================+===============+==========+=================+===================================================================================================================================================================================================================================================+
| Rounding          | rounding      | Choice   | rint            | Rounding type/operation to use when blocking fractional frames.\ **rint**: Round to the nearest integer value.\ **floor**: Round dound to the nearest integer value.\ **ceil**: Round up to the nearest integer value.\ **none**: Do not round.   |
+-------------------+---------------+----------+-----------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
