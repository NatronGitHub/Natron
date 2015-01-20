.. module:: NatronEngine
.. _Roto:

Roto
****

.. inheritance-diagram:: Roto
    :parts: 2

Synopsis
--------

Functions
^^^^^^^^^
.. container:: function_list

*    def :meth:`createBezier<NatronEngine.Roto.createBezier>` (x, y, time)
*    def :meth:`createEllipse<NatronEngine.Roto.createEllipse>` (x, y, diameter, fromCenter, time)
*    def :meth:`createLayer<NatronEngine.Roto.createLayer>` ()
*    def :meth:`createRectangle<NatronEngine.Roto.createRectangle>` (x, y, size, time)
*    def :meth:`getBaseLayer<NatronEngine.Roto.getBaseLayer>` ()
*    def :meth:`getItemByName<NatronEngine.Roto.getItemByName>` (name)


Detailed Description
--------------------






.. method:: NatronEngine.Roto.createBezier(x, y, time)


    :param x: :class:`PySide.QtCore.double`
    :param y: :class:`PySide.QtCore.double`
    :param time: :class:`PySide.QtCore.int`
    :rtype: :class:`NatronEngine.BezierCurve`






.. method:: NatronEngine.Roto.createEllipse(x, y, diameter, fromCenter, time)


    :param x: :class:`PySide.QtCore.double`
    :param y: :class:`PySide.QtCore.double`
    :param diameter: :class:`PySide.QtCore.double`
    :param fromCenter: :class:`PySide.QtCore.bool`
    :param time: :class:`PySide.QtCore.int`
    :rtype: :class:`NatronEngine.BezierCurve`






.. method:: NatronEngine.Roto.createLayer()


    :rtype: :class:`NatronEngine.Layer`






.. method:: NatronEngine.Roto.createRectangle(x, y, size, time)


    :param x: :class:`PySide.QtCore.double`
    :param y: :class:`PySide.QtCore.double`
    :param size: :class:`PySide.QtCore.double`
    :param time: :class:`PySide.QtCore.int`
    :rtype: :class:`NatronEngine.BezierCurve`






.. method:: NatronEngine.Roto.getBaseLayer()


    :rtype: :class:`NatronEngine.Layer`






.. method:: NatronEngine.Roto.getItemByName(name)


    :param name: :class:`NatronEngine.std::string`
    :rtype: :class:`NatronEngine.ItemBase`







