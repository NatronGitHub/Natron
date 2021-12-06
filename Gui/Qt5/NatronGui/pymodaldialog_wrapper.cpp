
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
#include <qapp_macro.h>

QT_WARNING_DISABLE_DEPRECATED

#include <typeinfo>
#include <algorithm>
#include <set>
#include <iterator>

// module include
#include "natrongui_python.h"

// main header
#include "pymodaldialog_wrapper.h"

// inner classes
NATRON_NAMESPACE_USING NATRON_PYTHON_NAMESPACE_USING

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

// Native ---------------------------------------------------------

void PyModalDialogWrapper::pysideInitQtMetaTypes()
{
}

void PyModalDialogWrapper::resetPyMethodCache()
{
    std::fill_n(m_PyMethodCache, sizeof(m_PyMethodCache) / sizeof(m_PyMethodCache[0]), false);
}

void PyModalDialogWrapper::accept()
{
    if (m_PyMethodCache[0]) {
        return this->::QDialog::accept();
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "accept";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[0] = true;
        return this->::QDialog::accept();
    }

    Shiboken::AutoDecRef pyArgs(PyTuple_New(0));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
}

void PyModalDialogWrapper::closeEvent(QCloseEvent * arg__1)
{
    if (m_PyMethodCache[1]) {
        return this->::QDialog::closeEvent(arg__1);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "closeEvent";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[1] = true;
        return this->::QDialog::closeEvent(arg__1);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QCLOSEEVENT_IDX]), arg__1)
    ));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
}

void PyModalDialogWrapper::contextMenuEvent(QContextMenuEvent * arg__1)
{
    if (m_PyMethodCache[2]) {
        return this->::QDialog::contextMenuEvent(arg__1);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "contextMenuEvent";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[2] = true;
        return this->::QDialog::contextMenuEvent(arg__1);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QCONTEXTMENUEVENT_IDX]), arg__1)
    ));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
}

void PyModalDialogWrapper::done(int arg__1)
{
    if (m_PyMethodCache[3]) {
        return this->::QDialog::done(arg__1);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "done";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[3] = true;
        return this->::QDialog::done(arg__1);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(i)",
    arg__1
    ));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
}

bool PyModalDialogWrapper::eventFilter(QObject * arg__1, QEvent * arg__2)
{
    if (m_PyMethodCache[4])
        return this->::QDialog::eventFilter(arg__1, arg__2);
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return false;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "eventFilter";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[4] = true;
        return this->::QDialog::eventFilter(arg__1, arg__2);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(NN)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtCoreTypes[SBK_QOBJECT_IDX]), arg__1),
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtCoreTypes[SBK_QEVENT_IDX]), arg__2)
    ));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return false;
    }
    // Check return type
    PythonToCppFunc pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), pyResult);
    if (!pythonToCpp) {
        Shiboken::warning(PyExc_RuntimeWarning, 2, "Invalid return value in function %s, expected %s, got %s.", "PyModalDialog.eventFilter", "bool", Py_TYPE(pyResult)->tp_name);
        return false;
    }
    bool cppResult;
    pythonToCpp(pyResult, &cppResult);
    return cppResult;
}

int PyModalDialogWrapper::exec()
{
    if (m_PyMethodCache[5])
        return this->::QDialog::exec();
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return 0;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "exec_";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[5] = true;
        return this->::QDialog::exec();
    }

    Shiboken::AutoDecRef pyArgs(PyTuple_New(0));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return 0;
    }
    // Check return type
    PythonToCppFunc pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), pyResult);
    if (!pythonToCpp) {
        Shiboken::warning(PyExc_RuntimeWarning, 2, "Invalid return value in function %s, expected %s, got %s.", "PyModalDialog.exec_", "int", Py_TYPE(pyResult)->tp_name);
        return 0;
    }
    int cppResult;
    pythonToCpp(pyResult, &cppResult);
    return cppResult;
}

void PyModalDialogWrapper::keyPressEvent(QKeyEvent * arg__1)
{
    if (m_PyMethodCache[6]) {
        return this->::QDialog::keyPressEvent(arg__1);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "keyPressEvent";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[6] = true;
        return this->::QDialog::keyPressEvent(arg__1);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QKEYEVENT_IDX]), arg__1)
    ));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
}

