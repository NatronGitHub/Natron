
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
#include "pyguiapplication_wrapper.h"

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

void PyGuiApplicationWrapper::pysideInitQtMetaTypes()
{
}

void PyGuiApplicationWrapper::resetPyMethodCache()
{
    std::fill_n(m_PyMethodCache, sizeof(m_PyMethodCache) / sizeof(m_PyMethodCache[0]), false);
}

PyGuiApplicationWrapper::PyGuiApplicationWrapper() : PyGuiApplication()
{
    resetPyMethodCache();
    // ... middle
}

PyGuiApplicationWrapper::~PyGuiApplicationWrapper()
{
    SbkObject *wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static int
Sbk_PyGuiApplication_Init(PyObject *self, PyObject *args, PyObject *kwds)
{
    SbkObject *sbkSelf = reinterpret_cast<SbkObject *>(self);
PySide::Feature::Select(self);
    if (Shiboken::Object::isUserType(self) && !Shiboken::ObjectType::canCallConstructor(self->ob_type, Shiboken::SbkType< ::PyGuiApplication >()))
        return -1;

    ::PyGuiApplicationWrapper *cptr{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyGuiApplication.__init__";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // PyGuiApplication()
            cptr = new ::PyGuiApplicationWrapper();
        }
    }

    if (PyErr_Occurred() || !Shiboken::Object::setCppPointer(sbkSelf, Shiboken::SbkType< ::PyGuiApplication >(), cptr)) {
        delete cptr;
        return -1;
    }
    Shiboken::Object::setValidCpp(sbkSelf, true);
    Shiboken::Object::setHasCppWrapper(sbkSelf, true);
    if (Shiboken::BindingManager::instance().hasWrapper(cptr)) {
        Shiboken::BindingManager::instance().releaseWrapper(Shiboken::BindingManager::instance().retrieveWrapper(cptr));
    }
    Shiboken::BindingManager::instance().registerWrapper(sbkSelf, cptr);


    return 1;
}

