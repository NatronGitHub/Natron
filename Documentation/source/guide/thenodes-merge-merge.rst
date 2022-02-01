.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

Merge Node
==========

The Merge node allows one to complete compositing operations, by default it places image data from A overtop B with an "over" operation.

Usage
-----

Never consider RGB as being transparent by default - this is OK for unpremultiplied compositing (After Effects) but is invalid in a premultiplied compositor such as Natron or Nuke.

Users still have the option to ignore the alpha channel. (new from v2.4)


.. toctree::
    :maxdepth: 2



