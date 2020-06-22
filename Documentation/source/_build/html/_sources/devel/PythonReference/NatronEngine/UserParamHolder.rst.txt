.. module:: NatronEngine

.. _UserParamHolder:

UserParamHolder
***************

**Inherited by** : :doc:`Effect`, :doc:`../NatronGui/PyModalDialog`

Synopsis
--------

This is an abstract class that serves as a base interface for all objects that can hold
user parameters.
See :ref:`userParams.details`

Functions
^^^^^^^^^

- def :meth:`createBooleanParam<NatronEngine.UserParamHolder.createBooleanParam>` (name, label)
- def :meth:`createButtonParam<NatronEngine.UserParamHolder.createButtonParam>` (name, label)
- def :meth:`createChoiceParam<NatronEngine.UserParamHolder.createChoiceParam>` (name, label)
- def :meth:`createColorParam<NatronEngine.UserParamHolder.createColorParam>` (name, label, useAlpha)
- def :meth:`createDouble2DParam<NatronEngine.UserParamHolder.createDouble2DParam>` (name, label)
- def :meth:`createDouble3DParam<NatronEngine.UserParamHolder.createDouble3DParam>` (name, label)
- def :meth:`createDoubleParam<NatronEngine.UserParamHolder.createDoubleParam>` (name, label)
- def :meth:`createFileParam<NatronEngine.UserParamHolder.createFileParam>` (name, label)
- def :meth:`createGroupParam<NatronEngine.UserParamHolder.createGroupParam>` (name, label)
- def :meth:`createInt2DParam<NatronEngine.UserParamHolder.createInt2DParam>` (name, label)
- def :meth:`createInt3DParam<NatronEngine.UserParamHolder.createInt3DParam>` (name, label)
- def :meth:`createIntParam<NatronEngine.UserParamHolder.createIntParam>` (name, label)
- def :meth:`createOutputFileParam<NatronEngine.UserParamHolder.createOutputFileParam>` (name, label)
- def :meth:`createPageParam<NatronEngine.UserParamHolder.createPageParam>` (name, label)
- def :meth:`createParametricParam<NatronEngine.UserParamHolder.createParametricParam>` (name, label, nbCurves)
- def :meth:`createPathParam<NatronEngine.UserParamHolder.createPathParam>` (name, label)
- def :meth:`createStringParam<NatronEngine.UserParamHolder.createStringParam>` (name, label)
- def :meth:`removeParam<NatronEngine.UserParamHolder.removeParam>` (param)
- def :meth:`refreshUserParamsGUI<NatronEngine.UserParamHolder.refreshUserParamsGUI>` ()

.. _userParams.details:

Detailed Description
--------------------

To create a new user :doc:`parameter<Param>` on the object, use one of the **createXParam**
function. To remove a user parameter created, use the :func:`removeParam(param)<NatronEngine.UserParamHolder.removeParam>`
function. Note that this function can only be used to remove **user parameters** and cannot
be used to remove parameters that were defined by the OpenFX plug-in.

Once you have made modifications to the user parameters, you must call the
:func:`refreshUserParamsGUI()<NatronEngine.UserParamHolder.refreshUserParamsGUI>` function to notify
the GUI, otherwise no change will appear on the GUI.


Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^


.. method:: NatronEngine.UserParamHolder.createBooleanParam(name, label)


    :param name: :class:`str<NatronEngine.std::string>`
    :param label: :class:`str<NatronEngine.std::string>`
    :rtype: :class:`BooleanParam<NatronEngine.BooleanParam>`


Creates a new user :doc:`parameter<Param>` with the given *name* and *label*. See
:ref:`here<autoVar>` for an explanation of the difference between the *name* and *label*.
This function will return a new parameter of type boolean which will appear in the user
interface as a checkbox.

.. warning::

    After calling this function you should call :func:`refreshUserParamsGUI()<NatronEngine.UserParamHolder.refreshUserParamsGUI>`
    to refresh the user interface. The refreshing is done in a separate function because it may
    be expensive and thus allows you to make multiple changes to user parameters at once
    while keeping the user interface responsive.

.. method:: NatronEngine.UserParamHolder.createButtonParam(name, label)


    :param name: :class:`str<NatronEngine.std::string>`
    :param label: :class:`str<NatronEngine.std::string>`
    :rtype: :class:`ButtonParam<NatronEngine.ButtonParam>`


Creates a new user :doc:`parameter<Param>` with the given *name* and *label*. See
:ref:`here<autoVar>` for an explanation of the difference between the *name* and *label*.
This function will return a new parameter of type button which will appear as a
push button. Use the onParamChanged callback of the Effect to handle user clicks.

