.. _eu.gmic.ArrayRandomColors:

G’MIC Array Random Colors node
==============================

*This documentation is for version 1.0 of G’MIC Array Random Colors.*

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

============================================ ======= ======= =====================
Parameter / script name                      Type    Default Function
============================================ ======= ======= =====================
Source X-Tiles / ``Source_XTiles``           Integer 5        
Source Y-Tiles / ``Source_YTiles``           Integer 5        
Destination X-Tiles / ``Destination_XTiles`` Integer 7        
Destination Y-Tiles / ``Destination_YTiles`` Integer 7        
X-Tiles / ``XTiles``                         Integer 5        
Y-Tiles / ``YTiles``                         Integer 5        
Opacity / ``Opacity``                        Double  0.5      
Output Layer / ``Output_Layer``              Choice  Layer 0 .  
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
Resize Mode / ``Resize_Mode``                Choice  Dynamic .  
                                                             . **Fixed (Inplace)**
                                                             . **Dynamic**
                                                             . **Downsample 1/2**
                                                             . **Downsample 1/4**
                                                             . **Downsample 1/8**
                                                             . **Downsample 1/16**
Ignore Alpha / ``Ignore_Alpha``              Boolean Off      
Log Verbosity / ``Log_Verbosity``            Choice  Off     .  
                                                             . **Off**
                                                             . **Level 1**
                                                             . **Level 2**
                                                             . **Level 3**
============================================ ======= ======= =====================
