.. _eu.gmic.Montage:

G’MIC Montage node
==================

*This documentation is for version 1.0 of G’MIC Montage.*

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

=========================================== ======= ===================== ======================
Parameter / script name                     Type    Default               Function
=========================================== ======= ===================== ======================
Montage Type / ``Montage_Type``             Choice  Auto                  .  
                                                                          . **Auto**
                                                                          . **Custom Layout**
                                                                          . **Horizontal**
                                                                          . **Vertical**
                                                                          . **Horizontal Array**
                                                                          . **Vertical Array**
Custom Layout / ``Custom_Layout``           String  V(H(0,1),H(2,V(3,4)))  
Merging Mode / ``Merging_Mode``             Choice  Scaled                .  
                                                                          . **Aligned**
                                                                          . **Scaled**
Centering / Scale / ``Centering__Scale``    Double  0.5                    
Padding (px) / ``Padding_px``               Integer 0                      
Frame (px) / ``Frame_px``                   Integer 0                      
Frame Color / ``Frame_Color``               Color   r: 0 g: 0 b: 0 a: 0    
Angle / ``Angle``                           Double  0                      
Angle Variations / ``Angle_Variations``     Double  0                      
Cycle Layers / ``Cycle_Layers``             Integer 0                      
Revert Layer Order / ``Revert_Layer_Order`` Boolean Off                    
Output As / ``Output_As``                   Choice  Single Layer          .  
                                                                          . **Single Layer**
                                                                          . **Multiple Layers**
Output Layer / ``Output_Layer``             Choice  Layer 0               .  
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
Resize Mode / ``Resize_Mode``               Choice  Dynamic               .  
                                                                          . **Fixed (Inplace)**
                                                                          . **Dynamic**
                                                                          . **Downsample 1/2**
                                                                          . **Downsample 1/4**
                                                                          . **Downsample 1/8**
                                                                          . **Downsample 1/16**
Ignore Alpha / ``Ignore_Alpha``             Boolean Off                    
Preview/Draft Mode / ``PreviewDraft_Mode``  Boolean Off                    
Log Verbosity / ``Log_Verbosity``           Choice  Off                   .  
                                                                          . **Off**
                                                                          . **Level 1**
                                                                          . **Level 2**
                                                                          . **Level 3**
=========================================== ======= ===================== ======================
