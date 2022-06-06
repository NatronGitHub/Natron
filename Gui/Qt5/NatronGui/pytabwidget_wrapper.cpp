
// default includes
#include "Global/Macros.h"
CLANG_DIAG_OFF(mismatched-tags)
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_OFF(missing-field-initializers)
GCC_DIAG_OFF(missing-declarations)
GCC_DIAG_OFF(uninitialized)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <pysidesignal.h>
#include <shiboken.h> // produces many warnings
#ifndef QT_NO_VERSION_TAGGING
#  define QT_NO_VERSION_TAGGING
#endif
#include <QDebug>
#include <pysidesignal.h>
#include <pysideproperty.h>
#include <pyside.h>
#include <pysideqenum.h>
#include <feature_select.h>
QT_WARNING_DISABLE_DEPRECATED

#include <typeinfo>
#include <iterator>

// module include
#include "natrongui_python.h"

// main header
#include "pytabwidget_wrapper.h"

// inner classes
NATRON_NAMESPACE_USING NATRON_PYTHON_NAMESPACE_USING

// Extra includes
#include <PythonPanels.h>
#include <qwidget.h>


#include <cctype>
#include <cstring>



template <class T>
static const char *typeNameOf(const T &t)
{
    const char *typeName =  typeid(t).name();
    auto size = std::strlen(typeName);
#if defined(Q_CC_MSVC) // MSVC: "class QPaintDevice * __ptr64"
    if (auto lastStar = strchr(typeName, '*')) {
        // MSVC: "class QPaintDevice * __ptr64"
        while (*--lastStar == ' ') {
        }
        size = lastStar - typeName + 1;
    }
#else // g++, Clang: "QPaintDevice *" -> "P12QPaintDevice"
    if (size > 2 && typeName[0] == 'P' && std::isdigit(typeName[1])) {
        ++typeName;
        --size;
    }
#endif
    char *result = new char[size + 1];
    result[size] = '\0';
    memcpy(result, typeName, size);
    return result;
}


// Target ---------------------------------------------------------

