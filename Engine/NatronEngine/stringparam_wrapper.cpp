
// default includes
#include "Global/Macros.h"
CLANG_DIAG_OFF(mismatched-tags)
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_OFF(missing-field-initializers)
GCC_DIAG_OFF(missing-declarations)
GCC_DIAG_OFF(uninitialized)
#include <shiboken.h> // produces many warnings
#include <pysidesignal.h>
#include <pysideproperty.h>
#include <pyside.h>
#include <typeresolver.h>
#include <typeinfo>
#include "natronengine_python.h"

#include "stringparam_wrapper.h"

// Extra includes
NATRON_NAMESPACE_USING
#include <PyParameter.h>


// Native ---------------------------------------------------------

void StringParamWrapper::pysideInitQtMetaTypes()
{
    qRegisterMetaType< ::StringParam::TypeEnum >("StringParam::TypeEnum");
}

StringParamWrapper::~StringParamWrapper()
{
    SbkObject* wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject* Sbk_StringParamFunc_setType(PyObject* self, PyObject* pyArg)
{
    StringParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (StringParamWrapper*)((::StringParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_STRINGPARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setType(StringParam::TypeEnum)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SBK_CONVERTER(SbkNatronEngineTypes[SBK_STRINGPARAM_TYPEENUM_IDX]), (pyArg)))) {
        overloadId = 0; // setType(StringParam::TypeEnum)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_StringParamFunc_setType_TypeError;

    // Call function/method
    {
        ::StringParam::TypeEnum cppArg0 = ((::StringParam::TypeEnum)0);
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setType(StringParam::TypeEnum)
            cppSelf->setType(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_StringParamFunc_setType_TypeError:
        const char* overloads[] = {"NatronEngine.StringParam.TypeEnum", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.StringParam.setType", overloads);
        return 0;
}

static PyMethodDef Sbk_StringParam_methods[] = {
    {"setType", (PyCFunction)Sbk_StringParamFunc_setType, METH_O},

    {0} // Sentinel
};

} // extern "C"

static int Sbk_StringParam_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_StringParam_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_StringParam_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.StringParam",
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
    /*tp_traverse*/         Sbk_StringParam_traverse,
    /*tp_clear*/            Sbk_StringParam_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_StringParam_methods,
    /*tp_members*/          0,
    /*tp_getset*/           0,
    /*tp_base*/             0,
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

static void* Sbk_StringParam_typeDiscovery(void* cptr, SbkObjectType* instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType*>(Shiboken::SbkType< ::Param >()))
        return dynamic_cast< ::StringParam*>(reinterpret_cast< ::Param*>(cptr));
    return 0;
}


// Type conversion functions.

// Python to C++ enum conversion.
static void StringParam_TypeEnum_PythonToCpp_StringParam_TypeEnum(PyObject* pyIn, void* cppOut) {
    *((::StringParam::TypeEnum*)cppOut) = (::StringParam::TypeEnum) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_StringParam_TypeEnum_PythonToCpp_StringParam_TypeEnum_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_STRINGPARAM_TYPEENUM_IDX]))
        return StringParam_TypeEnum_PythonToCpp_StringParam_TypeEnum;
    return 0;
}
static PyObject* StringParam_TypeEnum_CppToPython_StringParam_TypeEnum(const void* cppIn) {
    int castCppIn = *((::StringParam::TypeEnum*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_STRINGPARAM_TYPEENUM_IDX], castCppIn);

}

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void StringParam_PythonToCpp_StringParam_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_StringParam_Type, pyIn, cppOut);
}
static PythonToCppFunc is_StringParam_PythonToCpp_StringParam_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_StringParam_Type))
        return StringParam_PythonToCpp_StringParam_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* StringParam_PTR_CppToPython_StringParam(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::StringParam*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_StringParam_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_StringParam(PyObject* module)
{
    SbkNatronEngineTypes[SBK_STRINGPARAM_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_StringParam_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "StringParam", "StringParam*",
        &Sbk_StringParam_Type, &Shiboken::callCppDestructor< ::StringParam >, (SbkObjectType*)SbkNatronEngineTypes[SBK_STRINGPARAMBASE_IDX])) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_StringParam_Type,
        StringParam_PythonToCpp_StringParam_PTR,
        is_StringParam_PythonToCpp_StringParam_PTR_Convertible,
        StringParam_PTR_CppToPython_StringParam);

    Shiboken::Conversions::registerConverterName(converter, "StringParam");
    Shiboken::Conversions::registerConverterName(converter, "StringParam*");
    Shiboken::Conversions::registerConverterName(converter, "StringParam&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::StringParam).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::StringParamWrapper).name());


    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(&Sbk_StringParam_Type, &Sbk_StringParam_typeDiscovery);

    // Initialization of enums.

    // Initialization of enum 'TypeEnum'.
    SbkNatronEngineTypes[SBK_STRINGPARAM_TYPEENUM_IDX] = Shiboken::Enum::createScopedEnum(&Sbk_StringParam_Type,
        "TypeEnum",
        "NatronEngine.StringParam.TypeEnum",
        "StringParam::TypeEnum");
    if (!SbkNatronEngineTypes[SBK_STRINGPARAM_TYPEENUM_IDX])
        return ;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_STRINGPARAM_TYPEENUM_IDX],
        &Sbk_StringParam_Type, "eStringTypeLabel", (long) StringParam::eStringTypeLabel))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_STRINGPARAM_TYPEENUM_IDX],
        &Sbk_StringParam_Type, "eStringTypeMultiLine", (long) StringParam::eStringTypeMultiLine))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_STRINGPARAM_TYPEENUM_IDX],
        &Sbk_StringParam_Type, "eStringTypeRichTextMultiLine", (long) StringParam::eStringTypeRichTextMultiLine))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_STRINGPARAM_TYPEENUM_IDX],
        &Sbk_StringParam_Type, "eStringTypeCustom", (long) StringParam::eStringTypeCustom))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_STRINGPARAM_TYPEENUM_IDX],
        &Sbk_StringParam_Type, "eStringTypeDefault", (long) StringParam::eStringTypeDefault))
        return ;
    // Register converter for enum 'StringParam::TypeEnum'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_STRINGPARAM_TYPEENUM_IDX],
            StringParam_TypeEnum_CppToPython_StringParam_TypeEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            StringParam_TypeEnum_PythonToCpp_StringParam_TypeEnum,
            is_StringParam_TypeEnum_PythonToCpp_StringParam_TypeEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_STRINGPARAM_TYPEENUM_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_STRINGPARAM_TYPEENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "StringParam::TypeEnum");
        Shiboken::Conversions::registerConverterName(converter, "TypeEnum");
    }
    // End of 'TypeEnum' enum.


    StringParamWrapper::pysideInitQtMetaTypes();
}
