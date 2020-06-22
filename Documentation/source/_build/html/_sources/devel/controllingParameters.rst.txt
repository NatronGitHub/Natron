.. _controlParams:

Controlling parameters
=======================


Accessing a node's parameters:
------------------------------

As for nodes, :doc:`parameters<PythonReference/NatronEngine/Param>` are :ref:`auto-declared<autoVar>` objects.
You can access an existing parameter of a node by its *script-name*::

    app.BlurCImg1.size

Note that you can also access a parameter with the :func:`getParam(scriptName)<NatronEngine.Effect.getParam>` function::

    param = app.BlurCImg1.getParam("size")

but you should not ever need it because Natron pre-declared all variables for you.

The *script-name* of a parameter is visible in the user interface when hovering the parameter
in the settings panel with the mouse. This is the name in **bold**:

.. figure:: paramScriptName.png
    :width: 200px
    :align: center

Parameters type:
----------------

Each parameter has a type to represent internally different data-types, here is a list of
all existing parameters:

    * :doc:`IntParam<PythonReference/NatronEngine/IntParam>` to store 1-dimensional integers

    * :doc:`Int2DParam<PythonReference/NatronEngine/Int2DParam>` to store 2-dimensional integers

    * :doc:`Int3DParam<PythonReference/NatronEngine/Int3DParam>` to store 3-dimensional integers

    * :doc:`DoubleParam<PythonReference/NatronEngine/DoubleParam>` to store 1-dimensional floating point

    * :doc:`Double2DParam<PythonReference/NatronEngine/Double2DParam>` to store 2-dimensional floating point

    * :doc:`Double3DParam<PythonReference/NatronEngine/Double3DParam>` to store 3-dimensional floating point

    * :doc:`BooleanParam<PythonReference/NatronEngine/BooleanParam>` to store 1-dimensional boolean (checkbox)

    * :doc:`ButtonParam<PythonReference/NatronEngine/ButtonParam>` to add a push-button

    * :doc:`ChoiceParam<PythonReference/NatronEngine/ChoiceParam>` a 1-dimensional drop-down (combobox)

    * :doc:`StringParam<PythonReference/NatronEngine/DoubleParam>` to store a 1-dimensional string

    * :doc:`FileParam<PythonReference/NatronEngine/FileParam>` to specify an input-file

    * :doc:`OutputFileParam<PythonReference/NatronEngine/OutputFileParam>` to specify an output-file param

    * :doc:`PathParam<PythonReference/NatronEngine/PathParam>` to specify a path to a single or multiple directories

    * :doc:`ParametricParam<PythonReference/NatronEngine/ParametricParam>` to store N-dimensional parametric curves

    * :doc:`GroupParam<PythonReference/NatronEngine/GroupParam>` to graphically gather parameters under a group

    * :doc:`PageParam<PythonReference/NatronEngine/PageParam>` to store parameters into a page


Retrieving a parameter's value:
--------------------------------


Since each underlying type is different for parameters, each sub-class has its own version
of the functions.

To get the value of the parameter at the timeline's current time, call the :func:`get()<>` or
:func:`getValue()<>` function.

If the parameter is animated and you want to retrieve its value at a specific time on the timeline,
you would use the :func:`get(frame)<>` or :func:`getValueAtTime(frame,dimension)<>` function.

Note that when animated and the given *frame* time is not a time at which a keyframe exists,
Natron will interpolate the value of the parameter between surrounding keyframes with the
interpolation filter selected (by default it is *smooth*).


Modifying a parameter's value:
------------------------------


You would set the parameter value by calling the :func:`set(value)<>` or :func:`setValue(value)<>` function.
If the parameter is animated (= has 1 or more keyframe) then calling this function would
create (or modify) a keyframe at the timeline's current time.

To add a new keyframe the :func:`set(value,frame)<>` or :func:`setValueAtTime(value,frame,dimension)<>` function can be used.

To remove a keyframe you can use the :func:`deleteValueAtTime(frame,dimension)<>` function.
If you want to remove all the animation on the parameter at a given *dimension*, use the
:func:`removeAnimation(dimension)<>` function.

.. warning::

    Note that the dimension is a 0-based index referring to the dimension on which to operate.
    For instance a :doc:`Double2DParam<PythonReference/NatronEngine/Double2DParam>` has 2 dimensions *x* and *y*.
    To set a value on **x** you would use *dimension = 0*, to set a value on **y** you would use *dimension = 1*.


Controlling other properties of parameters:
-------------------------------------------

See the documentation for the :doc:`Param<PythonReference/NatronEngine/Param>` class for a detailed
explanation of other properties and how they affect the parameter.

Creating new parameters:
------------------------

In Natron, the user has the possibility to add new parameters, called *User parameters*.
They are pretty much the same than the parameters defined by the underlying OpenFX plug-in itself.

In the Python API, to create a new *user parameter*, you would need to call one of the
:func:`createXParam(name,label,...)<>` of the :doc:`PythonReference/NatronEngine/Effect` class.

These parameters can have their default values and properties changed as explained in the
documentation page of the :doc:`PythonReference/NatronEngine/Param` class.

To remove a user created parameter you would need to call the :func:`removeParam(param)<>` function
of the :doc:`PythonReference/NatronEngine/Effect` class.

.. warning::

    Only **user parameters** can be removed. Removing parameters defined by the OpenFX plug-in
    will not work.
