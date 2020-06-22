.. module:: NatronGui
.. _pyTabWidget:

PyTabWidget
************


Synopsis
-------------

A PyTabWidget is one of the GUI pane onto which the user can dock tabs such as the
NodeGraph, CurveEditor...
See :ref:`detailed<pyTabWidget.details>` description...

Functions
^^^^^^^^^

- def :meth:`appendTab<NatronGui.PyTabWidget.appendTab>` (tab)
- def :meth:`closeCurrentTab<NatronGui.PyTabWidget.closeCurrentTab>` ()
- def :meth:`closeTab<NatronGui.PyTabWidget.closeTab>` (index)
- def :meth:`closePane<NatronGui.PyTabWidget.closePane>` ()
- def :meth:`count<NatronGui.PyTabWidget.count>` ()
- def :meth:`currentWidget<NatronGui.PyTabWidget.currentWidget>` ()
- def :meth:`floatCurrentTab<NatronGui.PyTabWidget.floatCurrentTab>` ()
- def :meth:`floatPane<NatronGui.PyTabWidget.floatPane>` ()
- def :meth:`getCurrentIndex<NatronGui.PyTabWidget.getCurrentIndex>` ()
- def :meth:`getScriptName<NatronGui.PyTabWidget.getScriptName>` ()
- def :meth:`getTabLabel<NatronGui.PyTabWidget.getTabLabel>` (index)
- def :meth:`insertTab<NatronGui.PyTabWidget.insertTab>` (index,tab)
- def :meth:`removeTab<NatronGui.PyTabWidget.removeTab>` (tab)
- def :meth:`removeTab<NatronGui.PyTabWidget.removeTab>` (index)
- def :meth:`setCurrentIndex<NatronGui.PyTabWidget.setCurrentIndex>` (index)
- def :meth:`setNextTabCurrent<NatronGui.PyTabWidget.setNextTabCurrent>` ()
- def :meth:`splitHorizontally<NatronGui.PyTabWidget.splitHorizontally>` ()
- def :meth:`splitVertically<NatronGui.PyTabWidget.splitVertically>` ()

.. _pyTabWidget.details:

Detailed Description
---------------------------

The :doc:`PyTabWidget` class is used to represent panes visible in the user interface:

.. figure:: ../../tabwidgets.png
    :width: 500px
    :align: center

On the screenshot above, each :doc:`PyTabWidget` is surrounded by a red box.

You cannot construct tab widgets on your own, you must call one of the
:func:`splitVertically()<NatronGui.PyTabWidget.splitVertically>` or
:func:`splitHorizontally()<NatronGui.PyTabWidget.splitHorizontally>` functions to make a new
one based on another existing ones.

By default the GUI of Natron cannot have less than 1 tab widget active, hence you can always
split it to make new panes.

To retrieve an existing :doc:`PyTabWidget` you can call the :func:`getTabWidget(scriptName)<NatronGui.GuiApp.getTabWidget>`
function of :doc:`GuiApp`.

    pane1 = app.getTabWidget("Pane1")

Note that the script-name of a pane can be seen on the graphical user interface by hovering
the mouse over the "Manage layout" button (in the top left hand corner of a pane).

.. figure:: ../../paneScriptName.png
    :width: 300px
    :align: center


Managing tabs
^^^^^^^^^^^^^^

To insert tabs in the TabWidget you can call either :func:`appendTab(tab)<NatronGui.PyTabWidget.appendTab>`
or :func:`insertTab(index,tab)<NatronGui.PyTabWidget.insertTab>`.

.. warning::

    Note that to insert a tab, it must be first removed from the tab into which it was.

To remove a tab, use the function :func:`removeTab(tab)<NatronGui.PyTabWidget.removeTab>` on the parent :doc:`PyTabWidget`

For convenience to move tabs around, there is a  :func:`moveTab(tab,pane)<NatronGui.GuiApp.moveTab>`
function in :doc:`GuiApp`.

The function :func:`closeTab(index)<NatronGui.PyTabWidget.closeTab>` can be used to close permanently
a tab, effectively destroying it.

To change the current tab, you can use one of the following functions:

    * `setCurrentIndex(index)<NatronGui.PyTabWidget.setCurrentIndex>`
    * `setNextTabCurrent()<NatronGui.PyTabWidget.setNextTabCurrent>`

