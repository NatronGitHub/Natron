.. module:: NatronEngine
.. _App:

App
***

.. inheritance-diagram:: App
    :parts: 2

**Inherited by:** :ref:`GuiApp`

Synopsis
--------

Functions
^^^^^^^^^
.. container:: function_list

*    def :meth:`createNode<NatronEngine.App.createNode>` (pluginID[, majorVersion=-1[, group=None]])
*    def :meth:`getAppID<NatronEngine.App.getAppID>` ()
*    def :meth:`getProjectParam<NatronEngine.App.getProjectParam>` (name)
*    def :meth:`getSettings<NatronEngine.App.getSettings>` ()
*    def :meth:`render<NatronEngine.App.render>` (task)
*    def :meth:`render<NatronEngine.App.render>` (tasks)
*    def :meth:`timelineGetLeftBound<NatronEngine.App.timelineGetLeftBound>` ()
*    def :meth:`timelineGetRightBound<NatronEngine.App.timelineGetRightBound>` ()
*    def :meth:`timelineGetTime<NatronEngine.App.timelineGetTime>` ()


Detailed Description
--------------------






.. method:: NatronEngine.App.createNode(pluginID[, majorVersion=-1[, group=None]])


    :param pluginID: :class:`NatronEngine.std::string`
    :param majorVersion: :class:`PySide.QtCore.int`
    :param group: :class:`NatronEngine.Group`
    :rtype: :class:`NatronEngine.Effect`






.. method:: NatronEngine.App.getAppID()


    :rtype: :class:`PySide.QtCore.int`






.. method:: NatronEngine.App.getProjectParam(name)


    :param name: :class:`NatronEngine.std::string`
    :rtype: :class:`NatronEngine.Param`






.. method:: NatronEngine.App.getSettings()


    :rtype: :class:`NatronEngine.AppSettings`






.. method:: NatronEngine.App.render(task)


    :param task: :class:`NatronEngine.RenderTask`






.. method:: NatronEngine.App.render(tasks)


    :param tasks: 






.. method:: NatronEngine.App.timelineGetLeftBound()


    :rtype: :class:`PySide.QtCore.int`






.. method:: NatronEngine.App.timelineGetRightBound()


    :rtype: :class:`PySide.QtCore.int`






.. method:: NatronEngine.App.timelineGetTime()


    :rtype: :class:`PySide.QtCore.int`







