.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

.. _compositing-exprs:

Expressions
===========

An expression computes the value of a parameter instead of using a fixed
value or keyframes. It can use the values of other parameters, of this node
or of other nodes, the current frame number, and mathematical functions.
Expressions are useful to:

- make a parameter follow another one, with an offset or a factor;
- make a value depend on time, for example a regular oscillation;
- add reproducible random variations;
- reuse an existing animation, for example looped, reversed or delayed.

In Natron 2.6, expressions are written in **Python**. Most expressions are a
single line, such as ``Transform1.translate.get(frame).x * 2``.

This page is an introduction for users. The
:ref:`Python expressions <paramExpr>` page of the Developers Guide has more
details and examples.


Setting an expression
---------------------

Right-click a parameter in the properties panel and choose
**Set expression...**. For a parameter with several dimensions (such as the x
and y of a position, or the r, g, b and a of a color), this entry sets an
expression on the dimension that you right-clicked only, and
**Set expression (all dimensions)** sets the same expression on all
dimensions. See :doc:`getstarted-environment-anim` for more about
dimensions.

A dialog opens:

- the top of the dialog reminds you of the variables that you can use (see
  `Variables`_ below);
- type the expression in the text area;
- the **Result** field shows the value of the expression at the current
  frame while you type, or an error message;
- the **Multi-line** button lets you write an expression of several lines
  (see `Multi-line expressions`_);
- the **Help** button shows a short help.

Click **OK** to set the expression.

The field of a parameter with an expression is green. Its slider is
disabled, and a value typed in the field has no effect while the expression
is set. To change the expression, right-click the parameter
and choose **Edit expression...**. To remove it, choose
**Clear expression** (or **Clear expression (all dimensions)**): the
parameter then uses its keyframes, or its value, again.

You can also:

- type an expression in the field under the curves in the
  :doc:`Curve Editor <getstarted-environment-anim>`, after selecting a curve;
- hold **Ctrl+Shift** (**Cmd+Shift** on macOS) and drag a parameter onto
  another one. This writes an expression on the target parameter that
  multiplies the value of the dragged parameter by the animation of the
  target parameter. Holding only **Ctrl** creates a plain link instead (see
  :doc:`getstarted-environment-anim`).

In the expression dialog, you can also hold **Ctrl** (**Cmd** on macOS) and
drag a parameter from a properties panel into the text area to insert a
reference to it. The inserted text has the form
``Node.param.get()[dimension]``, which reads the value at the current frame:
replace ``get()`` with ``get(frame)`` if the other parameter is animated (see
the note below).


Referring to other parameters
-----------------------------

Nodes and parameters are referred to by their **script name**, which is not
always the same as the name displayed in the interface:

- the Node Graph shows the *label* of a node. Its script name is the same,
  with spaces and special characters replaced by ``_``: a node labelled
  ``My Blur`` has the script name ``My_Blur``. Hover the name field at the
  top of the properties panel of the node to see its script name (see
  :ref:`nodeScriptName`);
- the script name of a parameter is written in bold at the top of the
  tooltip that appears when you hover the parameter, for example
  ``translate`` for the *Translate* parameter of a Transform node (see
  :ref:`paramScriptName`).

``Transform1.translate`` is the *Translate* parameter of the node
``Transform1``. To get its value, use one of these functions:

``get(frame)``
    The value at a given frame. For a parameter with several dimensions,
    it returns all the values: get one of them with its index, starting
    at 0, as in ``Transform1.translate.get(frame)[0]``, or with its name:
    ``.x``, ``.y``, ``.z`` for positions and sizes, ``.r``, ``.g``, ``.b``,
    ``.a`` for colors, as in ``Transform1.translate.get(frame).x``. Use another
    frame to get an earlier or later value:
    ``Transform1.translate.get(frame - 5).x`` is the value 5 frames earlier.

``getValueAtTime(frame, dimension)``
    The value of one dimension at a given frame:
    ``Transform1.translate.getValueAtTime(frame, 1)`` is the y of the
    translation.

``get()`` and ``getValue(dimension)``
    The same, at the current frame of the timeline.

.. note::

   ``get()`` and ``getValue()`` return the value at the current frame
   (usually the frame of the timeline), not at ``frame``. They can give wrong
   results when the value is computed at another frame, for example in the
   Curve Editor, or when another expression asks for this parameter at
   another frame. Use ``get(frame)`` or ``getValueAtTime(frame, dimension)``
   whenever the value of the other parameter can change over time.

An expression can use the parameters of:

- the nodes in the same group as the node (or the top-level nodes, for a
  node that is not in a group), by their script name, for example
  ``Blur1``, or through ``thisGroup``, as in ``thisGroup.Blur1``;
- the group that contains the node, with the ``thisGroup`` variable, for
  example ``thisGroup.mySize.get(frame)``;
- the nodes inside the node, if the node is itself a group, for example
  ``Group1.Blur1.size.get(frame)[0]`` in an expression of a parameter of
  ``Group1``.

When you rename a node, Natron updates the expressions that refer to it.

.. tip::

   In a group that will be saved as a :ref:`PyPlug <groups>`, prefer
   ``thisGroup.Blur1`` to ``Blur1`` in expressions. Both work inside the
   group, but ``thisGroup.`` makes it clear that the node is in the same
   group, and keeps working when the group is instantiated again.


Variables
---------

These variables are defined in every expression:

``frame``
    The frame at which the value is computed. This is usually the current
    frame of the timeline, but it can be another frame, for example when
    another expression calls ``get(frame - 1)`` on this parameter.

