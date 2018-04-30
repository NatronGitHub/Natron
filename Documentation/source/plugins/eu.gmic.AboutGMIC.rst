.. _eu.gmic.AboutGMIC:

About G’MIC node
================

*This documentation is for version 0.3 of About G’MIC.*

Description
-----------

( GREYC’s Magic for Image Computing )is proposed to you by

David Tschumperle: http://tschumperle.users.greyc.fr/

Sebastien Fourey: https://foureys.users.greyc.fr/

( IMAGE Team / GREYC Laboratory - CNRS UMR 6072 ): http://www.greyc.fr/node/36

This plug-in is based on our open-source libraries G’MIC and CImg (C++ Template Image Processing Library),

available at:

http://gmic.eu/

and

http://cimg.eu/

If you appreciate G’MIC, you are welcome to send us a nice postcard from your place, at:

David Tschumperle, Laboratoire GREYC (CNRS UMR 6072), Equipe Image,

6 Bd du Marechal Juin, 14050 Caen Cedex / France.

Postcards senders automatically enter the Friends Hall of Fame :) !

Wrapper for the G’MIC framework (http://gmic.eu) written by Tobias Fleischer (http://www.reduxfx.com).

Inputs
------

+-------+-------------+----------+
| Input | Description | Optional |
+=======+=============+==========+
| Input |             | No       |
+-------+-------------+----------+

Controls
--------

.. tabularcolumns:: |>{\raggedright}p{0.2\columnwidth}|>{\raggedright}p{0.06\columnwidth}|>{\raggedright}p{0.07\columnwidth}|p{0.63\columnwidth}|

.. cssclass:: longtable

+-----------------------------------+---------+---------+-----------------------+
| Parameter / script name           | Type    | Default | Function              |
+===================================+=========+=========+=======================+
| Output Layer / ``Output_Layer``   | Choice  | Layer 0 | |                     |
|                                   |         |         | | **Merged**          |
|                                   |         |         | | **Layer 0**         |
|                                   |         |         | | **Layer 1**         |
|                                   |         |         | | **Layer 2**         |
|                                   |         |         | | **Layer 3**         |
|                                   |         |         | | **Layer 4**         |
|                                   |         |         | | **Layer 5**         |
|                                   |         |         | | **Layer 6**         |
|                                   |         |         | | **Layer 7**         |
|                                   |         |         | | **Layer 8**         |
|                                   |         |         | | **Layer 9**         |
+-----------------------------------+---------+---------+-----------------------+
| Resize Mode / ``Resize_Mode``     | Choice  | Dynamic | |                     |
|                                   |         |         | | **Fixed (Inplace)** |
|                                   |         |         | | **Dynamic**         |
|                                   |         |         | | **Downsample 1/2**  |
|                                   |         |         | | **Downsample 1/4**  |
|                                   |         |         | | **Downsample 1/8**  |
|                                   |         |         | | **Downsample 1/16** |
+-----------------------------------+---------+---------+-----------------------+
| Ignore Alpha / ``Ignore_Alpha``   | Boolean | Off     |                       |
+-----------------------------------+---------+---------+-----------------------+
| Log Verbosity / ``Log_Verbosity`` | Choice  | Off     | |                     |
|                                   |         |         | | **Off**             |
|                                   |         |         | | **Level 1**         |
|                                   |         |         | | **Level 2**         |
|                                   |         |         | | **Level 3**         |
+-----------------------------------+---------+---------+-----------------------+
