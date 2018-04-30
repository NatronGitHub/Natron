.. _eu.gmic.Colorizelineartpropagation:

G’MIC Colorize lineart propagation node
=======================================

*This documentation is for version 0.3 of G’MIC Colorize lineart propagation.*

Description
-----------

Layers ordering:

Note: You probably need to select All for the Input layers option on the left.

Color spots = your layer with color indications.

Lineart = your layer with line-art (b&w or transparent).

Extrapolated colors = the G’MIC generated layer with flat colors.

Warnings:

- Do not rely too much on the preview, it is probably not accurate !

- Activate option Extrapolate color as one layer per single color/region only if you have a lot of available memory !

Click here for a detailed description of this filter.: http://www.gimpchat.com/viewtopic.php?f=28&t=7567

Authors: David Tschumperle, Timothee Giet and David Revoy. Latest update: 2013/19/06.

Wrapper for the G’MIC framework (http://gmic.eu) written by Tobias Fleischer (http://www.reduxfx.com).

Inputs
------

+-----------+-------------+----------+
| Input     | Description | Optional |
+===========+=============+==========+
| Input     |             | No       |
+-----------+-------------+----------+
| Ext. In 1 |             | Yes      |
+-----------+-------------+----------+
| Ext. In 2 |             | Yes      |
+-----------+-------------+----------+
| Ext. In 3 |             | Yes      |
+-----------+-------------+----------+

Controls
--------

.. tabularcolumns:: |>{\raggedright}p{0.2\columnwidth}|>{\raggedright}p{0.06\columnwidth}|>{\raggedright}p{0.07\columnwidth}|p{0.63\columnwidth}|

.. cssclass:: longtable

+---------------------------------------------------+---------+-------------------------------+---------------------------------------------------+
| Parameter / script name                           | Type    | Default                       | Function                                          |
+===================================================+=========+===============================+===================================================+
| Input layers / ``Input_layers``                   | Choice  | Color spots + lineart         | |                                                 |
|                                                   |         |                               | | **Color spots + lineart**                       |
|                                                   |         |                               | | **Lineart + color spots**                       |
|                                                   |         |                               | | **Color spots + extrapolated colors + lineart** |
|                                                   |         |                               | | **Lineart + color spots + extrapolated colors** |
+---------------------------------------------------+---------+-------------------------------+---------------------------------------------------+
| Output layers / ``Output_layers``                 | Choice  | Extrapolated colors + lineart | |                                                 |
|                                                   |         |                               | | **Single (merged)**                             |
|                                                   |         |                               | | **Extrapolated colors + lineart**               |
|                                                   |         |                               | | **Lineart + extrapolated colors**               |
|                                                   |         |                               | | **Color spots + extrapolated colors + lineart** |
|                                                   |         |                               | | **Lineart + color spots + extrapolated colors** |
+---------------------------------------------------+---------+-------------------------------+---------------------------------------------------+
| Extrapolate colors as / ``Extrapolate_colors_as`` | Choice  | One layer                     | |                                                 |
|                                                   |         |                               | | **One layer**                                   |
|                                                   |         |                               | | **Two layers**                                  |
|                                                   |         |                               | | **Three layers**                                |
|                                                   |         |                               | | **Four layers**                                 |
|                                                   |         |                               | | **Five layers**                                 |
|                                                   |         |                               | | **Six layers**                                  |
|                                                   |         |                               | | **Seven layers**                                |
|                                                   |         |                               | | **Eight layers**                                |
|                                                   |         |                               | | **Nine layers**                                 |
|                                                   |         |                               | | **Ten layers**                                  |
|                                                   |         |                               | | **One layer per single color**                  |
|                                                   |         |                               | | **One layer per single region**                 |
+---------------------------------------------------+---------+-------------------------------+---------------------------------------------------+
| Smoothness / ``Smoothness``                       | Double  | 0.05                          |                                                   |
+---------------------------------------------------+---------+-------------------------------+---------------------------------------------------+
| Output Layer / ``Output_Layer``                   | Choice  | Layer 0                       | |                                                 |
|                                                   |         |                               | | **Merged**                                      |
|                                                   |         |                               | | **Layer 0**                                     |
|                                                   |         |                               | | **Layer 1**                                     |
|                                                   |         |                               | | **Layer 2**                                     |
|                                                   |         |                               | | **Layer 3**                                     |
|                                                   |         |                               | | **Layer 4**                                     |
|                                                   |         |                               | | **Layer 5**                                     |
|                                                   |         |                               | | **Layer 6**                                     |
|                                                   |         |                               | | **Layer 7**                                     |
|                                                   |         |                               | | **Layer 8**                                     |
|                                                   |         |                               | | **Layer 9**                                     |
+---------------------------------------------------+---------+-------------------------------+---------------------------------------------------+
| Resize Mode / ``Resize_Mode``                     | Choice  | Dynamic                       | |                                                 |
|                                                   |         |                               | | **Fixed (Inplace)**                             |
|                                                   |         |                               | | **Dynamic**                                     |
|                                                   |         |                               | | **Downsample 1/2**                              |
|                                                   |         |                               | | **Downsample 1/4**                              |
|                                                   |         |                               | | **Downsample 1/8**                              |
|                                                   |         |                               | | **Downsample 1/16**                             |
+---------------------------------------------------+---------+-------------------------------+---------------------------------------------------+
| Ignore Alpha / ``Ignore_Alpha``                   | Boolean | Off                           |                                                   |
+---------------------------------------------------+---------+-------------------------------+---------------------------------------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode``        | Boolean | Off                           |                                                   |
+---------------------------------------------------+---------+-------------------------------+---------------------------------------------------+
| Log Verbosity / ``Log_Verbosity``                 | Choice  | Off                           | |                                                 |
|                                                   |         |                               | | **Off**                                         |
|                                                   |         |                               | | **Level 1**                                     |
|                                                   |         |                               | | **Level 2**                                     |
|                                                   |         |                               | | **Level 3**                                     |
+---------------------------------------------------+---------+-------------------------------+---------------------------------------------------+
