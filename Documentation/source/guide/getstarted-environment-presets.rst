.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

Using node presets
==================

A node preset is a file that stores the settings of a node, so that you can
apply them again to another node of the same type, in the same project or in
another one. Natron saves presets in files with the ``.nps`` extension.

Presets are managed from the *Settings and presets* menu of the
:doc:`properties panel <getstarted-environment-proppanels>` (called the
*Presets menu* in :ref:`Presets_menu`): this is the button next to the node
icon, at the top of the panel.

Saving a preset
---------------

1. Set up the node as you want it.
2. In its properties panel, open the *Settings and presets* menu and choose
   **Export as presets**.
3. Choose a folder and a file name in the file dialog. Natron adds the
   ``.nps`` extension if you leave it out.

You can save the file anywhere: Natron does not have a preset folder, and it
does not list presets in its menus. Keep your presets in a folder of your
choice, for example next to your projects or in a shared folder for your team.

Loading a preset
----------------

1. Create a node of the same type as the one the preset was saved from, or
   select an existing one.
2. In its properties panel, open the *Settings and presets* menu and choose
   **Import presets**.
3. Select the ``.nps`` file.

The node gets the settings from the file. A preset can only be loaded on a
node made by the same plug-in: loading a Blur preset on a Grade node shows an
error and changes nothing.

Loading a preset is a single step in the undo history of the properties
panel, so you can usually undo it (see :doc:`getstarted-environment-undoredo`).
For Roto, RotoPaint and Tracker nodes, see the warning below.

The two menu entries are disabled for a clone node (created with
*Clone Nodes*, :kbd:`Alt+K`, :kbd:`Option+K` on macOS), whose parameters
follow the node it was cloned from. They are also disabled for the nodes
inside a PyPlug that has not been made editable.

What a preset contains
----------------------

A preset stores all the parameters of the node, on every tab of the
properties panel. This includes the file name of a Read or Write node, and the
settings of the *Node* tab, such as the label, *Disable* and *Lifetime*:

- the values of the parameters, their animation curves and their
  expressions;
- the user parameters added with *Manage user parameters...*, which are
  re-created when the preset is loaded;
- the shapes and strokes of a :ref:`Roto <fr.inria.built-in.Roto>` or
  :ref:`RotoPaint <fr.inria.built-in.RotoPaint>` node, and the tracks of a
  :ref:`Tracker <fr.inria.built-in.Tracker>` node.

.. warning::

   On a Roto, RotoPaint or Tracker node, loading a preset *adds* the shapes,
   strokes or tracks of the preset to those already in the node: it does not
   replace them. Undo does not reliably bring back the previous state either.
   Load such presets on a new node, and do not rely on Undo.

Loading a preset does not change the name of the node, its position or color
in the Node Graph, or its connections.

Expressions and links that refer to other nodes are stored as they are. If the
project where you load the preset has no node with the same name, these
expressions fail. Presets work best with nodes whose parameters only depend on
the node itself.

For a :ref:`Group <fr.inria.built-in.Group>` node, a preset only restores the
parameters of the group node itself, not the nodes inside the group. To reuse
a whole group, export it as a PyPlug (see below).

Other ways to reuse settings
----------------------------

Presets are one of several ways to reuse work in Natron:

Copy and paste nodes
  Select nodes in the :doc:`Node Graph <getstarted-environment-nodegraph>`
  and copy them (:kbd:`Ctrl+C`, :kbd:`Cmd+C` on macOS). Natron puts the nodes
  on the system clipboard as text, so you can paste them in another project,
  or keep them in a text file and copy them from there later. Unlike a preset,
  this keeps several nodes and their connections. Some users keep a project
  that serves as a library of ready-made nodes.

PyPlugs
  A Group node can be exported as a Python plug-in, called a PyPlug, with the
  **Export as PyPlug** button on the *Node* tab of its properties
  panel. A PyPlug re-creates the whole group. It appears in the
  :doc:`Toolbar <getstarted-environment-toolbar>` like any other node, but
  only after you restart Natron, and only if you saved it in a folder of the
  PyPlug search path. If the folder you choose is not in the search path, the
  export dialog offers to add it.
  Use a preset to reuse the settings of one node, and a PyPlug to make a new
  tool out of several nodes. See :ref:`groups` in the Developers Guide.

Changing the default values
  To change the values that a node gets when it is created, use a Python
  startup script (see :ref:`startupScripts`). The change then applies to every
  new node of that type.
