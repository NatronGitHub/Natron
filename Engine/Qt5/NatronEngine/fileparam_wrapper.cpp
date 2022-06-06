
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
#include "fileparam_wrapper.h"

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

void FileParamWrapper::pysideInitQtMetaTypes()
{
}

void FileParamWrapper::resetPyMethodCache()
{
    std::fill_n(m_PyMethodCache, sizeof(m_PyMethodCache) / sizeof(m_PyMethodCache[0]), false);
}

FileParamWrapper::~FileParamWrapper()
{
    SbkObject *wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject *Sbk_FileParamFunc_openFile(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::FileParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_FILEPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.FileParam.openFile";
    SBK_UNUSED(fullName)

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

static PyObject *Sbk_FileParamFunc_reloadFile(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::FileParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_FILEPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.FileParam.reloadFile";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // reloadFile()
            cppSelf->reloadFile();
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;
}

static PyObject *Sbk_FileParamFunc_setSequenceEnabled(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::FileParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_FILEPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.FileParam.setSequenceEnabled";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: FileParam::setSequenceEnabled(bool)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArg)))) {
        overloadId = 0; // setSequenceEnabled(bool)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_FileParamFunc_setSequenceEnabled_TypeError;

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

    Sbk_FileParamFunc_setSequenceEnabled_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}


static const char *Sbk_FileParam_PropertyStrings[] = {
    nullptr // Sentinel
};

static PyMethodDef Sbk_FileParam_methods[] = {
    {"openFile", reinterpret_cast<PyCFunction>(Sbk_FileParamFunc_openFile), METH_NOARGS},
    {"reloadFile", reinterpret_cast<PyCFunction>(Sbk_FileParamFunc_reloadFile), METH_NOARGS},
    {"setSequenceEnabled", reinterpret_cast<PyCFunction>(Sbk_FileParamFunc_setSequenceEnabled), METH_O},

    {nullptr, nullptr} // Sentinel
};

static int Sbk_FileParam_setattro(PyObject *self, PyObject *name, PyObject *value)
{
    PySide::Feature::Select(self);
    if (value && PyCallable_Check(value)) {
        auto plain_inst = reinterpret_cast< ::FileParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_FILEPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
        auto inst = dynamic_cast<FileParamWrapper *>(plain_inst);
        if (inst)
            inst->resetPyMethodCache();
    }
    return PyObject_GenericSetAttr(self, name, value);
}

} // extern "C"

static int Sbk_FileParam_traverse(PyObject *self, visitproc visit, void *arg)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_traverse(self, visit, arg);
}
static int Sbk_FileParam_clear(PyObject *self)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType *_Sbk_FileParam_Type = nullptr;
static SbkObjectType *Sbk_FileParam_TypeF(void)
{
    return _Sbk_FileParam_Type;
}

static PyType_Slot Sbk_FileParam_slots[] = {
    {Py_tp_base,        nullptr}, // inserted by introduceWrapperType
    {Py_tp_dealloc,     reinterpret_cast<void *>(&SbkDeallocWrapper)},
    {Py_tp_repr,        nullptr},
    {Py_tp_hash,        nullptr},
    {Py_tp_call,        nullptr},
    {Py_tp_str,         nullptr},
    {Py_tp_getattro,    nullptr},
    {Py_tp_setattro,    reinterpret_cast<void *>(Sbk_FileParam_setattro)},
    {Py_tp_traverse,    reinterpret_cast<void *>(Sbk_FileParam_traverse)},
    {Py_tp_clear,       reinterpret_cast<void *>(Sbk_FileParam_clear)},
    {Py_tp_richcompare, nullptr},
    {Py_tp_iter,        nullptr},
    {Py_tp_iternext,    nullptr},
    {Py_tp_methods,     reinterpret_cast<void *>(Sbk_FileParam_methods)},
    {Py_tp_getset,      nullptr},
    {Py_tp_init,        nullptr},
    {Py_tp_new,         reinterpret_cast<void *>(SbkDummyNew /* PYSIDE-595: Prevent replacement of "0" with base->tp_new. */)},
    {0, nullptr}
};
static PyType_Spec Sbk_FileParam_spec = {
    "1:NatronEngine.FileParam",
    sizeof(SbkObject),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    Sbk_FileParam_slots
};

} //extern "C"

