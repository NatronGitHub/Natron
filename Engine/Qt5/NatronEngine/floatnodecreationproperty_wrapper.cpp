
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
#include <iterator>

// module include
#include "natronengine_python.h"

// main header
#include "floatnodecreationproperty_wrapper.h"

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

void FloatNodeCreationPropertyWrapper::pysideInitQtMetaTypes()
{
}

void FloatNodeCreationPropertyWrapper::resetPyMethodCache()
{
    std::fill_n(m_PyMethodCache, sizeof(m_PyMethodCache) / sizeof(m_PyMethodCache[0]), false);
}

FloatNodeCreationPropertyWrapper::FloatNodeCreationPropertyWrapper(const std::vector<double > & values) : FloatNodeCreationProperty(values)
{
    resetPyMethodCache();
    // ... middle
}

FloatNodeCreationPropertyWrapper::FloatNodeCreationPropertyWrapper(double value) : FloatNodeCreationProperty(value)
{
    resetPyMethodCache();
    // ... middle
}

FloatNodeCreationPropertyWrapper::~FloatNodeCreationPropertyWrapper()
{
    SbkObject *wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static int
Sbk_FloatNodeCreationProperty_Init(PyObject *self, PyObject *args, PyObject *kwds)
{
    SbkObject *sbkSelf = reinterpret_cast<SbkObject *>(self);
    if (Shiboken::Object::isUserType(self) && !Shiboken::ObjectType::canCallConstructor(self->ob_type, Shiboken::SbkType< ::FloatNodeCreationProperty >()))
        return -1;

    ::FloatNodeCreationPropertyWrapper *cptr{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.FloatNodeCreationProperty(): too many arguments");
        return -1;
    }

    if (!PyArg_ParseTuple(args, "|O:FloatNodeCreationProperty", &(pyArgs[0])))
        return -1;


    // Overloaded function decisor
    // 0: FloatNodeCreationProperty::FloatNodeCreationProperty(std::vector<double>)
    // 1: FloatNodeCreationProperty::FloatNodeCreationProperty(double)
    if (numArgs == 0) {
        overloadId = 0; // FloatNodeCreationProperty(std::vector<double>)
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))) {
        overloadId = 1; // FloatNodeCreationProperty(double)
    } else if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_VECTOR_DOUBLE_IDX], (pyArgs[0])))) {
        overloadId = 0; // FloatNodeCreationProperty(std::vector<double>)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_FloatNodeCreationProperty_Init_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // FloatNodeCreationProperty(const std::vector<double > & values)
        {
            if (kwds) {
                PyObject *keyName = nullptr;
                PyObject *value = nullptr;
                keyName = Py_BuildValue("s","values");
                if (PyDict_Contains(kwds, keyName)) {
                    value = PyDict_GetItem(kwds, keyName);
                    if (value && pyArgs[0]) {
                        PyErr_SetString(PyExc_TypeError, "NatronEngine.FloatNodeCreationProperty(): got multiple values for keyword argument 'values'.");
                        return -1;
                    }
                    if (value) {
                        pyArgs[0] = value;
                        if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_VECTOR_DOUBLE_IDX], (pyArgs[0]))))
                            goto Sbk_FloatNodeCreationProperty_Init_TypeError;
                    }
                }
            }
            ::std::vector<double > cppArg0;
            if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // FloatNodeCreationProperty(std::vector<double>)
                cptr = new ::FloatNodeCreationPropertyWrapper(cppArg0);
            }
            break;
        }
        case 1: // FloatNodeCreationProperty(double value)
        {
            double cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // FloatNodeCreationProperty(double)
                cptr = new ::FloatNodeCreationPropertyWrapper(cppArg0);
            }
            break;
        }
    }

    if (PyErr_Occurred() || !Shiboken::Object::setCppPointer(sbkSelf, Shiboken::SbkType< ::FloatNodeCreationProperty >(), cptr)) {
        delete cptr;
        return -1;
    }
    if (!cptr) goto Sbk_FloatNodeCreationProperty_Init_TypeError;

    Shiboken::Object::setValidCpp(sbkSelf, true);
    Shiboken::Object::setHasCppWrapper(sbkSelf, true);
    if (Shiboken::BindingManager::instance().hasWrapper(cptr)) {
        Shiboken::BindingManager::instance().releaseWrapper(Shiboken::BindingManager::instance().retrieveWrapper(cptr));
    }
    Shiboken::BindingManager::instance().registerWrapper(sbkSelf, cptr);


    return 1;

    Sbk_FloatNodeCreationProperty_Init_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.FloatNodeCreationProperty");
        return -1;
}

