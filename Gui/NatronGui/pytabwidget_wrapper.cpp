
// default includes
#include "Global/Macros.h"
CLANG_DIAG_OFF(mismatched-tags)
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_OFF(missing-field-initializers)
GCC_DIAG_OFF(missing-declarations)
#include <shiboken.h> // produces many warnings
#include <pysidesignal.h>
#include <pysideproperty.h>
#include <pyside.h>
#include <typeresolver.h>
#include <typeinfo>
#include "natrongui_python.h"

#include "pytabwidget_wrapper.h"

// Extra includes
#include <PythonPanels.h>
#include <qwidget.h>



// Target ---------------------------------------------------------

extern "C" {
static PyObject* Sbk_PyTabWidgetFunc_appendTab(PyObject* self, PyObject* pyArg)
{
    ::PyTabWidget* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyTabWidget*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: appendTab(PyPanel*)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppPointerConvertible((SbkObjectType*)SbkNatronGuiTypes[SBK_PYPANEL_IDX], (pyArg)))) {
        overloadId = 0; // appendTab(PyPanel*)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyTabWidgetFunc_appendTab_TypeError;

    // Call function/method
    {
        if (!Shiboken::Object::isValid(pyArg))
            return 0;
        ::PyPanel* cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // appendTab(PyPanel*)
            // Begin code injection

            bool cppResult = cppSelf->appendTab(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_PyTabWidgetFunc_appendTab_TypeError:
        const char* overloads[] = {"NatronGui.PyPanel", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronGui.PyTabWidget.appendTab", overloads);
        return 0;
}

static PyObject* Sbk_PyTabWidgetFunc_closeCurrentTab(PyObject* self)
{
    ::PyTabWidget* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyTabWidget*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], (SbkObject*)self));

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // closeCurrentTab()
            cppSelf->closeCurrentTab();
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;
}

static PyObject* Sbk_PyTabWidgetFunc_closePane(PyObject* self)
{
    ::PyTabWidget* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyTabWidget*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], (SbkObject*)self));

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // closePane()
            cppSelf->closePane();
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;
}

static PyObject* Sbk_PyTabWidgetFunc_closeTab(PyObject* self, PyObject* pyArg)
{
    ::PyTabWidget* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyTabWidget*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: closeTab(int)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // closeTab(int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyTabWidgetFunc_closeTab_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // closeTab(int)
            cppSelf->closeTab(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_PyTabWidgetFunc_closeTab_TypeError:
        const char* overloads[] = {"int", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronGui.PyTabWidget.closeTab", overloads);
        return 0;
}

static PyObject* Sbk_PyTabWidgetFunc_count(PyObject* self)
{
    ::PyTabWidget* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyTabWidget*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // count()
            int cppResult = cppSelf->count();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_PyTabWidgetFunc_currentWidget(PyObject* self)
{
    ::PyTabWidget* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyTabWidget*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // currentWidget()
            QWidget * cppResult = cppSelf->currentWidget();
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkPySide_QtGuiTypes[SBK_QWIDGET_IDX], cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_PyTabWidgetFunc_floatCurrentTab(PyObject* self)
{
    ::PyTabWidget* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyTabWidget*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], (SbkObject*)self));

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // floatCurrentTab()
            cppSelf->floatCurrentTab();
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;
}

static PyObject* Sbk_PyTabWidgetFunc_floatPane(PyObject* self)
{
    ::PyTabWidget* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyTabWidget*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], (SbkObject*)self));

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // floatPane()
            cppSelf->floatPane();
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;
}

