.. _eu.gmic.3DTiles:

G’MIC 3D Tiles node
===================

*This documentation is for version 1.0 of G’MIC 3D Tiles.*

Description
-----------

Wrapper for the G’MIC framework (http://gmic.eu) written by Tobias Fleischer (http://www.reduxfx.com) and Frederic Devernay.

Inputs
------

======== =========== ========
Input    Description Optional
======== =========== ========
Source               No
Layer -1             Yes
Layer -2             Yes
Layer -3             Yes
======== =========== ========

Controls
--------

.. tabularcolumns:: |>{\raggedright}p{0.2\columnwidth}|>{\raggedright}p{0.06\columnwidth}|>{\raggedright}p{0.07\columnwidth}|p{0.63\columnwidth}|

.. cssclass:: longtable

============================================= ======= ======= =====================
Parameter / script name                       Type    Default Function
============================================= ======= ======= =====================
Inter-Frames / ``InterFrames``                Integer 10       
X-Tiles / ``XTiles``                          Integer 8        
Y-Tiles / ``YTiles``                          Integer 8        
X-Rotation / ``XRotation``                    String  1        
Y-Rotation / ``YRotation``                    String  1        
Z-Rotation / ``ZRotation``                    String  0        
Focale / ``Focale``                           Double  800      
Enable Antialiasing / ``Enable_Antialiasing`` Boolean On       
Output Layer / ``Output_Layer``               Choice  Layer 0 .  
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
Resize Mode / ``Resize_Mode``                 Choice  Dynamic .  
                                                              . **Fixed (Inplace)**
                                                              . **Dynamic**
                                                              . **Downsample 1/2**
                                                              . **Downsample 1/4**
                                                              . **Downsample 1/8**
                                                              . **Downsample 1/16**
Ignore Alpha / ``Ignore_Alpha``               Boolean Off      
Preview/Draft Mode / ``PreviewDraft_Mode``    Boolean Off      
Log Verbosity / ``Log_Verbosity``             Choice  Off     .  
                                                              . **Off**
                                                              . **Level 1**
                                                              . **Level 2**
                                                              . **Level 3**
============================================= ======= ======= =====================
