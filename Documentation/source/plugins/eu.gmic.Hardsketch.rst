.. _eu.gmic.Hardsketch:

G’MIC Hard sketch node
======================

*This documentation is for version 0.3 of G’MIC Hard sketch.*

Description
-----------

Note: This filter uses lot of random values to generate its result, so running it twice will give you different results !

Click here for a detailed description of this filter.: http://www.gimpchat.com/viewtopic.php?f=28&t=10036

Author: David Tschumperle. Latest update: 2014/25/04.

Author: David Tschumperle. Latest update: 2010/29/12.

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

+---------------------------------------------+---------+----------------+----------------------------------+
| Parameter / script name                     | Type    | Default        | Function                         |
+=============================================+=========+================+==================================+
| Detail level / ``Detail_level``             | Double  | 0.8            |                                  |
+---------------------------------------------+---------+----------------+----------------------------------+
| Amplitude / ``Amplitude``                   | Double  | 300            |                                  |
+---------------------------------------------+---------+----------------+----------------------------------+
| Density / ``Density``                       | Double  | 50             |                                  |
+---------------------------------------------+---------+----------------+----------------------------------+
| Smoothness / ``Smoothness``                 | Double  | 1              |                                  |
+---------------------------------------------+---------+----------------+----------------------------------+
| Opacity / ``Opacity``                       | Double  | 0.1            |                                  |
+---------------------------------------------+---------+----------------+----------------------------------+
| Edge / ``Edge``                             | Double  | 20             |                                  |
+---------------------------------------------+---------+----------------+----------------------------------+
| Fast approximation / ``Fast_approximation`` | Boolean | Off            |                                  |
+---------------------------------------------+---------+----------------+----------------------------------+
| Color model / ``Color_model``               | Choice  | Color on white | |                                |
|                                             |         |                | | **Black on white**             |
|                                             |         |                | | **White on black**             |
|                                             |         |                | | **Black on transparent white** |
|                                             |         |                | | **White on transparent black** |
|                                             |         |                | | **Color on white**             |
+---------------------------------------------+---------+----------------+----------------------------------+
| Preview type / ``Preview_type``             | Choice  | Full           | |                                |
|                                             |         |                | | **Full**                       |
|                                             |         |                | | **Forward horizontal**         |
|                                             |         |                | | **Forward vertical**           |
|                                             |         |                | | **Backward horizontal**        |
|                                             |         |                | | **Backward vertical**          |
|                                             |         |                | | **Duplicate top**              |
|                                             |         |                | | **Duplicate left**             |
|                                             |         |                | | **Duplicate bottom**           |
|                                             |         |                | | **Duplicate right**            |
|                                             |         |                | | **Duplicate horizontal**       |
|                                             |         |                | | **Duplicate vertical**         |
|                                             |         |                | | **Checkered**                  |
|                                             |         |                | | **Checkered inverse)**         |
|                                             |         |                | | **Preview split = point(50**   |
|                                             |         |                | | **50**                         |
|                                             |         |                | | **0**                          |
|                                             |         |                | | **0**                          |
|                                             |         |                | | **200**                        |
|                                             |         |                | | **200**                        |
|                                             |         |                | | **200**                        |
|                                             |         |                | | **0**                          |
|                                             |         |                | | **10**                         |
|                                             |         |                | | **0**                          |
+---------------------------------------------+---------+----------------+----------------------------------+
| Output Layer / ``Output_Layer``             | Choice  | Layer 0        | |                                |
|                                             |         |                | | **Merged**                     |
|                                             |         |                | | **Layer 0**                    |
|                                             |         |                | | **Layer 1**                    |
|                                             |         |                | | **Layer 2**                    |
|                                             |         |                | | **Layer 3**                    |
|                                             |         |                | | **Layer 4**                    |
|                                             |         |                | | **Layer 5**                    |
|                                             |         |                | | **Layer 6**                    |
|                                             |         |                | | **Layer 7**                    |
|                                             |         |                | | **Layer 8**                    |
|                                             |         |                | | **Layer 9**                    |
+---------------------------------------------+---------+----------------+----------------------------------+
| Resize Mode / ``Resize_Mode``               | Choice  | Dynamic        | |                                |
|                                             |         |                | | **Fixed (Inplace)**            |
|                                             |         |                | | **Dynamic**                    |
|                                             |         |                | | **Downsample 1/2**             |
|                                             |         |                | | **Downsample 1/4**             |
|                                             |         |                | | **Downsample 1/8**             |
|                                             |         |                | | **Downsample 1/16**            |
+---------------------------------------------+---------+----------------+----------------------------------+
| Ignore Alpha / ``Ignore_Alpha``             | Boolean | Off            |                                  |
+---------------------------------------------+---------+----------------+----------------------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode``  | Boolean | Off            |                                  |
+---------------------------------------------+---------+----------------+----------------------------------+
| Log Verbosity / ``Log_Verbosity``           | Choice  | Off            | |                                |
|                                             |         |                | | **Off**                        |
|                                             |         |                | | **Level 1**                    |
|                                             |         |                | | **Level 2**                    |
|                                             |         |                | | **Level 3**                    |
+---------------------------------------------+---------+----------------+----------------------------------+
