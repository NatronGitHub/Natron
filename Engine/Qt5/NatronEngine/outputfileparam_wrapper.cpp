
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
#include "outputfileparam_wrapper.h"

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

void OutputFileParamWrapper::pysideInitQtMetaTypes()
{
}

void OutputFileParamWrapper::resetPyMethodCache()
{
    std::fill_n(m_PyMethodCache, sizeof(m_PyMethodCache) / sizeof(m_PyMethodCache[0]), false);
}

OutputFileParamWrapper::~OutputFileParamWrapper()
{
    SbkObject *wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject *Sbk_OutputFileParamFunc_openFile(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::OutputFileParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_OUTPUTFILEPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // openFile()
            cppSelf->openFile();
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;
}

static PyObject *Sbk_OutputFileParamFunc_setSequenceEnabled(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::OutputFileParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_OUTPUTFILEPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: OutputFileParam::setSequenceEnabled(bool)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArg)))) {
        overloadId = 0; // setSequenceEnabled(bool)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_OutputFileParamFunc_setSequenceEnabled_TypeError;

    // Call function/method
    {
        bool cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setSequenceEnabled(bool)
            cppSelf->setSequenceEnabled(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_OutputFileParamFunc_setSequenceEnabled_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.OutputFileParam.setSequenceEnabled");
        return {};
}


static const char *Sbk_OutputFileParam_PropertyStrings[] = {
    nullptr // Sentinel
};

static PyMethodDef Sbk_OutputFileParam_methods[] = {
    {"openFile", reinterpret_cast<PyCFunction>(Sbk_OutputFileParamFunc_openFile), METH_NOARGS},
    {"setSequenceEnabled", reinterpret_cast<PyCFunction>(Sbk_OutputFileParamFunc_setSequenceEnabled), METH_O},

    {nullptr, nullptr} // Sentinel
};

static int Sbk_OutputFileParam_setattro(PyObject *self, PyObject *name, PyObject *value)
{
    PySide::Feature::Select(self);
    if (value && PyCallable_Check(value)) {
        auto plain_inst = reinterpret_cast< ::OutputFileParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_OUTPUTFILEPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
        auto inst = dynamic_cast<OutputFileParamWrapper *>(plain_inst);
        if (inst)
            inst->resetPyMethodCache();
    }
    return PyObject_GenericSetAttr(self, name, value);
}

} // extern "C"

static int Sbk_OutputFileParam_traverse(PyObject *self, visitproc visit, void *arg)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_traverse(self, visit, arg);
}
static int Sbk_OutputFileParam_clear(PyObject *self)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType *_Sbk_OutputFileParam_Type = nullptr;
static SbkObjectType *Sbk_OutputFileParam_TypeF(void)
{
    return _Sbk_OutputFileParam_Type;
}

static PyType_Slot Sbk_OutputFileParam_slots[] = {
    {Py_tp_base,        nullptr}, // inserted by introduceWrapperType
    {Py_tp_dealloc,     reinterpret_cast<void *>(&SbkDeallocWrapper)},
    {Py_tp_repr,        nullptr},
    {Py_tp_hash,        nullptr},
    {Py_tp_call,        nullptr},
    {Py_tp_str,         nullptr},
    {Py_tp_getattro,    nullptr},
    {Py_tp_setattro,    reinterpret_cast<void *>(Sbk_OutputFileParam_setattro)},
    {Py_tp_traverse,    reinterpret_cast<void *>(Sbk_OutputFileParam_traverse)},
    {Py_tp_clear,       reinterpret_cast<void *>(Sbk_OutputFileParam_clear)},
    {Py_tp_richcompare, nullptr},
    {Py_tp_iter,        nullptr},
    {Py_tp_iternext,    nullptr},
    {Py_tp_methods,     reinterpret_cast<void *>(Sbk_OutputFileParam_methods)},
    {Py_tp_getset,      nullptr},
    {Py_tp_init,        nullptr},
    {Py_tp_new,         reinterpret_cast<void *>(SbkDummyNew /* PYSIDE-595: Prevent replacement of "0" with base->tp_new. */)},
    {0, nullptr}
};
static PyType_Spec Sbk_OutputFileParam_spec = {
    "1:NatronEngine.OutputFileParam",
    sizeof(SbkObject),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    Sbk_OutputFileParam_slots
};

} //extern "C"