To float the current tab into a new floating window, use the `floatCurrentTab()<NatronGui.PyTabWidget.floatCurrentTab>` function.

Managing the pane
^^^^^^^^^^^^^^^^^^^

To close the pane permanently, use the `closePane()<NatronGui.PyTabWidget.closePane>` function.
To float the pane into a new floating window with all its tabs, use the :func:`floatPane()<NatronGui.PyTabWidget.floatPane>` function.

Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. method:: NatronGui.PyTabWidget.appendTab(tab)

    :param tab: `QWidget <https://pyside.github.io/docs/pyside/PySide/QtGui/QWidget.html>`

Appends a new tab to the tab widget and makes it current.



.. method:: NatronGui.PyTabWidget.closeCurrentTab()

Closes the current tab, effectively destroying it.


.. method:: NatronGui.PyTabWidget.closeTab(index)

Closes the tab at the given *index*, effectively destroying it.


.. method:: NatronGui.PyTabWidget.closePane()

Closes this pane, effectively destroying it. Note that all tabs will not be destroyed but instead
moved to another existing pane.

.. warning::

    If this pane is the last one on the GUI, this function does nothing.


.. method:: NatronGui.PyTabWidget.count()

    :rtype: :class:`int`

Returns the number of tabs in this pane.



.. method:: NatronGui.PyTabWidget.currentWidget()

    :rtype: `QWidget <https://pyside.github.io/docs/pyside/PySide/QtGui/QWidget.html>`

Returns the current active tab.


.. method:: NatronGui.PyTabWidget.floatCurrentTab()

Make a new floating window with a single pane and moves the current tab of this pane to
the new pane of the floating window.



.. method:: NatronGui.PyTabWidget.floatPane()

Make a new floating window and moves this pane to the new window (including all tabs).


.. method:: NatronGui.PyTabWidget.getCurrentIndex()

    :rtype: :class:`int`

Returns the index of the current tab. This is 0-based (starting from the left).


.. method:: NatronGui.PyTabWidget.getScriptName()

    :rtype: :class:`str`

Returns the script-name of the pane, as used by the :func:`getTabWidget(scriptName)<NatronGui.GuiApp.getTabWidget>` function.



.. method:: NatronGui.PyTabWidget.getTabLabel(index)

    :param index: :class:`int`
    :rtype: :class:`str`

Returns the name of the tab at the given *index* if it exists or an empty string otherwise.


.. method:: NatronGui.PyTabWidget.insertTab(index,tab)

    :param tab: `QWidget <https://pyside.github.io/docs/pyside/PySide/QtGui/QWidget.html>`
    :param index: :class:`int`

Inserts the given *tab* at the given *index* in this tab-widget.


.. method:: NatronGui.PyTabWidget.removeTab(tab)

    :param tab: `QWidget <https://pyside.github.io/docs/pyside/PySide/QtGui/QWidget.html>`

Removes the given *tab* from this pane if it is found. Note that this function
does not destroy the *tab*, unlike :func:`closeTab(index)<NatronGui.PyTabWidget.closeTab>`.

This is used internally by :func:`moveTab(tab,pane)<NatronGui.GuiApp.moveTab>`.

.. method:: NatronGui.PyTabWidget.removeTab(index)

    :param index: :class:`int`

Same as :func:`removeTab(tab)<NatronGui.PyTabWidget.removeTab>` but the *index* of a tab
is given instead.


.. method:: NatronGui.PyTabWidget.setCurrentIndex(index)

    :param index: :class:`int`

Makes the tab at the given *index* (0-based) the current one (if the index is valid).


.. method:: NatronGui.PyTabWidget.setNextTabCurrent()

Set the tab at :func:`getCurrentIndex()<NatronGui.PyTabWidget.getCurrentIndex>` + 1 the current one.
This functions cycles back to the first tab once the last tab is reached.



.. method:: NatronGui.PyTabWidget.splitHorizontally()

    :rtype: :class:`PyTabWidget<NatronGui.PyTabWidget>`

Splits this pane into 2 horizontally-separated panes. The new pane will be returned.


.. method:: NatronGui.PyTabWidget.splitVertically()

    :rtype: :class:`PyTabWidget<NatronGui.PyTabWidget>`

Splits this pane into 2 vertically-separated panes. The new pane will be returned.