void PyModalDialogWrapper::open()
{
    if (m_PyMethodCache[7]) {
        return this->::QDialog::open();
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "open";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[7] = true;
        return this->::QDialog::open();
    }

    Shiboken::AutoDecRef pyArgs(PyTuple_New(0));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
}

void PyModalDialogWrapper::reject()
{
    if (m_PyMethodCache[8]) {
        return this->::QDialog::reject();
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "reject";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[8] = true;
        return this->::QDialog::reject();
    }

    Shiboken::AutoDecRef pyArgs(PyTuple_New(0));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
}

void PyModalDialogWrapper::resizeEvent(QResizeEvent * arg__1)
{
    if (m_PyMethodCache[9]) {
        return this->::QDialog::resizeEvent(arg__1);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "resizeEvent";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[9] = true;
        return this->::QDialog::resizeEvent(arg__1);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QRESIZEEVENT_IDX]), arg__1)
    ));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
}

void PyModalDialogWrapper::setVisible(bool visible)
{
    if (m_PyMethodCache[10]) {
        return this->::QDialog::setVisible(visible);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "setVisible";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[10] = true;
        return this->::QDialog::setVisible(visible);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &visible)
    ));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
}

void PyModalDialogWrapper::showEvent(QShowEvent * arg__1)
{
    if (m_PyMethodCache[11]) {
        return this->::QDialog::showEvent(arg__1);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "showEvent";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[11] = true;
        return this->::QDialog::showEvent(arg__1);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QSHOWEVENT_IDX]), arg__1)
    ));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
}

