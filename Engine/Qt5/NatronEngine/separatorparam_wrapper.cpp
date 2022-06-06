
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
#include "separatorparam_wrapper.h"

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

void SeparatorParamWrapper::pysideInitQtMetaTypes()
{
}

void SeparatorParamWrapper::resetPyMethodCache()
{
    std::fill_n(m_PyMethodCache, sizeof(m_PyMethodCache) / sizeof(m_PyMethodCache[0]), false);
}

SeparatorParamWrapper::~SeparatorParamWrapper()
{
    SbkObject *wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {

static const char *Sbk_SeparatorParam_PropertyStrings[] = {
    nullptr // Sentinel
};

static PyMethodDef Sbk_SeparatorParam_methods[] = {

    {nullptr, nullptr} // Sentinel
};

static int Sbk_SeparatorParam_setattro(PyObject *self, PyObject *name, PyObject *value)
{
    PySide::Feature::Select(self);
    if (value && PyCallable_Check(value)) {
        auto plain_inst = reinterpret_cast< ::SeparatorParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_SEPARATORPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
        auto inst = dynamic_cast<SeparatorParamWrapper *>(plain_inst);
        if (inst)
            inst->resetPyMethodCache();
    }
    return PyObject_GenericSetAttr(self, name, value);
}

} // extern "C"

static int Sbk_SeparatorParam_traverse(PyObject *self, visitproc visit, void *arg)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_traverse(self, visit, arg);
}
static int Sbk_SeparatorParam_clear(PyObject *self)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType *_Sbk_SeparatorParam_Type = nullptr;
static SbkObjectType *Sbk_SeparatorParam_TypeF(void)
{
    return _Sbk_SeparatorParam_Type;
}

static PyType_Slot Sbk_SeparatorParam_slots[] = {
    {Py_tp_base,        nullptr}, // inserted by introduceWrapperType
    {Py_tp_dealloc,     reinterpret_cast<void *>(&SbkDeallocWrapper)},
    {Py_tp_repr,        nullptr},
    {Py_tp_hash,        nullptr},
    {Py_tp_call,        nullptr},
    {Py_tp_str,         nullptr},
    {Py_tp_getattro,    nullptr},
    {Py_tp_setattro,    reinterpret_cast<void *>(Sbk_SeparatorParam_setattro)},
    {Py_tp_traverse,    reinterpret_cast<void *>(Sbk_SeparatorParam_traverse)},
    {Py_tp_clear,       reinterpret_cast<void *>(Sbk_SeparatorParam_clear)},
    {Py_tp_richcompare, nullptr},
    {Py_tp_iter,        nullptr},
    {Py_tp_iternext,    nullptr},
    {Py_tp_methods,     reinterpret_cast<void *>(Sbk_SeparatorParam_methods)},
    {Py_tp_getset,      nullptr},
    {Py_tp_init,        nullptr},
    {Py_tp_new,         reinterpret_cast<void *>(SbkDummyNew /* PYSIDE-595: Prevent replacement of "0" with base->tp_new. */)},
    {0, nullptr}
};
static PyType_Spec Sbk_SeparatorParam_spec = {
    "1:NatronEngine.SeparatorParam",
    sizeof(SbkObject),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    Sbk_SeparatorParam_slots
};

} //extern "C"

static void *Sbk_SeparatorParam_typeDiscovery(void *cptr, SbkObjectType *instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType *>(Shiboken::SbkType< ::Param >()))
        return dynamic_cast< ::SeparatorParam *>(reinterpret_cast< ::Param *>(cptr));
    return {};
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void SeparatorParam_PythonToCpp_SeparatorParam_PTR(PyObject *pyIn, void *cppOut) {
    Shiboken::Conversions::pythonToCppPointer(Sbk_SeparatorParam_TypeF(), pyIn, cppOut);
}
static PythonToCppFunc is_SeparatorParam_PythonToCpp_SeparatorParam_PTR_Convertible(PyObject *pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, reinterpret_cast<PyTypeObject *>(Sbk_SeparatorParam_TypeF())))
        return SeparatorParam_PythonToCpp_SeparatorParam_PTR;
    return {};
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject *SeparatorParam_PTR_CppToPython_SeparatorParam(const void *cppIn) {
    auto pyOut = reinterpret_cast<PyObject *>(Shiboken::BindingManager::instance().retrieveWrapper(cppIn));
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    bool changedTypeName = false;
    auto tCppIn = reinterpret_cast<const ::SeparatorParam *>(cppIn);
    const char *typeName = typeid(*tCppIn).name();
    auto sbkType = Shiboken::ObjectType::typeForTypeName(typeName);
    if (sbkType && Shiboken::ObjectType::hasSpecialCastFunction(sbkType)) {
        typeName = typeNameOf(tCppIn);
        changedTypeName = true;
    }
    PyObject *result = Shiboken::Object::newObject(Sbk_SeparatorParam_TypeF(), const_cast<void *>(cppIn), false, /* exactType */ changedTypeName, typeName);
    if (changedTypeName)
        delete [] typeName;
    return result;
}

// The signatures string for the functions.
// Multiple signatures have their index "n:" in front.
static const char *SeparatorParam_SignatureStrings[] = {
    nullptr}; // Sentinel

void init_SeparatorParam(PyObject *module)
{
    _Sbk_SeparatorParam_Type = Shiboken::ObjectType::introduceWrapperType(
        module,
        "SeparatorParam",
        "SeparatorParam*",
        &Sbk_SeparatorParam_spec,
        &Shiboken::callCppDestructor< ::SeparatorParam >,
        reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_PARAM_IDX]),
        0,
        0    );
    
    auto pyType = reinterpret_cast<PyTypeObject *>(_Sbk_SeparatorParam_Type);
    InitSignatureStrings(pyType, SeparatorParam_SignatureStrings);
    SbkObjectType_SetPropertyStrings(reinterpret_cast<PyTypeObject *>(_Sbk_SeparatorParam_Type), Sbk_SeparatorParam_PropertyStrings);
    SbkNatronEngineTypes[SBK_SEPARATORPARAM_IDX]
        = reinterpret_cast<PyTypeObject *>(Sbk_SeparatorParam_TypeF());

    // Register Converter
    SbkConverter *converter = Shiboken::Conversions::createConverter(Sbk_SeparatorParam_TypeF(),
        SeparatorParam_PythonToCpp_SeparatorParam_PTR,
        is_SeparatorParam_PythonToCpp_SeparatorParam_PTR_Convertible,
        SeparatorParam_PTR_CppToPython_SeparatorParam);

    Shiboken::Conversions::registerConverterName(converter, "SeparatorParam");
    Shiboken::Conversions::registerConverterName(converter, "SeparatorParam*");
    Shiboken::Conversions::registerConverterName(converter, "SeparatorParam&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::SeparatorParam).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::SeparatorParamWrapper).name());


    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(Sbk_SeparatorParam_TypeF(), &Sbk_SeparatorParam_typeDiscovery);

    SeparatorParamWrapper::pysideInitQtMetaTypes();
}
