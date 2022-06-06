
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
#include <signalmanager.h>
CLANG_DIAG_OFF(header-guard)
#include <pysidemetafunction.h> // has wrong header guards in pyside 1.2.2
#include <algorithm>
#include <set>
#include <iterator>

// module include
#include "natrongui_python.h"

// main header
#include "pypanel_wrapper.h"

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

void PyPanelWrapper::pysideInitQtMetaTypes()
{
}

void PyPanelWrapper::resetPyMethodCache()
{
    std::fill_n(m_PyMethodCache, sizeof(m_PyMethodCache) / sizeof(m_PyMethodCache[0]), false);
}

PyPanelWrapper::PyPanelWrapper(const QString & scriptName, const QString & label, bool useUserParameters, GuiApp * app) : PyPanel(scriptName, label, useUserParameters, app)
{
    resetPyMethodCache();
    // ... middle
}

void PyPanelWrapper::actionEvent(QActionEvent * event)
{
    if (m_PyMethodCache[0]) {
        return this->::QWidget::actionEvent(event);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "actionEvent";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[0] = true;
        return this->::QWidget::actionEvent(event);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QACTIONEVENT_IDX]), event)
    ));
    bool invalidateArg1 = PyTuple_GET_ITEM(pyArgs, 0)->ob_refcnt == 1;

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
    if (invalidateArg1)
        Shiboken::Object::invalidate(PyTuple_GET_ITEM(pyArgs, 0));
}

void PyPanelWrapper::changeEvent(QEvent * event)
{
    if (m_PyMethodCache[1]) {
        return this->::QWidget::changeEvent(event);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "changeEvent";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[1] = true;
        return this->::QWidget::changeEvent(event);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtCoreTypes[SBK_QEVENT_IDX]), event)
    ));
    bool invalidateArg1 = PyTuple_GET_ITEM(pyArgs, 0)->ob_refcnt == 1;

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
    if (invalidateArg1)
        Shiboken::Object::invalidate(PyTuple_GET_ITEM(pyArgs, 0));
}

void PyPanelWrapper::childEvent(QChildEvent * event)
{
    if (m_PyMethodCache[2]) {
        return this->::QObject::childEvent(event);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "childEvent";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[2] = true;
        return this->::QObject::childEvent(event);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtCoreTypes[SBK_QCHILDEVENT_IDX]), event)
    ));
    bool invalidateArg1 = PyTuple_GET_ITEM(pyArgs, 0)->ob_refcnt == 1;

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
    if (invalidateArg1)
        Shiboken::Object::invalidate(PyTuple_GET_ITEM(pyArgs, 0));
}

void PyPanelWrapper::closeEvent(QCloseEvent * event)
{
    if (m_PyMethodCache[3]) {
        return this->::QWidget::closeEvent(event);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "closeEvent";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[3] = true;
        return this->::QWidget::closeEvent(event);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QCLOSEEVENT_IDX]), event)
    ));
    bool invalidateArg1 = PyTuple_GET_ITEM(pyArgs, 0)->ob_refcnt == 1;

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
    if (invalidateArg1)
        Shiboken::Object::invalidate(PyTuple_GET_ITEM(pyArgs, 0));
}

void PyPanelWrapper::connectNotify(const QMetaMethod & signal)
{
    if (m_PyMethodCache[4]) {
        return this->::QObject::connectNotify(signal);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "connectNotify";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[4] = true;
        return this->::QObject::connectNotify(signal);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::copyToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtCoreTypes[SBK_QMETAMETHOD_IDX]), &signal)
    ));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
}

void PyPanelWrapper::contextMenuEvent(QContextMenuEvent * event)
{
    if (m_PyMethodCache[5]) {
        return this->::QWidget::contextMenuEvent(event);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "contextMenuEvent";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[5] = true;
        return this->::QWidget::contextMenuEvent(event);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QCONTEXTMENUEVENT_IDX]), event)
    ));
    bool invalidateArg1 = PyTuple_GET_ITEM(pyArgs, 0)->ob_refcnt == 1;

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
    if (invalidateArg1)
        Shiboken::Object::invalidate(PyTuple_GET_ITEM(pyArgs, 0));
}

void PyPanelWrapper::customEvent(QEvent * event)
{
    if (m_PyMethodCache[6]) {
        return this->::QObject::customEvent(event);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "customEvent";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[6] = true;
        return this->::QObject::customEvent(event);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtCoreTypes[SBK_QEVENT_IDX]), event)
    ));
    bool invalidateArg1 = PyTuple_GET_ITEM(pyArgs, 0)->ob_refcnt == 1;

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
    if (invalidateArg1)
        Shiboken::Object::invalidate(PyTuple_GET_ITEM(pyArgs, 0));
}

int PyPanelWrapper::devType() const
{
    if (m_PyMethodCache[7])
        return this->::QWidget::devType();
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return 0;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "devType";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[7] = true;
        return this->::QWidget::devType();
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
        Shiboken::warning(PyExc_RuntimeWarning, 2, "Invalid return value in function %s, expected %s, got %s.", "PyPanel.devType", "int", Py_TYPE(pyResult)->tp_name);
        return 0;
    }
    int cppResult;
    pythonToCpp(pyResult, &cppResult);
    return cppResult;
}

void PyPanelWrapper::disconnectNotify(const QMetaMethod & signal)
{
    if (m_PyMethodCache[8]) {
        return this->::QObject::disconnectNotify(signal);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "disconnectNotify";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[8] = true;
        return this->::QObject::disconnectNotify(signal);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::copyToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtCoreTypes[SBK_QMETAMETHOD_IDX]), &signal)
    ));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
}

void PyPanelWrapper::dragEnterEvent(QDragEnterEvent * event)
{
    if (m_PyMethodCache[9]) {
        return this->::QWidget::dragEnterEvent(event);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "dragEnterEvent";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[9] = true;
        return this->::QWidget::dragEnterEvent(event);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QDRAGENTEREVENT_IDX]), event)
    ));
    bool invalidateArg1 = PyTuple_GET_ITEM(pyArgs, 0)->ob_refcnt == 1;

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
    if (invalidateArg1)
        Shiboken::Object::invalidate(PyTuple_GET_ITEM(pyArgs, 0));
}

void PyPanelWrapper::dragLeaveEvent(QDragLeaveEvent * event)
{
    if (m_PyMethodCache[10]) {
        return this->::QWidget::dragLeaveEvent(event);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "dragLeaveEvent";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[10] = true;
        return this->::QWidget::dragLeaveEvent(event);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QDRAGLEAVEEVENT_IDX]), event)
    ));
    bool invalidateArg1 = PyTuple_GET_ITEM(pyArgs, 0)->ob_refcnt == 1;

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
    if (invalidateArg1)
        Shiboken::Object::invalidate(PyTuple_GET_ITEM(pyArgs, 0));
}

void PyPanelWrapper::dragMoveEvent(QDragMoveEvent * event)
{
    if (m_PyMethodCache[11]) {
        return this->::QWidget::dragMoveEvent(event);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "dragMoveEvent";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[11] = true;
        return this->::QWidget::dragMoveEvent(event);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QDRAGMOVEEVENT_IDX]), event)
    ));
    bool invalidateArg1 = PyTuple_GET_ITEM(pyArgs, 0)->ob_refcnt == 1;

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
    if (invalidateArg1)
        Shiboken::Object::invalidate(PyTuple_GET_ITEM(pyArgs, 0));
}

