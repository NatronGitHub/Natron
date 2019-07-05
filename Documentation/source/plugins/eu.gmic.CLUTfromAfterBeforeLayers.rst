.. _eu.gmic.CLUTfromAfterBeforeLayers:

G’MIC CLUT from After / Before Layers node
==========================================

*This documentation is for version 1.0 of G’MIC CLUT from After / Before Layers.*

Description
-----------

What is this filter for?

This filter requires at least two input layers to work properly.

It assumes you have an input top layer A and a base layer B such that A and B both represent the same image but with only color variations (typically A has been obtained from B using the color curves tool).

This filter is then able to estimate and outputs a color HaldCLUT H so that applying H on the base layer B gives back A.

This is useful when you have a color transformation between two images, that you want to recover and re-apply on a bunch of other images.

Author: David Tschumperle. Latest Update: 2018/07/25.

Wrapper for the G’MIC framework (http://gmic.eu) written by Tobias Fleischer (http://www.reduxfx.com) and Frederic Devernay.

Inputs
------

+----------+-------------+----------+
| Input    | Description | Optional |
+==========+=============+==========+
| Source   |             | No       |
+----------+-------------+----------+
| Layer -1 |             | Yes      |
+----------+-------------+----------+
| Layer -2 |             | Yes      |
+----------+-------------+----------+
| Layer -3 |             | Yes      |
+----------+-------------+----------+

Controls
--------

.. tabularcolumns:: |>{\raggedright}p{0.2\columnwidth}|>{\raggedright}p{0.06\columnwidth}|>{\raggedright}p{0.07\columnwidth}|p{0.63\columnwidth}|

.. cssclass:: longtable

+------------------------------------------------------------------+---------+-----------+-----------------------+
| Parameter / script name                                          | Type    | Default   | Function              |
+==================================================================+=========+===========+=======================+
| Output CLUT Resolution / ``Output_CLUT_Resolution``              | Choice  | 512 x 512 | |                     |
|                                                                  |         |           | | **512 x 512**       |
|                                                                  |         |           | | **4096 x 4096**     |
+------------------------------------------------------------------+---------+-----------+-----------------------+
| Influence of Color Samples (%) / ``Influence_of_Color_Samples_`` | Double  | 50        |                       |
+------------------------------------------------------------------+---------+-----------+-----------------------+
| Output Layer / ``Output_Layer``                                  | Choice  | Layer 0   | |                     |
|                                                                  |         |           | | **Merged**          |
|                                                                  |         |           | | **Layer 0**         |
|                                                                  |         |           | | **Layer -1**        |
|                                                                  |         |           | | **Layer -2**        |
|                                                                  |         |           | | **Layer -3**        |
|                                                                  |         |           | | **Layer -4**        |
|                                                                  |         |           | | **Layer -5**        |
|                                                                  |         |           | | **Layer -6**        |
|                                                                  |         |           | | **Layer -7**        |
|                                                                  |         |           | | **Layer -8**        |
|                                                                  |         |           | | **Layer -9**        |
+------------------------------------------------------------------+---------+-----------+-----------------------+
| Resize Mode / ``Resize_Mode``                                    | Choice  | Dynamic   | |                     |
|                                                                  |         |           | | **Fixed (Inplace)** |
|                                                                  |         |           | | **Dynamic**         |
|                                                                  |         |           | | **Downsample 1/2**  |
|                                                                  |         |           | | **Downsample 1/4**  |
|                                                                  |         |           | | **Downsample 1/8**  |
|                                                                  |         |           | | **Downsample 1/16** |
+------------------------------------------------------------------+---------+-----------+-----------------------+
| Ignore Alpha / ``Ignore_Alpha``                                  | Boolean | Off       |                       |
+------------------------------------------------------------------+---------+-----------+-----------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode``                       | Boolean | Off       |                       |
+------------------------------------------------------------------+---------+-----------+-----------------------+
| Log Verbosity / ``Log_Verbosity``                                | Choice  | Off       | |                     |
|                                                                  |         |           | | **Off**             |
|                                                                  |         |           | | **Level 1**         |
|                                                                  |         |           | | **Level 2**         |
|                                                                  |         |           | | **Level 3**         |
+------------------------------------------------------------------+---------+-----------+-----------------------+
