.. module:: NatronEngine
.. _PageParam:

PageParam
*********

**Inherits** :doc:`Param`

Synopsis
--------

A page param is a container for other parameters.
See :ref:`detailed<page.details>` description.

Functions
^^^^^^^^^

- def :meth:`addParam<NatronEngine.PageParam.addParam>` (param)

.. _page.details:

Detailed Description
--------------------

A page param does not hold any relevant value. Rather this is a purely graphical
element that is used to gather parameters under a tab.
On the graphical interface a PageParam looks like this (e.g. the *Controls* tab of the panel)

.. figure:: pageParam.png
    :width: 400px
    :align: center


.. warning::

    All parameters **MUST** be in a container, being a :doc:`group<GroupParam>` or a :doc:`page<PageParam>`.
    If a :doc:`Param` is not added into any container, Natron will add it by default to the
    *User* page.




.. method:: NatronEngine.PageParam.addParam(param)


    :param param: :class:`Param<NatronEngine.Param>`

   Adds *param* into the page.


.. warning::

    Note that this function cannot be called on pages that are not user parameters (i.e: created
    either by script or by the "Manage user parameters" user interface)


.. warning::

    Once called, you should call :func:`refreshUserParamsGUI()<NatronEngine.Effect.refreshUserParamsGUI>`
    to update the user interface.