static PyObject *Sbk_FloatNodeCreationPropertyFunc_getValues(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::FloatNodeCreationProperty *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_FLOATNODECREATIONPROPERTY_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getValues()const
            const std::vector<double > & cppResult = const_cast<const ::FloatNodeCreationProperty *>(cppSelf)->getValues();
            pyResult = Shiboken::Conversions::copyToPython(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_VECTOR_DOUBLE_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_FloatNodeCreationPropertyFunc_setValue(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::FloatNodeCreationProperty *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_FLOATNODECREATIONPROPERTY_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.FloatNodeCreationProperty.setValue(): too many arguments");
        return {};
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.FloatNodeCreationProperty.setValue(): not enough arguments");
        return {};
    }

    if (!PyArg_ParseTuple(args, "|OO:setValue", &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: FloatNodeCreationProperty::setValue(double,int)
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // setValue(double,int)
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
            overloadId = 0; // setValue(double,int)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_FloatNodeCreationPropertyFunc_setValue_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject *keyName = nullptr;
            PyObject *value = nullptr;
            keyName = Py_BuildValue("s","index");
            if (PyDict_Contains(kwds, keyName)) {
                value = PyDict_GetItem(kwds, keyName);
                if (value && pyArgs[1]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.FloatNodeCreationProperty.setValue(): got multiple values for keyword argument 'index'.");
                    return {};
                }
                if (value) {
                    pyArgs[1] = value;
                    if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                        goto Sbk_FloatNodeCreationPropertyFunc_setValue_TypeError;
                }
            }
        }
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1 = 0;
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // setValue(double,int)
            cppSelf->setValue(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_FloatNodeCreationPropertyFunc_setValue_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.FloatNodeCreationProperty.setValue");
        return {};
}


static const char *Sbk_FloatNodeCreationProperty_PropertyStrings[] = {
    nullptr // Sentinel
};

static PyMethodDef Sbk_FloatNodeCreationProperty_methods[] = {
    {"getValues", reinterpret_cast<PyCFunction>(Sbk_FloatNodeCreationPropertyFunc_getValues), METH_NOARGS},
    {"setValue", reinterpret_cast<PyCFunction>(Sbk_FloatNodeCreationPropertyFunc_setValue), METH_VARARGS|METH_KEYWORDS},

    {nullptr, nullptr} // Sentinel
};

static int Sbk_FloatNodeCreationProperty_setattro(PyObject *self, PyObject *name, PyObject *value)
{
    PySide::Feature::Select(self);
    if (value && PyCallable_Check(value)) {
        auto plain_inst = reinterpret_cast< ::FloatNodeCreationProperty *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_FLOATNODECREATIONPROPERTY_IDX], reinterpret_cast<SbkObject *>(self)));
        auto inst = dynamic_cast<FloatNodeCreationPropertyWrapper *>(plain_inst);
        if (inst)
            inst->resetPyMethodCache();
    }
    return PyObject_GenericSetAttr(self, name, value);
}

} // extern "C"

static int Sbk_FloatNodeCreationProperty_traverse(PyObject *self, visitproc visit, void *arg)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_traverse(self, visit, arg);
}
static int Sbk_FloatNodeCreationProperty_clear(PyObject *self)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType *_Sbk_FloatNodeCreationProperty_Type = nullptr;
static SbkObjectType *Sbk_FloatNodeCreationProperty_TypeF(void)
{
    return _Sbk_FloatNodeCreationProperty_Type;
}

static PyType_Slot Sbk_FloatNodeCreationProperty_slots[] = {
    {Py_tp_base,        nullptr}, // inserted by introduceWrapperType
    {Py_tp_dealloc,     reinterpret_cast<void *>(&SbkDeallocWrapper)},
    {Py_tp_repr,        nullptr},
    {Py_tp_hash,        nullptr},
    {Py_tp_call,        nullptr},
    {Py_tp_str,         nullptr},
    {Py_tp_getattro,    nullptr},
    {Py_tp_setattro,    reinterpret_cast<void *>(Sbk_FloatNodeCreationProperty_setattro)},
    {Py_tp_traverse,    reinterpret_cast<void *>(Sbk_FloatNodeCreationProperty_traverse)},
    {Py_tp_clear,       reinterpret_cast<void *>(Sbk_FloatNodeCreationProperty_clear)},
    {Py_tp_richcompare, nullptr},
    {Py_tp_iter,        nullptr},
    {Py_tp_iternext,    nullptr},
    {Py_tp_methods,     reinterpret_cast<void *>(Sbk_FloatNodeCreationProperty_methods)},
    {Py_tp_getset,      nullptr},
    {Py_tp_init,        reinterpret_cast<void *>(Sbk_FloatNodeCreationProperty_Init)},
    {Py_tp_new,         reinterpret_cast<void *>(SbkObjectTpNew)},
    {0, nullptr}
};
static PyType_Spec Sbk_FloatNodeCreationProperty_spec = {
    "1:NatronEngine.FloatNodeCreationProperty",
    sizeof(SbkObject),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    Sbk_FloatNodeCreationProperty_slots
};

} //extern "C"

