.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf
   
Working with nodes
==================

For a brief introduction to the concept of nodes and images in Natron see: :doc:`Main concepts <getstarted-about-mainconcepts>`



Main rules
----------

.. image:: _images/TheNodes-Image/node_connections.gif

- Nodes can have 1 or more inputs.
   Most processing nodes have only 1 input (blur, color corrections...)

   Merging nodes have several inputs that are turned into one "mixed" output
- mask input is present on many nodes.
   It's purpose usually is to limit the effect of the node to the part of the image defined by the white part of the mask
- Nodes always have exactly 1 output. 
   If several nodes B and C connect their inputs to the same output of node A they will receive the same image
   
   The only node without any output is the Viewer node. It determines what node is shown in the Viewer pane

.. image:: _images/TheNodes-Image/node_viewer.gif

When a node has several inputs (eg. Merge node) the B input is the "background" and A is the "foreground" image. If you disable the node, B input is passed unmodified
Merge nodes can have many inputs added when required to allow many merge operations at once

.. image:: _images/TheNodes-Image/node_multiple_inputs.gif
   :width: 600

Connections
------------
To connect a node to another:
grab the tip of the input arrow and drop it onto the input node

.. image:: _images/TheNodes-Image/node_connect_disconnect.gif

insert in the graph:
hold ``ctrl`` + drag and drop node C onto a existing connection between A and B will insert the node inbetween. resulting in A->C->B
To show you where the node will be inserted, a green arrow is displayed

.. image:: _images/TheNodes-Image/node_insert.gif

To disconnect a node:
select it, press ``ctrl+shift+x``

.. image:: _images/TheNodes-Image/node_extract.gif

**community scripts**
To connect distant nodes, select output node, input node, press ``y``

.. image:: _images/TheNodes-Image/node_distant_connect.gif



For more in depth information on how to manage your nodegraph see :doc:`Nodegraph <getstarted-environment-nodegraph>`