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

:doc:`PythonReference/NatronGui/PyModalDialog` inherits from `QDialog <https://doc.qt.io/qtforpython-6/PySide6/QtWidgets/QDialog.html>`_, which inherits from `QObject <https://doc.qt.io/qtforpython-6/PySide6/QtCore/QObject.html>`_, which has a `children() <https://doc.qt.io/qtforpython-6/PySide6/QtCore/QObject.html#PySide6.QtCore.QObject.children>`_ method.

