.. module:: NatronEngine
.. _ImageLayer:

ImageLayer
***********


Synopsis
--------

A small object representing a layer of an image. For example, the base image layer is
the color layer, or sometimes called "RGBA". Some other default layers include
ForwardMotion, BackwardMotion, DisparityLeft,DisparityRight, etc...
.

See :ref:`detailed<ImageLayer.details>` description...

Functions
^^^^^^^^^

- def :meth:`ImageLayer<NatronEngine.ImageLayer.ImageLayer>` (layerName,componentsPrettyName,componentsName)
- def :meth:`isColorPlane<NatronEngine.ImageLayer.isColorPlane>` ()
- def :meth:`getNumComponents<NatronEngine.ImageLayer.getNumComponents>` ()
- def :meth:`getLayerName<NatronEngine.ImageLayer.getLayerName>` ()
- def :meth:`getComponentsNames<NatronEngine.ImageLayer.getComponentsNames>` ()
- def :meth:`getComponentsPrettyName<NatronEngine.ImageLayer.getComponentsPrettyName>` ()
- def :meth:`getNoneComponents<NatronEngine.ImageLayer.getNoneComponents>` ()
- def :meth:`getRGBAComponents<NatronEngine.ImageLayer.getRGBAComponents>` ()
- def :meth:`getRGBComponents<NatronEngine.ImageLayer.getRGBComponents>` ()
- def :meth:`getAlphaComponents<NatronEngine.ImageLayer.getAlphaComponents>` ()
- def :meth:`getBackwardMotionComponents<NatronEngine.ImageLayer.getBackwardMotionComponents>` ()
- def :meth:`getForwardMotionComponents<NatronEngine.ImageLayer.getForwardMotionComponents>` ()
- def :meth:`getDisparityLeftComponents<NatronEngine.ImageLayer.getDisparityLeftComponents>` ()
- def :meth:`getDisparityRightComponents<NatronEngine.ImageLayer.getDisparityRightComponents>` ()

.. _ImageLayer.details:

Detailed Description
--------------------

A Layer is constituted of a layer *name* and a set of channel names (also called components).
You can get a sequence with all the channels in the layer with the function :func:`getComponentsNames()<NatronEngine.ImageLayer.getComponentsNames>`.
For some default layers, the components may be represented by a prettier name for the end-user,
such as *DisparityLeft* instead of XY.
When the ImageLayer does not have a pretty name, its pretty name will just be a concatenation
of all channel names in order.

There is one special layer in Natron: the color layer. It be represented as 3 different types:
RGBA, RGB or Alpha. If the ImageLayer is a color layer, the method :func:`isColorPlane()<NatronEngine.ImageLayer.isColorPlane>` will
return True


Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^



.. method:: NatronEngine.ImageLayer.ImageLayer(layerName,componentsPrettyName,componentsName)


    :param layerName: :class:`str<NatronEngine.std::string>`

    Make a new image layer with the given layer name, optional components pretty name and
    the set of channels (also called components) in the layer.

.. method:: NatronEngine.ImageLayer.isColorPlane()


    :rtype: :class:`bool<PySide.QtCore.bool>`

    Returns True if this layer is a color layer, i.e: it is RGBA, RGB or alpha.
    The color layer is what is output by default by all nodes in Natron.


.. method:: NatronEngine.ImageLayer.getNumComponents()


    :rtype: :class:`int<PySide.QtCore.int>`

    Returns the number of channels in this layer. Can be between 0 and 4 included.

.. method:: NatronEngine.ImageLayer.getLayerName()

    :rtype: :class:`str<NatronEngine.std::string>`

    Returns the layer name

.. method:: NatronEngine.ImageLayer.getComponentsNames()

    :rtype: :class:`Sequence`

    Returns a sequence with all channels in this layer in order

.. method:: NatronEngine.ImageLayer.getComponentsPrettyName()

    :rtype: :class:`str<NatronEngine.std::string>`

    Returns the channels pretty name. E.g: DisparityLeft instead of XY

.. method:: NatronEngine.ImageLayer.getNoneComponents()

    :rtype: :class:`ImageLayer<NatronEngine.ImageLayer>`

    Returns the default "none" layer

.. method:: NatronEngine.ImageLayer.getRGBAComponents()

    :rtype: :class:`ImageLayer<NatronEngine.ImageLayer>`

    Returns the default "RGBA" layer

.. method:: NatronEngine.ImageLayer.getRGBComponents()

    :rtype: :class:`ImageLayer<NatronEngine.ImageLayer>`

    Returns the default "RGB" layer

.. method:: NatronEngine.ImageLayer.getAlphaComponents()

    :rtype: :class:`ImageLayer<NatronEngine.ImageLayer>`

    Returns the default "Alpha" layer

.. method:: NatronEngine.ImageLayer.getBackwardMotionComponents()

    :rtype: :class:`ImageLayer<NatronEngine.ImageLayer>`

    Returns the default "Backward" layer

.. method:: NatronEngine.ImageLayer.getForwardMotionComponents()

    :rtype: :class:`ImageLayer<NatronEngine.ImageLayer>`

    Returns the default "Forward" layer

.. method:: NatronEngine.ImageLayer.getDisparityLeftComponents()

    :rtype: :class:`ImageLayer<NatronEngine.ImageLayer>`

    Returns the default "DisparityLeft" layer

.. method:: NatronEngine.ImageLayer.getDisparityRightComponents()

    :rtype: :class:`ImageLayer<NatronEngine.ImageLayer>`

    Returns the default "DisparityRight" layer
