.. _paramExpr:

Parameters expressions
======================

The value of a :doc:`parameter<PythonReference/NatronEngine/Param>` can be set by
Python expressions. An expression is a line of code that can either reference the value
of other parameters or apply mathematical functions to the current value.

The expression will be executed every times the value of the parameter is fetched from a call
to :func:`getValue(dimension)<>` or :func:`get()<>`.

.. warning::

    Note that when an expression is active, all animation is ignored
    and only the result of the expression will be used to return the value of the parameter.

When executing an expression, the expression itself has a **scope**.
The **scope** of the expression defines all nodes and parameters that are possible to use
in the expression in order to produce the output value.

Any node in the *scope* can has a variable declared corresponding to its script-name::

    Blur1

You would then access a parameter of Blur1 also by its script-name::

    Blur1.size

    Group1.Blur1.size

.. warning::

    Referencing the value of the same parameter which expression is being edited can lead
    to an infinite recursion which Python should warn you about



In fact this is exactly like referencing :ref:`auto-declared<autoVar>` nodes via the *Script Editor*
except that the *app* prefix was removed for nodes in the scope.

See :ref:`this section<nodeScriptName>` to learn how to determine the *script-name* of a node.

See :ref:`this section<paramScriptName>` to learn how to determine the *script-name* of a parameter.

By default a parameter's expression can only refer to parameters of nodes belonging to the
same Group, or to parameters belonging to the parent :doc:`PythonReference/NatronEngine/Group` node.

Parameters of a Group node are also granted in the scope the parameters contained within that group.

For instance if your graph hierarchy looks like this::

    Read1
    Blur1
    Group1/
        Input1
        Blur1
        Convolve1
        Roto1
        Output1
    Viewer1


A parameter of *Read1* would be able to reference any parameter of *Read1*, *Blur1*, *Group1*, *Viewer1*
but could not reference any parameter of the nodes within *Group1*.

Similarly, a parameter of *Group1.Blur1* would be able to reference any parameter of
*Group1*, *Group1.Input1* , *Group1.Blur1* , *Group1.Convolve1* , *Group1.Roto1* , *Group1.Output1* but
would not be able to reference any top-level node (*Read1*, *Blur1*, *Viewer1*) except the
*Group1* node.

A parameter of *Group1* would on the other hand be able to reference any parameter in
top-level nodes and in the nodes of *Group1*.

The *scope* was introduced to deal with problems where the user would write expressions
referencing parameters that would probably no longer be referable when loading the script
again in another project.

.. warning::

    Note that you would still be able to reach any node or parameter in the project using the
    *app1* (or *app* prefix in command-line mode)  but is not recommended to do so::

        app1.Blur1.size


All functions available in the Python API are made available to expressions. Also for
convenience the **math** Python module has been made available by default to expressions.

.. _settingExpr:

Setting an expression:
-----------------------

To create an expression from the user interface, right click a parameter and choose *Set Expression...*

.. figure:: setExprRightClick.png
    :width: 200px
    :align: center

Note that for multi-dimensional parameters such as :doc:`PythonReference/NatronEngine/ColorParam`, the
*Set Expression...* entry will **only set an expression for the right-clicked dimension**.

The *Set Expression (all dimensions)* entry will on the other hand set the same expression on all
dimensions of the parameter at once.

.. figure:: multiDimSetExprMenu.png
    :width: 300px
    :align: center

A dialog will open where you can write the expression:

.. figure:: setExprDialog.png
    :width: 600px
    :align: center


By default you do not have to assign any variable as the result of the expression, Natron
will do it by itself::

    #Expression for Blur1.size

    Transform1.translate.get[0]

    #Will be expanded automatically by Natron to

    ret = Transform1.translate.get[0]

However if you were to write an expression that spans over multiple lines you would need
to specifically set the **ret** variable yourself and toggle-on the *multi-line* button::

    a = acos(Transform1.translate.get[0])
    b = sin(Transform1.rotate.get())
    ret = (tan(a * b) / pi) + Group1.customParam.get()


You can also set an expression from a script using the :func:`setExpression(expr,hasRetVariable,dimension)<>` function
of :doc:`PythonReference/NatronEngine/AnimatedParam`.

Writing an expression:
----------------------

For convenience the following variables have been declared to Python when executing the expression:

    * **thisNode**: It references the node holding the parameter being edited

    * **thisGroup**: It references the group containing *thisNode*

    * **thisParam**: It references the param being edited

    * **dimension**: Defined only for multi-dimensional parameters, it indicates the dimension (0-based index)
      of the parameter on which the expression has effect.

    * **frame**: It references the current time on the timeline

    * The **app** variable will be set so it points to the correct :ref:`application instance<App>`.

To reference the value of another parameter use the :func:`get()<>` function which retrieves the value
of the parameter at the current timeline's time. If the parameter is multi-dimensional, you need to use
the subscript operator to retrieve the value of a particular dimension.

The :func:`getValue(dimension)<>` does the same thing but takes a *dimension* parameter to retrieve
the value of the parameter at a specific *dimension*. The following is equivalent::

    ColorCorrect1.MasterSaturation.get()[dimension]

    ColorCorrect1.MasterSaturation.getValue(dimension)