.. warning::

    After calling this function you should call :func:`refreshUserParamsGUI()<NatronEngine.UserParamHolder.refreshUserParamsGUI>`
    to refresh the user interface. The refreshing is done in a separate function because it may
    be expensive and thus allows you to make multiple changes to user parameters at once
    while keeping the user interface responsive.



.. method:: NatronEngine.UserParamHolder.createChoiceParam(name, label)


    :param name: :class:`str<NatronEngine.std::string>`
    :param label: :class:`str<NatronEngine.std::string>`
    :rtype: :class:`ChoiceParam<NatronEngine.ChoiceParam>`

Creates a new user :doc:`parameter<Param>` with the given *name* and *label*. See
:ref:`here<autoVar>` for an explanation of the difference between the *name* and *label*.
This function will return a new parameter of type choice which will appear as a
dropdown combobox.

.. warning::

    After calling this function you should call :func:`refreshUserParamsGUI()<NatronEngine.UserParamHolder.refreshUserParamsGUI>`
    to refresh the user interface. The refreshing is done in a separate function because it may
    be expensive and thus allows you to make multiple changes to user parameters at once
    while keeping the user interface responsive.


.. method:: NatronEngine.UserParamHolder.createColorParam(name, label, useAlpha)


    :param name: :class:`str<NatronEngine.std::string>`
    :param label: :class:`str<NatronEngine.std::string>`
    :param useAlpha: :class:`bool<PySide.QtCore.bool>`
    :rtype: :class:`ColorParam<NatronEngine.ColorParam>`

Creates a new user :doc:`parameter<Param>` with the given *name* and *label*. See
:ref:`here<autoVar>` for an explanation of the difference between the *name* and *label*.
This function will return a new parameter of type color.

.. warning::

    After calling this function you should call :func:`refreshUserParamsGUI()<NatronEngine.UserParamHolder.refreshUserParamsGUI>`
    to refresh the user interface. The refreshing is done in a separate function because it may
    be expensive and thus allows you to make multiple changes to user parameters at once
    while keeping the user interface responsive.


.. method:: NatronEngine.UserParamHolder.createDouble2DParam(name, label)


    :param name: :class:`str<NatronEngine.std::string>`
    :param label: :class:`str<NatronEngine.std::string>`
    :rtype: :class:`Double2DParam<NatronEngine.Double2DParam>`

Creates a new user :doc:`parameter<Param>` with the given *name* and *label*. See
:ref:`here<autoVar>` for an explanation of the difference between the *name* and *label*.
This function will return a new parameter of type double with 2 dimensions.


.. warning::

    After calling this function you should call :func:`refreshUserParamsGUI()<NatronEngine.UserParamHolder.refreshUserParamsGUI>`
    to refresh the user interface. The refreshing is done in a separate function because it may
    be expensive and thus allows you to make multiple changes to user parameters at once
    while keeping the user interface responsive.

.. method:: NatronEngine.UserParamHolder.createDouble3DParam(name, label)


    :param name: :class:`str<NatronEngine.std::string>`
    :param label: :class:`str<NatronEngine.std::string>`
    :rtype: :class:`Double3DParam<NatronEngine.Double3DParam>`


Creates a new user :doc:`parameter<Param>` with the given *name* and *label*. See
:ref:`here<autoVar>` for an explanation of the difference between the *name* and *label*.
This function will return a new parameter of type double with 3 dimensions.

.. warning::

    After calling this function you should call :func:`refreshUserParamsGUI()<NatronEngine.UserParamHolder.refreshUserParamsGUI>`
    to refresh the user interface. The refreshing is done in a separate function because it may
    be expensive and thus allows you to make multiple changes to user parameters at once
    while keeping the user interface responsive.

.. method:: NatronEngine.UserParamHolder.createDoubleParam(name, label)


    :param name: :class:`str<NatronEngine.std::string>`
    :param label: :class:`str<NatronEngine.std::string>`
    :rtype: :class:`DoubleParam<NatronEngine.DoubleParam>`


Creates a new user :doc:`parameter<Param>` with the given *name* and *label*. See
:ref:`here<autoVar>` for an explanation of the difference between the *name* and *label*.
This function will return a new parameter of type double with single dimension.
A double is similar to a floating point value.

.. warning::

    After calling this function you should call :func:`refreshUserParamsGUI()<NatronEngine.UserParamHolder.refreshUserParamsGUI>`
    to refresh the user interface. The refreshing is done in a separate function because it may
    be expensive and thus allows you to make multiple changes to user parameters at once
    while keeping the user interface responsive.