static void *Sbk_OutputFileParam_typeDiscovery(void *cptr, SbkObjectType *instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType *>(Shiboken::SbkType< ::Param >()))
        return dynamic_cast< ::OutputFileParam *>(reinterpret_cast< ::Param *>(cptr));
    return {};
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void OutputFileParam_PythonToCpp_OutputFileParam_PTR(PyObject *pyIn, void *cppOut) {
    Shiboken::Conversions::pythonToCppPointer(Sbk_OutputFileParam_TypeF(), pyIn, cppOut);
}
static PythonToCppFunc is_OutputFileParam_PythonToCpp_OutputFileParam_PTR_Convertible(PyObject *pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, reinterpret_cast<PyTypeObject *>(Sbk_OutputFileParam_TypeF())))
        return OutputFileParam_PythonToCpp_OutputFileParam_PTR;
    return {};
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject *OutputFileParam_PTR_CppToPython_OutputFileParam(const void *cppIn) {
    auto pyOut = reinterpret_cast<PyObject *>(Shiboken::BindingManager::instance().retrieveWrapper(cppIn));
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    bool changedTypeName = false;
    auto tCppIn = reinterpret_cast<const ::OutputFileParam *>(cppIn);
    const char *typeName = typeid(*tCppIn).name();
    auto sbkType = Shiboken::ObjectType::typeForTypeName(typeName);
    if (sbkType && Shiboken::ObjectType::hasSpecialCastFunction(sbkType)) {
        typeName = typeNameOf(tCppIn);
        changedTypeName = true;
    }
    PyObject *result = Shiboken::Object::newObject(Sbk_OutputFileParam_TypeF(), const_cast<void *>(cppIn), false, /* exactType */ changedTypeName, typeName);
    if (changedTypeName)
        delete [] typeName;
    return result;
}

// The signatures string for the functions.
// Multiple signatures have their index "n:" in front.
static const char *OutputFileParam_SignatureStrings[] = {
    "NatronEngine.OutputFileParam.openFile(self)",
    "NatronEngine.OutputFileParam.setSequenceEnabled(self,enabled:bool)",
    nullptr}; // Sentinel

void init_OutputFileParam(PyObject *module)
{
    _Sbk_OutputFileParam_Type = Shiboken::ObjectType::introduceWrapperType(
        module,
        "OutputFileParam",
        "OutputFileParam*",
        &Sbk_OutputFileParam_spec,
        &Shiboken::callCppDestructor< ::OutputFileParam >,
        reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_STRINGPARAMBASE_IDX]),
        0,
        0    );
    
    auto pyType = reinterpret_cast<PyTypeObject *>(_Sbk_OutputFileParam_Type);
    InitSignatureStrings(pyType, OutputFileParam_SignatureStrings);
    SbkObjectType_SetPropertyStrings(reinterpret_cast<PyTypeObject *>(_Sbk_OutputFileParam_Type), Sbk_OutputFileParam_PropertyStrings);
    SbkNatronEngineTypes[SBK_OUTPUTFILEPARAM_IDX]
        = reinterpret_cast<PyTypeObject *>(Sbk_OutputFileParam_TypeF());

    // Register Converter
    SbkConverter *converter = Shiboken::Conversions::createConverter(Sbk_OutputFileParam_TypeF(),
        OutputFileParam_PythonToCpp_OutputFileParam_PTR,
        is_OutputFileParam_PythonToCpp_OutputFileParam_PTR_Convertible,
        OutputFileParam_PTR_CppToPython_OutputFileParam);

    Shiboken::Conversions::registerConverterName(converter, "OutputFileParam");
    Shiboken::Conversions::registerConverterName(converter, "OutputFileParam*");
    Shiboken::Conversions::registerConverterName(converter, "OutputFileParam&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::OutputFileParam).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::OutputFileParamWrapper).name());


    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(Sbk_OutputFileParam_TypeF(), &Sbk_OutputFileParam_typeDiscovery);


    OutputFileParamWrapper::pysideInitQtMetaTypes();
}
