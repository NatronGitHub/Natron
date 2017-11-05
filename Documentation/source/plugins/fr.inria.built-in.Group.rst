.. _fr.inria.built-in.Group:

Group node
==========

*This documentation is for version 1.0 of Group.*

Description
-----------

Use this to nest multiple nodes into a single node. The original nodes will be replaced by the Group node and its content is available in a separate NodeGraph tab. You can add user parameters to the Group node which can drive parameters of nodes nested within the Group. To specify the outputs and inputs of the Group node, you may add multiple Input node within the group and exactly 1 Output node.

Inputs
------

+-------+-------------+----------+
| Input | Description | Optional |
+=======+=============+==========+
+-------+-------------+----------+

Controls
--------

.. tabularcolumns:: |>{\raggedright}p{0.2\columnwidth}|>{\raggedright}p{0.06\columnwidth}|>{\raggedright}p{0.07\columnwidth}|p{0.63\columnwidth}|

.. cssclass:: longtable

+---------------------------------------+--------+---------+-----------------------------------------------------------------------------------------------------+
| Parameter / script name               | Type   | Default | Function                                                                                            |
+=======================================+========+=========+=====================================================================================================+
| Convert to Group / ``convertToGroup`` | Button |         | Converts this node to a Group: the internal node-graph and the user parameters will become editable |
+---------------------------------------+--------+---------+-----------------------------------------------------------------------------------------------------+