.. method:: NatronEngine.UserParamHolder.createFileParam(name, label)


    :param name: :class:`str<NatronEngine.std::string>`
    :param label: :class:`str<NatronEngine.std::string>`
    :rtype: :class:`FileParam<NatronEngine.FileParam>`


Creates a new user :doc:`parameter<Param>` with the given *name* and *label*. See
:ref:`here<autoVar>` for an explanation of the difference between the *name* and *label*.
This function will return a new parameter of type double with 2 dimensions.

.. warning::

    After calling this function you should call :func:`refreshUserParamsGUI()<NatronEngine.UserParamHolder.refreshUserParamsGUI>`
    to refresh the user interface. The refreshing is done in a separate function because it may
    be expensive and thus allows you to make multiple changes to user parameters at once
    while keeping the user interface responsive.

.. method:: NatronEngine.UserParamHolder.createGroupParam(name, label)


    :param name: :class:`str<NatronEngine.std::string>`
    :param label: :class:`str<NatronEngine.std::string>`
    :rtype: :class:`GroupParam<NatronEngine.GroupParam>`


Creates a new user :doc:`parameter<Param>` with the given *name* and *label*. See
:ref:`here<autoVar>` for an explanation of the difference between the *name* and *label*.
This function will return a new parameter of type group. It can contain other
children parameters and can be expanded or folded.

.. warning::

    After calling this function you should call :func:`refreshUserParamsGUI()<NatronEngine.UserParamHolder.refreshUserParamsGUI>`
    to refresh the user interface. The refreshing is done in a separate function because it may
    be expensive and thus allows you to make multiple changes to user parameters at once
    while keeping the user interface responsive.

.. method:: NatronEngine.UserParamHolder.createInt2DParam(name, label)


    :param name: :class:`str<NatronEngine.std::string>`
    :param label: :class:`str<NatronEngine.std::string>`
    :rtype: :class:`Int2DParam<NatronEngine.Int2DParam>`

Creates a new user :doc:`parameter<Param>` with the given *name* and *label*. See
:ref:`here<autoVar>` for an explanation of the difference between the *name* and *label*.
This function will return a new parameter of type integer with 2 dimensions.

.. warning::

    After calling this function you should call :func:`refreshUserParamsGUI()<NatronEngine.UserParamHolder.refreshUserParamsGUI>`
    to refresh the user interface. The refreshing is done in a separate function because it may
    be expensive and thus allows you to make multiple changes to user parameters at once
    while keeping the user interface responsive.


.. method:: NatronEngine.UserParamHolder.createInt3DParam(name, label)


    :param name: :class:`str<NatronEngine.std::string>`
    :param label: :class:`str<NatronEngine.std::string>`
    :rtype: :class:`Int3DParam<NatronEngine.Int3DParam>`


Creates a new user :doc:`parameter<Param>` with the given *name* and *label*. See
:ref:`here<autoVar>` for an explanation of the difference between the *name* and *label*.
This function will return a new parameter of type integer with 3 dimensions.

.. warning::

    After calling this function you should call :func:`refreshUserParamsGUI()<NatronEngine.UserParamHolder.refreshUserParamsGUI>`
    to refresh the user interface. The refreshing is done in a separate function because it may
    be expensive and thus allows you to make multiple changes to user parameters at once
    while keeping the user interface responsive.

.. method:: NatronEngine.UserParamHolder.createIntParam(name, label)


    :param name: :class:`str<NatronEngine.std::string>`
    :param label: :class:`str<NatronEngine.std::string>`
    :rtype: :class:`IntParam<NatronEngine.IntParam>`

Creates a new user :doc:`parameter<Param>` with the given *name* and *label*. See
:ref:`here<autoVar>` for an explanation of the difference between the *name* and *label*.
This function will return a new parameter of type integer with a single dimension.

.. warning::

    After calling this function you should call :func:`refreshUserParamsGUI()<NatronEngine.UserParamHolder.refreshUserParamsGUI>`
    to refresh the user interface. The refreshing is done in a separate function because it may
    be expensive and thus allows you to make multiple changes to user parameters at once
    while keeping the user interface responsive.


.. method:: NatronEngine.UserParamHolder.createOutputFileParam(name, label)


    :param name: :class:`str<NatronEngine.std::string>`
    :param label: :class:`str<NatronEngine.std::string>`
    :rtype: :class:`OutputFileParam<NatronEngine.OutputFileParam>`

Creates a new user :doc:`parameter<Param>` with the given *name* and *label*. See
:ref:`here<autoVar>` for an explanation of the difference between the *name* and *label*.
This function will return a new parameter of type string dedicated to specify
paths to output files.

