.. _roto:

Using the roto functionalities
==============================

All rotoscoping functionalities are gathered in the :ref:`Roto<Roto>` class.
For now, only the roto node can have a :ref:`Roto<Roto>` object.
The :ref:`Roto<Roto>`object is :ref:`auto-declared<autoVar>` by Natron and can be accessed
as an attribute of the roto node::

    app.Roto1.roto

:ref:`Beziers<BezierCurve>` and :ref:`layers<ItemBase>` can be accessed via their script-name directly::

    app.Roto1.roto.Layer1.Bezier1

The *script-name* of the roto items can be found in the :ref:`settings panel<rotoScriptName>` of the Roto node.


Moving items within layers
---------------------------

In Natron, all the items in a layer are rendered from top to bottom, meaning the bottom-most items will always
appear on top of the others.

You can re-organize the tree using the functions available in the :ref:`Layer<Layer>` class.

.. warning::

    Removing an item from a layer or inserting it in a layer will change the auto-declared variable, e.g::

        fromLayer = app.Roto1.roto.Layer1
        toLayer = app.Roto1.roto.Layer2
        item = app.Roto1.roto.Layer1.Bezier1
        toLayer.addItem(item)

        #Now item is referenced from app.Roto1.roto.Layer2.Bezier1

Creating layers
----------------

To create a new :ref:`BezierCurve<BezierCurve>`, use the :func:`createLayer()<>` function made available by the :ref:`Roto<Roto>` class.

Creating shapes
----------------

To create a new :ref:`BezierCurve<BezierCurve>`, use one of the following functions made available by the :ref:`Roto<Roto>` class:

    * :func:`createBezier(x,y,time)<>`
    * :func:`createEllipse(x,y,diameter,fromCenter,time)<>`
    * :func:`createRectangle(x,y,size,time)<>`

Once created, the Bezier will have at least 1 control point (4 for ellipses and rectangles) and one keyframe
at the time specified in parameter.
