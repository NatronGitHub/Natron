.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

Undoing and redoing
===================

Most changes you make in Natron can be undone and redone. Unlike many
applications, Natron does not keep a single undo history for the whole
project: it keeps a separate history for each part of the interface.

Shortcuts and menus
-------------------

- **Undo**: :kbd:`Ctrl+Z` (:kbd:`Cmd+Z` on macOS).
- **Redo**: :kbd:`Ctrl+Y` on Windows, :kbd:`Ctrl+Shift+Z` on Linux, and
  :kbd:`Cmd+Shift+Z` on macOS.

The same commands are at the bottom of the
:doc:`Edit menu <getstarted-environment-menus>`. Their text tells you which
change will be undone or redone, for example *Undo Move node(s)* or
*Undo Set value of Blur1.Size*.

Which history is used
---------------------

Natron keeps one undo history for each of these:

- the :doc:`Node Graph <getstarted-environment-nodegraph>`, and the Node Graph
  of each Group node;
- the :doc:`properties panel <getstarted-environment-proppanels>` of each
  node;
- the Curve Editor;
- the Dope Sheet.

The shortcuts and the Edit menu act on the *active* history. A history
becomes active when you move the mouse pointer over its panel, and when you
make a change in it. For example, with the pointer over the Node Graph,
:kbd:`Ctrl+Z` undoes the last change made to the graph, even if you changed a
parameter after that. Move the pointer over the properties panel of the node
to undo the parameter change instead. Check the text of the Undo entry in the
Edit menu if you are not sure what will be undone.

.. note::

   When a text or value field has the keyboard focus, for example while you
   type a value, the active history does not follow the mouse pointer, and
   :kbd:`Ctrl+Z` undoes the typing in the field. Press :kbd:`Enter` or click
   outside the field first.

The properties panel of each node also has **Undo** and **Redo** buttons in its
header. They always act on the history of that node, wherever the mouse
pointer is.

What can be undone
------------------

In the Node Graph:
  creating, deleting, moving, renaming, enabling and disabling nodes;
  connecting and disconnecting them, inserting and extracting a node;
  pasting nodes; creating a group from a selection and expanding a group;
  de-cloning nodes; auto-arranging nodes; resizing a backdrop.

In a properties panel:
  changing a parameter value, setting and removing keyframes, setting
  an expression, pasting values or animation, resetting parameters to their
  defaults, loading presets, and renaming the node. Loading a preset on a
  Roto, RotoPaint or Tracker node cannot be undone reliably (see
  :doc:`getstarted-environment-presets`).

In the Viewer:
  moving the handles that plug-ins draw in the Viewer, such as those of
  :ref:`Transform <net.sf.openfx.TransformPlugin>` or
  :ref:`Crop <net.sf.openfx.CropPlugin>`; editing :ref:`Roto <fr.inria.built-in.Roto>` and
  :ref:`RotoPaint <fr.inria.built-in.RotoPaint>` shapes and strokes (drawing
  a shape or a stroke, adding, moving and removing points, moving tangents and
  feathers, transforming a selection, opening or closing a curve), and adding
  or removing :ref:`Tracker <fr.inria.built-in.Tracker>` tracks. These
  changes are recorded in the history of the node's properties panel.

In the Curve Editor and the Dope Sheet:
  moving, adding, deleting and pasting keyframes, scaling them, changing
  their interpolation and moving their tangents. In the Dope Sheet, trimming
  and slipping a clip are also undoable.

Successive changes to the same parameter are merged into a single step: if
you drag a slider, or type several values in the same field one after the
other, one Undo restores the value the parameter had before the first change.
Changes that create a keyframe are not merged.

Limits
------

- Some changes are not recorded, and cannot be undone. These include changes
  made by Python scripts, colors set with the color selector or picked from a
  Viewer (see :doc:`getstarted-environment-color`), and changes to the
  Preferences (:menuselection:`Edit --> Preferences...`, in the
  :menuselection:`Natron` menu on macOS).
- The history is not saved with the project. When you open a project, its
  history is empty.

If you cannot undo a change, you can go back to the last saved version of the
project with :menuselection:`File --> Reload Project`, or open an earlier
version with :menuselection:`File --> Open Project...`, if you saved versions
with :menuselection:`File --> New Project Version`.
