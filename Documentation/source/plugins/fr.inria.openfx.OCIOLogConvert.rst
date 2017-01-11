.. _fr.inria.openfx.OCIOLogConvert:

OCIOLogConvert node
===================

|pluginIcon| 

*This documentation is for version 1.0 of OCIOLogConvert.*

Use OpenColorIO to convert from SCENE\_LINEAR to COMPOSITING\_LOG (or back).

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

+-----------------------------------------+-----------+--------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Parameter / script name                 | Type      | Default      | Function                                                                                                                                                                             |
+=========================================+===========+==============+======================================================================================================================================================================================+
| OCIO Config File / ``ocioConfigFile``   | N/A       |              | OpenColorIO configuration file                                                                                                                                                       |
+-----------------------------------------+-----------+--------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| OCIO config help... / ``ocioHelp``      | Button    |              | Help about the OpenColorIO configuration.                                                                                                                                            |
+-----------------------------------------+-----------+--------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Operation / ``operation``               | Choice    | Log to Lin   | Operation to perform. Lin is the SCENE\_LINEAR profile and Log is the COMPOSITING\_LOG profile of the OCIO configuration.                                                            |
+-----------------------------------------+-----------+--------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Enable GPU Render / ``enableGPU``       | Boolean   | On           | | Enable GPU-based OpenGL render.                                                                                                                                                    |
|                                         |           |              | | If the checkbox is checked but is not enabled (i.e. it cannot be unchecked), GPU render can not be enabled or disabled from the plugin and is probably part of the host options.   |
|                                         |           |              | | If the checkbox is not checked and is not enabled (i.e. it cannot be checked), GPU render is not available on this host.                                                           |
+-----------------------------------------+-----------+--------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| (Un)premult / ``premult``               | Boolean   | Off          | Divide the image by the alpha channel before processing, and re-multiply it afterwards. Use if the input images are premultiplied.                                                   |
+-----------------------------------------+-----------+--------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Invert Mask / ``maskInvert``            | Boolean   | Off          | When checked, the effect is fully applied where the mask is 0.                                                                                                                       |
+-----------------------------------------+-----------+--------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| Mix / ``mix``                           | Double    | 1            | Mix factor between the original and the transformed image.                                                                                                                           |
+-----------------------------------------+-----------+--------------+--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+

.. |pluginIcon| image:: fr.inria.openfx.OCIOLogConvert.png
   :width: 10.0%