``thisNode``
    The node that holds the parameter, for example ``thisNode.size.get(frame)[0]``.

``thisParam``
    The parameter whose expression is computed.

``thisGroup``
    The group that contains the node. For top-level nodes, it is the
    application object (``app1``), which gives access to the top-level
    nodes.

``dimension``
    The index of the dimension being computed: 0 for x or r, 1 for y or g,
    etc. This is useful with **Set expression (all dimensions)**: the
    expression ``Transform1.translate.get(frame)[dimension]`` copies x to x
    and y to y.

``curve(frame, dimension)``
    The value of the animation (the keyframes) of this parameter, at a given
    frame. ``curve(frame)`` is the animation of the first dimension at the
    current frame.

``random(min, max, frame, seed)`` and ``randomInt(min, max, frame, seed)``
    A random number between ``min`` and ``max`` (``max`` excluded). The value
    only depends on ``frame``, on ``seed`` (an integer, 0 if omitted) and on
    the state of the node and its inputs: it is the same each time the frame
    is computed, and changing a parameter of the node, or of a node upstream,
    gives new random values (undo does not bring the old ones back; two
    identical new nodes also differ). It does not depend on the parameter nor
    on the dimension: use a different ``seed`` for each random value, each
    parameter and each dimension, for example ``random(-2, 2, frame,
    dimension)``, else the dimensions of a position get the same value and it
    only moves along the diagonal. Always pass ``frame``: ``random(min, max)``
    without it continues a sequence of the parameter. Natron starts that
    sequence again from the frame when it computes the expression of the
    parameter, but anywhere else (a script, or ``Node1.param.random()`` in
    another expression) its value depends on the values computed before, so
    it is not reproducible. ``randomInt`` rounds
    toward zero, so with a negative ``min`` the results are uneven:
    ``randomInt(-5, 5, frame)`` practically never returns -5, and returns 0
    twice as often as the other values.

All the functions of the Python ``math`` module can be used directly, for
example ``sin()``, ``cos()``, ``sqrt()``, ``floor()``, ``pi``. More functions
(noise, smooth steps...) are in the
:ref:`ExprUtils <ExprUtils>` class, for example
``NatronEngine.ExprUtils.noise(frame * 0.1)``.


Examples
--------

Follow another parameter, with an offset (on the *x* of a translation)::

    Transform1.translate.get(frame).x + 100

Copy the translation of another node, on both dimensions (use **Set
expression (all dimensions)**)::

    Transform1.translate.get(frame)[dimension]

Follow another parameter with a delay of 5 frames::

    Transform1.translate.get(frame - 5)[dimension]

Oscillate between -10 and 10, with a period of about 31 frames::

    10 * sin(frame / 5)

Add a random variation between -2 and 2 to the animation of this
parameter::

    curve(frame, dimension) + random(-2, 2, frame, dimension)

Play the animation of this parameter twice as fast::

    curve(frame * 2, dimension)

Multiply the value of another parameter by the animation of this one (this is
what **Ctrl+Shift** drag-and-drop writes, with ``thisGroup.`` instead of
``app1.`` inside a group)::

    app1.Blur1.size.getValue(dimension) * curve(frame,dimension)

As explained above, ``getValueAtTime(frame, dimension)`` is safer than
``getValue(dimension)`` when the other parameter is animated.

The *Predefined* entries of the Curve Editor right-click menu (*Loop*,
*Reverse*, *Negate*) also write expressions based on ``curve()``. See
:doc:`getstarted-environment-anim`.


Multi-line expressions
----------------------

For more complex computations, click the **Multi-line** button in the
expression dialog. The expression is then a sequence of Python statements,
and it must store its result in a variable named ``ret``::

    s = Transform1.scale.get(frame).x
    ret = curve(frame * s, dimension)

Without **Multi-line**, the expression must fit on one line, and Natron
assigns its result to ``ret`` for you.


Expressions and keyframes
-------------------------

When a parameter has an expression, its keyframes are ignored: the value
comes from the expression only. The keyframes are kept, and the expression
can use them with ``curve()``. When you clear the expression, the parameter
uses its keyframes again.

The Curve Editor draws the values produced by the expression, together with
the curve of the keyframes.


When an expression is wrong
---------------------------

If an expression cannot be computed (a syntax error, a node that does not
exist anymore, a division by zero...), a warning icon appears next to the
parameter. Hover it to read the error message. While the expression is
invalid, the parameter uses its animation, or its value without expression.

In the Node Graph, a green arrow joins a node to each node whose parameters
its expressions use, and a small green **E** indicator is shown on the node.
Hover the arrow to see the parameters involved. Press **Shift+E** in the Node
Graph to show or hide these arrows.

Before deleting a node, Natron warns you if other parameters depend on it
through an expression or a link.


Performance
-----------

Each expression is a small Python program that runs every time the value is
needed, which can happen many times per frame. Many complex expressions can
slow down the interface and rendering. When a simple link is enough, prefer
a link (see :doc:`getstarted-environment-anim`).


Going further
-------------

The Developers Guide covers expressions in more depth:

- :ref:`paramExpr`: the scope of expressions, ``random()``, persistence in
  PyPlugs, and more examples, such as showing parameter values in the label
  of a node;
- :ref:`Param <Param>` and :ref:`AnimatedParam <AnimatedParam>`: the Python
  functions available on parameters, such as ``setExpression()`` to set an
  expression from a script;
- :ref:`ExprUtils <ExprUtils>`: noise and interpolation functions.
