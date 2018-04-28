.. _eu.gmic.Gamesdemos:

G’MIC Games & demos node
========================

*This documentation is for version 0.3 of G’MIC Games & demos.*

Description
-----------

Note: This filter proposes a showcase of some interactive demos, all written as G’MIC scripts.

On most demos, you can use the keyboard shortcut CTRL+D to double the window size (and CTRL+C to go back to the original size).

Also, feel free to use the mouse buttons, as they are often used to perform an action.

Author: David Tschumperle. Latest update: 2014/10/09.

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

+--------------------------------------------+---------+---------+---------------------------+
| Parameter / script name                    | Type    | Default | Function                  |
+============================================+=========+=========+===========================+
| Selection / ``Selection``                  | Choice  | 2048    | |                         |
|                                            |         |         | | **2048**                |
|                                            |         |         | | **Blobs editor**        |
|                                            |         |         | | **Bouncing balls**      |
|                                            |         |         | | **Connect-4**           |
|                                            |         |         | | **Fire effect**         |
|                                            |         |         | | **Fireworks**           |
|                                            |         |         | | **Fish-eye effect**     |
|                                            |         |         | | **Fourier filtering**   |
|                                            |         |         | | **Hanoi tower**         |
|                                            |         |         | | **Histogram**           |
|                                            |         |         | | **Hough transform**     |
|                                            |         |         | | **Jawbreaker**          |
|                                            |         |         | | **Virtual landscape**   |
|                                            |         |         | | **The game of life**    |
|                                            |         |         | | **Light effect**        |
|                                            |         |         | | **Mandelbrot explorer** |
|                                            |         |         | | **3d metaballs**        |
|                                            |         |         | | **Minesweeper**         |
|                                            |         |         | | **Minimal path**        |
|                                            |         |         | | **Pacman**              |
|                                            |         |         | | **Paint**               |
|                                            |         |         | | **Plasma effect**       |
|                                            |         |         | | **RGB quantization**    |
|                                            |         |         | | **3d reflection**       |
|                                            |         |         | | **3d rubber object**    |
|                                            |         |         | | **Shadebobs**           |
|                                            |         |         | | **Spline editor**       |
|                                            |         |         | | **3d starfield**        |
|                                            |         |         | | **Tetris**              |
|                                            |         |         | | **Tic-tac-toe**         |
|                                            |         |         | | **3d waves**            |
|                                            |         |         | | **Fractal whirl**       |
+--------------------------------------------+---------+---------+---------------------------+
| Output Layer / ``Output_Layer``            | Choice  | Layer 0 | |                         |
|                                            |         |         | | **Merged**              |
|                                            |         |         | | **Layer 0**             |
|                                            |         |         | | **Layer 1**             |
|                                            |         |         | | **Layer 2**             |
|                                            |         |         | | **Layer 3**             |
|                                            |         |         | | **Layer 4**             |
|                                            |         |         | | **Layer 5**             |
|                                            |         |         | | **Layer 6**             |
|                                            |         |         | | **Layer 7**             |
|                                            |         |         | | **Layer 8**             |
|                                            |         |         | | **Layer 9**             |
+--------------------------------------------+---------+---------+---------------------------+
| Resize Mode / ``Resize_Mode``              | Choice  | Dynamic | |                         |
|                                            |         |         | | **Fixed (Inplace)**     |
|                                            |         |         | | **Dynamic**             |
|                                            |         |         | | **Downsample 1/2**      |
|                                            |         |         | | **Downsample 1/4**      |
|                                            |         |         | | **Downsample 1/8**      |
|                                            |         |         | | **Downsample 1/16**     |
+--------------------------------------------+---------+---------+---------------------------+
| Ignore Alpha / ``Ignore_Alpha``            | Boolean | Off     |                           |
+--------------------------------------------+---------+---------+---------------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode`` | Boolean | Off     |                           |
+--------------------------------------------+---------+---------+---------------------------+
| Log Verbosity / ``Log_Verbosity``          | Choice  | Off     | |                         |
|                                            |         |         | | **Off**                 |
|                                            |         |         | | **Level 1**             |
|                                            |         |         | | **Level 2**             |
|                                            |         |         | | **Level 3**             |
+--------------------------------------------+---------+---------+---------------------------+