void PyPanelWrapper::dropEvent(QDropEvent * event)
{
    if (m_PyMethodCache[12]) {
        return this->::QWidget::dropEvent(event);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "dropEvent";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[12] = true;
        return this->::QWidget::dropEvent(event);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QDROPEVENT_IDX]), event)
    ));
    bool invalidateArg1 = PyTuple_GET_ITEM(pyArgs, 0)->ob_refcnt == 1;

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
    if (invalidateArg1)
        Shiboken::Object::invalidate(PyTuple_GET_ITEM(pyArgs, 0));
}

void PyPanelWrapper::enterEvent(QEvent * e)
{
    if (m_PyMethodCache[13]) {
        return this->::PyPanel::enterEvent(e);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "enterEvent";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[13] = true;
        return this->::PyPanel::enterEvent(e);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtCoreTypes[SBK_QEVENT_IDX]), e)
    ));
    bool invalidateArg1 = PyTuple_GET_ITEM(pyArgs, 0)->ob_refcnt == 1;

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
    if (invalidateArg1)
        Shiboken::Object::invalidate(PyTuple_GET_ITEM(pyArgs, 0));
}

bool PyPanelWrapper::event(QEvent * event)
{
    if (m_PyMethodCache[14])
        return this->::QWidget::event(event);
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return false;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "event";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[14] = true;
        return this->::QWidget::event(event);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtCoreTypes[SBK_QEVENT_IDX]), event)
    ));
    bool invalidateArg1 = PyTuple_GET_ITEM(pyArgs, 0)->ob_refcnt == 1;

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return false;
    }
    // Check return type
    PythonToCppFunc pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), pyResult);
    if (!pythonToCpp) {
        Shiboken::warning(PyExc_RuntimeWarning, 2, "Invalid return value in function %s, expected %s, got %s.", "PyPanel.event", "bool", Py_TYPE(pyResult)->tp_name);
        return false;
    }
    bool cppResult;
    pythonToCpp(pyResult, &cppResult);
    if (invalidateArg1)
        Shiboken::Object::invalidate(PyTuple_GET_ITEM(pyArgs, 0));
    return cppResult;
}

bool PyPanelWrapper::eventFilter(QObject * watched, QEvent * event)
{
    if (m_PyMethodCache[15])
        return this->::QObject::eventFilter(watched, event);
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return false;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "eventFilter";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[15] = true;
        return this->::QObject::eventFilter(watched, event);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(NN)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtCoreTypes[SBK_QOBJECT_IDX]), watched),
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtCoreTypes[SBK_QEVENT_IDX]), event)
    ));
    bool invalidateArg2 = PyTuple_GET_ITEM(pyArgs, 1)->ob_refcnt == 1;

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return false;
    }
    // Check return type
    PythonToCppFunc pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), pyResult);
    if (!pythonToCpp) {
        Shiboken::warning(PyExc_RuntimeWarning, 2, "Invalid return value in function %s, expected %s, got %s.", "PyPanel.eventFilter", "bool", Py_TYPE(pyResult)->tp_name);
        return false;
    }
    bool cppResult;
    pythonToCpp(pyResult, &cppResult);
    if (invalidateArg2)
        Shiboken::Object::invalidate(PyTuple_GET_ITEM(pyArgs, 1));
    return cppResult;
}

void PyPanelWrapper::focusInEvent(QFocusEvent * event)
{
    if (m_PyMethodCache[16]) {
        return this->::QWidget::focusInEvent(event);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "focusInEvent";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[16] = true;
        return this->::QWidget::focusInEvent(event);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QFOCUSEVENT_IDX]), event)
    ));
    bool invalidateArg1 = PyTuple_GET_ITEM(pyArgs, 0)->ob_refcnt == 1;

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
    if (invalidateArg1)
        Shiboken::Object::invalidate(PyTuple_GET_ITEM(pyArgs, 0));
}

bool PyPanelWrapper::focusNextPrevChild(bool next)
{
    if (m_PyMethodCache[17])
        return this->::QWidget::focusNextPrevChild(next);
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return false;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "focusNextPrevChild";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[17] = true;
        return this->::QWidget::focusNextPrevChild(next);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &next)
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
        Shiboken::warning(PyExc_RuntimeWarning, 2, "Invalid return value in function %s, expected %s, got %s.", "PyPanel.focusNextPrevChild", "bool", Py_TYPE(pyResult)->tp_name);
        return false;
    }
    bool cppResult;
    pythonToCpp(pyResult, &cppResult);
    return cppResult;
}

void PyPanelWrapper::focusOutEvent(QFocusEvent * event)
{
    if (m_PyMethodCache[18]) {
        return this->::QWidget::focusOutEvent(event);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "focusOutEvent";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[18] = true;
        return this->::QWidget::focusOutEvent(event);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QFOCUSEVENT_IDX]), event)
    ));
    bool invalidateArg1 = PyTuple_GET_ITEM(pyArgs, 0)->ob_refcnt == 1;

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
    if (invalidateArg1)
        Shiboken::Object::invalidate(PyTuple_GET_ITEM(pyArgs, 0));
}

bool PyPanelWrapper::hasHeightForWidth() const
{
    if (m_PyMethodCache[19])
        return this->::QWidget::hasHeightForWidth();
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return false;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "hasHeightForWidth";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[19] = true;
        return this->::QWidget::hasHeightForWidth();
    }

    Shiboken::AutoDecRef pyArgs(PyTuple_New(0));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return false;
    }
    // Check return type
    PythonToCppFunc pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), pyResult);
    if (!pythonToCpp) {
        Shiboken::warning(PyExc_RuntimeWarning, 2, "Invalid return value in function %s, expected %s, got %s.", "PyPanel.hasHeightForWidth", "bool", Py_TYPE(pyResult)->tp_name);
        return false;
    }
    bool cppResult;
    pythonToCpp(pyResult, &cppResult);
    return cppResult;
}

int PyPanelWrapper::heightForWidth(int arg__1) const
{
    if (m_PyMethodCache[20])
        return this->::QWidget::heightForWidth(arg__1);
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return 0;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "heightForWidth";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[20] = true;
        return this->::QWidget::heightForWidth(arg__1);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(i)",
    arg__1
    ));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return 0;
    }
    // Check return type
    PythonToCppFunc pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), pyResult);
    if (!pythonToCpp) {
        Shiboken::warning(PyExc_RuntimeWarning, 2, "Invalid return value in function %s, expected %s, got %s.", "PyPanel.heightForWidth", "int", Py_TYPE(pyResult)->tp_name);
        return 0;
    }
    int cppResult;
    pythonToCpp(pyResult, &cppResult);
    return cppResult;
}

void PyPanelWrapper::hideEvent(QHideEvent * event)
{
    if (m_PyMethodCache[21]) {
        return this->::QWidget::hideEvent(event);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "hideEvent";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[21] = true;
        return this->::QWidget::hideEvent(event);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QHIDEEVENT_IDX]), event)
    ));
    bool invalidateArg1 = PyTuple_GET_ITEM(pyArgs, 0)->ob_refcnt == 1;

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
    if (invalidateArg1)
        Shiboken::Object::invalidate(PyTuple_GET_ITEM(pyArgs, 0));
}

void PyPanelWrapper::initPainter(QPainter * painter) const
{
    if (m_PyMethodCache[22]) {
        return this->::QWidget::initPainter(painter);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "initPainter";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[22] = true;
        return this->::QWidget::initPainter(painter);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QPAINTER_IDX]), painter)
    ));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
}

