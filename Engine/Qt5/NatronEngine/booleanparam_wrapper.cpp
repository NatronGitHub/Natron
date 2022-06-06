
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
#include "natronengine_python.h"

// main header
#include "booleanparam_wrapper.h"

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

void BooleanParamWrapper::pysideInitQtMetaTypes()
{
}

void BooleanParamWrapper::resetPyMethodCache()
{
    std::fill_n(m_PyMethodCache, sizeof(m_PyMethodCache) / sizeof(m_PyMethodCache[0]), false);
}

BooleanParamWrapper::~BooleanParamWrapper()
{
    SbkObject *wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject *Sbk_BooleanParamFunc_addAsDependencyOf(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BooleanParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BOOLEANPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.BooleanParam.addAsDependencyOf";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "addAsDependencyOf", 3, 3, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return {};


    // Overloaded function decisor
    // 0: BooleanParam::addAsDependencyOf(int,Param*,int)
    if (numArgs == 3
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppPointerConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_PARAM_IDX]), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2])))) {
        overloadId = 0; // addAsDependencyOf(int,Param*,int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BooleanParamFunc_addAsDependencyOf_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        if (!Shiboken::Object::isValid(pyArgs[1]))
            return {};
        ::Param *cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        int cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // addAsDependencyOf(int,Param*,int)
            bool cppResult = cppSelf->addAsDependencyOf(cppArg0, cppArg1, cppArg2);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_BooleanParamFunc_addAsDependencyOf_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_BooleanParamFunc_get(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BooleanParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BOOLEANPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.BooleanParam.get";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "get", 0, 1, &(pyArgs[0])))
        return {};


    // Overloaded function decisor
    // 0: BooleanParam::get()const
    // 1: BooleanParam::get(double)const
    if (numArgs == 0) {
        overloadId = 0; // get()const
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))) {
        overloadId = 1; // get(double)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BooleanParamFunc_get_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // get() const
        {

            if (!PyErr_Occurred()) {
                // get()const
                bool cppResult = const_cast<const ::BooleanParam *>(cppSelf)->get();
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
            }
            break;
        }
        case 1: // get(double frame) const
        {
            double cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // get(double)const
                bool cppResult = const_cast<const ::BooleanParam *>(cppSelf)->get(cppArg0);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
            }
            break;
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_BooleanParamFunc_get_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_BooleanParamFunc_getDefaultValue(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BooleanParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BOOLEANPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.BooleanParam.getDefaultValue";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getDefaultValue()const
            bool cppResult = const_cast<const ::BooleanParam *>(cppSelf)->getDefaultValue();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_BooleanParamFunc_getValue(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BooleanParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BOOLEANPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.BooleanParam.getValue";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getValue()const
            bool cppResult = const_cast<const ::BooleanParam *>(cppSelf)->getValue();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_BooleanParamFunc_getValueAtTime(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BooleanParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BOOLEANPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.BooleanParam.getValueAtTime";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: BooleanParam::getValueAtTime(double)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArg)))) {
        overloadId = 0; // getValueAtTime(double)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BooleanParamFunc_getValueAtTime_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getValueAtTime(double)const
            bool cppResult = const_cast<const ::BooleanParam *>(cppSelf)->getValueAtTime(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_BooleanParamFunc_getValueAtTime_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_BooleanParamFunc_restoreDefaultValue(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BooleanParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BOOLEANPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.BooleanParam.restoreDefaultValue";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // restoreDefaultValue()
            cppSelf->restoreDefaultValue();
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;
}

static PyObject *Sbk_BooleanParamFunc_set(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BooleanParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BOOLEANPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.BooleanParam.set";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "set", 1, 2, &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: BooleanParam::set(bool)
    // 1: BooleanParam::set(bool,double)
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // set(bool)
        } else if (numArgs == 2
            && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
            overloadId = 1; // set(bool,double)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BooleanParamFunc_set_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // set(bool x)
        {
            bool cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // set(bool)
                cppSelf->set(cppArg0);
            }
            break;
        }
        case 1: // set(bool x, double frame)
        {
            bool cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            double cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);

            if (!PyErr_Occurred()) {
                // set(bool,double)
                cppSelf->set(cppArg0, cppArg1);
            }
            break;
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_BooleanParamFunc_set_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_BooleanParamFunc_setDefaultValue(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BooleanParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BOOLEANPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.BooleanParam.setDefaultValue";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: BooleanParam::setDefaultValue(bool)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArg)))) {
        overloadId = 0; // setDefaultValue(bool)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BooleanParamFunc_setDefaultValue_TypeError;

    // Call function/method
    {
        bool cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setDefaultValue(bool)
            cppSelf->setDefaultValue(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_BooleanParamFunc_setDefaultValue_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_BooleanParamFunc_setValue(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BooleanParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BOOLEANPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.BooleanParam.setValue";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: BooleanParam::setValue(bool)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArg)))) {
        overloadId = 0; // setValue(bool)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BooleanParamFunc_setValue_TypeError;

    // Call function/method
    {
        bool cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setValue(bool)
            cppSelf->setValue(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_BooleanParamFunc_setValue_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_BooleanParamFunc_setValueAtTime(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::BooleanParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BOOLEANPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.BooleanParam.setValueAtTime";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "setValueAtTime", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: BooleanParam::setValueAtTime(bool,double)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
        overloadId = 0; // setValueAtTime(bool,double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_BooleanParamFunc_setValueAtTime_TypeError;

    // Call function/method
    {
        bool cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // setValueAtTime(bool,double)
            cppSelf->setValueAtTime(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_BooleanParamFunc_setValueAtTime_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}


static const char *Sbk_BooleanParam_PropertyStrings[] = {
    nullptr // Sentinel
};

static PyMethodDef Sbk_BooleanParam_methods[] = {
    {"addAsDependencyOf", reinterpret_cast<PyCFunction>(Sbk_BooleanParamFunc_addAsDependencyOf), METH_VARARGS},
    {"get", reinterpret_cast<PyCFunction>(Sbk_BooleanParamFunc_get), METH_VARARGS},
    {"getDefaultValue", reinterpret_cast<PyCFunction>(Sbk_BooleanParamFunc_getDefaultValue), METH_NOARGS},
    {"getValue", reinterpret_cast<PyCFunction>(Sbk_BooleanParamFunc_getValue), METH_NOARGS},
    {"getValueAtTime", reinterpret_cast<PyCFunction>(Sbk_BooleanParamFunc_getValueAtTime), METH_O},
    {"restoreDefaultValue", reinterpret_cast<PyCFunction>(Sbk_BooleanParamFunc_restoreDefaultValue), METH_NOARGS},
    {"set", reinterpret_cast<PyCFunction>(Sbk_BooleanParamFunc_set), METH_VARARGS},
    {"setDefaultValue", reinterpret_cast<PyCFunction>(Sbk_BooleanParamFunc_setDefaultValue), METH_O},
    {"setValue", reinterpret_cast<PyCFunction>(Sbk_BooleanParamFunc_setValue), METH_O},
    {"setValueAtTime", reinterpret_cast<PyCFunction>(Sbk_BooleanParamFunc_setValueAtTime), METH_VARARGS},

    {nullptr, nullptr} // Sentinel
};

static int Sbk_BooleanParam_setattro(PyObject *self, PyObject *name, PyObject *value)
{
    PySide::Feature::Select(self);
    if (value && PyCallable_Check(value)) {
        auto plain_inst = reinterpret_cast< ::BooleanParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BOOLEANPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
        auto inst = dynamic_cast<BooleanParamWrapper *>(plain_inst);
        if (inst)
            inst->resetPyMethodCache();
    }
    return PyObject_GenericSetAttr(self, name, value);
}

} // extern "C"

static int Sbk_BooleanParam_traverse(PyObject *self, visitproc visit, void *arg)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_traverse(self, visit, arg);
}
static int Sbk_BooleanParam_clear(PyObject *self)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType *_Sbk_BooleanParam_Type = nullptr;
static SbkObjectType *Sbk_BooleanParam_TypeF(void)
{
    return _Sbk_BooleanParam_Type;
}

static PyType_Slot Sbk_BooleanParam_slots[] = {
    {Py_tp_base,        nullptr}, // inserted by introduceWrapperType
    {Py_tp_dealloc,     reinterpret_cast<void *>(&SbkDeallocWrapper)},
    {Py_tp_repr,        nullptr},
    {Py_tp_hash,        nullptr},
    {Py_tp_call,        nullptr},
    {Py_tp_str,         nullptr},
    {Py_tp_getattro,    nullptr},
    {Py_tp_setattro,    reinterpret_cast<void *>(Sbk_BooleanParam_setattro)},
    {Py_tp_traverse,    reinterpret_cast<void *>(Sbk_BooleanParam_traverse)},
    {Py_tp_clear,       reinterpret_cast<void *>(Sbk_BooleanParam_clear)},
    {Py_tp_richcompare, nullptr},
    {Py_tp_iter,        nullptr},
    {Py_tp_iternext,    nullptr},
    {Py_tp_methods,     reinterpret_cast<void *>(Sbk_BooleanParam_methods)},
    {Py_tp_getset,      nullptr},
    {Py_tp_init,        nullptr},
    {Py_tp_new,         reinterpret_cast<void *>(SbkDummyNew /* PYSIDE-595: Prevent replacement of "0" with base->tp_new. */)},
    {0, nullptr}
};
static PyType_Spec Sbk_BooleanParam_spec = {
    "1:NatronEngine.BooleanParam",
    sizeof(SbkObject),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    Sbk_BooleanParam_slots
};

} //extern "C"

static void *Sbk_BooleanParam_typeDiscovery(void *cptr, SbkObjectType *instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType *>(Shiboken::SbkType< ::Param >()))
        return dynamic_cast< ::BooleanParam *>(reinterpret_cast< ::Param *>(cptr));
    return {};
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void BooleanParam_PythonToCpp_BooleanParam_PTR(PyObject *pyIn, void *cppOut) {
    Shiboken::Conversions::pythonToCppPointer(Sbk_BooleanParam_TypeF(), pyIn, cppOut);
}
static PythonToCppFunc is_BooleanParam_PythonToCpp_BooleanParam_PTR_Convertible(PyObject *pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, reinterpret_cast<PyTypeObject *>(Sbk_BooleanParam_TypeF())))
        return BooleanParam_PythonToCpp_BooleanParam_PTR;
    return {};
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject *BooleanParam_PTR_CppToPython_BooleanParam(const void *cppIn) {
    auto pyOut = reinterpret_cast<PyObject *>(Shiboken::BindingManager::instance().retrieveWrapper(cppIn));
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    bool changedTypeName = false;
    auto tCppIn = reinterpret_cast<const ::BooleanParam *>(cppIn);
    const char *typeName = typeid(*tCppIn).name();
    auto sbkType = Shiboken::ObjectType::typeForTypeName(typeName);
    if (sbkType && Shiboken::ObjectType::hasSpecialCastFunction(sbkType)) {
        typeName = typeNameOf(tCppIn);
        changedTypeName = true;
    }
    PyObject *result = Shiboken::Object::newObject(Sbk_BooleanParam_TypeF(), const_cast<void *>(cppIn), false, /* exactType */ changedTypeName, typeName);
    if (changedTypeName)
        delete [] typeName;
    return result;
}

// The signatures string for the functions.
// Multiple signatures have their index "n:" in front.
static const char *BooleanParam_SignatureStrings[] = {
    "NatronEngine.BooleanParam.addAsDependencyOf(self,fromExprDimension:int,param:NatronEngine.Param,thisDimension:int)->bool",
    "1:NatronEngine.BooleanParam.get(self)->bool",
    "0:NatronEngine.BooleanParam.get(self,frame:double)->bool",
    "NatronEngine.BooleanParam.getDefaultValue(self)->bool",
    "NatronEngine.BooleanParam.getValue(self)->bool",
    "NatronEngine.BooleanParam.getValueAtTime(self,time:double)->bool",
    "NatronEngine.BooleanParam.restoreDefaultValue(self)",
    "1:NatronEngine.BooleanParam.set(self,x:bool)",
    "0:NatronEngine.BooleanParam.set(self,x:bool,frame:double)",
    "NatronEngine.BooleanParam.setDefaultValue(self,value:bool)",
    "NatronEngine.BooleanParam.setValue(self,value:bool)",
    "NatronEngine.BooleanParam.setValueAtTime(self,value:bool,time:double)",
    nullptr}; // Sentinel

void init_BooleanParam(PyObject *module)
{
    _Sbk_BooleanParam_Type = Shiboken::ObjectType::introduceWrapperType(
        module,
        "BooleanParam",
        "BooleanParam*",
        &Sbk_BooleanParam_spec,
        &Shiboken::callCppDestructor< ::BooleanParam >,
        reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX]),
        0,
        0    );
    
    auto pyType = reinterpret_cast<PyTypeObject *>(_Sbk_BooleanParam_Type);
    InitSignatureStrings(pyType, BooleanParam_SignatureStrings);
    SbkObjectType_SetPropertyStrings(reinterpret_cast<PyTypeObject *>(_Sbk_BooleanParam_Type), Sbk_BooleanParam_PropertyStrings);
    SbkNatronEngineTypes[SBK_BOOLEANPARAM_IDX]
        = reinterpret_cast<PyTypeObject *>(Sbk_BooleanParam_TypeF());

    // Register Converter
    SbkConverter *converter = Shiboken::Conversions::createConverter(Sbk_BooleanParam_TypeF(),
        BooleanParam_PythonToCpp_BooleanParam_PTR,
        is_BooleanParam_PythonToCpp_BooleanParam_PTR_Convertible,
        BooleanParam_PTR_CppToPython_BooleanParam);

    Shiboken::Conversions::registerConverterName(converter, "BooleanParam");
    Shiboken::Conversions::registerConverterName(converter, "BooleanParam*");
    Shiboken::Conversions::registerConverterName(converter, "BooleanParam&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::BooleanParam).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::BooleanParamWrapper).name());


    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(Sbk_BooleanParam_TypeF(), &Sbk_BooleanParam_typeDiscovery);

    BooleanParamWrapper::pysideInitQtMetaTypes();
}
