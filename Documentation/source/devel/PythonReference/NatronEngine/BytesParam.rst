.. currentmodule:: NatronEngine
.. _BytesParam:

BytesParam
**********

**Inherits** :doc:`Param`

Synopsis
--------

A Bytes parameter holds opaque binary data defined by an OpenFX plug-in (OFX 1.5
``kOfxParamTypeBytes``). It has no user interface, and it cannot be animated.
Its value is a Python ``bytes`` object, which may contain any byte, including NUL bytes::

    param = node.getParam("data")
    data = param.getValue()        # bytes
    param.setValue(b"\x00\x01\x02") # any bytes-like object: bytes, bytearray...

Only plug-ins create Bytes parameters: there is no ``createBytesParam()``.

Functions
^^^^^^^^^

- def :meth:`getValue<NatronEngine.BytesParam.getValue>` ()
- def :meth:`setValue<NatronEngine.BytesParam.setValue>` (value)
- def :meth:`getDefaultValue<NatronEngine.BytesParam.getDefaultValue>` ()
- def :meth:`restoreDefaultValue<NatronEngine.BytesParam.restoreDefaultValue>` ()

Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. method:: NatronEngine.BytesParam.getValue()

    :rtype: :class:`bytes`

Returns the value of the parameter.

.. method:: NatronEngine.BytesParam.setValue(value)

    :param value: :class:`bytes`

Sets the value of the parameter. *value* may be any bytes-like object (``bytes``,
``bytearray``, ``memoryview``...); a ``str`` raises :class:`TypeError`. A value larger
than about 1.5 GB raises :class:`MemoryError`.

.. method:: NatronEngine.BytesParam.getDefaultValue()

    :rtype: :class:`bytes`

Returns the default value of the parameter, as defined by the plug-in.

.. method:: NatronEngine.BytesParam.restoreDefaultValue()

Restores the default value of the parameter.