void PyPanelWrapper::inputMethodEvent(QInputMethodEvent * event)
{
    if (m_PyMethodCache[23]) {
        return this->::QWidget::inputMethodEvent(event);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "inputMethodEvent";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[23] = true;
        return this->::QWidget::inputMethodEvent(event);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QINPUTMETHODEVENT_IDX]), event)
    ));
    bool invalidateArg1 = PyTuple_GET_ITEM(pyArgs, 0)->ob_refcnt == 1;

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
    if (invalidateArg1)
        Shiboken::Object::invalidate(PyTuple_GET_ITEM(pyArgs, 0));
}

QVariant PyPanelWrapper::inputMethodQuery(Qt::InputMethodQuery arg__1) const
{
    if (m_PyMethodCache[24])
        return this->::QWidget::inputMethodQuery(arg__1);
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return ::QVariant();
    static PyObject *nameCache[2] = {};
    static const char *funcName = "inputMethodQuery";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[24] = true;
        return this->::QWidget::inputMethodQuery(arg__1);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::copyToPython(*PepType_SGTP(SbkPySide2_QtCoreTypes[SBK_QT_INPUTMETHODQUERY_IDX])->converter, &arg__1)
    ));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return ::QVariant();
    }
    // Check return type
    PythonToCppFunc pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QVARIANT_IDX], pyResult);
    if (!pythonToCpp) {
        Shiboken::warning(PyExc_RuntimeWarning, 2, "Invalid return value in function %s, expected %s, got %s.", "PyPanel.inputMethodQuery", "QVariant", Py_TYPE(pyResult)->tp_name);
        return ::QVariant();
    }
    ::QVariant cppResult;
    pythonToCpp(pyResult, &cppResult);
    return cppResult;
}

void PyPanelWrapper::keyPressEvent(QKeyEvent * e)
{
    if (m_PyMethodCache[25]) {
        return this->::PyPanel::keyPressEvent(e);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "keyPressEvent";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[25] = true;
        return this->::PyPanel::keyPressEvent(e);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QKEYEVENT_IDX]), e)
    ));
    bool invalidateArg1 = PyTuple_GET_ITEM(pyArgs, 0)->ob_refcnt == 1;

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
    if (invalidateArg1)
        Shiboken::Object::invalidate(PyTuple_GET_ITEM(pyArgs, 0));
}

void PyPanelWrapper::keyReleaseEvent(QKeyEvent * event)
{
    if (m_PyMethodCache[26]) {
        return this->::QWidget::keyReleaseEvent(event);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "keyReleaseEvent";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[26] = true;
        return this->::QWidget::keyReleaseEvent(event);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QKEYEVENT_IDX]), event)
    ));
    bool invalidateArg1 = PyTuple_GET_ITEM(pyArgs, 0)->ob_refcnt == 1;

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
    if (invalidateArg1)
        Shiboken::Object::invalidate(PyTuple_GET_ITEM(pyArgs, 0));
}

void PyPanelWrapper::leaveEvent(QEvent * e)
{
    if (m_PyMethodCache[27]) {
        return this->::PyPanel::leaveEvent(e);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "leaveEvent";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[27] = true;
        return this->::PyPanel::leaveEvent(e);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtCoreTypes[SBK_QEVENT_IDX]), e)
    ));
    bool invalidateArg1 = PyTuple_GET_ITEM(pyArgs, 0)->ob_refcnt == 1;

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
    if (invalidateArg1)
        Shiboken::Object::invalidate(PyTuple_GET_ITEM(pyArgs, 0));
}

int PyPanelWrapper::metric(QPaintDevice::PaintDeviceMetric arg__1) const
{
    if (m_PyMethodCache[29])
        return this->::QWidget::metric(arg__1);
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return 0;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "metric";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[29] = true;
        return this->::QWidget::metric(arg__1);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::copyToPython(*PepType_SGTP(SbkPySide2_QtGuiTypes[SBK_QPAINTDEVICE_PAINTDEVICEMETRIC_IDX])->converter, &arg__1)
    ));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return 0;
    }
    // Check return type
    PythonToCppFunc pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), pyResult);
    if (!pythonToCpp) {
        Shiboken::warning(PyExc_RuntimeWarning, 2, "Invalid return value in function %s, expected %s, got %s.", "PyPanel.metric", "int", Py_TYPE(pyResult)->tp_name);
        return 0;
    }
    int cppResult;
    pythonToCpp(pyResult, &cppResult);
    return cppResult;
}

QSize PyPanelWrapper::minimumSizeHint() const
{
    if (m_PyMethodCache[30])
        return this->::QWidget::minimumSizeHint();
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return {};
    static PyObject *nameCache[2] = {};
    // This method belongs to a property.
    static const char *funcName = "1:minimumSizeHint";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[30] = true;
        return this->::QWidget::minimumSizeHint();
    }

    Shiboken::AutoDecRef pyArgs(PyTuple_New(0));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return {};
    }
    // Check return type
    PythonToCppFunc pythonToCpp = Shiboken::Conversions::isPythonToCppValueConvertible(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtCoreTypes[SBK_QSIZE_IDX]), pyResult);
    if (!pythonToCpp) {
        Shiboken::warning(PyExc_RuntimeWarning, 2, "Invalid return value in function %s, expected %s, got %s.", "PyPanel.minimumSizeHint", reinterpret_cast<PyTypeObject *>(Shiboken::SbkType< QSize >())->tp_name, Py_TYPE(pyResult)->tp_name);
        return {};
    }
    ::QSize cppResult;
    pythonToCpp(pyResult, &cppResult);
    return cppResult;
}

void PyPanelWrapper::mouseDoubleClickEvent(QMouseEvent * event)
{
    if (m_PyMethodCache[31]) {
        return this->::QWidget::mouseDoubleClickEvent(event);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "mouseDoubleClickEvent";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[31] = true;
        return this->::QWidget::mouseDoubleClickEvent(event);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QMOUSEEVENT_IDX]), event)
    ));
    bool invalidateArg1 = PyTuple_GET_ITEM(pyArgs, 0)->ob_refcnt == 1;

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
    if (invalidateArg1)
        Shiboken::Object::invalidate(PyTuple_GET_ITEM(pyArgs, 0));
}

void PyPanelWrapper::mouseMoveEvent(QMouseEvent * event)
{
    if (m_PyMethodCache[32]) {
        return this->::QWidget::mouseMoveEvent(event);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "mouseMoveEvent";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[32] = true;
        return this->::QWidget::mouseMoveEvent(event);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QMOUSEEVENT_IDX]), event)
    ));
    bool invalidateArg1 = PyTuple_GET_ITEM(pyArgs, 0)->ob_refcnt == 1;

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
    if (invalidateArg1)
        Shiboken::Object::invalidate(PyTuple_GET_ITEM(pyArgs, 0));
}

void PyPanelWrapper::mousePressEvent(QMouseEvent * e)
{
    if (m_PyMethodCache[33]) {
        return this->::PyPanel::mousePressEvent(e);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "mousePressEvent";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[33] = true;
        return this->::PyPanel::mousePressEvent(e);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QMOUSEEVENT_IDX]), e)
    ));
    bool invalidateArg1 = PyTuple_GET_ITEM(pyArgs, 0)->ob_refcnt == 1;

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
    if (invalidateArg1)
        Shiboken::Object::invalidate(PyTuple_GET_ITEM(pyArgs, 0));
}

