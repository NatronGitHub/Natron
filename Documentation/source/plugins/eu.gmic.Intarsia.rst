.. _eu.gmic.Intarsia:

G’MIC Intarsia node
===================

*This documentation is for version 0.3 of G’MIC Intarsia.*

Description
-----------

Note:

Intarsia is a method of Crochet/Knitting with a number of colours, in which a separate ball of yarn

is used for each area of colour.

This filter creates a HTML version of a graph chart which is solely used for this purpose

Author: David Tschumperle. Latest update: 2015/09/07.

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

+---------------------------------------------------------------------+---------+---------------+------------------------+
| Parameter / script name                                             | Type    | Default       | Function               |
+=====================================================================+=========+===============+========================+
| Output directory / ``Output_directory``                             | N/A     |               |                        |
+---------------------------------------------------------------------+---------+---------------+------------------------+
| Output HTML file / ``Output_HTML_file``                             | String  | intarsia.html |                        |
+---------------------------------------------------------------------+---------+---------------+------------------------+
| Maximum image size / ``Maximum_image_size``                         | Integer | 512           |                        |
+---------------------------------------------------------------------+---------+---------------+------------------------+
| Maximum number of image colors / ``Maximum_number_of_image_colors`` | Integer | 12            |                        |
+---------------------------------------------------------------------+---------+---------------+------------------------+
| Starting point / ``Starting_point``                                 | Choice  | Top right     | |                      |
|                                                                     |         |               | | **Top left**         |
|                                                                     |         |               | | **Top right**        |
|                                                                     |         |               | | **Bottom left**      |
|                                                                     |         |               | | **Bottom right**     |
+---------------------------------------------------------------------+---------+---------------+------------------------+
| Loop method / ``Loop_method``                                       | Choice  | Row by row    | |                      |
|                                                                     |         |               | | **Row by row**       |
|                                                                     |         |               | | **Column by column** |
+---------------------------------------------------------------------+---------+---------------+------------------------+
| Add comment area in HTML page / ``Add_comment_area_in_HTML_page``   | Boolean | On            |                        |
+---------------------------------------------------------------------+---------+---------------+------------------------+
| Preview progress (%) / ``Preview_progress_``                        | Double  | 100           |                        |
+---------------------------------------------------------------------+---------+---------------+------------------------+
| Output Layer / ``Output_Layer``                                     | Choice  | Layer 0       | |                      |
|                                                                     |         |               | | **Merged**           |
|                                                                     |         |               | | **Layer 0**          |
|                                                                     |         |               | | **Layer 1**          |
|                                                                     |         |               | | **Layer 2**          |
|                                                                     |         |               | | **Layer 3**          |
|                                                                     |         |               | | **Layer 4**          |
|                                                                     |         |               | | **Layer 5**          |
|                                                                     |         |               | | **Layer 6**          |
|                                                                     |         |               | | **Layer 7**          |
|                                                                     |         |               | | **Layer 8**          |
|                                                                     |         |               | | **Layer 9**          |
+---------------------------------------------------------------------+---------+---------------+------------------------+
| Resize Mode / ``Resize_Mode``                                       | Choice  | Dynamic       | |                      |
|                                                                     |         |               | | **Fixed (Inplace)**  |
|                                                                     |         |               | | **Dynamic**          |
|                                                                     |         |               | | **Downsample 1/2**   |
|                                                                     |         |               | | **Downsample 1/4**   |
|                                                                     |         |               | | **Downsample 1/8**   |
|                                                                     |         |               | | **Downsample 1/16**  |
+---------------------------------------------------------------------+---------+---------------+------------------------+
| Ignore Alpha / ``Ignore_Alpha``                                     | Boolean | Off           |                        |
+---------------------------------------------------------------------+---------+---------------+------------------------+
| Preview/Draft Mode / ``PreviewDraft_Mode``                          | Boolean | Off           |                        |
+---------------------------------------------------------------------+---------+---------------+------------------------+
| Log Verbosity / ``Log_Verbosity``                                   | Choice  | Off           | |                      |
|                                                                     |         |               | | **Off**              |
|                                                                     |         |               | | **Level 1**          |
|                                                                     |         |               | | **Level 2**          |
|                                                                     |         |               | | **Level 3**          |
+---------------------------------------------------------------------+---------+---------------+------------------------+
