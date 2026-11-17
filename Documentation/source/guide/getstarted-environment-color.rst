.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

Using the color controls
========================

Many nodes have color parameters: the gain, gamma and offset of a
:ref:`Grade <net.sf.openfx.GradePlugin>` or
:ref:`ColorCorrect <net.sf.openfx.ColorCorrectPlugin>` node, the color of a
:ref:`Constant <net.sf.openfx.ConstantPlugin>` or of a shape, and so on.
In the :doc:`properties panel <getstarted-environment-proppanels>`, a color
parameter has three or four channels (red, green, blue and, for some
parameters, alpha), and a few extra controls next to its value fields.

Values are linear
-----------------

Natron works on linear images, and the value fields show the linear values
that the node uses. They are not limited to the 0 to 1 range: a gain of 2 or
a negative offset are valid values. The small swatch next to the fields only
gives an idea of the color: it shows the value converted to sRGB and clipped
to the 0 to 1 range.

The parts of a color parameter
------------------------------

From left to right, a color parameter shows:

- the value fields, one per channel, or a single field and a slider when the
  channels are folded (see below);
- a button showing the number of channels (``3`` or ``4``), which folds and
  unfolds the channels;
- the color swatch, which also turns color picking on and off;
- the color wheel button, which opens the color selector.

Folding and unfolding the channels
----------------------------------

When all channels have the same value, the parameter is folded: a single
field and a slider set all channels at once. This is the usual case for a
gain or a gamma, which is often a gray value.

Click the button labeled ``3`` or ``4`` to unfold the parameter and edit each
channel on its own. Each field then gets a colored underline (red, green,
blue, and gray for alpha), and the slider is hidden. Click the button again to
fold the channels. A parameter whose channels have different values is
always shown unfolded.

As with other numeric parameters, :kbd:`Alt` + click on the slider resets the
parameter to its default value.

Animating the channels
----------------------

Color parameters are animated like any other parameter (see
:doc:`getstarted-environment-anim`). Right-click on a value field to open the
parameter menu. When the parameter is unfolded, the menu entries such as
*Set Key*, *Remove Key* or *Set expression...* act on the channel you
clicked, and the entries marked *(all dimensions)* act on all channels at once.
Each channel can thus have its own keyframes or its own expression.

If a parameter is animated, a change made with the color selector or the color
picker sets a keyframe at the current frame, as when you type a value.

The color selector
------------------

Click the color wheel button (tooltip: *Open Color Selector*) to open the
color selector. It contains:

- a color triangle: drag on the outer ring to choose the hue, and inside the
  triangle to choose the saturation and brightness;
- the **RGB** and **HSV** buttons, which switch the sliders between red,
  green and blue, and hue, saturation and value. The RGB sliders show the
  linear values of the parameter;
- an **A** slider for the alpha channel, when the parameter has one;
- a **Hex** field: type an HTML color such as ``#FF8000``, including the
  leading ``#``, and press :kbd:`Enter`. Like the color triangle, this value is
  in sRGB, and it is converted to linear before being set;
- a palette of 24 color slots.

The parameter changes as soon as you change the color in the selector.
The selector only works in the 0 to 1 range: values outside this range are
clipped when the selector opens. To set a value above 1 or below 0, type it in
the value fields of the properties panel.

The palette
~~~~~~~~~~~

The palette stores colors that you want to reuse:

- **Add** (or :kbd:`Shift+A` while the selector is open) stores the current
  color in the next free slot. When all slots are used, Natron starts again
  from the first slot;
- click a slot to apply its color;
- **Clear** (or :kbd:`Shift+C`) clears all slots.

All color parameters share the same palette, and Natron keeps it between
sessions.

Picking a color in the Viewer
-----------------------------

You can set a color parameter from the image shown in a
:doc:`Viewer <getstarted-environment-viewer>`:

1. Click the color swatch of the parameter. A picker icon is drawn on the
   swatch, and the viewers show their information bar.
2. In a viewer, :kbd:`Ctrl` + click (:kbd:`Cmd` + click on macOS) on a pixel
   to set the parameter to the color of that pixel. Keep the mouse button
   pressed and move the mouse to pick continuously.
3. To pick the average color of an area, :kbd:`Ctrl+Shift` + drag
   (:kbd:`Cmd+Shift` + drag on macOS) to draw a rectangle.
4. When you are done, click the swatch again, or click anywhere in the viewer
   without a modifier key.

Only one color parameter can pick at a time: turning on the picker of a
parameter turns off the picker of the previous one. The picked color is set on
all the channels of the parameter, including alpha when the parameter has an
alpha channel. Unchecking the picker button of the viewer toolbar (tooltip:
*Show/Hide information bar...*) hides the information bar and also turns off
any active picker.

:kbd:`Ctrl` + click in a viewer always shows the color under the mouse in
the information bar of the viewer, even when no parameter is picking. A tilde
(``~``) before the values means that the viewer shows a reduced-resolution
image, so the value is approximate. Nodes that have curves in their
properties panel, such as :ref:`ColorLookup <net.sf.openfx.ColorLookupPlugin>`,
also show the picked values on their curves.

Linear or displayed colors
~~~~~~~~~~~~~~~~~~~~~~~~~~

By default, the picked color is the linear value of the pixel, which is what
the node expects. To pick the value as displayed by the viewer instead (in the
colorspace of the viewer), uncheck **Linear color pickers** in
:menuselection:`Edit --> Preferences --> User Interface`.

Simplified color controls
-------------------------

Some color parameters, for example those shown in a viewer toolbar, only
show the swatch and the color wheel button. Right-click the swatch to open the
parameter menu. These parameters cannot pick a color from the viewer.
