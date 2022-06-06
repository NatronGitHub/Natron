
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
#include "double2dparam_wrapper.h"

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

void Double2DParamWrapper::pysideInitQtMetaTypes()
{
}

void Double2DParamWrapper::resetPyMethodCache()
{
    std::fill_n(m_PyMethodCache, sizeof(m_PyMethodCache) / sizeof(m_PyMethodCache[0]), false);
}

Double2DParamWrapper::~Double2DParamWrapper()
{
    SbkObject *wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject *Sbk_Double2DParamFunc_get(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Double2DParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_DOUBLE2DPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Double2DParam.get";
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
    // 0: DoubleParam::get()const
    // 1: DoubleParam::get(double)const
    if (numArgs == 0) {
        overloadId = 0; // get()const
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))) {
        overloadId = 1; // get(double)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_Double2DParamFunc_get_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // get() const
        {

            if (!PyErr_Occurred()) {
                // get()const
                Double2DTuple* cppResult = new Double2DTuple(const_cast<const ::Double2DParam *>(cppSelf)->get());
                pyResult = Shiboken::Object::newObject(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_DOUBLE2DTUPLE_IDX]), cppResult, true, true);
            }
            break;
        }
        case 1: // get(double frame) const
        {
            double cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // get(double)const
                Double2DTuple* cppResult = new Double2DTuple(const_cast<const ::Double2DParam *>(cppSelf)->get(cppArg0));
                pyResult = Shiboken::Object::newObject(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_DOUBLE2DTUPLE_IDX]), cppResult, true, true);
            }
            break;
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_Double2DParamFunc_get_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_Double2DParamFunc_set(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Double2DParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_DOUBLE2DPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Double2DParam.set";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "set", 1, 3, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return {};


    // Overloaded function decisor
    // 0: DoubleParam::set(double,double)
    // 1: DoubleParam::set(double)
    // 2: Double2DParam::set(double,double,double)
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 1; // set(double)
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
            if (numArgs == 2) {
                overloadId = 0; // set(double,double)
            } else if (numArgs == 3
                && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))) {
                overloadId = 2; // set(double,double,double)
            }
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_Double2DParamFunc_set_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // set(double x, double y)
        {
            double cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            double cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);

            if (!PyErr_Occurred()) {
                // set(double,double)
                cppSelf->set(cppArg0, cppArg1);
            }
            break;
        }
        case 1: // set(double x)
        {
            double cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // set(double)
                reinterpret_cast<DoubleParam *>(cppSelf)->set(cppArg0);
            }
            break;
        }
        case 2: // set(double x, double y, double frame)
        {
            double cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            double cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);
            double cppArg2;
            pythonToCpp[2](pyArgs[2], &cppArg2);

            if (!PyErr_Occurred()) {
                // set(double,double,double)
                cppSelf->set(cppArg0, cppArg1, cppArg2);
            }
            break;
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_Double2DParamFunc_set_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_Double2DParamFunc_setCanAutoFoldDimensions(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Double2DParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_DOUBLE2DPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Double2DParam.setCanAutoFoldDimensions";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: Double2DParam::setCanAutoFoldDimensions(bool)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArg)))) {
        overloadId = 0; // setCanAutoFoldDimensions(bool)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_Double2DParamFunc_setCanAutoFoldDimensions_TypeError;

    // Call function/method
    {
        bool cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setCanAutoFoldDimensions(bool)
            cppSelf->setCanAutoFoldDimensions(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_Double2DParamFunc_setCanAutoFoldDimensions_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_Double2DParamFunc_setUsePointInteract(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Double2DParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_DOUBLE2DPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Double2DParam.setUsePointInteract";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: Double2DParam::setUsePointInteract(bool)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArg)))) {
        overloadId = 0; // setUsePointInteract(bool)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_Double2DParamFunc_setUsePointInteract_TypeError;

    // Call function/method
    {
        bool cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setUsePointInteract(bool)
            cppSelf->setUsePointInteract(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_Double2DParamFunc_setUsePointInteract_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}


static const char *Sbk_Double2DParam_PropertyStrings[] = {
    nullptr // Sentinel
};

static PyMethodDef Sbk_Double2DParam_methods[] = {
    {"get", reinterpret_cast<PyCFunction>(Sbk_Double2DParamFunc_get), METH_VARARGS},
    {"set", reinterpret_cast<PyCFunction>(Sbk_Double2DParamFunc_set), METH_VARARGS},
    {"setCanAutoFoldDimensions", reinterpret_cast<PyCFunction>(Sbk_Double2DParamFunc_setCanAutoFoldDimensions), METH_O},
    {"setUsePointInteract", reinterpret_cast<PyCFunction>(Sbk_Double2DParamFunc_setUsePointInteract), METH_O},

    {nullptr, nullptr} // Sentinel
};

static int Sbk_Double2DParam_setattro(PyObject *self, PyObject *name, PyObject *value)
{
    PySide::Feature::Select(self);
    if (value && PyCallable_Check(value)) {
        auto plain_inst = reinterpret_cast< ::Double2DParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_DOUBLE2DPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
        auto inst = dynamic_cast<Double2DParamWrapper *>(plain_inst);
        if (inst)
            inst->resetPyMethodCache();
    }
    return PyObject_GenericSetAttr(self, name, value);
}

} // extern "C"

static int Sbk_Double2DParam_traverse(PyObject *self, visitproc visit, void *arg)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_traverse(self, visit, arg);
}
static int Sbk_Double2DParam_clear(PyObject *self)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType *_Sbk_Double2DParam_Type = nullptr;
static SbkObjectType *Sbk_Double2DParam_TypeF(void)
{
    return _Sbk_Double2DParam_Type;
}

static PyType_Slot Sbk_Double2DParam_slots[] = {
    {Py_tp_base,        nullptr}, // inserted by introduceWrapperType
    {Py_tp_dealloc,     reinterpret_cast<void *>(&SbkDeallocWrapper)},
    {Py_tp_repr,        nullptr},
    {Py_tp_hash,        nullptr},
    {Py_tp_call,        nullptr},
    {Py_tp_str,         nullptr},
    {Py_tp_getattro,    nullptr},
    {Py_tp_setattro,    reinterpret_cast<void *>(Sbk_Double2DParam_setattro)},
    {Py_tp_traverse,    reinterpret_cast<void *>(Sbk_Double2DParam_traverse)},
    {Py_tp_clear,       reinterpret_cast<void *>(Sbk_Double2DParam_clear)},
    {Py_tp_richcompare, nullptr},
    {Py_tp_iter,        nullptr},
    {Py_tp_iternext,    nullptr},
    {Py_tp_methods,     reinterpret_cast<void *>(Sbk_Double2DParam_methods)},
    {Py_tp_getset,      nullptr},
    {Py_tp_init,        nullptr},
    {Py_tp_new,         reinterpret_cast<void *>(SbkDummyNew /* PYSIDE-595: Prevent replacement of "0" with base->tp_new. */)},
    {0, nullptr}
};
static PyType_Spec Sbk_Double2DParam_spec = {
    "1:NatronEngine.Double2DParam",
    sizeof(SbkObject),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    Sbk_Double2DParam_slots
};

} //extern "C"

static void *Sbk_Double2DParam_typeDiscovery(void *cptr, SbkObjectType *instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType *>(Shiboken::SbkType< ::Param >()))
        return dynamic_cast< ::Double2DParam *>(reinterpret_cast< ::Param *>(cptr));
    return {};
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void Double2DParam_PythonToCpp_Double2DParam_PTR(PyObject *pyIn, void *cppOut) {
    Shiboken::Conversions::pythonToCppPointer(Sbk_Double2DParam_TypeF(), pyIn, cppOut);
}
static PythonToCppFunc is_Double2DParam_PythonToCpp_Double2DParam_PTR_Convertible(PyObject *pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, reinterpret_cast<PyTypeObject *>(Sbk_Double2DParam_TypeF())))
        return Double2DParam_PythonToCpp_Double2DParam_PTR;
    return {};
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject *Double2DParam_PTR_CppToPython_Double2DParam(const void *cppIn) {
    auto pyOut = reinterpret_cast<PyObject *>(Shiboken::BindingManager::instance().retrieveWrapper(cppIn));
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    bool changedTypeName = false;
    auto tCppIn = reinterpret_cast<const ::Double2DParam *>(cppIn);
    const char *typeName = typeid(*tCppIn).name();
    auto sbkType = Shiboken::ObjectType::typeForTypeName(typeName);
    if (sbkType && Shiboken::ObjectType::hasSpecialCastFunction(sbkType)) {
        typeName = typeNameOf(tCppIn);
        changedTypeName = true;
    }
    PyObject *result = Shiboken::Object::newObject(Sbk_Double2DParam_TypeF(), const_cast<void *>(cppIn), false, /* exactType */ changedTypeName, typeName);
    if (changedTypeName)
        delete [] typeName;
    return result;
}

// The signatures string for the functions.
// Multiple signatures have their index "n:" in front.
static const char *Double2DParam_SignatureStrings[] = {
    "1:NatronEngine.Double2DParam.get(self)->NatronEngine.Double2DTuple",
    "0:NatronEngine.Double2DParam.get(self,frame:double)->NatronEngine.Double2DTuple",
    "2:NatronEngine.Double2DParam.set(self,x:double,y:double)",
    "1:NatronEngine.Double2DParam.set(self,x:double)",
    "0:NatronEngine.Double2DParam.set(self,x:double,y:double,frame:double)",
    "NatronEngine.Double2DParam.setCanAutoFoldDimensions(self,can:bool)",
    "NatronEngine.Double2DParam.setUsePointInteract(self,use:bool)",
    nullptr}; // Sentinel

void init_Double2DParam(PyObject *module)
{
    _Sbk_Double2DParam_Type = Shiboken::ObjectType::introduceWrapperType(
        module,
        "Double2DParam",
        "Double2DParam*",
        &Sbk_Double2DParam_spec,
        &Shiboken::callCppDestructor< ::Double2DParam >,
        reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_DOUBLEPARAM_IDX]),
        0,
        0    );
    
    auto pyType = reinterpret_cast<PyTypeObject *>(_Sbk_Double2DParam_Type);
    InitSignatureStrings(pyType, Double2DParam_SignatureStrings);
    SbkObjectType_SetPropertyStrings(reinterpret_cast<PyTypeObject *>(_Sbk_Double2DParam_Type), Sbk_Double2DParam_PropertyStrings);
    SbkNatronEngineTypes[SBK_DOUBLE2DPARAM_IDX]
        = reinterpret_cast<PyTypeObject *>(Sbk_Double2DParam_TypeF());

    // Register Converter
    SbkConverter *converter = Shiboken::Conversions::createConverter(Sbk_Double2DParam_TypeF(),
        Double2DParam_PythonToCpp_Double2DParam_PTR,
        is_Double2DParam_PythonToCpp_Double2DParam_PTR_Convertible,
        Double2DParam_PTR_CppToPython_Double2DParam);

    Shiboken::Conversions::registerConverterName(converter, "Double2DParam");
    Shiboken::Conversions::registerConverterName(converter, "Double2DParam*");
    Shiboken::Conversions::registerConverterName(converter, "Double2DParam&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::Double2DParam).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::Double2DParamWrapper).name());


    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(Sbk_Double2DParam_TypeF(), &Sbk_Double2DParam_typeDiscovery);

    Double2DParamWrapper::pysideInitQtMetaTypes();
}
