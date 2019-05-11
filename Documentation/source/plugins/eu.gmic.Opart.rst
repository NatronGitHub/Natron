.. _eu.gmic.Opart:

G’MIC Op art node
=================

*This documentation is for version 1.0 of G’MIC Op art.*

Description
-----------

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

+--------------------------------------------+---------+---------------+----------------------------+
| Parameter / script name                    | Type    | Default       | Function                   |
+============================================+=========+===============+============================+
| Shape / ``Shape``                          | Choice  | Circles       | |                          |
|                                            |         |               | | **Custom layers**        |
|                                            |         |               | | **Circles**              |
|                                            |         |               | | **Squares**              |
|                                            |         |               | | **Diamonds**             |
|                                            |         |               | | **Triangles**            |
|                                            |         |               | | **Horizontal stripes**   |
|                                            |         |               | | **Vertical stripes**     |
|                                            |         |               | | **Balls**                |
|                                            |         |               | | **Hearts**               |
|                                            |         |               | | **Stars**                |
|                                            |         |               | | **Arrows**               |
|                                            |         |               | | **Truchet**              |
|                                            |         |               | | **Circles (outline)**    |
|                                            |         |               | | **Squares (outline)**    |
|                                            |         |               | | **Diamonds (outline)**   |
|                                            |         |               | | **Triangles (outline)**  |
|                                            |         |               | | **Hearts (outline)**     |
|                                            |         |               | | **Stars (outline)**      |
|                                            |         |               | | **Arrows (outline)**     |
+--------------------------------------------+---------+---------------+----------------------------+
| Number of scales / ``Number_of_scales``    | Integer | 16            |                            |
+--------------------------------------------+---------+---------------+----------------------------+
| Resolution / ``Resolution``                | Double  | 10            |                            |
+--------------------------------------------+---------+---------------+----------------------------+
| Zoom factor / ``Zoom_factor``              | Integer | 2             |                            |
+--------------------------------------------+---------+---------------+----------------------------+
| Minimal size / ``Minimal_size``            | Double  | 5             |                            |
+--------------------------------------------+---------+---------------+----------------------------+
| Maximal size / ``Maximal_size``            | Double  | 90            |                            |
+--------------------------------------------+---------+---------------+----------------------------+
| Stencil type / ``Stencil_type``            | Choice  | Black & white | |                          |
|                                            |         |               | | **Black & white**        |
|                                            |         |               | | **RGB**                  |
|                                            |         |               | | **Color**                |
+--------------------------------------------+---------+---------------+----------------------------+
| Allow angle / ``Allow_angle``              | Choice  | 0 deg.        | |                          |
|                                            |         |               | | **0 deg.**               |
|                                            |         |               | | **90 deg.**              |
|                                            |         |               | | **180 deg.**             |
+--------------------------------------------+---------+---------------+----------------------------+
| Negative / ``Negative``                    | Boolean | On            |                            |
+--------------------------------------------+---------+---------------+----------------------------+
| Antialiasing / ``Antialiasing``            | Boolean | On            |                            |
+--------------------------------------------+---------+---------------+----------------------------+
| Preview type / ``Preview_type``            | Choice  | Full          | |                          |
|                                            |         |               | | **Full**                 |
|                                            |         |               | | **Forward horizontal**   |
|                                            |         |               | | **Forward vertical**     |
|                                            |         |               | | **Backward horizontal**  |
|                                            |         |               | | **Backward vertical**    |
|                                            |         |               | | **Duplicate top**        |
|                                            |         |               | | **Duplicate left**       |
|                                            |         |               | | **Duplicate bottom**     |
|                                            |         |               | | **Duplicate right**      |
|                                            |         |               | | **Duplicate horizontal** |
|                                            |         |               | | **Duplicate vertical**   |
|                                            |         |               | | **Checkered**            |
|                                            |         |               | | **Checkered inverse**    |
+--------------------------------------------+---------+---------------+----------------------------+
| Output Layer / ``Output_Layer``            | Choice  | Layer 0       | |                          |
|                                            |         |               | | **Merged**               |
|                                            |         |               | | **Layer 0**              |
|                                            |         |               | | **Layer -1**             |
|                                            |         |               | | **Layer -2**             |
|                                            |         |               | | **Layer -3**             |
|                                            |         |               | | **Layer -4**             |
|                                            |         |               | | **Layer -5**             |
|                                            |         |               | | **Layer -6**             |
|                                            |         |               | | **Layer -7**             |
|                                            |         |               | | **Layer -8**             |
|                                            |         |               | | **Layer -9**             |
+--------------------------------------------+---------+---------------+----------------------------+
| Resize Mode / ``Resize_Mode``              | Choice  | Dynamic       | |                          |
|                                            |         |               | | **Fixed (Inplace)**      |
|                                            |         |               | | **Dynamic**              |
|                                            |         |               | | **Downsample 1/2**       |
|                                            |         |               | | **Downsample 1/4**       |
|                                            |         |               | | **Downsample 1/8**       |
|                                            |         |               | | **Downsample 1/16**      |
+--------------------------------------------+---------+---------------+----------------------------+
| Ignore Alpha / ``Ignore_Alpha``            | Boolean | Off           |                            |
+--------------------------------------------+---------+---------------+----------------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode`` | Boolean | Off           |                            |
+--------------------------------------------+---------+---------------+----------------------------+
| Log Verbosity / ``Log_Verbosity``          | Choice  | Off           | |                          |
|                                            |         |               | | **Off**                  |
|                                            |         |               | | **Level 1**              |
|                                            |         |               | | **Level 2**              |
|                                            |         |               | | **Level 3**              |
+--------------------------------------------+---------+---------------+----------------------------+