static void *Sbk_FloatNodeCreationProperty_typeDiscovery(void *cptr, SbkObjectType *instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType *>(Shiboken::SbkType< ::NodeCreationProperty >()))
        return dynamic_cast< ::FloatNodeCreationProperty *>(reinterpret_cast< ::NodeCreationProperty *>(cptr));
    return {};
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void FloatNodeCreationProperty_PythonToCpp_FloatNodeCreationProperty_PTR(PyObject *pyIn, void *cppOut) {
    Shiboken::Conversions::pythonToCppPointer(Sbk_FloatNodeCreationProperty_TypeF(), pyIn, cppOut);
}
static PythonToCppFunc is_FloatNodeCreationProperty_PythonToCpp_FloatNodeCreationProperty_PTR_Convertible(PyObject *pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, reinterpret_cast<PyTypeObject *>(Sbk_FloatNodeCreationProperty_TypeF())))
        return FloatNodeCreationProperty_PythonToCpp_FloatNodeCreationProperty_PTR;
    return {};
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject *FloatNodeCreationProperty_PTR_CppToPython_FloatNodeCreationProperty(const void *cppIn) {
    auto pyOut = reinterpret_cast<PyObject *>(Shiboken::BindingManager::instance().retrieveWrapper(cppIn));
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    bool changedTypeName = false;
    auto tCppIn = reinterpret_cast<const ::FloatNodeCreationProperty *>(cppIn);
    const char *typeName = typeid(*tCppIn).name();
    auto sbkType = Shiboken::ObjectType::typeForTypeName(typeName);
    if (sbkType && Shiboken::ObjectType::hasSpecialCastFunction(sbkType)) {
        typeName = typeNameOf(tCppIn);
        changedTypeName = true;
    }
    PyObject *result = Shiboken::Object::newObject(Sbk_FloatNodeCreationProperty_TypeF(), const_cast<void *>(cppIn), false, /* exactType */ changedTypeName, typeName);
    if (changedTypeName)
        delete [] typeName;
    return result;
}

// The signatures string for the functions.
// Multiple signatures have their index "n:" in front.
static const char *FloatNodeCreationProperty_SignatureStrings[] = {
    "1:NatronEngine.FloatNodeCreationProperty(self,values:std.vector[double]=std.vector< double >())",
    "0:NatronEngine.FloatNodeCreationProperty(self,value:double)",
    "NatronEngine.FloatNodeCreationProperty.getValues(self)->std.vector[double]",
    "NatronEngine.FloatNodeCreationProperty.setValue(self,value:double,index:int=0)",
    nullptr}; // Sentinel

void init_FloatNodeCreationProperty(PyObject *module)
{
    _Sbk_FloatNodeCreationProperty_Type = Shiboken::ObjectType::introduceWrapperType(
        module,
        "FloatNodeCreationProperty",
        "FloatNodeCreationProperty*",
        &Sbk_FloatNodeCreationProperty_spec,
        &Shiboken::callCppDestructor< ::FloatNodeCreationProperty >,
        reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_NODECREATIONPROPERTY_IDX]),
        0,
        0    );
    
    auto pyType = reinterpret_cast<PyTypeObject *>(_Sbk_FloatNodeCreationProperty_Type);
    InitSignatureStrings(pyType, FloatNodeCreationProperty_SignatureStrings);
    SbkObjectType_SetPropertyStrings(reinterpret_cast<PyTypeObject *>(_Sbk_FloatNodeCreationProperty_Type), Sbk_FloatNodeCreationProperty_PropertyStrings);
    SbkNatronEngineTypes[SBK_FLOATNODECREATIONPROPERTY_IDX]
        = reinterpret_cast<PyTypeObject *>(Sbk_FloatNodeCreationProperty_TypeF());

    // Register Converter
    SbkConverter *converter = Shiboken::Conversions::createConverter(Sbk_FloatNodeCreationProperty_TypeF(),
        FloatNodeCreationProperty_PythonToCpp_FloatNodeCreationProperty_PTR,
        is_FloatNodeCreationProperty_PythonToCpp_FloatNodeCreationProperty_PTR_Convertible,
        FloatNodeCreationProperty_PTR_CppToPython_FloatNodeCreationProperty);

    Shiboken::Conversions::registerConverterName(converter, "FloatNodeCreationProperty");
    Shiboken::Conversions::registerConverterName(converter, "FloatNodeCreationProperty*");
    Shiboken::Conversions::registerConverterName(converter, "FloatNodeCreationProperty&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::FloatNodeCreationProperty).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::FloatNodeCreationPropertyWrapper).name());


    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(Sbk_FloatNodeCreationProperty_TypeF(), &Sbk_FloatNodeCreationProperty_typeDiscovery);


    FloatNodeCreationPropertyWrapper::pysideInitQtMetaTypes();
}