PyModalDialogWrapper::~PyModalDialogWrapper()
{
    SbkObject *wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject *Sbk_PyModalDialogFunc_addWidget(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<PyModalDialogWrapper *>(reinterpret_cast< ::PyModalDialog *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYMODALDIALOG_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: PyModalDialog::addWidget(QWidget*)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppPointerConvertible(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtWidgetsTypes[SBK_QWIDGET_IDX]), (pyArg)))) {
        overloadId = 0; // addWidget(QWidget*)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyModalDialogFunc_addWidget_TypeError;

    // Call function/method
    {
        if (!Shiboken::Object::isValid(pyArg))
            return {};
        ::QWidget *cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // addWidget(QWidget*)
            cppSelf->addWidget(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_PyModalDialogFunc_addWidget_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronGui.PyModalDialog.addWidget");
        return {};
}

static PyObject *Sbk_PyModalDialogFunc_getParam(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<PyModalDialogWrapper *>(reinterpret_cast< ::PyModalDialog *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYMODALDIALOG_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: PyModalDialog::getParam(QString)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // getParam(QString)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyModalDialogFunc_getParam_TypeError;

    // Call function/method
    {
        ::QString cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getParam(QString)const
            Param * cppResult = const_cast<const ::PyModalDialogWrapper *>(cppSelf)->getParam(cppArg0);
            pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_PARAM_IDX]), cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_PyModalDialogFunc_getParam_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronGui.PyModalDialog.getParam");
        return {};
}

static PyObject *Sbk_PyModalDialogFunc_insertWidget(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<PyModalDialogWrapper *>(reinterpret_cast< ::PyModalDialog *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYMODALDIALOG_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "insertWidget", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: PyModalDialog::insertWidget(int,QWidget*)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppPointerConvertible(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtWidgetsTypes[SBK_QWIDGET_IDX]), (pyArgs[1])))) {
        overloadId = 0; // insertWidget(int,QWidget*)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyModalDialogFunc_insertWidget_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        if (!Shiboken::Object::isValid(pyArgs[1]))
            return {};
        ::QWidget *cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // insertWidget(int,QWidget*)
            cppSelf->insertWidget(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_PyModalDialogFunc_insertWidget_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronGui.PyModalDialog.insertWidget");
        return {};
}

static PyObject *Sbk_PyModalDialogFunc_minimumSizeHint(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<PyModalDialogWrapper *>(reinterpret_cast< ::PyModalDialog *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYMODALDIALOG_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // minimumSizeHint()const
            QSize cppResult = Shiboken::Object::hasCppWrapper(reinterpret_cast<SbkObject *>(self))
                ? const_cast<const ::PyModalDialogWrapper *>(cppSelf)->::PyModalDialog::minimumSizeHint()
                : const_cast<const ::PyModalDialogWrapper *>(cppSelf)->minimumSizeHint();
            pyResult = Shiboken::Conversions::copyToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtCoreTypes[SBK_QSIZE_IDX]), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_PyModalDialogFunc_setParamChangedCallback(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<PyModalDialogWrapper *>(reinterpret_cast< ::PyModalDialog *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYMODALDIALOG_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: PyModalDialog::setParamChangedCallback(QString)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // setParamChangedCallback(QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyModalDialogFunc_setParamChangedCallback_TypeError;

    // Call function/method
    {
        ::QString cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setParamChangedCallback(QString)
            cppSelf->setParamChangedCallback(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_PyModalDialogFunc_setParamChangedCallback_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronGui.PyModalDialog.setParamChangedCallback");
        return {};
}

static PyObject *Sbk_PyModalDialogFunc_sizeHint(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<PyModalDialogWrapper *>(reinterpret_cast< ::PyModalDialog *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYMODALDIALOG_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // sizeHint()const
            QSize cppResult = Shiboken::Object::hasCppWrapper(reinterpret_cast<SbkObject *>(self))
                ? const_cast<const ::PyModalDialogWrapper *>(cppSelf)->::PyModalDialog::sizeHint()
                : const_cast<const ::PyModalDialogWrapper *>(cppSelf)->sizeHint();
            pyResult = Shiboken::Conversions::copyToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtCoreTypes[SBK_QSIZE_IDX]), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}


static const char *Sbk_PyModalDialog_PropertyStrings[] = {
    nullptr // Sentinel
};

static PyMethodDef Sbk_PyModalDialog_methods[] = {
    {"addWidget", reinterpret_cast<PyCFunction>(Sbk_PyModalDialogFunc_addWidget), METH_O},
    {"getParam", reinterpret_cast<PyCFunction>(Sbk_PyModalDialogFunc_getParam), METH_O},
    {"insertWidget", reinterpret_cast<PyCFunction>(Sbk_PyModalDialogFunc_insertWidget), METH_VARARGS},
    {"minimumSizeHint", reinterpret_cast<PyCFunction>(Sbk_PyModalDialogFunc_minimumSizeHint), METH_NOARGS},
    {"setParamChangedCallback", reinterpret_cast<PyCFunction>(Sbk_PyModalDialogFunc_setParamChangedCallback), METH_O},
    {"sizeHint", reinterpret_cast<PyCFunction>(Sbk_PyModalDialogFunc_sizeHint), METH_NOARGS},

    {nullptr, nullptr} // Sentinel
};

static int Sbk_PyModalDialog_setattro(PyObject *self, PyObject *name, PyObject *value)
{
    PySide::Feature::Select(self);
    if (value && PyCallable_Check(value)) {
        auto plain_inst = reinterpret_cast< ::PyModalDialog *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYMODALDIALOG_IDX], reinterpret_cast<SbkObject *>(self)));
        auto inst = dynamic_cast<PyModalDialogWrapper *>(plain_inst);
        if (inst)
            inst->resetPyMethodCache();
    }
    return PyObject_GenericSetAttr(self, name, value);
}

} // extern "C"

static int Sbk_PyModalDialog_traverse(PyObject *self, visitproc visit, void *arg)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_traverse(self, visit, arg);
}
static int Sbk_PyModalDialog_clear(PyObject *self)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_clear(self);
}
static int mi_offsets[] = { -1, -1, -1, -1, -1 };
int *
Sbk_PyModalDialog_mi_init(const void *cptr)
{
    if (mi_offsets[0] == -1) {
        std::set<int> offsets;
        const auto *class_ptr = reinterpret_cast<const PyModalDialog *>(cptr);
        const auto base = reinterpret_cast<uintptr_t>(class_ptr);
        offsets.insert(int(reinterpret_cast<uintptr_t>(static_cast<const QDialog *>(class_ptr)) - base));
        offsets.insert(int(reinterpret_cast<uintptr_t>(static_cast<const QDialog *>(static_cast<const PyModalDialog *>(static_cast<const void *>(class_ptr)))) - base));
        offsets.insert(int(reinterpret_cast<uintptr_t>(static_cast<const UserParamHolder *>(class_ptr)) - base));
        offsets.insert(int(reinterpret_cast<uintptr_t>(static_cast<const UserParamHolder *>(static_cast<const PyModalDialog *>(static_cast<const void *>(class_ptr)))) - base));

        offsets.erase(0);

        std::copy(offsets.cbegin(), offsets.cend(), mi_offsets);
    }
    return mi_offsets;
}
static void * Sbk_PyModalDialogSpecialCastFunction(void *obj, SbkObjectType *desiredType)
{
    auto me = reinterpret_cast< ::PyModalDialog *>(obj);
    if (desiredType == reinterpret_cast<SbkObjectType *>(SbkPySide2_QtWidgetsTypes[SBK_QDIALOG_IDX]))
        return static_cast< ::QDialog *>(me);
    else if (desiredType == reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_USERPARAMHOLDER_IDX]))
        return static_cast< ::UserParamHolder *>(me);
    return me;
}


// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType *_Sbk_PyModalDialog_Type = nullptr;
static SbkObjectType *Sbk_PyModalDialog_TypeF(void)
{
    return _Sbk_PyModalDialog_Type;
}

static PyType_Slot Sbk_PyModalDialog_slots[] = {
    {Py_tp_base,        nullptr}, // inserted by introduceWrapperType
    {Py_tp_dealloc,     reinterpret_cast<void *>(&SbkDeallocWrapper)},
    {Py_tp_repr,        nullptr},
    {Py_tp_hash,        nullptr},
    {Py_tp_call,        nullptr},
    {Py_tp_str,         nullptr},
    {Py_tp_getattro,    nullptr},
    {Py_tp_setattro,    reinterpret_cast<void *>(Sbk_PyModalDialog_setattro)},
    {Py_tp_traverse,    reinterpret_cast<void *>(Sbk_PyModalDialog_traverse)},
    {Py_tp_clear,       reinterpret_cast<void *>(Sbk_PyModalDialog_clear)},
    {Py_tp_richcompare, nullptr},
    {Py_tp_iter,        nullptr},
    {Py_tp_iternext,    nullptr},
    {Py_tp_methods,     reinterpret_cast<void *>(Sbk_PyModalDialog_methods)},
    {Py_tp_getset,      nullptr},
    {Py_tp_init,        nullptr},
    {Py_tp_new,         reinterpret_cast<void *>(SbkDummyNew /* PYSIDE-595: Prevent replacement of "0" with base->tp_new. */)},
    {0, nullptr}
};
static PyType_Spec Sbk_PyModalDialog_spec = {
    "1:NatronGui.PyModalDialog",
    sizeof(SbkObject),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    Sbk_PyModalDialog_slots
};

} //extern "C"

static void *Sbk_PyModalDialog_typeDiscovery(void *cptr, SbkObjectType *instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType *>(Shiboken::SbkType< ::QDialog >()))
        return dynamic_cast< ::PyModalDialog *>(reinterpret_cast< ::QDialog *>(cptr));
    if (instanceType == reinterpret_cast<SbkObjectType *>(Shiboken::SbkType< ::UserParamHolder >()))
        return dynamic_cast< ::PyModalDialog *>(reinterpret_cast< ::UserParamHolder *>(cptr));
    return {};
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void PyModalDialog_PythonToCpp_PyModalDialog_PTR(PyObject *pyIn, void *cppOut) {
    Shiboken::Conversions::pythonToCppPointer(Sbk_PyModalDialog_TypeF(), pyIn, cppOut);
}
static PythonToCppFunc is_PyModalDialog_PythonToCpp_PyModalDialog_PTR_Convertible(PyObject *pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, reinterpret_cast<PyTypeObject *>(Sbk_PyModalDialog_TypeF())))
        return PyModalDialog_PythonToCpp_PyModalDialog_PTR;
    return {};
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject *PyModalDialog_PTR_CppToPython_PyModalDialog(const void *cppIn) {
    auto pyOut = reinterpret_cast<PyObject *>(Shiboken::BindingManager::instance().retrieveWrapper(cppIn));
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    bool changedTypeName = false;
    auto tCppIn = reinterpret_cast<const ::PyModalDialog *>(cppIn);
    const char *typeName = typeid(*tCppIn).name();
    auto sbkType = Shiboken::ObjectType::typeForTypeName(typeName);
    if (sbkType && Shiboken::ObjectType::hasSpecialCastFunction(sbkType)) {
        typeName = typeNameOf(tCppIn);
        changedTypeName = true;
    }
    PyObject *result = Shiboken::Object::newObject(Sbk_PyModalDialog_TypeF(), const_cast<void *>(cppIn), false, /* exactType */ changedTypeName, typeName);
    if (changedTypeName)
        delete [] typeName;
    return result;
}

