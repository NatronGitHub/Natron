.. _eu.gmic.Droste:

G’MIC Droste node
=================

*This documentation is for version 1.0 of G’MIC Droste.*

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

=========================================== ======= ======= ========================
Parameter / script name                     Type    Default Function
=========================================== ======= ======= ========================
X0 / ``X0``                                 Double  20       
Y0 / ``Y0``                                 Double  20       
X1 / ``X1``                                 Double  80       
Y1 / ``Y1``                                 Double  20       
X2 / ``X2``                                 Double  80       
Y2 / ``Y2``                                 Double  80       
X3 / ``X3``                                 Double  20       
Y3 / ``Y3``                                 Double  80       
Iterations / ``Iterations``                 Integer 1        
X-Shift / ``XShift``                        Double  0        
Y-Shift / ``YShift``                        Double  0        
Angle / ``Angle``                           Double  0        
Zoom / ``Zoom``                             Double  1        
Mirror / ``Mirror``                         Choice  None    .  
                                                            . **None**
                                                            . **X-Axis**
                                                            . **Y-Axis**
                                                            . **XY-Axes**
Boundary / ``Boundary``                     Choice  Nearest .  
                                                            . **Transparent**
                                                            . **Nearest**
                                                            . **Periodic**
                                                            . **Mirror**
Drawing Mode / ``Drawing_Mode``             Choice  Replace .  
                                                            . **Replace**
                                                            . **Replace (Sharpest)**
                                                            . **Behind**
                                                            . **Below**
View Outlines Only / ``View_Outlines_Only`` Boolean Off      
Output Layer / ``Output_Layer``             Choice  Layer 0 .  
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
Resize Mode / ``Resize_Mode``               Choice  Dynamic .  
                                                            . **Fixed (Inplace)**
                                                            . **Dynamic**
                                                            . **Downsample 1/2**
                                                            . **Downsample 1/4**
                                                            . **Downsample 1/8**
                                                            . **Downsample 1/16**
Ignore Alpha / ``Ignore_Alpha``             Boolean Off      
Preview/Draft Mode / ``PreviewDraft_Mode``  Boolean Off      
Log Verbosity / ``Log_Verbosity``           Choice  Off     .  
                                                            . **Off**
                                                            . **Level 1**
                                                            . **Level 2**
                                                            . **Level 3**
=========================================== ======= ======= ========================
