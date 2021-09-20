.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf
   
Nuke to Natron transition guide
===============================

.. toctree::
   :maxdepth: 2

This document is an very incomplete stage.


Natron and Nuke are very similar. We will focus here on the differences between them


Nodes names
-----------
Many nodes have similar names in Natron and Nuke. Here are the the ones thar are different



+----------------+------------------+
| Nuke           | Natron           |
+================+==================+
|  CurveTool     | ImageStatistics  |
+----------------+------------------+
|    Copy        | Shuffle          |
+----------------+------------------+


What's not in Nuke?
-------------------

#. cloning node groups and pyplugs is possible. This is very powerful as it mean you can apply the same complex process to different images without constantly copy / pasting when you change parameters. Beware that the nodes connexions must not be changed. Only the parameters of te nodes will be updated not their connexions.
#. cloning roto nodes
#. hide the unmodified parameters of a node. click on the 4th icon from the right in the properties panel. This will make the window far more readable and help you focus on what you're working on.

What's not in Natron?
---------------------

#. Mainly 3D functions are not implemented. But Natron is very good for compositing 3D images from other software. For example multi pass EXR from a 3D app.
#. Some missing features can be filled by adding OFXPlugins from other software vendors.

.. note::
   **Tip:**
   CommunityScripts have a tool named PMCard3D and PMcamera that can bring a very little bit of 3D functionality




Python scripting
----------------

How to get the value of a pixel: Use image statistics node + retrieve value with an expression