// The signatures string for the functions.
// Multiple signatures have their index "n:" in front.
static const char *PyModalDialog_SignatureStrings[] = {
    "NatronGui.PyModalDialog.addWidget(self,widget:PySide2.QtWidgets.QWidget)",
    "NatronGui.PyModalDialog.getParam(self,scriptName:QString)->NatronEngine.Param",
    "NatronGui.PyModalDialog.insertWidget(self,index:int,widget:PySide2.QtWidgets.QWidget)",
    "NatronGui.PyModalDialog.minimumSizeHint(self)->PySide2.QtCore.QSize",
    "NatronGui.PyModalDialog.setParamChangedCallback(self,callback:QString)",
    "NatronGui.PyModalDialog.sizeHint(self)->PySide2.QtCore.QSize",
    nullptr}; // Sentinel

void init_PyModalDialog(PyObject *module)
{
    PyObject *Sbk_PyModalDialog_Type_bases = PyTuple_Pack(2,
        reinterpret_cast<PyObject *>(SbkPySide2_QtWidgetsTypes[SBK_QDIALOG_IDX]),
        reinterpret_cast<PyObject *>(SbkNatronEngineTypes[SBK_USERPARAMHOLDER_IDX]));

    _Sbk_PyModalDialog_Type = Shiboken::ObjectType::introduceWrapperType(
        module,
        "PyModalDialog",
        "PyModalDialog*",
        &Sbk_PyModalDialog_spec,
        &Shiboken::callCppDestructor< ::PyModalDialog >,
        reinterpret_cast<SbkObjectType *>(SbkPySide2_QtWidgetsTypes[SBK_QDIALOG_IDX]),
        Sbk_PyModalDialog_Type_bases,
        0    );
    
    auto pyType = reinterpret_cast<PyTypeObject *>(_Sbk_PyModalDialog_Type);
    InitSignatureStrings(pyType, PyModalDialog_SignatureStrings);
    SbkObjectType_SetPropertyStrings(reinterpret_cast<PyTypeObject *>(_Sbk_PyModalDialog_Type), Sbk_PyModalDialog_PropertyStrings);
    SbkNatronGuiTypes[SBK_PYMODALDIALOG_IDX]
        = reinterpret_cast<PyTypeObject *>(Sbk_PyModalDialog_TypeF());

    // Register Converter
    SbkConverter *converter = Shiboken::Conversions::createConverter(Sbk_PyModalDialog_TypeF(),
        PyModalDialog_PythonToCpp_PyModalDialog_PTR,
        is_PyModalDialog_PythonToCpp_PyModalDialog_PTR_Convertible,
        PyModalDialog_PTR_CppToPython_PyModalDialog);

    Shiboken::Conversions::registerConverterName(converter, "PyModalDialog");
    Shiboken::Conversions::registerConverterName(converter, "PyModalDialog*");
    Shiboken::Conversions::registerConverterName(converter, "PyModalDialog&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::PyModalDialog).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::PyModalDialogWrapper).name());


    MultipleInheritanceInitFunction func = Sbk_PyModalDialog_mi_init;
    Shiboken::ObjectType::setMultipleInheritanceFunction(Sbk_PyModalDialog_TypeF(), func);
    Shiboken::ObjectType::setCastFunction(Sbk_PyModalDialog_TypeF(), &Sbk_PyModalDialogSpecialCastFunction);
    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(Sbk_PyModalDialog_TypeF(), &Sbk_PyModalDialog_typeDiscovery);

    PySide::Signal::registerSignals(Sbk_PyModalDialog_TypeF(), &::PyModalDialog::staticMetaObject);

    PyModalDialogWrapper::pysideInitQtMetaTypes();
}