static void *Sbk_FileParam_typeDiscovery(void *cptr, SbkObjectType *instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType *>(Shiboken::SbkType< ::Param >()))
        return dynamic_cast< ::FileParam *>(reinterpret_cast< ::Param *>(cptr));
    return {};
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void FileParam_PythonToCpp_FileParam_PTR(PyObject *pyIn, void *cppOut) {
    Shiboken::Conversions::pythonToCppPointer(Sbk_FileParam_TypeF(), pyIn, cppOut);
}
static PythonToCppFunc is_FileParam_PythonToCpp_FileParam_PTR_Convertible(PyObject *pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, reinterpret_cast<PyTypeObject *>(Sbk_FileParam_TypeF())))
        return FileParam_PythonToCpp_FileParam_PTR;
    return {};
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject *FileParam_PTR_CppToPython_FileParam(const void *cppIn) {
    auto pyOut = reinterpret_cast<PyObject *>(Shiboken::BindingManager::instance().retrieveWrapper(cppIn));
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    bool changedTypeName = false;
    auto tCppIn = reinterpret_cast<const ::FileParam *>(cppIn);
    const char *typeName = typeid(*tCppIn).name();
    auto sbkType = Shiboken::ObjectType::typeForTypeName(typeName);
    if (sbkType && Shiboken::ObjectType::hasSpecialCastFunction(sbkType)) {
        typeName = typeNameOf(tCppIn);
        changedTypeName = true;
    }
    PyObject *result = Shiboken::Object::newObject(Sbk_FileParam_TypeF(), const_cast<void *>(cppIn), false, /* exactType */ changedTypeName, typeName);
    if (changedTypeName)
        delete [] typeName;
    return result;
}

// The signatures string for the functions.
// Multiple signatures have their index "n:" in front.
static const char *FileParam_SignatureStrings[] = {
    "NatronEngine.FileParam.openFile(self)",
    "NatronEngine.FileParam.reloadFile(self)",
    "NatronEngine.FileParam.setSequenceEnabled(self,enabled:bool)",
    nullptr}; // Sentinel

void init_FileParam(PyObject *module)
{
    _Sbk_FileParam_Type = Shiboken::ObjectType::introduceWrapperType(
        module,
        "FileParam",
        "FileParam*",
        &Sbk_FileParam_spec,
        &Shiboken::callCppDestructor< ::FileParam >,
        reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_STRINGPARAMBASE_IDX]),
        0,
        0    );
    
    auto pyType = reinterpret_cast<PyTypeObject *>(_Sbk_FileParam_Type);
    InitSignatureStrings(pyType, FileParam_SignatureStrings);
    SbkObjectType_SetPropertyStrings(reinterpret_cast<PyTypeObject *>(_Sbk_FileParam_Type), Sbk_FileParam_PropertyStrings);
    SbkNatronEngineTypes[SBK_FILEPARAM_IDX]
        = reinterpret_cast<PyTypeObject *>(Sbk_FileParam_TypeF());

    // Register Converter
    SbkConverter *converter = Shiboken::Conversions::createConverter(Sbk_FileParam_TypeF(),
        FileParam_PythonToCpp_FileParam_PTR,
        is_FileParam_PythonToCpp_FileParam_PTR_Convertible,
        FileParam_PTR_CppToPython_FileParam);

    Shiboken::Conversions::registerConverterName(converter, "FileParam");
    Shiboken::Conversions::registerConverterName(converter, "FileParam*");
    Shiboken::Conversions::registerConverterName(converter, "FileParam&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::FileParam).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::FileParamWrapper).name());


    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(Sbk_FileParam_TypeF(), &Sbk_FileParam_typeDiscovery);

    FileParamWrapper::pysideInitQtMetaTypes();
}