extern "C" {
static PyObject *Sbk_PyTabWidgetFunc_appendTab(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyTabWidget *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyTabWidget.appendTab";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: PyTabWidget::appendTab(PyPanel*)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppPointerConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronGuiTypes[SBK_PYPANEL_IDX]), (pyArg)))) {
        overloadId = 0; // appendTab(PyPanel*)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyTabWidgetFunc_appendTab_TypeError;

    // Call function/method
    {
        if (!Shiboken::Object::isValid(pyArg))
            return {};
        ::PyPanel *cppArg0;
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
        return {};
    }
    return pyResult;

    Sbk_PyTabWidgetFunc_appendTab_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_PyTabWidgetFunc_closeCurrentTab(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyTabWidget *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyTabWidget.closeCurrentTab";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // closeCurrentTab()
            cppSelf->closeCurrentTab();
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;
}

static PyObject *Sbk_PyTabWidgetFunc_closePane(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyTabWidget *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyTabWidget.closePane";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // closePane()
            cppSelf->closePane();
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;
}

static PyObject *Sbk_PyTabWidgetFunc_closeTab(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyTabWidget *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyTabWidget.closeTab";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: PyTabWidget::closeTab(int)
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
        return {};
    }
    Py_RETURN_NONE;

    Sbk_PyTabWidgetFunc_closeTab_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_PyTabWidgetFunc_count(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyTabWidget *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyTabWidget.count";
    SBK_UNUSED(fullName)

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
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_PyTabWidgetFunc_currentWidget(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyTabWidget *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyTabWidget.currentWidget";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // currentWidget()
            QWidget * cppResult = cppSelf->currentWidget();
            pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtWidgetsTypes[SBK_QWIDGET_IDX]), cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_PyTabWidgetFunc_floatCurrentTab(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyTabWidget *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyTabWidget.floatCurrentTab";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // floatCurrentTab()
            cppSelf->floatCurrentTab();
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;
}

static PyObject *Sbk_PyTabWidgetFunc_floatPane(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyTabWidget *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyTabWidget.floatPane";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // floatPane()
            cppSelf->floatPane();
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;
}

static PyObject *Sbk_PyTabWidgetFunc_getCurrentIndex(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyTabWidget *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyTabWidget.getCurrentIndex";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getCurrentIndex()const
            int cppResult = const_cast<const ::PyTabWidget *>(cppSelf)->getCurrentIndex();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_PyTabWidgetFunc_getScriptName(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyTabWidget *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyTabWidget.getScriptName";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getScriptName()const
            QString cppResult = const_cast<const ::PyTabWidget *>(cppSelf)->getScriptName();
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_PyTabWidgetFunc_getTabLabel(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyTabWidget *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyTabWidget.getTabLabel";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: PyTabWidget::getTabLabel(int)const
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
            QString cppResult = const_cast<const ::PyTabWidget *>(cppSelf)->getTabLabel(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_PyTabWidgetFunc_getTabLabel_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_PyTabWidgetFunc_insertTab(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyTabWidget *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyTabWidget.insertTab";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "insertTab", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: PyTabWidget::insertTab(int,PyPanel*)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppPointerConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronGuiTypes[SBK_PYPANEL_IDX]), (pyArgs[1])))) {
        overloadId = 0; // insertTab(int,PyPanel*)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyTabWidgetFunc_insertTab_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        if (!Shiboken::Object::isValid(pyArgs[1]))
            return {};
        ::PyPanel *cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // insertTab(int,PyPanel*)
            // Begin code injection
            cppSelf->insertTab(cppArg0,cppArg1);

            // End of code injection

        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_PyTabWidgetFunc_insertTab_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_PyTabWidgetFunc_removeTab(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyTabWidget *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyTabWidget.removeTab";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: PyTabWidget::removeTab(QWidget*)
    // 1: PyTabWidget::removeTab(int)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 1; // removeTab(int)
    } else if ((pythonToCpp = Shiboken::Conversions::isPythonToCppPointerConvertible(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtWidgetsTypes[SBK_QWIDGET_IDX]), (pyArg)))) {
        overloadId = 0; // removeTab(QWidget*)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyTabWidgetFunc_removeTab_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // removeTab(QWidget * tab)
        {
            if (!Shiboken::Object::isValid(pyArg))
                return {};
            ::QWidget *cppArg0;
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
        return {};
    }
    Py_RETURN_NONE;

    Sbk_PyTabWidgetFunc_removeTab_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_PyTabWidgetFunc_setCurrentIndex(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyTabWidget *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyTabWidget.setCurrentIndex";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: PyTabWidget::setCurrentIndex(int)
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
        return {};
    }
    Py_RETURN_NONE;

    Sbk_PyTabWidgetFunc_setCurrentIndex_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_PyTabWidgetFunc_setNextTabCurrent(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyTabWidget *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyTabWidget.setNextTabCurrent";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // setNextTabCurrent()
            cppSelf->setNextTabCurrent();
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;
}

static PyObject *Sbk_PyTabWidgetFunc_splitHorizontally(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyTabWidget *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyTabWidget.splitHorizontally";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // splitHorizontally()
            PyTabWidget * cppResult = cppSelf->splitHorizontally();
            pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX]), cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_PyTabWidgetFunc_splitVertically(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyTabWidget *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyTabWidget.splitVertically";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // splitVertically()
            PyTabWidget * cppResult = cppSelf->splitVertically();
            pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX]), cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}


static const char *Sbk_PyTabWidget_PropertyStrings[] = {
    nullptr // Sentinel
};

static PyMethodDef Sbk_PyTabWidget_methods[] = {
    {"appendTab", reinterpret_cast<PyCFunction>(Sbk_PyTabWidgetFunc_appendTab), METH_O},
    {"closeCurrentTab", reinterpret_cast<PyCFunction>(Sbk_PyTabWidgetFunc_closeCurrentTab), METH_NOARGS},
    {"closePane", reinterpret_cast<PyCFunction>(Sbk_PyTabWidgetFunc_closePane), METH_NOARGS},
    {"closeTab", reinterpret_cast<PyCFunction>(Sbk_PyTabWidgetFunc_closeTab), METH_O},
    {"count", reinterpret_cast<PyCFunction>(Sbk_PyTabWidgetFunc_count), METH_NOARGS},
    {"currentWidget", reinterpret_cast<PyCFunction>(Sbk_PyTabWidgetFunc_currentWidget), METH_NOARGS},
    {"floatCurrentTab", reinterpret_cast<PyCFunction>(Sbk_PyTabWidgetFunc_floatCurrentTab), METH_NOARGS},
    {"floatPane", reinterpret_cast<PyCFunction>(Sbk_PyTabWidgetFunc_floatPane), METH_NOARGS},
    {"getCurrentIndex", reinterpret_cast<PyCFunction>(Sbk_PyTabWidgetFunc_getCurrentIndex), METH_NOARGS},
    {"getScriptName", reinterpret_cast<PyCFunction>(Sbk_PyTabWidgetFunc_getScriptName), METH_NOARGS},
    {"getTabLabel", reinterpret_cast<PyCFunction>(Sbk_PyTabWidgetFunc_getTabLabel), METH_O},
    {"insertTab", reinterpret_cast<PyCFunction>(Sbk_PyTabWidgetFunc_insertTab), METH_VARARGS},
    {"removeTab", reinterpret_cast<PyCFunction>(Sbk_PyTabWidgetFunc_removeTab), METH_O},
    {"setCurrentIndex", reinterpret_cast<PyCFunction>(Sbk_PyTabWidgetFunc_setCurrentIndex), METH_O},
    {"setNextTabCurrent", reinterpret_cast<PyCFunction>(Sbk_PyTabWidgetFunc_setNextTabCurrent), METH_NOARGS},
    {"splitHorizontally", reinterpret_cast<PyCFunction>(Sbk_PyTabWidgetFunc_splitHorizontally), METH_NOARGS},
    {"splitVertically", reinterpret_cast<PyCFunction>(Sbk_PyTabWidgetFunc_splitVertically), METH_NOARGS},

    {nullptr, nullptr} // Sentinel
};

} // extern "C"

static int Sbk_PyTabWidget_traverse(PyObject *self, visitproc visit, void *arg)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_traverse(self, visit, arg);
}
static int Sbk_PyTabWidget_clear(PyObject *self)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType *_Sbk_PyTabWidget_Type = nullptr;
static SbkObjectType *Sbk_PyTabWidget_TypeF(void)
{
    return _Sbk_PyTabWidget_Type;
}

static PyType_Slot Sbk_PyTabWidget_slots[] = {
    {Py_tp_base,        nullptr}, // inserted by introduceWrapperType
    {Py_tp_dealloc,     reinterpret_cast<void *>(&SbkDeallocWrapper)},
    {Py_tp_repr,        nullptr},
    {Py_tp_hash,        nullptr},
    {Py_tp_call,        nullptr},
    {Py_tp_str,         nullptr},
    {Py_tp_getattro,    nullptr},
    {Py_tp_setattro,    nullptr},
    {Py_tp_traverse,    reinterpret_cast<void *>(Sbk_PyTabWidget_traverse)},
    {Py_tp_clear,       reinterpret_cast<void *>(Sbk_PyTabWidget_clear)},
    {Py_tp_richcompare, nullptr},
    {Py_tp_iter,        nullptr},
    {Py_tp_iternext,    nullptr},
    {Py_tp_methods,     reinterpret_cast<void *>(Sbk_PyTabWidget_methods)},
    {Py_tp_getset,      nullptr},
    {Py_tp_init,        nullptr},
    {Py_tp_new,         reinterpret_cast<void *>(SbkDummyNew /* PYSIDE-595: Prevent replacement of "0" with base->tp_new. */)},
    {0, nullptr}
};
static PyType_Spec Sbk_PyTabWidget_spec = {
    "1:NatronGui.PyTabWidget",
    sizeof(SbkObject),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    Sbk_PyTabWidget_slots
};

} //extern "C"


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void PyTabWidget_PythonToCpp_PyTabWidget_PTR(PyObject *pyIn, void *cppOut) {
    Shiboken::Conversions::pythonToCppPointer(Sbk_PyTabWidget_TypeF(), pyIn, cppOut);
}
static PythonToCppFunc is_PyTabWidget_PythonToCpp_PyTabWidget_PTR_Convertible(PyObject *pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, reinterpret_cast<PyTypeObject *>(Sbk_PyTabWidget_TypeF())))
        return PyTabWidget_PythonToCpp_PyTabWidget_PTR;
    return {};
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject *PyTabWidget_PTR_CppToPython_PyTabWidget(const void *cppIn) {
    auto pyOut = reinterpret_cast<PyObject *>(Shiboken::BindingManager::instance().retrieveWrapper(cppIn));
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    bool changedTypeName = false;
    auto tCppIn = reinterpret_cast<const ::PyTabWidget *>(cppIn);
    const char *typeName = typeid(*tCppIn).name();
    auto sbkType = Shiboken::ObjectType::typeForTypeName(typeName);
    if (sbkType && Shiboken::ObjectType::hasSpecialCastFunction(sbkType)) {
        typeName = typeNameOf(tCppIn);
        changedTypeName = true;
    }
    PyObject *result = Shiboken::Object::newObject(Sbk_PyTabWidget_TypeF(), const_cast<void *>(cppIn), false, /* exactType */ changedTypeName, typeName);
    if (changedTypeName)
        delete [] typeName;
    return result;
}

// The signatures string for the functions.
// Multiple signatures have their index "n:" in front.
static const char *PyTabWidget_SignatureStrings[] = {
    "NatronGui.PyTabWidget.appendTab(self,tab:NatronGui.PyPanel)->bool",
    "NatronGui.PyTabWidget.closeCurrentTab(self)",
    "NatronGui.PyTabWidget.closePane(self)",
    "NatronGui.PyTabWidget.closeTab(self,index:int)",
    "NatronGui.PyTabWidget.count(self)->int",
    "NatronGui.PyTabWidget.currentWidget(self)->PySide2.QtWidgets.QWidget",
    "NatronGui.PyTabWidget.floatCurrentTab(self)",
    "NatronGui.PyTabWidget.floatPane(self)",
    "NatronGui.PyTabWidget.getCurrentIndex(self)->int",
    "NatronGui.PyTabWidget.getScriptName(self)->QString",
    "NatronGui.PyTabWidget.getTabLabel(self,index:int)->QString",
    "NatronGui.PyTabWidget.insertTab(self,index:int,tab:NatronGui.PyPanel)",
    "1:NatronGui.PyTabWidget.removeTab(self,tab:PySide2.QtWidgets.QWidget)",
    "0:NatronGui.PyTabWidget.removeTab(self,index:int)",
    "NatronGui.PyTabWidget.setCurrentIndex(self,index:int)",
    "NatronGui.PyTabWidget.setNextTabCurrent(self)",
    "NatronGui.PyTabWidget.splitHorizontally(self)->NatronGui.PyTabWidget",
    "NatronGui.PyTabWidget.splitVertically(self)->NatronGui.PyTabWidget",
    nullptr}; // Sentinel

void init_PyTabWidget(PyObject *module)
{
    _Sbk_PyTabWidget_Type = Shiboken::ObjectType::introduceWrapperType(
        module,
        "PyTabWidget",
        "PyTabWidget*",
        &Sbk_PyTabWidget_spec,
        &Shiboken::callCppDestructor< ::PyTabWidget >,
        0,
        0,
        0    );
    
    auto pyType = reinterpret_cast<PyTypeObject *>(_Sbk_PyTabWidget_Type);
    InitSignatureStrings(pyType, PyTabWidget_SignatureStrings);
    SbkObjectType_SetPropertyStrings(reinterpret_cast<PyTypeObject *>(_Sbk_PyTabWidget_Type), Sbk_PyTabWidget_PropertyStrings);
    SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX]
        = reinterpret_cast<PyTypeObject *>(Sbk_PyTabWidget_TypeF());

    // Register Converter
    SbkConverter *converter = Shiboken::Conversions::createConverter(Sbk_PyTabWidget_TypeF(),
        PyTabWidget_PythonToCpp_PyTabWidget_PTR,
        is_PyTabWidget_PythonToCpp_PyTabWidget_PTR_Convertible,
        PyTabWidget_PTR_CppToPython_PyTabWidget);

    Shiboken::Conversions::registerConverterName(converter, "PyTabWidget");
    Shiboken::Conversions::registerConverterName(converter, "PyTabWidget*");
    Shiboken::Conversions::registerConverterName(converter, "PyTabWidget&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::PyTabWidget).name());


}