static PyObject *Sbk_PyGuiApplicationFunc_addMenuCommand(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyGuiApplication *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYGUIAPPLICATION_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyGuiApplication.addMenuCommand";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs == 3)
        goto Sbk_PyGuiApplicationFunc_addMenuCommand_TypeError;

    if (!PyArg_UnpackTuple(args, "addMenuCommand", 2, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return {};


    // Overloaded function decisor
    // 0: PyGuiApplication::addMenuCommand(QString,QString)
    // 1: PyGuiApplication::addMenuCommand(QString,QString,Qt::Key,QFlags<Qt::KeyboardModifier>)
    if (numArgs >= 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1])))) {
        if (numArgs == 2) {
            overloadId = 0; // addMenuCommand(QString,QString)
        } else if (numArgs == 4
            && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(*PepType_SGTP(SbkPySide2_QtCoreTypes[SBK_QT_KEY_IDX])->converter, (pyArgs[2])))
            && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(*PepType_SGTP(SbkPySide2_QtCoreTypes[SBK_QFLAGS_QT_KEYBOARDMODIFIER_IDX])->converter, (pyArgs[3])))) {
            overloadId = 1; // addMenuCommand(QString,QString,Qt::Key,QFlags<Qt::KeyboardModifier>)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyGuiApplicationFunc_addMenuCommand_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // addMenuCommand(const QString & grouping, const QString & pythonFunctionName)
        {
            ::QString cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            ::QString cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);

            if (!PyErr_Occurred()) {
                // addMenuCommand(QString,QString)
                cppSelf->addMenuCommand(cppArg0, cppArg1);
            }
            break;
        }
        case 1: // addMenuCommand(const QString & grouping, const QString & pythonFunctionName, Qt::Key key, const QFlags<Qt::KeyboardModifier> & modifiers)
        {
            ::QString cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            ::QString cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);
            ::Qt::Key cppArg2 = static_cast< ::Qt::Key>(0);
            pythonToCpp[2](pyArgs[2], &cppArg2);
            ::QFlags<Qt::KeyboardModifier> cppArg3 = QFlags<Qt::KeyboardModifier>(0);
            pythonToCpp[3](pyArgs[3], &cppArg3);

            if (!PyErr_Occurred()) {
                // addMenuCommand(QString,QString,Qt::Key,QFlags<Qt::KeyboardModifier>)
                cppSelf->addMenuCommand(cppArg0, cppArg1, cppArg2, cppArg3);
            }
            break;
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_PyGuiApplicationFunc_addMenuCommand_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_PyGuiApplicationFunc_errorDialog(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyGuiApplication *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYGUIAPPLICATION_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyGuiApplication.errorDialog";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "errorDialog", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: PyGuiApplication::errorDialog(QString,QString)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1])))) {
        overloadId = 0; // errorDialog(QString,QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyGuiApplicationFunc_errorDialog_TypeError;

    // Call function/method
    {
        ::QString cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::QString cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // errorDialog(QString,QString)
            cppSelf->errorDialog(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_PyGuiApplicationFunc_errorDialog_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_PyGuiApplicationFunc_getGuiInstance(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyGuiApplication *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYGUIAPPLICATION_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyGuiApplication.getGuiInstance";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: PyGuiApplication::getGuiInstance(int)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // getGuiInstance(int)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyGuiApplicationFunc_getGuiInstance_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getGuiInstance(int)const
            GuiApp * cppResult = const_cast<const ::PyGuiApplication *>(cppSelf)->getGuiInstance(cppArg0);
            pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronGuiTypes[SBK_GUIAPP_IDX]), cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_PyGuiApplicationFunc_getGuiInstance_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_PyGuiApplicationFunc_getIcon(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyGuiApplication *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYGUIAPPLICATION_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyGuiApplication.getIcon";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: PyGuiApplication::getIcon(NATRON_ENUM::PixmapEnum)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(*PepType_SGTP(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX])->converter, (pyArg)))) {
        overloadId = 0; // getIcon(NATRON_ENUM::PixmapEnum)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyGuiApplicationFunc_getIcon_TypeError;

    // Call function/method
    {
        ::NATRON_ENUM::PixmapEnum cppArg0{NATRON_ENUM::NATRON_PIXMAP_PLAYER_PREVIOUS};
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getIcon(NATRON_ENUM::PixmapEnum)const
            QPixmap cppResult = const_cast<const ::PyGuiApplication *>(cppSelf)->getIcon(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(reinterpret_cast<SbkObjectType *>(SbkPySide2_QtGuiTypes[SBK_QPIXMAP_IDX]), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_PyGuiApplicationFunc_getIcon_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_PyGuiApplicationFunc_informationDialog(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyGuiApplication *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYGUIAPPLICATION_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyGuiApplication.informationDialog";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "informationDialog", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: PyGuiApplication::informationDialog(QString,QString)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1])))) {
        overloadId = 0; // informationDialog(QString,QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyGuiApplicationFunc_informationDialog_TypeError;

    // Call function/method
    {
        ::QString cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::QString cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // informationDialog(QString,QString)
            cppSelf->informationDialog(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_PyGuiApplicationFunc_informationDialog_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_PyGuiApplicationFunc_questionDialog(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyGuiApplication *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYGUIAPPLICATION_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyGuiApplication.questionDialog";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "questionDialog", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: PyGuiApplication::questionDialog(QString,QString)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1])))) {
        overloadId = 0; // questionDialog(QString,QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyGuiApplicationFunc_questionDialog_TypeError;

    // Call function/method
    {
        ::QString cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::QString cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // questionDialog(QString,QString)
            NATRON_ENUM::StandardButtonEnum cppResult = NATRON_ENUM::StandardButtonEnum(cppSelf->questionDialog(cppArg0, cppArg1));
            pyResult = Shiboken::Conversions::copyToPython(*PepType_SGTP(SbkNatronEngineTypes[SBK_NATRON_ENUM_STANDARDBUTTONENUM_IDX])->converter, &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_PyGuiApplicationFunc_questionDialog_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_PyGuiApplicationFunc_warningDialog(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyGuiApplication *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYGUIAPPLICATION_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronGui.PyGuiApplication.warningDialog";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "warningDialog", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: PyGuiApplication::warningDialog(QString,QString)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1])))) {
        overloadId = 0; // warningDialog(QString,QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyGuiApplicationFunc_warningDialog_TypeError;

    // Call function/method
    {
        ::QString cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::QString cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // warningDialog(QString,QString)
            cppSelf->warningDialog(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_PyGuiApplicationFunc_warningDialog_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}


static const char *Sbk_PyGuiApplication_PropertyStrings[] = {
    nullptr // Sentinel
};

static PyMethodDef Sbk_PyGuiApplication_methods[] = {
    {"addMenuCommand", reinterpret_cast<PyCFunction>(Sbk_PyGuiApplicationFunc_addMenuCommand), METH_VARARGS},
    {"errorDialog", reinterpret_cast<PyCFunction>(Sbk_PyGuiApplicationFunc_errorDialog), METH_VARARGS},
    {"getGuiInstance", reinterpret_cast<PyCFunction>(Sbk_PyGuiApplicationFunc_getGuiInstance), METH_O},
    {"getIcon", reinterpret_cast<PyCFunction>(Sbk_PyGuiApplicationFunc_getIcon), METH_O},
    {"informationDialog", reinterpret_cast<PyCFunction>(Sbk_PyGuiApplicationFunc_informationDialog), METH_VARARGS},
    {"questionDialog", reinterpret_cast<PyCFunction>(Sbk_PyGuiApplicationFunc_questionDialog), METH_VARARGS},
    {"warningDialog", reinterpret_cast<PyCFunction>(Sbk_PyGuiApplicationFunc_warningDialog), METH_VARARGS},

    {nullptr, nullptr} // Sentinel
};

static int Sbk_PyGuiApplication_setattro(PyObject *self, PyObject *name, PyObject *value)
{
    PySide::Feature::Select(self);
    if (value && PyCallable_Check(value)) {
        auto plain_inst = reinterpret_cast< ::PyGuiApplication *>(Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYGUIAPPLICATION_IDX], reinterpret_cast<SbkObject *>(self)));
        auto inst = dynamic_cast<PyGuiApplicationWrapper *>(plain_inst);
        if (inst)
            inst->resetPyMethodCache();
    }
    return PyObject_GenericSetAttr(self, name, value);
}

} // extern "C"

static int Sbk_PyGuiApplication_traverse(PyObject *self, visitproc visit, void *arg)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_traverse(self, visit, arg);
}
static int Sbk_PyGuiApplication_clear(PyObject *self)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType *_Sbk_PyGuiApplication_Type = nullptr;
static SbkObjectType *Sbk_PyGuiApplication_TypeF(void)
{
    return _Sbk_PyGuiApplication_Type;
}

static PyType_Slot Sbk_PyGuiApplication_slots[] = {
    {Py_tp_base,        nullptr}, // inserted by introduceWrapperType
    {Py_tp_dealloc,     reinterpret_cast<void *>(&SbkDeallocWrapper)},
    {Py_tp_repr,        nullptr},
    {Py_tp_hash,        nullptr},
    {Py_tp_call,        nullptr},
    {Py_tp_str,         nullptr},
    {Py_tp_getattro,    nullptr},
    {Py_tp_setattro,    reinterpret_cast<void *>(Sbk_PyGuiApplication_setattro)},
    {Py_tp_traverse,    reinterpret_cast<void *>(Sbk_PyGuiApplication_traverse)},
    {Py_tp_clear,       reinterpret_cast<void *>(Sbk_PyGuiApplication_clear)},
    {Py_tp_richcompare, nullptr},
    {Py_tp_iter,        nullptr},
    {Py_tp_iternext,    nullptr},
    {Py_tp_methods,     reinterpret_cast<void *>(Sbk_PyGuiApplication_methods)},
    {Py_tp_getset,      nullptr},
    {Py_tp_init,        reinterpret_cast<void *>(Sbk_PyGuiApplication_Init)},
    {Py_tp_new,         reinterpret_cast<void *>(SbkObjectTpNew)},
    {0, nullptr}
};
static PyType_Spec Sbk_PyGuiApplication_spec = {
    "1:NatronGui.PyGuiApplication",
    sizeof(SbkObject),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    Sbk_PyGuiApplication_slots
};

} //extern "C"

static void *Sbk_PyGuiApplication_typeDiscovery(void *cptr, SbkObjectType *instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType *>(Shiboken::SbkType< ::PyCoreApplication >()))
        return dynamic_cast< ::PyGuiApplication *>(reinterpret_cast< ::PyCoreApplication *>(cptr));
    return {};
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void PyGuiApplication_PythonToCpp_PyGuiApplication_PTR(PyObject *pyIn, void *cppOut) {
    Shiboken::Conversions::pythonToCppPointer(Sbk_PyGuiApplication_TypeF(), pyIn, cppOut);
}
static PythonToCppFunc is_PyGuiApplication_PythonToCpp_PyGuiApplication_PTR_Convertible(PyObject *pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, reinterpret_cast<PyTypeObject *>(Sbk_PyGuiApplication_TypeF())))
        return PyGuiApplication_PythonToCpp_PyGuiApplication_PTR;
    return {};
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject *PyGuiApplication_PTR_CppToPython_PyGuiApplication(const void *cppIn) {
    auto pyOut = reinterpret_cast<PyObject *>(Shiboken::BindingManager::instance().retrieveWrapper(cppIn));
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    bool changedTypeName = false;
    auto tCppIn = reinterpret_cast<const ::PyGuiApplication *>(cppIn);
    const char *typeName = typeid(*tCppIn).name();
    auto sbkType = Shiboken::ObjectType::typeForTypeName(typeName);
    if (sbkType && Shiboken::ObjectType::hasSpecialCastFunction(sbkType)) {
        typeName = typeNameOf(tCppIn);
        changedTypeName = true;
    }
    PyObject *result = Shiboken::Object::newObject(Sbk_PyGuiApplication_TypeF(), const_cast<void *>(cppIn), false, /* exactType */ changedTypeName, typeName);
    if (changedTypeName)
        delete [] typeName;
    return result;
}

// The signatures string for the functions.
// Multiple signatures have their index "n:" in front.
static const char *PyGuiApplication_SignatureStrings[] = {
    "NatronGui.PyGuiApplication(self)",
    "1:NatronGui.PyGuiApplication.addMenuCommand(self,grouping:QString,pythonFunctionName:QString)",
    "0:NatronGui.PyGuiApplication.addMenuCommand(self,grouping:QString,pythonFunctionName:QString,key:PySide2.QtCore.Qt.Key,modifiers:PySide2.QtCore.Qt.KeyboardModifiers)",
    "NatronGui.PyGuiApplication.errorDialog(self,title:QString,message:QString)",
    "NatronGui.PyGuiApplication.getGuiInstance(self,idx:int)->NatronGui.GuiApp",
    "NatronGui.PyGuiApplication.getIcon(self,val:NatronEngine.NATRON_ENUM.PixmapEnum)->PySide2.QtGui.QPixmap",
    "NatronGui.PyGuiApplication.informationDialog(self,title:QString,message:QString)",
    "NatronGui.PyGuiApplication.questionDialog(self,title:QString,message:QString)->NatronEngine.NATRON_ENUM.StandardButtonEnum",
    "NatronGui.PyGuiApplication.warningDialog(self,title:QString,message:QString)",
    nullptr}; // Sentinel

void init_PyGuiApplication(PyObject *module)
{
    _Sbk_PyGuiApplication_Type = Shiboken::ObjectType::introduceWrapperType(
        module,
        "PyGuiApplication",
        "PyGuiApplication*",
        &Sbk_PyGuiApplication_spec,
        &Shiboken::callCppDestructor< ::PyGuiApplication >,
        reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_PYCOREAPPLICATION_IDX]),
        0,
        0    );
    
    auto pyType = reinterpret_cast<PyTypeObject *>(_Sbk_PyGuiApplication_Type);
    InitSignatureStrings(pyType, PyGuiApplication_SignatureStrings);
    SbkObjectType_SetPropertyStrings(reinterpret_cast<PyTypeObject *>(_Sbk_PyGuiApplication_Type), Sbk_PyGuiApplication_PropertyStrings);
    SbkNatronGuiTypes[SBK_PYGUIAPPLICATION_IDX]
        = reinterpret_cast<PyTypeObject *>(Sbk_PyGuiApplication_TypeF());

    // Register Converter
    SbkConverter *converter = Shiboken::Conversions::createConverter(Sbk_PyGuiApplication_TypeF(),
        PyGuiApplication_PythonToCpp_PyGuiApplication_PTR,
        is_PyGuiApplication_PythonToCpp_PyGuiApplication_PTR_Convertible,
        PyGuiApplication_PTR_CppToPython_PyGuiApplication);

    Shiboken::Conversions::registerConverterName(converter, "PyGuiApplication");
    Shiboken::Conversions::registerConverterName(converter, "PyGuiApplication*");
    Shiboken::Conversions::registerConverterName(converter, "PyGuiApplication&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::PyGuiApplication).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::PyGuiApplicationWrapper).name());


    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(Sbk_PyGuiApplication_TypeF(), &Sbk_PyGuiApplication_typeDiscovery);

    PyGuiApplicationWrapper::pysideInitQtMetaTypes();
}