void PyPanelWrapper::mouseReleaseEvent(QMouseEvent * event)
{
    if (m_PyMethodCache[34]) {
        return this->::QWidget::mouseReleaseEvent(event);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "mouseReleaseEvent";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[34] = true;
        return this->::QWidget::mouseReleaseEvent(event);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QMOUSEEVENT_IDX]), event)
    ));
    bool invalidateArg1 = PyTuple_GET_ITEM(pyArgs, 0)->ob_refcnt == 1;

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
    if (invalidateArg1)
        Shiboken::Object::invalidate(PyTuple_GET_ITEM(pyArgs, 0));
}

void PyPanelWrapper::moveEvent(QMoveEvent * event)
{
    if (m_PyMethodCache[35]) {
        return this->::QWidget::moveEvent(event);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "moveEvent";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[35] = true;
        return this->::QWidget::moveEvent(event);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QMOVEEVENT_IDX]), event)
    ));
    bool invalidateArg1 = PyTuple_GET_ITEM(pyArgs, 0)->ob_refcnt == 1;

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
    if (invalidateArg1)
        Shiboken::Object::invalidate(PyTuple_GET_ITEM(pyArgs, 0));
}

bool PyPanelWrapper::nativeEvent(const QByteArray & eventType, void * message, long * result)
{
    if (m_PyMethodCache[36]) {
        return this->::QWidget::nativeEvent(eventType, message, result);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return false;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "nativeEvent";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[36] = true;
        return this->::QWidget::nativeEvent(eventType, message, result);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(NN)",
    Shiboken::Conversions::copyToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtCoreTypes[SBK_QBYTEARRAY_IDX]), &eventType),
    Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<void *>(), message)
    ));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return false;
    }
    // Begin code injection
    // TEMPLATE - return_native_eventfilter_conversion - START
    bool cppResult = false;
    if (PySequence_Check(pyResult) && (PySequence_Size(pyResult) == 2)) {
        Shiboken::AutoDecRef pyItem(PySequence_GetItem(pyResult, 0));
        Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), pyItem, &(cppResult));
        if (result) {
            Shiboken::AutoDecRef pyResultItem(PySequence_GetItem(pyResult, 1));
            Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<long>(), pyResultItem, (result));
        }
    }
    // TEMPLATE - return_native_eventfilter_conversion - END

    // End of code injection


    return cppResult;
}

QPaintEngine * PyPanelWrapper::paintEngine() const
{
    if (m_PyMethodCache[37])
        return this->::QWidget::paintEngine();
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return nullptr;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "paintEngine";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[37] = true;
        return this->::QWidget::paintEngine();
    }

    Shiboken::AutoDecRef pyArgs(PyTuple_New(0));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return nullptr;
    }
    // Check return type
    PythonToCppFunc pythonToCpp = Shiboken::Conversions::isPythonToCppPointerConvertible(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QPAINTENGINE_IDX]), pyResult);
    if (!pythonToCpp) {
        Shiboken::warning(PyExc_RuntimeWarning, 2, "Invalid return value in function %s, expected %s, got %s.", "PyPanel.paintEngine", reinterpret_cast<PyTypeObject *>(Shiboken::SbkType< QPaintEngine >())->tp_name, Py_TYPE(pyResult)->tp_name);
        return nullptr;
    }
    ::QPaintEngine *cppResult;
    pythonToCpp(pyResult, &cppResult);
    return cppResult;
}

void PyPanelWrapper::paintEvent(QPaintEvent * event)
{
    if (m_PyMethodCache[38]) {
        return this->::QWidget::paintEvent(event);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "paintEvent";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[38] = true;
        return this->::QWidget::paintEvent(event);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QPAINTEVENT_IDX]), event)
    ));
    bool invalidateArg1 = PyTuple_GET_ITEM(pyArgs, 0)->ob_refcnt == 1;

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
    if (invalidateArg1)
        Shiboken::Object::invalidate(PyTuple_GET_ITEM(pyArgs, 0));
}

QPaintDevice * PyPanelWrapper::redirected(QPoint * offset) const
{
    if (m_PyMethodCache[39])
        return this->::QWidget::redirected(offset);
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return nullptr;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "redirected";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[39] = true;
        return this->::QWidget::redirected(offset);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtCoreTypes[SBK_QPOINT_IDX]), offset)
    ));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return nullptr;
    }
    // Check return type
    PythonToCppFunc pythonToCpp = Shiboken::Conversions::isPythonToCppPointerConvertible(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QPAINTDEVICE_IDX]), pyResult);
    if (!pythonToCpp) {
        Shiboken::warning(PyExc_RuntimeWarning, 2, "Invalid return value in function %s, expected %s, got %s.", "PyPanel.redirected", reinterpret_cast<PyTypeObject *>(Shiboken::SbkType< QPaintDevice >())->tp_name, Py_TYPE(pyResult)->tp_name);
        return nullptr;
    }
    ::QPaintDevice *cppResult;
    pythonToCpp(pyResult, &cppResult);
    return cppResult;
}

void PyPanelWrapper::resizeEvent(QResizeEvent * event)
{
    if (m_PyMethodCache[40]) {
        return this->::QWidget::resizeEvent(event);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "resizeEvent";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[40] = true;
        return this->::QWidget::resizeEvent(event);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QRESIZEEVENT_IDX]), event)
    ));
    bool invalidateArg1 = PyTuple_GET_ITEM(pyArgs, 0)->ob_refcnt == 1;

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
    if (invalidateArg1)
        Shiboken::Object::invalidate(PyTuple_GET_ITEM(pyArgs, 0));
}

void PyPanelWrapper::restore(const QString & arg__1)
{
    if (m_PyMethodCache[41]) {
        return this->::PyPanel::restore(arg__1);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "restore";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[41] = true;
        return this->::PyPanel::restore(arg__1);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::copyToPython(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], &arg__1)
    ));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
}

QString PyPanelWrapper::save()
{
    if (m_PyMethodCache[42])
        return this->::PyPanel::save();
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return ::QString();
    static PyObject *nameCache[2] = {};
    static const char *funcName = "save";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[42] = true;
        return this->::PyPanel::save();
    }

    Shiboken::AutoDecRef pyArgs(PyTuple_New(0));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return ::QString();
    }
    // Check return type
    PythonToCppFunc pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], pyResult);
    if (!pythonToCpp) {
        Shiboken::warning(PyExc_RuntimeWarning, 2, "Invalid return value in function %s, expected %s, got %s.", "PyPanel.save", "QString", Py_TYPE(pyResult)->tp_name);
        return ::QString();
    }
    ::QString cppResult;
    pythonToCpp(pyResult, &cppResult);
    return cppResult;
}

void PyPanelWrapper::setVisible(bool visible)
{
    if (m_PyMethodCache[43]) {
        return this->::QWidget::setVisible(visible);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    // This method belongs to a property.
    static const char *funcName = "2:setVisible";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[43] = true;
        return this->::QWidget::setVisible(visible);
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

QPainter * PyPanelWrapper::sharedPainter() const
{
    if (m_PyMethodCache[44])
        return this->::QWidget::sharedPainter();
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return nullptr;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "sharedPainter";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[44] = true;
        return this->::QWidget::sharedPainter();
    }

    Shiboken::AutoDecRef pyArgs(PyTuple_New(0));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return nullptr;
    }
    // Check return type
    PythonToCppFunc pythonToCpp = Shiboken::Conversions::isPythonToCppPointerConvertible(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QPAINTER_IDX]), pyResult);
    if (!pythonToCpp) {
        Shiboken::warning(PyExc_RuntimeWarning, 2, "Invalid return value in function %s, expected %s, got %s.", "PyPanel.sharedPainter", reinterpret_cast<PyTypeObject *>(Shiboken::SbkType< QPainter >())->tp_name, Py_TYPE(pyResult)->tp_name);
        return nullptr;
    }
    ::QPainter *cppResult;
    pythonToCpp(pyResult, &cppResult);
    return cppResult;
}

