
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
#include "stringparam_wrapper.h"

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

void StringParamWrapper::pysideInitQtMetaTypes()
{
    qRegisterMetaType< ::StringParam::TypeEnum >("StringParam::TypeEnum");
}

void StringParamWrapper::resetPyMethodCache()
{
    std::fill_n(m_PyMethodCache, sizeof(m_PyMethodCache) / sizeof(m_PyMethodCache[0]), false);
}

StringParamWrapper::~StringParamWrapper()
{
    SbkObject *wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject *Sbk_StringParamFunc_setType(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::StringParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_STRINGPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.StringParam.setType";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: StringParam::setType(StringParam::TypeEnum)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(*PepType_SGTP(SbkNatronEngineTypes[SBK_STRINGPARAM_TYPEENUM_IDX])->converter, (pyArg)))) {
        overloadId = 0; // setType(StringParam::TypeEnum)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_StringParamFunc_setType_TypeError;

    // Call function/method
    {
        ::StringParam::TypeEnum cppArg0{StringParam::eStringTypeLabel};
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setType(StringParam::TypeEnum)
            cppSelf->setType(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_StringParamFunc_setType_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}


static const char *Sbk_StringParam_PropertyStrings[] = {
    nullptr // Sentinel
};

static PyMethodDef Sbk_StringParam_methods[] = {
    {"setType", reinterpret_cast<PyCFunction>(Sbk_StringParamFunc_setType), METH_O},

    {nullptr, nullptr} // Sentinel
};

static int Sbk_StringParam_setattro(PyObject *self, PyObject *name, PyObject *value)
{
    PySide::Feature::Select(self);
    if (value && PyCallable_Check(value)) {
        auto plain_inst = reinterpret_cast< ::StringParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_STRINGPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
        auto inst = dynamic_cast<StringParamWrapper *>(plain_inst);
        if (inst)
            inst->resetPyMethodCache();
    }
    return PyObject_GenericSetAttr(self, name, value);
}

} // extern "C"

static int Sbk_StringParam_traverse(PyObject *self, visitproc visit, void *arg)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_traverse(self, visit, arg);
}
static int Sbk_StringParam_clear(PyObject *self)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType *_Sbk_StringParam_Type = nullptr;
static SbkObjectType *Sbk_StringParam_TypeF(void)
{
    return _Sbk_StringParam_Type;
}

static PyType_Slot Sbk_StringParam_slots[] = {
    {Py_tp_base,        nullptr}, // inserted by introduceWrapperType
    {Py_tp_dealloc,     reinterpret_cast<void *>(&SbkDeallocWrapper)},
    {Py_tp_repr,        nullptr},
    {Py_tp_hash,        nullptr},
    {Py_tp_call,        nullptr},
    {Py_tp_str,         nullptr},
    {Py_tp_getattro,    nullptr},
    {Py_tp_setattro,    reinterpret_cast<void *>(Sbk_StringParam_setattro)},
    {Py_tp_traverse,    reinterpret_cast<void *>(Sbk_StringParam_traverse)},
    {Py_tp_clear,       reinterpret_cast<void *>(Sbk_StringParam_clear)},
    {Py_tp_richcompare, nullptr},
    {Py_tp_iter,        nullptr},
    {Py_tp_iternext,    nullptr},
    {Py_tp_methods,     reinterpret_cast<void *>(Sbk_StringParam_methods)},
    {Py_tp_getset,      nullptr},
    {Py_tp_init,        nullptr},
    {Py_tp_new,         reinterpret_cast<void *>(SbkDummyNew /* PYSIDE-595: Prevent replacement of "0" with base->tp_new. */)},
    {0, nullptr}
};
static PyType_Spec Sbk_StringParam_spec = {
    "1:NatronEngine.StringParam",
    sizeof(SbkObject),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    Sbk_StringParam_slots
};

} //extern "C"

static void *Sbk_StringParam_typeDiscovery(void *cptr, SbkObjectType *instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType *>(Shiboken::SbkType< ::Param >()))
        return dynamic_cast< ::StringParam *>(reinterpret_cast< ::Param *>(cptr));
    return {};
}


// Type conversion functions.

