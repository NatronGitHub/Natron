.. _devel-faq:

Natron Python FAQ
=================

Here are a few frequently asked questions.

There may be more answers in the `Natron forum <https://discuss.pixls.us/tags/natron-python>`_, especially in the `All About Natron Python Scripting <https://discuss.pixls.us/t/all-about-natron-python-scripting/13031>`_ topic.

Q: How can I get the location of the current Natron executable?
::

    import sys
    print(sys.executable)

Q: How can I get all widgets from a :doc:`modal dialog <PythonReference/NatronGui/PyModalDialog>`?

:doc:`PythonReference/NatronGui/PyModalDialog` inherits from `Qdialog <https://pyside.github.io/docs/pyside/PySide/QtGui/QDialog.html>`_, which inherits from `QObject <https://pyside.github.io/docs/pyside/PySide/QtCore/QObject.html>`_, which has a `QObject::children() <https://pyside.github.io/docs/pyside/PySide/QtCore/QObject.html#PySide.QtCore.PySide.QtCore.QObject.children>`_ method.