void PyPanelWrapper::showEvent(QShowEvent * event)
{
    if (m_PyMethodCache[45]) {
        return this->::QWidget::showEvent(event);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "showEvent";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[45] = true;
        return this->::QWidget::showEvent(event);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QSHOWEVENT_IDX]), event)
    ));
    bool invalidateArg1 = PyTuple_GET_ITEM(pyArgs, 0)->ob_refcnt == 1;

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
    if (invalidateArg1)
        Shiboken::Object::invalidate(PyTuple_GET_ITEM(pyArgs, 0));
}

QSize PyPanelWrapper::sizeHint() const
{
    if (m_PyMethodCache[46])
        return this->::QWidget::sizeHint();
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return {};
    static PyObject *nameCache[2] = {};
    // This method belongs to a property.
    static const char *funcName = "1:sizeHint";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[46] = true;
        return this->::QWidget::sizeHint();
    }

    Shiboken::AutoDecRef pyArgs(PyTuple_New(0));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return {};
    }
    // Check return type
    PythonToCppFunc pythonToCpp = Shiboken::Conversions::isPythonToCppValueConvertible(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtCoreTypes[SBK_QSIZE_IDX]), pyResult);
    if (!pythonToCpp) {
        Shiboken::warning(PyExc_RuntimeWarning, 2, "Invalid return value in function %s, expected %s, got %s.", "PyPanel.sizeHint", reinterpret_cast<PyTypeObject *>(Shiboken::SbkType< QSize >())->tp_name, Py_TYPE(pyResult)->tp_name);
        return {};
    }
    ::QSize cppResult;
    pythonToCpp(pyResult, &cppResult);
    return cppResult;
}

void PyPanelWrapper::tabletEvent(QTabletEvent * event)
{
    if (m_PyMethodCache[47]) {
        return this->::QWidget::tabletEvent(event);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "tabletEvent";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[47] = true;
        return this->::QWidget::tabletEvent(event);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QTABLETEVENT_IDX]), event)
    ));
    bool invalidateArg1 = PyTuple_GET_ITEM(pyArgs, 0)->ob_refcnt == 1;

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
    if (invalidateArg1)
        Shiboken::Object::invalidate(PyTuple_GET_ITEM(pyArgs, 0));
}

void PyPanelWrapper::timerEvent(QTimerEvent * event)
{
    if (m_PyMethodCache[48]) {
        return this->::QObject::timerEvent(event);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "timerEvent";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[48] = true;
        return this->::QObject::timerEvent(event);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtCoreTypes[SBK_QTIMEREVENT_IDX]), event)
    ));
    bool invalidateArg1 = PyTuple_GET_ITEM(pyArgs, 0)->ob_refcnt == 1;

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
    if (invalidateArg1)
        Shiboken::Object::invalidate(PyTuple_GET_ITEM(pyArgs, 0));
}

void PyPanelWrapper::wheelEvent(QWheelEvent * event)
{
    if (m_PyMethodCache[49]) {
        return this->::QWidget::wheelEvent(event);
    }
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return;
    static PyObject *nameCache[2] = {};
    static const char *funcName = "wheelEvent";
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, nameCache, funcName));
    if (pyOverride.isNull()) {
        gil.release();
        m_PyMethodCache[49] = true;
        return this->::QWidget::wheelEvent(event);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
    Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QWHEELEVENT_IDX]), event)
    ));
    bool invalidateArg1 = PyTuple_GET_ITEM(pyArgs, 0)->ob_refcnt == 1;

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, nullptr));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return;
    }
    if (invalidateArg1)
        Shiboken::Object::invalidate(PyTuple_GET_ITEM(pyArgs, 0));
}

const QMetaObject *PyPanelWrapper::metaObject() const
{
    if (QObject::d_ptr->metaObject)
        return QObject::d_ptr->dynamicMetaObject();
    SbkObject *pySelf = Shiboken::BindingManager::instance().retrieveWrapper(this);
    if (pySelf == nullptr)
        return PyPanel::metaObject();
    return PySide::SignalManager::retrieveMetaObject(reinterpret_cast<PyObject *>(pySelf));
}

int PyPanelWrapper::qt_metacall(QMetaObject::Call call, int id, void **args)
{
    int result = PyPanel::qt_metacall(call, id, args);
    return result < 0 ? result : PySide::SignalManager::qt_metacall(this, call, id, args);
}

void *PyPanelWrapper::qt_metacast(const char *_clname)
{
        if (!_clname) return {};
        SbkObject *pySelf = Shiboken::BindingManager::instance().retrieveWrapper(this);
        if (pySelf && PySide::inherits(Py_TYPE(pySelf), _clname))
                return static_cast<void *>(const_cast< PyPanelWrapper *>(this));
        return PyPanel::qt_metacast(_clname);
}

PyPanelWrapper::~PyPanelWrapper()
{
    SbkObject *wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static int
Sbk_PyPanel_Init(PyObject *self, PyObject *args, PyObject *kwds)
{
    const QMetaObject *metaObject;
    SbkObject *sbkSelf = reinterpret_cast<SbkObject *>(self);
    SbkObjectType *type = reinterpret_cast<SbkObjectType *>(self->ob_type);
    SbkObjectType *myType = reinterpret_cast<SbkObjectType *>(SbkNatronGuiTypes[SBK_PYPANEL_IDX]);
    if (type != myType)
        Shiboken::ObjectType::copyMultipleInheritance(type, myType);

PySide::Feature::Select(self);
    if (Shiboken::Object::isUserType(self) && !Shiboken::ObjectType::canCallConstructor(self->ob_type, Shiboken::SbkType< ::PyPanel >()))
        return -1;

    ::PyPanelWrapper *cptr{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyPanel.__init__";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "PyPanel", 4, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return -1;


    // Overloaded function decisor
    // 0: PyPanel::PyPanel(QString,QString,bool,GuiApp*)
    if (numArgs == 4
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppPointerConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronGuiTypes[SBK_GUIAPP_IDX]), (pyArgs[3])))) {
        overloadId = 0; // PyPanel(QString,QString,bool,GuiApp*)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyPanel_Init_TypeError;

    // Call function/method
    {
        ::QString cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::QString cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        bool cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);
        if (!Shiboken::Object::isValid(pyArgs[3]))
            return -1;
        ::GuiApp *cppArg3;
        pythonToCpp[3](pyArgs[3], &cppArg3);

        if (!PyErr_Occurred()) {
            // PyPanel(QString,QString,bool,GuiApp*)
            void *addr = PySide::nextQObjectMemoryAddr();
            if (addr) {
                cptr = new (addr) ::PyPanelWrapper(cppArg0, cppArg1, cppArg2, cppArg3);
                PySide::setNextQObjectMemoryAddr(0);
            } else {
                cptr = new ::PyPanelWrapper(cppArg0, cppArg1, cppArg2, cppArg3);
            }

        }
    }

    if (PyErr_Occurred() || !Shiboken::Object::setCppPointer(sbkSelf, Shiboken::SbkType< ::PyPanel >(), cptr)) {
        delete cptr;
        Py_XDECREF(errInfo);
        return -1;
    }
    if (!cptr) goto Sbk_PyPanel_Init_TypeError;

    Shiboken::Object::setValidCpp(sbkSelf, true);
    Shiboken::Object::setHasCppWrapper(sbkSelf, true);
    if (Shiboken::BindingManager::instance().hasWrapper(cptr)) {
        Shiboken::BindingManager::instance().releaseWrapper(Shiboken::BindingManager::instance().retrieveWrapper(cptr));
    }
    Shiboken::BindingManager::instance().registerWrapper(sbkSelf, cptr);

    // QObject setup
    PySide::Signal::updateSourceObject(self);
    metaObject = cptr->metaObject(); // <- init python qt properties
    if (errInfo && PyDict_Check(errInfo)) {
        if (!PySide::fillQtProperties(self, metaObject, errInfo))
            goto Sbk_PyPanel_Init_TypeError;
        Py_DECREF(errInfo);
    };


    return 1;

    Sbk_PyPanel_Init_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return -1;
}

