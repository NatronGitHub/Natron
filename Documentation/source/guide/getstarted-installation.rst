.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf
   
Installation
============

This chapter will guide you through the installation of Natron on Windows, Mac and Linux.

.. toctree::
   :maxdepth: 2
   
   getstarted-installation-windows
   getstarted-installation-mac
   getstarted-installation-linux

Additional Elements
===================

Community scripts
-----------------

Many scripts that bring additional functionality can be downloaded from:
https://github.com/NatronGitHub/natron-python-scripting

To install these:

- Copy the content of this repository to your .Natron folder.:ref: 'Natron plug-in paths'
- Restart Natron
- Enjoy the new items available mostly in Tools and Edit menu.


These tool add predefined roto shapes, multilayer EXR extraction, node connexion tools, and more.
They will bring Natron closer to the Nuke interface.
Albeit experimental, these scripts are a recommended download, more specifically for previous Nuke users.


Community plugins
-----------------

Additional Python plugins (PyPlugs) can be downloaded from:
https://github.com/NatronGitHub/natron-plugins

To install these:

- Copy the content of this repository to any folder of your choice.
- Open Natron preferences from the menubar, select Plugins->PyPlugs search path->Add.. and enter the extracted file location.
- Save preferences.
- Restart Natron.
- Enjoy the new tools available in the left toolbar.

These tools bring animated textures for motion designers, as well as most common Nuke gizmos (DespillMadness, PushPixel,...).
Albeit experimental these scripts are a recommended download.
