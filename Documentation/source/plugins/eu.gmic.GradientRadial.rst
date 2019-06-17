.. _eu.gmic.GradientRadial:

G’MIC Gradient Radial node
==========================

*This documentation is for version 1.0 of G’MIC Gradient Radial.*

Description
-----------

Wrapper for the G’MIC framework (http://gmic.eu) written by Tobias Fleischer (http://www.reduxfx.com) and Frederic Devernay.

Inputs
------

====== =========== ========
Input  Description Optional
====== =========== ========
Source             No
====== =========== ========

Controls
--------

.. tabularcolumns:: |>{\raggedright}p{0.2\columnwidth}|>{\raggedright}p{0.06\columnwidth}|>{\raggedright}p{0.07\columnwidth}|p{0.63\columnwidth}|

.. cssclass:: longtable

=================================== ======= =================== =====================
Parameter / script name             Type    Default             Function
=================================== ======= =================== =====================
Starting Color / ``Starting_Color`` Color   r: 0 g: 0 b: 0 a: 0  
Ending Color / ``Ending_Color``     Color   r: 1 g: 1 b: 1 a: 1  
Swap Colors / ``Swap_Colors``       Boolean Off                  
Fade Start / ``Fade_Start``         Double  0                    
Fade End / ``Fade_End``             Double  100                  
Center (%) / ``Center_``            Double  x: 0.5 y: 0.5        
Colorspace / ``Colorspace``         Choice  sRGB                .  
                                                                . **sRGB**
                                                                . **Linear RGB**
                                                                . **Lab**
Output Layer / ``Output_Layer``     Choice  Layer 0             .  
                                                                . **Merged**
                                                                . **Layer 0**
                                                                . **Layer -1**
                                                                . **Layer -2**
                                                                . **Layer -3**
                                                                . **Layer -4**
                                                                . **Layer -5**
                                                                . **Layer -6**
                                                                . **Layer -7**
                                                                . **Layer -8**
                                                                . **Layer -9**
Resize Mode / ``Resize_Mode``       Choice  Dynamic             .  
                                                                . **Fixed (Inplace)**
                                                                . **Dynamic**
                                                                . **Downsample 1/2**
                                                                . **Downsample 1/4**
                                                                . **Downsample 1/8**
                                                                . **Downsample 1/16**
Ignore Alpha / ``Ignore_Alpha``     Boolean Off                  
Log Verbosity / ``Log_Verbosity``   Choice  Off                 .  
                                                                . **Off**
                                                                . **Level 1**
                                                                . **Level 2**
                                                                . **Level 3**
=================================== ======= =================== =====================