static PyObject *Sbk_PyPanelFunc_addWidget(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<PyPanelWrapper *>(reinterpret_cast< ::PyPanel *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYPANEL_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyPanel.addWidget";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: PyPanel::addWidget(QWidget*)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppPointerConvertible(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtWidgetsTypes[SBK_QWIDGET_IDX]), (pyArg)))) {
        overloadId = 0; // addWidget(QWidget*)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyPanelFunc_addWidget_TypeError;

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

    Sbk_PyPanelFunc_addWidget_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_PyPanelFunc_enterEvent(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<PyPanelWrapper *>(reinterpret_cast< ::PyPanel *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYPANEL_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyPanel.enterEvent";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: QWidget::enterEvent(QEvent*)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppPointerConvertible(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtCoreTypes[SBK_QEVENT_IDX]), (pyArg)))) {
        overloadId = 0; // enterEvent(QEvent*)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyPanelFunc_enterEvent_TypeError;

    // Call function/method
    {
        if (!Shiboken::Object::isValid(pyArg))
            return {};
        ::QEvent *cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // enterEvent(QEvent*)
            static_cast<::PyPanelWrapper *>(cppSelf)->PyPanelWrapper::enterEvent_protected(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_PyPanelFunc_enterEvent_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_PyPanelFunc_getPanelLabel(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<PyPanelWrapper *>(reinterpret_cast< ::PyPanel *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYPANEL_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyPanel.getPanelLabel";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getPanelLabel()const
            QString cppResult = const_cast<const ::PyPanelWrapper *>(cppSelf)->getPanelLabel();
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_PyPanelFunc_getPanelScriptName(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<PyPanelWrapper *>(reinterpret_cast< ::PyPanel *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYPANEL_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyPanel.getPanelScriptName";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getPanelScriptName()const
            QString cppResult = const_cast<const ::PyPanelWrapper *>(cppSelf)->getPanelScriptName();
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_PyPanelFunc_getParam(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<PyPanelWrapper *>(reinterpret_cast< ::PyPanel *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYPANEL_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyPanel.getParam";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: PyPanel::getParam(QString)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // getParam(QString)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyPanelFunc_getParam_TypeError;

    // Call function/method
    {
        ::QString cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getParam(QString)const
            Param * cppResult = const_cast<const ::PyPanelWrapper *>(cppSelf)->getParam(cppArg0);
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

    Sbk_PyPanelFunc_getParam_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_PyPanelFunc_getParams(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<PyPanelWrapper *>(reinterpret_cast< ::PyPanel *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYPANEL_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyPanel.getParams";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getParams()const
            // Begin code injection
            std::list<Param*> params = cppSelf->getParams();
            PyObject* ret = PyList_New((int) params.size());
            int idx = 0;
            for (std::list<Param*>::iterator it = params.begin(); it!=params.end(); ++it,++idx) {
            PyObject* item = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_PARAM_IDX]), *it);
            // Ownership transferences.
            Shiboken::Object::getOwnership(item);
            PyList_SET_ITEM(ret, idx, item);
            }
            return ret;

            // End of code injection

        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_PyPanelFunc_insertWidget(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<PyPanelWrapper *>(reinterpret_cast< ::PyPanel *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYPANEL_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyPanel.insertWidget";
    SBK_UNUSED(fullName)
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
    // 0: PyPanel::insertWidget(int,QWidget*)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppPointerConvertible(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtWidgetsTypes[SBK_QWIDGET_IDX]), (pyArgs[1])))) {
        overloadId = 0; // insertWidget(int,QWidget*)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyPanelFunc_insertWidget_TypeError;

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

    Sbk_PyPanelFunc_insertWidget_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_PyPanelFunc_keyPressEvent(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<PyPanelWrapper *>(reinterpret_cast< ::PyPanel *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYPANEL_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyPanel.keyPressEvent";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: QWidget::keyPressEvent(QKeyEvent*)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppPointerConvertible(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QKEYEVENT_IDX]), (pyArg)))) {
        overloadId = 0; // keyPressEvent(QKeyEvent*)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyPanelFunc_keyPressEvent_TypeError;

    // Call function/method
    {
        if (!Shiboken::Object::isValid(pyArg))
            return {};
        ::QKeyEvent *cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // keyPressEvent(QKeyEvent*)
            static_cast<::PyPanelWrapper *>(cppSelf)->PyPanelWrapper::keyPressEvent_protected(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_PyPanelFunc_keyPressEvent_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_PyPanelFunc_leaveEvent(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<PyPanelWrapper *>(reinterpret_cast< ::PyPanel *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYPANEL_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyPanel.leaveEvent";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: QWidget::leaveEvent(QEvent*)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppPointerConvertible(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtCoreTypes[SBK_QEVENT_IDX]), (pyArg)))) {
        overloadId = 0; // leaveEvent(QEvent*)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyPanelFunc_leaveEvent_TypeError;

    // Call function/method
    {
        if (!Shiboken::Object::isValid(pyArg))
            return {};
        ::QEvent *cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // leaveEvent(QEvent*)
            static_cast<::PyPanelWrapper *>(cppSelf)->PyPanelWrapper::leaveEvent_protected(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_PyPanelFunc_leaveEvent_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_PyPanelFunc_mousePressEvent(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<PyPanelWrapper *>(reinterpret_cast< ::PyPanel *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYPANEL_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyPanel.mousePressEvent";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: QWidget::mousePressEvent(QMouseEvent*)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppPointerConvertible(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QMOUSEEVENT_IDX]), (pyArg)))) {
        overloadId = 0; // mousePressEvent(QMouseEvent*)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyPanelFunc_mousePressEvent_TypeError;

    // Call function/method
    {
        if (!Shiboken::Object::isValid(pyArg))
            return {};
        ::QMouseEvent *cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // mousePressEvent(QMouseEvent*)
            static_cast<::PyPanelWrapper *>(cppSelf)->PyPanelWrapper::mousePressEvent_protected(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_PyPanelFunc_mousePressEvent_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_PyPanelFunc_onUserDataChanged(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<PyPanelWrapper *>(reinterpret_cast< ::PyPanel *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYPANEL_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyPanel.onUserDataChanged";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // onUserDataChanged()
            static_cast<::PyPanelWrapper *>(cppSelf)->PyPanelWrapper::onUserDataChanged_protected();
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;
}

static PyObject *Sbk_PyPanelFunc_restore(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<PyPanelWrapper *>(reinterpret_cast< ::PyPanel *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYPANEL_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyPanel.restore";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: PyPanel::restore(QString)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // restore(QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyPanelFunc_restore_TypeError;

    // Call function/method
    {
        ::QString cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // restore(QString)
            Shiboken::Object::hasCppWrapper(reinterpret_cast<SbkObject *>(self))
                ? cppSelf->::PyPanel::restore(cppArg0)
                : cppSelf->restore(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_PyPanelFunc_restore_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_PyPanelFunc_save(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<PyPanelWrapper *>(reinterpret_cast< ::PyPanel *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYPANEL_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyPanel.save";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // save()
            QString cppResult = static_cast<::PyPanelWrapper *>(cppSelf)->PyPanelWrapper::save_protected();
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_PyPanelFunc_setPanelLabel(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<PyPanelWrapper *>(reinterpret_cast< ::PyPanel *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYPANEL_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyPanel.setPanelLabel";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: PyPanel::setPanelLabel(QString)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // setPanelLabel(QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyPanelFunc_setPanelLabel_TypeError;

    // Call function/method
    {
        ::QString cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setPanelLabel(QString)
            cppSelf->setPanelLabel(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_PyPanelFunc_setPanelLabel_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_PyPanelFunc_setParamChangedCallback(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<PyPanelWrapper *>(reinterpret_cast< ::PyPanel *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYPANEL_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyPanel.setParamChangedCallback";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: PyPanel::setParamChangedCallback(QString)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // setParamChangedCallback(QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyPanelFunc_setParamChangedCallback_TypeError;

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

    Sbk_PyPanelFunc_setParamChangedCallback_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}


static const char *Sbk_PyPanel_PropertyStrings[] = {
    nullptr // Sentinel
};

static PyMethodDef Sbk_PyPanel_methods[] = {
    {"addWidget", reinterpret_cast<PyCFunction>(Sbk_PyPanelFunc_addWidget), METH_O},
    {"enterEvent", reinterpret_cast<PyCFunction>(Sbk_PyPanelFunc_enterEvent), METH_O},
    {"getPanelLabel", reinterpret_cast<PyCFunction>(Sbk_PyPanelFunc_getPanelLabel), METH_NOARGS},
    {"getPanelScriptName", reinterpret_cast<PyCFunction>(Sbk_PyPanelFunc_getPanelScriptName), METH_NOARGS},
    {"getParam", reinterpret_cast<PyCFunction>(Sbk_PyPanelFunc_getParam), METH_O},
    {"getParams", reinterpret_cast<PyCFunction>(Sbk_PyPanelFunc_getParams), METH_NOARGS},
    {"insertWidget", reinterpret_cast<PyCFunction>(Sbk_PyPanelFunc_insertWidget), METH_VARARGS},
    {"keyPressEvent", reinterpret_cast<PyCFunction>(Sbk_PyPanelFunc_keyPressEvent), METH_O},
    {"leaveEvent", reinterpret_cast<PyCFunction>(Sbk_PyPanelFunc_leaveEvent), METH_O},
    {"mousePressEvent", reinterpret_cast<PyCFunction>(Sbk_PyPanelFunc_mousePressEvent), METH_O},
    {"onUserDataChanged", reinterpret_cast<PyCFunction>(Sbk_PyPanelFunc_onUserDataChanged), METH_NOARGS},
    {"restore", reinterpret_cast<PyCFunction>(Sbk_PyPanelFunc_restore), METH_O},
    {"save", reinterpret_cast<PyCFunction>(Sbk_PyPanelFunc_save), METH_NOARGS},
    {"setPanelLabel", reinterpret_cast<PyCFunction>(Sbk_PyPanelFunc_setPanelLabel), METH_O},
    {"setParamChangedCallback", reinterpret_cast<PyCFunction>(Sbk_PyPanelFunc_setParamChangedCallback), METH_O},

    {nullptr, nullptr} // Sentinel
};

static int Sbk_PyPanel_setattro(PyObject *self, PyObject *name, PyObject *value)
{
    PySide::Feature::Select(self);
    if (value && PyCallable_Check(value)) {
        auto plain_inst = reinterpret_cast< ::PyPanel *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYPANEL_IDX], reinterpret_cast<SbkObject *>(self)));
        auto inst = dynamic_cast<PyPanelWrapper *>(plain_inst);
        if (inst)
            inst->resetPyMethodCache();
    }
    Shiboken::AutoDecRef pp(reinterpret_cast<PyObject *>(PySide::Property::getObject(self, name)));
    if (!pp.isNull())
        return PySide::Property::setValue(reinterpret_cast<PySideProperty *>(pp.object()), self, value);
    return PyObject_GenericSetAttr(self, name, value);
}

} // extern "C"

static int Sbk_PyPanel_traverse(PyObject *self, visitproc visit, void *arg)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_traverse(self, visit, arg);
}
static int Sbk_PyPanel_clear(PyObject *self)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_clear(self);
}
static int mi_offsets[] = { -1, -1, -1, -1, -1, -1, -1, -1, -1 };
int *
Sbk_PyPanel_mi_init(const void *cptr)
{
    if (mi_offsets[0] == -1) {
        std::set<int> offsets;
        const auto *class_ptr = reinterpret_cast<const PyPanel *>(cptr);
        const auto base = reinterpret_cast<uintptr_t>(class_ptr);
        offsets.insert(int(reinterpret_cast<uintptr_t>(static_cast<const QWidget *>(class_ptr)) - base));
        offsets.insert(int(reinterpret_cast<uintptr_t>(static_cast<const QWidget *>(static_cast<const PyPanel *>(static_cast<const void *>(class_ptr)))) - base));
        offsets.insert(int(reinterpret_cast<uintptr_t>(static_cast<const UserParamHolder *>(class_ptr)) - base));
        offsets.insert(int(reinterpret_cast<uintptr_t>(static_cast<const UserParamHolder *>(static_cast<const PyPanel *>(static_cast<const void *>(class_ptr)))) - base));
        offsets.insert(int(reinterpret_cast<uintptr_t>(static_cast<const QObject *>(class_ptr)) - base));
        offsets.insert(int(reinterpret_cast<uintptr_t>(static_cast<const QObject *>(static_cast<const QWidget *>(static_cast<const void *>(class_ptr)))) - base));
        offsets.insert(int(reinterpret_cast<uintptr_t>(static_cast<const QPaintDevice *>(class_ptr)) - base));
        offsets.insert(int(reinterpret_cast<uintptr_t>(static_cast<const QPaintDevice *>(static_cast<const QWidget *>(static_cast<const void *>(class_ptr)))) - base));

        offsets.erase(0);

        std::copy(offsets.cbegin(), offsets.cend(), mi_offsets);
    }
    return mi_offsets;
}
static void * Sbk_PyPanelSpecialCastFunction(void *obj, SbkObjectType *desiredType)
{
    auto me = reinterpret_cast< ::PyPanel *>(obj);
    if (desiredType == reinterpret_cast<SbkObjectType *>(SbkPySide2_QtWidgetsTypes[SBK_QWIDGET_IDX]))
        return static_cast< ::QWidget *>(me);
    else if (desiredType == reinterpret_cast<SbkObjectType *>(SbkPySide2_QtCoreTypes[SBK_QOBJECT_IDX]))
        return static_cast< ::QObject *>(me);
    else if (desiredType == reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QPAINTDEVICE_IDX]))
        return static_cast< ::QPaintDevice *>(me);
    else if (desiredType == reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_USERPARAMHOLDER_IDX]))
        return static_cast< ::UserParamHolder *>(me);
    return me;
}


// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType *_Sbk_PyPanel_Type = nullptr;
static SbkObjectType *Sbk_PyPanel_TypeF(void)
{
    return _Sbk_PyPanel_Type;
}

static PyType_Slot Sbk_PyPanel_slots[] = {
    {Py_tp_base,        nullptr}, // inserted by introduceWrapperType
    {Py_tp_dealloc,     reinterpret_cast<void *>(&SbkDeallocWrapper)},
    {Py_tp_repr,        nullptr},
    {Py_tp_hash,        nullptr},
    {Py_tp_call,        nullptr},
    {Py_tp_str,         nullptr},
    {Py_tp_getattro,    nullptr},
    {Py_tp_setattro,    reinterpret_cast<void *>(Sbk_PyPanel_setattro)},
    {Py_tp_traverse,    reinterpret_cast<void *>(Sbk_PyPanel_traverse)},
    {Py_tp_clear,       reinterpret_cast<void *>(Sbk_PyPanel_clear)},
    {Py_tp_richcompare, nullptr},
    {Py_tp_iter,        nullptr},
    {Py_tp_iternext,    nullptr},
    {Py_tp_methods,     reinterpret_cast<void *>(Sbk_PyPanel_methods)},
    {Py_tp_getset,      nullptr},
    {Py_tp_init,        reinterpret_cast<void *>(Sbk_PyPanel_Init)},
    {Py_tp_new,         reinterpret_cast<void *>(SbkObjectTpNew)},
    {0, nullptr}
};
static PyType_Spec Sbk_PyPanel_spec = {
    "1:NatronGui.PyPanel",
    sizeof(SbkObject),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    Sbk_PyPanel_slots
};

} //extern "C"

static void *Sbk_PyPanel_typeDiscovery(void *cptr, SbkObjectType *instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType *>(Shiboken::SbkType< ::QObject >()))
        return dynamic_cast< ::PyPanel *>(reinterpret_cast< ::QObject *>(cptr));
    if (instanceType == reinterpret_cast<SbkObjectType *>(Shiboken::SbkType< ::QPaintDevice >()))
        return dynamic_cast< ::PyPanel *>(reinterpret_cast< ::QPaintDevice *>(cptr));
    if (instanceType == reinterpret_cast<SbkObjectType *>(Shiboken::SbkType< ::UserParamHolder >()))
        return dynamic_cast< ::PyPanel *>(reinterpret_cast< ::UserParamHolder *>(cptr));
    return {};
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void PyPanel_PythonToCpp_PyPanel_PTR(PyObject *pyIn, void *cppOut) {
    Shiboken::Conversions::pythonToCppPointer(Sbk_PyPanel_TypeF(), pyIn, cppOut);
}
static PythonToCppFunc is_PyPanel_PythonToCpp_PyPanel_PTR_Convertible(PyObject *pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, reinterpret_cast<PyTypeObject *>(Sbk_PyPanel_TypeF())))
        return PyPanel_PythonToCpp_PyPanel_PTR;
    return {};
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject *PyPanel_PTR_CppToPython_PyPanel(const void *cppIn) {
    return PySide::getWrapperForQObject(reinterpret_cast<::PyPanel *>(const_cast<void *>(cppIn)), Sbk_PyPanel_TypeF());

}

// The signatures string for the functions.
// Multiple signatures have their index "n:" in front.
static const char *PyPanel_SignatureStrings[] = {
    "NatronGui.PyPanel(self,scriptName:QString,label:QString,useUserParameters:bool,app:NatronGui.GuiApp)",
    "NatronGui.PyPanel.addWidget(self,widget:PySide2.QtWidgets.QWidget)",
    "NatronGui.PyPanel.enterEvent(self,e:PySide2.QtCore.QEvent)",
    "NatronGui.PyPanel.getPanelLabel(self)->QString",
    "NatronGui.PyPanel.getPanelScriptName(self)->QString",
    "NatronGui.PyPanel.getParam(self,scriptName:QString)->NatronEngine.Param",
    "NatronGui.PyPanel.getParams(self)->std.list[NatronEngine.Param]",
    "NatronGui.PyPanel.insertWidget(self,index:int,widget:PySide2.QtWidgets.QWidget)",
    "NatronGui.PyPanel.keyPressEvent(self,e:PySide2.QtGui.QKeyEvent)",
    "NatronGui.PyPanel.leaveEvent(self,e:PySide2.QtCore.QEvent)",
    "NatronGui.PyPanel.mousePressEvent(self,e:PySide2.QtGui.QMouseEvent)",
    "NatronGui.PyPanel.onUserDataChanged(self)",
    "NatronGui.PyPanel.restore(self,arg__1:QString)",
    "NatronGui.PyPanel.save(self)->QString",
    "NatronGui.PyPanel.setPanelLabel(self,label:QString)",
    "NatronGui.PyPanel.setParamChangedCallback(self,callback:QString)",
    nullptr}; // Sentinel

void init_PyPanel(PyObject *module)
{
    PyObject *Sbk_PyPanel_Type_bases = PyTuple_Pack(2,
        reinterpret_cast<PyObject *>(SbkPySide2_QtWidgetsTypes[SBK_QWIDGET_IDX]),
        reinterpret_cast<PyObject *>(SbkNatronEngineTypes[SBK_USERPARAMHOLDER_IDX]));

    _Sbk_PyPanel_Type = Shiboken::ObjectType::introduceWrapperType(
        module,
        "PyPanel",
        "PyPanel*",
        &Sbk_PyPanel_spec,
        &Shiboken::callCppDestructor< ::PyPanel >,
        reinterpret_cast<SbkObjectType *>(SbkPySide2_QtWidgetsTypes[SBK_QWIDGET_IDX]),
        Sbk_PyPanel_Type_bases,
        Shiboken::ObjectType::WrapperFlags::DeleteInMainThread    );
    
    auto pyType = reinterpret_cast<PyTypeObject *>(_Sbk_PyPanel_Type);
    InitSignatureStrings(pyType, PyPanel_SignatureStrings);
    SbkObjectType_SetPropertyStrings(reinterpret_cast<PyTypeObject *>(_Sbk_PyPanel_Type), Sbk_PyPanel_PropertyStrings);
    SbkNatronGuiTypes[SBK_PYPANEL_IDX]
        = reinterpret_cast<PyTypeObject *>(Sbk_PyPanel_TypeF());

    // Register Converter
    SbkConverter *converter = Shiboken::Conversions::createConverter(Sbk_PyPanel_TypeF(),
        PyPanel_PythonToCpp_PyPanel_PTR,
        is_PyPanel_PythonToCpp_PyPanel_PTR_Convertible,
        PyPanel_PTR_CppToPython_PyPanel);

    Shiboken::Conversions::registerConverterName(converter, "PyPanel");
    Shiboken::Conversions::registerConverterName(converter, "PyPanel*");
    Shiboken::Conversions::registerConverterName(converter, "PyPanel&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::PyPanel).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::PyPanelWrapper).name());


    MultipleInheritanceInitFunction func = Sbk_PyPanel_mi_init;
    Shiboken::ObjectType::setMultipleInheritanceFunction(Sbk_PyPanel_TypeF(), func);
    Shiboken::ObjectType::setCastFunction(Sbk_PyPanel_TypeF(), &Sbk_PyPanelSpecialCastFunction);
    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(Sbk_PyPanel_TypeF(), &Sbk_PyPanel_typeDiscovery);

    PySide::Signal::registerSignals(Sbk_PyPanel_TypeF(), &::PyPanel::staticMetaObject);
    PyPanelWrapper::pysideInitQtMetaTypes();
    Shiboken::ObjectType::setSubTypeInitHook(Sbk_PyPanel_TypeF(), &PySide::initQObjectSubType);
    PySide::initDynamicMetaObject(Sbk_PyPanel_TypeF(), &::PyPanel::staticMetaObject, sizeof(PyPanelWrapper));
}
