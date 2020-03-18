.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

=================
Using the menus
=================

Modifications of your project are done using items located in menus located in different places of the interface

The menu bar
############

.. image:: _images\menubar_01.jpg
   :width: 600px
   
File menu
---------

.. image:: _images\menu_file_01.png
   :width: 300px

New Project
  clear the node graph to start from scratch a new process
Open Project...
  load a .ntp files that is the description of a node graph. the .ntp contains no image data but only the instructions on how to process the images.
Open Recent...
  shortcut access to the most recently loaded .ntp files.

if a saved project is currently opened, the open functions will open in a new window

Reload Project
  Reload the current .ntp from disk. This can be used if you break something in your graph and don't know exactly what.
Close Project
  Close the current project but keep other projects opened if other Natron windows
Save Project
  Save the current node graph
Save Project As...
  Save the current node graph with a new name
New Project Version
  Increment the version number in the project file.
  
Project files are very small files. It is thus recommended to save different files for the different steps of your work. Would you want to recover a previous state or in case your .ntp gets corrupted.

Natron expects the version number to be in the form name_001.ntp, name_002.ntp and so on
  

.. note::  You can number your files with different patterns like "name_v01" but you will have to increment manually with "Save Project As..."

Export Project As Group
  With this item you can export a group of nodes to be reused later. This way you can create custom tools for Natron named plugins or Pyplugs.
  The group of nodes will appear as a single node when reused. This is why you must add one "output" node and "input" node(s) if relevant. So that Natron can determine how to connect your group when you will reuse it as part of another node graph

Edit menu
---------

.. image:: _images\menu_edit_01.png
   :width: 300px

Preferences...
  many preferences let you change the display of informations inside Natron. Many optimisation settings are also located in this menu.

Undo/Redo
The Undo item is modified dynamically to hint you about the last operation that can be undone.

Layout menu
-------------
.. image:: _images\menu_edit_01.png
   :width: 300px

Display menu
-------------
.. image:: _images\menu_edit_01.png
   :width: 300px
   
Render menu
-------------
.. image:: _images\menu_edit_01.png
   :width: 300px
   
Cache menu
------------
.. image:: _images\menu_edit_01.png
   :width: 300px
   
Help menu
-----------
 .. image:: _images\menu_edit_01.png
   :width: 300px

Tools menu
-----------
.. image:: _images\menu_edit_01.png
   :width: 300px


Menus Usage
-----------------

When a menu item has a keyboard shortcut associated, it is visible inside the menus

.. toctree::
   :maxdepth: 2