static PyObject* Sbk_PyTabWidgetFunc_getCurrentIndex(PyObject* self)
{
    ::PyTabWidget* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyTabWidget*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getCurrentIndex()const
            int cppResult = const_cast<const ::PyTabWidget*>(cppSelf)->getCurrentIndex();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_PyTabWidgetFunc_getScriptName(PyObject* self)
{
    ::PyTabWidget* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyTabWidget*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getScriptName()const
            std::string cppResult = const_cast<const ::PyTabWidget*>(cppSelf)->getScriptName();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_PyTabWidgetFunc_getTabLabel(PyObject* self, PyObject* pyArg)
{
    ::PyTabWidget* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyTabWidget*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: getTabLabel(int)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // getTabLabel(int)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyTabWidgetFunc_getTabLabel_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getTabLabel(int)const
            std::string cppResult = const_cast<const ::PyTabWidget*>(cppSelf)->getTabLabel(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_PyTabWidgetFunc_getTabLabel_TypeError:
        const char* overloads[] = {"int", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronGui.PyTabWidget.getTabLabel", overloads);
        return 0;
}

static PyObject* Sbk_PyTabWidgetFunc_insertTab(PyObject* self, PyObject* args)
{
    ::PyTabWidget* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyTabWidget*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "insertTab", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: insertTab(int,PyPanel*)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppPointerConvertible((SbkObjectType*)SbkNatronGuiTypes[SBK_PYPANEL_IDX], (pyArgs[1])))) {
        overloadId = 0; // insertTab(int,PyPanel*)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyTabWidgetFunc_insertTab_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        if (!Shiboken::Object::isValid(pyArgs[1]))
            return 0;
        ::PyPanel* cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // insertTab(int,PyPanel*)
            // Begin code injection

            cppSelf->insertTab(cppArg0,cppArg1);

            // End of code injection


        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_PyTabWidgetFunc_insertTab_TypeError:
        const char* overloads[] = {"int, NatronGui.PyPanel", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronGui.PyTabWidget.insertTab", overloads);
        return 0;
}

static PyObject* Sbk_PyTabWidgetFunc_removeTab(PyObject* self, PyObject* pyArg)
{
    ::PyTabWidget* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyTabWidget*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: removeTab(QWidget*)
    // 1: removeTab(int)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 1; // removeTab(int)
    } else if ((pythonToCpp = Shiboken::Conversions::isPythonToCppPointerConvertible((SbkObjectType*)SbkPySide_QtGuiTypes[SBK_QWIDGET_IDX], (pyArg)))) {
        overloadId = 0; // removeTab(QWidget*)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyTabWidgetFunc_removeTab_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // removeTab(QWidget * tab)
        {
            if (!Shiboken::Object::isValid(pyArg))
                return 0;
            ::QWidget* cppArg0;
            pythonToCpp(pyArg, &cppArg0);

            if (!PyErr_Occurred()) {
                // removeTab(QWidget*)
                cppSelf->removeTab(cppArg0);
            }
            break;
        }
        case 1: // removeTab(int index)
        {
            int cppArg0;
            pythonToCpp(pyArg, &cppArg0);

            if (!PyErr_Occurred()) {
                // removeTab(int)
                cppSelf->removeTab(cppArg0);
            }
            break;
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_PyTabWidgetFunc_removeTab_TypeError:
        const char* overloads[] = {"PySide.QtGui.QWidget", "int", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronGui.PyTabWidget.removeTab", overloads);
        return 0;
}

static PyObject* Sbk_PyTabWidgetFunc_setCurrentIndex(PyObject* self, PyObject* pyArg)
{
    ::PyTabWidget* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyTabWidget*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setCurrentIndex(int)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // setCurrentIndex(int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyTabWidgetFunc_setCurrentIndex_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setCurrentIndex(int)
            cppSelf->setCurrentIndex(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_PyTabWidgetFunc_setCurrentIndex_TypeError:
        const char* overloads[] = {"int", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronGui.PyTabWidget.setCurrentIndex", overloads);
        return 0;
}

static PyObject* Sbk_PyTabWidgetFunc_setNextTabCurrent(PyObject* self)
{
    ::PyTabWidget* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyTabWidget*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], (SbkObject*)self));

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // setNextTabCurrent()
            cppSelf->setNextTabCurrent();
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;
}

static PyObject* Sbk_PyTabWidgetFunc_splitHorizontally(PyObject* self)
{
    ::PyTabWidget* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyTabWidget*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // splitHorizontally()
            PyTabWidget * cppResult = cppSelf->splitHorizontally();
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_PyTabWidgetFunc_splitVertically(PyObject* self)
{
    ::PyTabWidget* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyTabWidget*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // splitVertically()
            PyTabWidget * cppResult = cppSelf->splitVertically();
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyMethodDef Sbk_PyTabWidget_methods[] = {
    {"appendTab", (PyCFunction)Sbk_PyTabWidgetFunc_appendTab, METH_O},
    {"closeCurrentTab", (PyCFunction)Sbk_PyTabWidgetFunc_closeCurrentTab, METH_NOARGS},
    {"closePane", (PyCFunction)Sbk_PyTabWidgetFunc_closePane, METH_NOARGS},
    {"closeTab", (PyCFunction)Sbk_PyTabWidgetFunc_closeTab, METH_O},
    {"count", (PyCFunction)Sbk_PyTabWidgetFunc_count, METH_NOARGS},
    {"currentWidget", (PyCFunction)Sbk_PyTabWidgetFunc_currentWidget, METH_NOARGS},
    {"floatCurrentTab", (PyCFunction)Sbk_PyTabWidgetFunc_floatCurrentTab, METH_NOARGS},
    {"floatPane", (PyCFunction)Sbk_PyTabWidgetFunc_floatPane, METH_NOARGS},
    {"getCurrentIndex", (PyCFunction)Sbk_PyTabWidgetFunc_getCurrentIndex, METH_NOARGS},
    {"getScriptName", (PyCFunction)Sbk_PyTabWidgetFunc_getScriptName, METH_NOARGS},
    {"getTabLabel", (PyCFunction)Sbk_PyTabWidgetFunc_getTabLabel, METH_O},
    {"insertTab", (PyCFunction)Sbk_PyTabWidgetFunc_insertTab, METH_VARARGS},
    {"removeTab", (PyCFunction)Sbk_PyTabWidgetFunc_removeTab, METH_O},
    {"setCurrentIndex", (PyCFunction)Sbk_PyTabWidgetFunc_setCurrentIndex, METH_O},
    {"setNextTabCurrent", (PyCFunction)Sbk_PyTabWidgetFunc_setNextTabCurrent, METH_NOARGS},
    {"splitHorizontally", (PyCFunction)Sbk_PyTabWidgetFunc_splitHorizontally, METH_NOARGS},
    {"splitVertically", (PyCFunction)Sbk_PyTabWidgetFunc_splitVertically, METH_NOARGS},

    {0} // Sentinel
};

} // extern "C"

static int Sbk_PyTabWidget_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_PyTabWidget_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_PyTabWidget_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronGui.PyTabWidget",
    /*tp_basicsize*/        sizeof(SbkObject),
    /*tp_itemsize*/         0,
    /*tp_dealloc*/          &SbkDeallocWrapper,
    /*tp_print*/            0,
    /*tp_getattr*/          0,
    /*tp_setattr*/          0,
    /*tp_compare*/          0,
    /*tp_repr*/             0,
    /*tp_as_number*/        0,
    /*tp_as_sequence*/      0,
    /*tp_as_mapping*/       0,
    /*tp_hash*/             0,
    /*tp_call*/             0,
    /*tp_str*/              0,
    /*tp_getattro*/         0,
    /*tp_setattro*/         0,
    /*tp_as_buffer*/        0,
    /*tp_flags*/            Py_TPFLAGS_DEFAULT|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    /*tp_doc*/              0,
    /*tp_traverse*/         Sbk_PyTabWidget_traverse,
    /*tp_clear*/            Sbk_PyTabWidget_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_PyTabWidget_methods,
    /*tp_members*/          0,
    /*tp_getset*/           0,
    /*tp_base*/             reinterpret_cast<PyTypeObject*>(&SbkObject_Type),
    /*tp_dict*/             0,
    /*tp_descr_get*/        0,
    /*tp_descr_set*/        0,
    /*tp_dictoffset*/       0,
    /*tp_init*/             0,
    /*tp_alloc*/            0,
    /*tp_new*/              0,
    /*tp_free*/             0,
    /*tp_is_gc*/            0,
    /*tp_bases*/            0,
    /*tp_mro*/              0,
    /*tp_cache*/            0,
    /*tp_subclasses*/       0,
    /*tp_weaklist*/         0
}, },
    /*priv_data*/           0
};
} //extern


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void PyTabWidget_PythonToCpp_PyTabWidget_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_PyTabWidget_Type, pyIn, cppOut);
}
static PythonToCppFunc is_PyTabWidget_PythonToCpp_PyTabWidget_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_PyTabWidget_Type))
        return PyTabWidget_PythonToCpp_PyTabWidget_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* PyTabWidget_PTR_CppToPython_PyTabWidget(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::PyTabWidget*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_PyTabWidget_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_PyTabWidget(PyObject* module)
{
    SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_PyTabWidget_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "PyTabWidget", "PyTabWidget*",
        &Sbk_PyTabWidget_Type, &Shiboken::callCppDestructor< ::PyTabWidget >)) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_PyTabWidget_Type,
        PyTabWidget_PythonToCpp_PyTabWidget_PTR,
        is_PyTabWidget_PythonToCpp_PyTabWidget_PTR_Convertible,
        PyTabWidget_PTR_CppToPython_PyTabWidget);

    Shiboken::Conversions::registerConverterName(converter, "PyTabWidget");
    Shiboken::Conversions::registerConverterName(converter, "PyTabWidget*");
    Shiboken::Conversions::registerConverterName(converter, "PyTabWidget&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::PyTabWidget).name());



}