// Python to C++ enum conversion.
static void StringParam_TypeEnum_PythonToCpp_StringParam_TypeEnum(PyObject *pyIn, void *cppOut) {
    *reinterpret_cast<::StringParam::TypeEnum *>(cppOut) =
        static_cast<::StringParam::TypeEnum>(Shiboken::Enum::getValue(pyIn));

}
static PythonToCppFunc is_StringParam_TypeEnum_PythonToCpp_StringParam_TypeEnum_Convertible(PyObject *pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_STRINGPARAM_TYPEENUM_IDX]))
        return StringParam_TypeEnum_PythonToCpp_StringParam_TypeEnum;
    return {};
}
static PyObject *StringParam_TypeEnum_CppToPython_StringParam_TypeEnum(const void *cppIn) {
    const int castCppIn = int(*reinterpret_cast<const ::StringParam::TypeEnum *>(cppIn));
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_STRINGPARAM_TYPEENUM_IDX], castCppIn);

}

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void StringParam_PythonToCpp_StringParam_PTR(PyObject *pyIn, void *cppOut) {
    Shiboken::Conversions::pythonToCppPointer(Sbk_StringParam_TypeF(), pyIn, cppOut);
}
static PythonToCppFunc is_StringParam_PythonToCpp_StringParam_PTR_Convertible(PyObject *pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, reinterpret_cast<PyTypeObject *>(Sbk_StringParam_TypeF())))
        return StringParam_PythonToCpp_StringParam_PTR;
    return {};
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject *StringParam_PTR_CppToPython_StringParam(const void *cppIn) {
    auto pyOut = reinterpret_cast<PyObject *>(Shiboken::BindingManager::instance().retrieveWrapper(cppIn));
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    bool changedTypeName = false;
    auto tCppIn = reinterpret_cast<const ::StringParam *>(cppIn);
    const char *typeName = typeid(*tCppIn).name();
    auto sbkType = Shiboken::ObjectType::typeForTypeName(typeName);
    if (sbkType && Shiboken::ObjectType::hasSpecialCastFunction(sbkType)) {
        typeName = typeNameOf(tCppIn);
        changedTypeName = true;
    }
    PyObject *result = Shiboken::Object::newObject(Sbk_StringParam_TypeF(), const_cast<void *>(cppIn), false, /* exactType */ changedTypeName, typeName);
    if (changedTypeName)
        delete [] typeName;
    return result;
}

// The signatures string for the functions.
// Multiple signatures have their index "n:" in front.
static const char *StringParam_SignatureStrings[] = {
    "NatronEngine.StringParam.setType(self,type:NatronEngine.StringParam.TypeEnum)",
    nullptr}; // Sentinel

void init_StringParam(PyObject *module)
{
    _Sbk_StringParam_Type = Shiboken::ObjectType::introduceWrapperType(
        module,
        "StringParam",
        "StringParam*",
        &Sbk_StringParam_spec,
        &Shiboken::callCppDestructor< ::StringParam >,
        reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_STRINGPARAMBASE_IDX]),
        0,
        0    );
    
    auto pyType = reinterpret_cast<PyTypeObject *>(_Sbk_StringParam_Type);
    InitSignatureStrings(pyType, StringParam_SignatureStrings);
    SbkObjectType_SetPropertyStrings(reinterpret_cast<PyTypeObject *>(_Sbk_StringParam_Type), Sbk_StringParam_PropertyStrings);
    SbkNatronEngineTypes[SBK_STRINGPARAM_IDX]
        = reinterpret_cast<PyTypeObject *>(Sbk_StringParam_TypeF());

    // Register Converter
    SbkConverter *converter = Shiboken::Conversions::createConverter(Sbk_StringParam_TypeF(),
        StringParam_PythonToCpp_StringParam_PTR,
        is_StringParam_PythonToCpp_StringParam_PTR_Convertible,
        StringParam_PTR_CppToPython_StringParam);

    Shiboken::Conversions::registerConverterName(converter, "StringParam");
    Shiboken::Conversions::registerConverterName(converter, "StringParam*");
    Shiboken::Conversions::registerConverterName(converter, "StringParam&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::StringParam).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::StringParamWrapper).name());


    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(Sbk_StringParam_TypeF(), &Sbk_StringParam_typeDiscovery);

    // Initialization of enums.

    // Initialization of enum 'TypeEnum'.
    SbkNatronEngineTypes[SBK_STRINGPARAM_TYPEENUM_IDX] = Shiboken::Enum::createScopedEnum(Sbk_StringParam_TypeF(),
        "TypeEnum",
        "1:NatronEngine.StringParam.TypeEnum",
        "StringParam::TypeEnum");
    if (!SbkNatronEngineTypes[SBK_STRINGPARAM_TYPEENUM_IDX])
        return;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_STRINGPARAM_TYPEENUM_IDX],
        Sbk_StringParam_TypeF(), "eStringTypeLabel", (long) StringParam::TypeEnum::eStringTypeLabel))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_STRINGPARAM_TYPEENUM_IDX],
        Sbk_StringParam_TypeF(), "eStringTypeMultiLine", (long) StringParam::TypeEnum::eStringTypeMultiLine))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_STRINGPARAM_TYPEENUM_IDX],
        Sbk_StringParam_TypeF(), "eStringTypeRichTextMultiLine", (long) StringParam::TypeEnum::eStringTypeRichTextMultiLine))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_STRINGPARAM_TYPEENUM_IDX],
        Sbk_StringParam_TypeF(), "eStringTypeCustom", (long) StringParam::TypeEnum::eStringTypeCustom))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_STRINGPARAM_TYPEENUM_IDX],
        Sbk_StringParam_TypeF(), "eStringTypeDefault", (long) StringParam::TypeEnum::eStringTypeDefault))
        return;
    // Register converter for enum 'StringParam::TypeEnum'.
    {
        SbkConverter *converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_STRINGPARAM_TYPEENUM_IDX],
            StringParam_TypeEnum_CppToPython_StringParam_TypeEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            StringParam_TypeEnum_PythonToCpp_StringParam_TypeEnum,
            is_StringParam_TypeEnum_PythonToCpp_StringParam_TypeEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_STRINGPARAM_TYPEENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "StringParam::TypeEnum");
        Shiboken::Conversions::registerConverterName(converter, "TypeEnum");
    }
    // End of 'TypeEnum' enum.

    StringParamWrapper::pysideInitQtMetaTypes();
}
