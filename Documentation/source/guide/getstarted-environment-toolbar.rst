.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf
   
Using the toolbar
=================

Each icon in the toolbar is a menu giving access to different categories of nodes (ie. image processing tools) that Natron offers to process or create images.

If you click on a tool, the corresponding node will be added to the node graph

.. note::  If a node is selected in the graph, the new node will be inserted below the selected one. It will be processed right after the selected one.

**Image tools:** The nodes to bring images in and out of Natron, plus a few utility nodes.

**Draw tools:** The nodes to create basic shapes.

**Time tools:** The nodes to change the timing of your clips.

**Channels tools:** The nodes to changes the order of your image channels (basic
channels are RGBA for Red Green Blue Alpha but others can be added).

**Colors tools:** Mainly color correction nodes.

**Filter tools:** Nodes to change the texture of the image (blur or sharpen for
example).

**Merge tools:** Nodes with multiple inputs that can merge multiple images into
one composite image.

**Transform tools:** Nodes to change the geometry of the images.

**Views tools:** Nodes to manage stereo images that considered as different views (left and right).

**Other tools:** Mainly utility nodes to keep the node graph clean and readable.

.. note::  Other entrys in the toolbar can be added with plugins / scripts. So your Natron installation may have other Tool icons (community plugins,...)

.. toctree::
   :maxdepth: 2


