.. module:: NatronEngine
.. _RenderTask:

RenderTask
**********


Detailed Description
--------------------

A utility class used as an argument of the :func:`render(task)<NatronEngine.App.render>` function.


Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. class:: RenderTask()
           RenderTask(writeNode, firstFrame, lastFrame)

    :param writeNode: :class:`Effect<NatronEngine.Effect>`
    :param firstFrame: :class:`int<PySide.QtCore.int>`
    :param lastFrame: :class:`int<PySide.QtCore.int>`

Constructs a new RenderTask object. The *writeNode* must point either to a Write node or
to a DiskCache node. The *firstFrame* and *lastFrame* indicates the range to render (including them).