Note that for 1-dimensional parameter, the :func:`get()<>` function cannot be used with subscript, e.g.:

    Blur1.size.get()

To retrieve the value of the parameter at a specific *frame* because the parameter is animated,
you can use the :func:`get(frame)<>` function.

Again the :func:`getValueAtTime(frame,dimension)<>` does the same thing but takes a *dimension* parameter to retrieve
the value of the parameter at a specific *dimension*. The following lines are equivalent to
the 2 lines above::

    ColorCorrect1.MasterSaturation.get(frame)[dimension]

    ColorCorrect1.MasterSaturation.getValueAtTime(frame,dimension)

We ask for the value of the *MasterSaturation* parameter of the *ColorCorrect1* node its value
at the current *frame* and at the current *dimension*, which is the same as calling the :func:`get()<>` function
without a *frame* in parameter.

Copying another parameter through expressions:
----------------------------------------------

If we want the value of the parameter **size** of the node *BlurCImg1*  to copy the
parameter **mix** of the node *DilateCImg1*, we would set the following expression on the
**size** parameter of the node *BlurCImg1* (see :ref:`setting an expression<settingExpr>`)::

     DilateCImg1.mix.get()


If mix has an animation and we wanted to get the value of the mix at the previous *frame*,
the following code would work::

     DilateCImg1.mix.get(frame - 1)


Note that when choosing the *Link to...* option when right-clicking a parameter, Natron writes
automatically an expression to copy the parameter to link to for you.


Using random in expressions:
-----------------------------

Sometimes it might be useful to add a random generator to produce noise on a value.
However the noise produced must be reproducible such that when rendering multiple times the same
frame or when loading the project again it would use the same value.

We had to add a specific random function in Natron that takes into account the state of a
parameter and the current time on the timeline as a seed function to random.

.. warning::

    We advise against using the functions provided by the module random.py of the Python
    standard library, as the values produced by these functions will not be reproducible
    between 2 runs of Natron for the same project.

The Natron specific random functions are exposed in the :ref:`Param<Param>` class.

When executing an expression, Natron pre-declares the **random()** function so that you do not
have to do stuff like::

    thisParam.random()

Instead you can just type the following in your expression::

    myOtherNode.myOtherNodeParam.get() * random()

The :func:`random(min = 0.,max = 1.)<>` function also takes 2 optional arguments indicating
the range into which the return value should fall in. The range is defined by \[min,max\[.

    #Returns a random floating point value in the range [1., 10.[
    random(1.,10.)

For integers, use the :func:`randomInt(min,max)<>` function instead::

    #Returns a random integer in the range [1,100[
    randomInt(1,100)

    #Using the randomInt function with a given seed
    seed = 5
    randomInt(1,100,frame,seed)

Advanced expressions:
----------------------

To write more advanced expressions based on fractal noise or perlin noise you may use
the functions available in the :ref:`ExprUtils<ExprUtils>` class.


Expressions persistence
------------------------

If you were to write a group plug-in and then want to have your expressions persist when
your group will be instantiated, it is important to prefix the name of the nodes you reference
in your expression by the **thisGroup.** prefix. Without it, Natron thinks you're referencing
a top-level node, i.e: a node which belongs to the main node-graph, however, since you're using
a group, all your nodes are no longer top-level and the expression will fail.

Examples
----------

Setting the label of a Node so it displays the value of a parameter on the node-graph:
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

For instance, we may want to have on the shuffle node, the values of the output RGBA channels
so we don't have to open the settings panel to understand what the node is doing.

To do so, we set an expression on the "Label" parameter located in the "Node" tab of the
settings panel.

.. figure:: shuffleSubLabel.png
    :width: 300px
    :align: center

.. figure:: shuffleLabelExpression.png
    :width: 300px
    :align: center


Set the following expression on the parameter
::

    thisNode.outputR.getOption(thisNode.outputR.get()) + "\n" + thisNode.outputG.getOption(thisNode.outputG.get()) + "\n" + thisNode.outputB.getOption(thisNode.outputB.get()) + "\n" + thisNode.outputA.getOption(thisNode.outputA.get())


Generating custom animation for motion editing:
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In this example we will demonstrate how to perform Loop,Negate and Reverse effects
on an animation even though this is already available as a preset in Natron.

To do be able to do this we make use of the :func:`curve(frame,dimension)<NatronEngine.Param.curve>`
function of the :ref:`Param<Param>` class. This function returns the value of the animation
curve (of the given dimension) at the given time.

If we were to write the following expression::

    curve(frame)

The result would be exactly the animation curve of the parameter.

On the other hand if we write::

    curve(-frame)

.. figure:: CE_reverse.png
    :width: 300px
    :align: center

We have just reversed the curve, meaning that the actual result at the frame F will be in fact
the value of the curve at the frame -F.

In the same way we can apply a negate effect::

    -curve(frame)

.. figure:: CE_negate.png
    :width: 300px
    :align: center

The loop effect is a bit more complicated and needs to have a frame-range in parameter::

    firstFrame = 0
    lastFrame = 10
    curve(((frame - firstFrame) % (lastFrame - firstFrame + 1)) + firstFrame)

.. figure:: CE_loop.png
    :width: 300px
    :align: center