.. warning::

    After calling this function you should call :func:`refreshUserParamsGUI()<NatronEngine.UserParamHolder.refreshUserParamsGUI>`
    to refresh the user interface. The refreshing is done in a separate function because it may
    be expensive and thus allows you to make multiple changes to user parameters at once
    while keeping the user interface responsive.


.. method:: NatronEngine.UserParamHolder.createPageParam(name, label)


    :param name: :class:`str<NatronEngine.std::string>`
    :param label: :class:`str<NatronEngine.std::string>`
    :rtype: :class:`PageParam<NatronEngine.PageParam>`


Creates a new user :doc:`parameter<Param>` with the given *name* and *label*. See
:ref:`here<autoVar>` for an explanation of the difference between the *name* and *label*.
This function will return a new parameter of type page. A page is a tab within the
settings panel of the node.

.. warning::

    After calling this function you should call :func:`refreshUserParamsGUI()<NatronEngine.UserParamHolder.refreshUserParamsGUI>`
    to refresh the user interface. The refreshing is done in a separate function because it may
    be expensive and thus allows you to make multiple changes to user parameters at once
    while keeping the user interface responsive.

.. method:: NatronEngine.UserParamHolder.createParametricParam(name, label, nbCurves)


    :param name: :class:`str<NatronEngine.std::string>`
    :param label: :class:`str<NatronEngine.std::string>`
    :param nbCurves: :class:`int<PySide.QtCore.int>`
    :rtype: :class:`ParametricParam<NatronEngine.ParametricParam>`


Creates a new user :doc:`parameter<Param>` with the given *name* and *label*. See
:ref:`here<autoVar>` for an explanation of the difference between the *name* and *label*.
This function will return a new parameter of type parametric. A parametric parameter
is what can be found in the ColorLookup node or in the Ranges tab of the ColorCorrect
node.

.. warning::

    After calling this function you should call :func:`refreshUserParamsGUI()<NatronEngine.UserParamHolder.refreshUserParamsGUI>`
    to refresh the user interface. The refreshing is done in a separate function because it may
    be expensive and thus allows you to make multiple changes to user parameters at once
    while keeping the user interface responsive.

.. method:: NatronEngine.UserParamHolder.createPathParam(name, label)


    :param name: :class:`str<NatronEngine.std::string>`
    :param label: :class:`str<NatronEngine.std::string>`
    :rtype: :class:`PathParam<NatronEngine.PathParam>`


Creates a new user :doc:`parameter<Param>` with the given *name* and *label*. See
:ref:`here<autoVar>` for an explanation of the difference between the *name* and *label*.
This function will return a new parameter of type string. This parameter is dedicated
to specify path to single or multiple directories.

.. warning::

    After calling this function you should call :func:`refreshUserParamsGUI()<NatronEngine.UserParamHolder.refreshUserParamsGUI>`
    to refresh the user interface. The refreshing is done in a separate function because it may
    be expensive and thus allows you to make multiple changes to user parameters at once
    while keeping the user interface responsive.

.. method:: NatronEngine.UserParamHolder.createStringParam(name, label)


    :param name: :class:`str<NatronEngine.std::string>`
    :param label: :class:`str<NatronEngine.std::string>`
    :rtype: :class:`StringParam<NatronEngine.StringParam>`

Creates a new user :doc:`parameter<Param>` with the given *name* and *label*. See
:ref:`here<autoVar>` for an explanation of the difference between the *name* and *label*.
This function will return a new parameter of type string.

.. warning::

    After calling this function you should call :func:`refreshUserParamsGUI()<NatronEngine.UserParamHolder.refreshUserParamsGUI>`
    to refresh the user interface. The refreshing is done in a separate function because it may
    be expensive and thus allows you to make multiple changes to user parameters at once
    while keeping the user interface responsive.

.. method:: NatronEngine.UserParamHolder.removeParam(param)


    :param param: :class:`Param<NatronEngine.Param>`
    :rtype: :class:`bool<PySide.QtCore.bool>`

Removes the given *param* from the parameters of this Effect.
This function works only if *param* is a user parameter and does nothing otherwise.
This function returns True upon success and False otherwise.

.. warning::

    After calling this function you should call :func:`refreshUserParamsGUI()<NatronEngine.UserParamHolder.refreshUserParamsGUI>`
    to refresh the user interface. The refreshing is done in a separate function because it may
    be expensive and thus allows you to make multiple changes to user parameters at once
    while keeping the user interface responsive.




.. method:: NatronEngine.UserParamHolder.refreshUserParamsGUI()



This function must be called after new user parameter were created or removed.
This will re-create the user interface for the parameters and can be expensive.

