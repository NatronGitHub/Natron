
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
#include "groupparam_wrapper.h"

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

void GroupParamWrapper::pysideInitQtMetaTypes()
{
}

void GroupParamWrapper::resetPyMethodCache()
{
    std::fill_n(m_PyMethodCache, sizeof(m_PyMethodCache) / sizeof(m_PyMethodCache[0]), false);
}

GroupParamWrapper::~GroupParamWrapper()
{
    SbkObject *wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject *Sbk_GroupParamFunc_addParam(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<GroupParamWrapper *>(reinterpret_cast< ::GroupParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_GROUPPARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.GroupParam.addParam";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: GroupParam::addParam(const Param*)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppPointerConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_PARAM_IDX]), (pyArg)))) {
        overloadId = 0; // addParam(const Param*)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_GroupParamFunc_addParam_TypeError;

    // Call function/method
    {
        if (!Shiboken::Object::isValid(pyArg))
            return {};
        ::Param *cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // addParam(const Param*)
            cppSelf->addParam(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_GroupParamFunc_addParam_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_GroupParamFunc_getIsOpened(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<GroupParamWrapper *>(reinterpret_cast< ::GroupParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_GROUPPARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.GroupParam.getIsOpened";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getIsOpened()const
            bool cppResult = const_cast<const ::GroupParamWrapper *>(cppSelf)->getIsOpened();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_GroupParamFunc_setAsTab(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<GroupParamWrapper *>(reinterpret_cast< ::GroupParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_GROUPPARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.GroupParam.setAsTab";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // setAsTab()
            cppSelf->setAsTab();
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;
}

static PyObject *Sbk_GroupParamFunc_setOpened(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<GroupParamWrapper *>(reinterpret_cast< ::GroupParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_GROUPPARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.GroupParam.setOpened";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: GroupParam::setOpened(bool)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArg)))) {
        overloadId = 0; // setOpened(bool)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_GroupParamFunc_setOpened_TypeError;

    // Call function/method
    {
        bool cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setOpened(bool)
            // Begin code injection
            cppSelf->setOpened(cppArg0);

            // End of code injection

        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_GroupParamFunc_setOpened_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}


static const char *Sbk_GroupParam_PropertyStrings[] = {
    nullptr // Sentinel
};

static PyMethodDef Sbk_GroupParam_methods[] = {
    {"addParam", reinterpret_cast<PyCFunction>(Sbk_GroupParamFunc_addParam), METH_O},
    {"getIsOpened", reinterpret_cast<PyCFunction>(Sbk_GroupParamFunc_getIsOpened), METH_NOARGS},
    {"setAsTab", reinterpret_cast<PyCFunction>(Sbk_GroupParamFunc_setAsTab), METH_NOARGS},
    {"setOpened", reinterpret_cast<PyCFunction>(Sbk_GroupParamFunc_setOpened), METH_O},

    {nullptr, nullptr} // Sentinel
};

static int Sbk_GroupParam_setattro(PyObject *self, PyObject *name, PyObject *value)
{
    PySide::Feature::Select(self);
    if (value && PyCallable_Check(value)) {
        auto plain_inst = reinterpret_cast< ::GroupParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_GROUPPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
        auto inst = dynamic_cast<GroupParamWrapper *>(plain_inst);
        if (inst)
            inst->resetPyMethodCache();
    }
    return PyObject_GenericSetAttr(self, name, value);
}

} // extern "C"

static int Sbk_GroupParam_traverse(PyObject *self, visitproc visit, void *arg)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_traverse(self, visit, arg);
}
static int Sbk_GroupParam_clear(PyObject *self)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType *_Sbk_GroupParam_Type = nullptr;
static SbkObjectType *Sbk_GroupParam_TypeF(void)
{
    return _Sbk_GroupParam_Type;
}

static PyType_Slot Sbk_GroupParam_slots[] = {
    {Py_tp_base,        nullptr}, // inserted by introduceWrapperType
    {Py_tp_dealloc,     reinterpret_cast<void *>(&SbkDeallocWrapper)},
    {Py_tp_repr,        nullptr},
    {Py_tp_hash,        nullptr},
    {Py_tp_call,        nullptr},
    {Py_tp_str,         nullptr},
    {Py_tp_getattro,    nullptr},
    {Py_tp_setattro,    reinterpret_cast<void *>(Sbk_GroupParam_setattro)},
    {Py_tp_traverse,    reinterpret_cast<void *>(Sbk_GroupParam_traverse)},
    {Py_tp_clear,       reinterpret_cast<void *>(Sbk_GroupParam_clear)},
    {Py_tp_richcompare, nullptr},
    {Py_tp_iter,        nullptr},
    {Py_tp_iternext,    nullptr},
    {Py_tp_methods,     reinterpret_cast<void *>(Sbk_GroupParam_methods)},
    {Py_tp_getset,      nullptr},
    {Py_tp_init,        nullptr},
    {Py_tp_new,         reinterpret_cast<void *>(SbkDummyNew /* PYSIDE-595: Prevent replacement of "0" with base->tp_new. */)},
    {0, nullptr}
};
static PyType_Spec Sbk_GroupParam_spec = {
    "1:NatronEngine.GroupParam",
    sizeof(SbkObject),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    Sbk_GroupParam_slots
};

} //extern "C"

static void *Sbk_GroupParam_typeDiscovery(void *cptr, SbkObjectType *instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType *>(Shiboken::SbkType< ::Param >()))
        return dynamic_cast< ::GroupParam *>(reinterpret_cast< ::Param *>(cptr));
    return {};
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void GroupParam_PythonToCpp_GroupParam_PTR(PyObject *pyIn, void *cppOut) {
    Shiboken::Conversions::pythonToCppPointer(Sbk_GroupParam_TypeF(), pyIn, cppOut);
}
static PythonToCppFunc is_GroupParam_PythonToCpp_GroupParam_PTR_Convertible(PyObject *pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, reinterpret_cast<PyTypeObject *>(Sbk_GroupParam_TypeF())))
        return GroupParam_PythonToCpp_GroupParam_PTR;
    return {};
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject *GroupParam_PTR_CppToPython_GroupParam(const void *cppIn) {
    auto pyOut = reinterpret_cast<PyObject *>(Shiboken::BindingManager::instance().retrieveWrapper(cppIn));
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    bool changedTypeName = false;
    auto tCppIn = reinterpret_cast<const ::GroupParam *>(cppIn);
    const char *typeName = typeid(*tCppIn).name();
    auto sbkType = Shiboken::ObjectType::typeForTypeName(typeName);
    if (sbkType && Shiboken::ObjectType::hasSpecialCastFunction(sbkType)) {
        typeName = typeNameOf(tCppIn);
        changedTypeName = true;
    }
    PyObject *result = Shiboken::Object::newObject(Sbk_GroupParam_TypeF(), const_cast<void *>(cppIn), false, /* exactType */ changedTypeName, typeName);
    if (changedTypeName)
        delete [] typeName;
    return result;
}

// The signatures string for the functions.
// Multiple signatures have their index "n:" in front.
static const char *GroupParam_SignatureStrings[] = {
    "NatronEngine.GroupParam.addParam(self,param:NatronEngine.Param)",
    "NatronEngine.GroupParam.getIsOpened(self)->bool",
    "NatronEngine.GroupParam.setAsTab(self)",
    "NatronEngine.GroupParam.setOpened(self,opened:bool)",
    nullptr}; // Sentinel

void init_GroupParam(PyObject *module)
{
    _Sbk_GroupParam_Type = Shiboken::ObjectType::introduceWrapperType(
        module,
        "GroupParam",
        "GroupParam*",
        &Sbk_GroupParam_spec,
        &Shiboken::callCppDestructor< ::GroupParam >,
        reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_PARAM_IDX]),
        0,
        0    );
    
    auto pyType = reinterpret_cast<PyTypeObject *>(_Sbk_GroupParam_Type);
    InitSignatureStrings(pyType, GroupParam_SignatureStrings);
    SbkObjectType_SetPropertyStrings(reinterpret_cast<PyTypeObject *>(_Sbk_GroupParam_Type), Sbk_GroupParam_PropertyStrings);
    SbkNatronEngineTypes[SBK_GROUPPARAM_IDX]
        = reinterpret_cast<PyTypeObject *>(Sbk_GroupParam_TypeF());

    // Register Converter
    SbkConverter *converter = Shiboken::Conversions::createConverter(Sbk_GroupParam_TypeF(),
        GroupParam_PythonToCpp_GroupParam_PTR,
        is_GroupParam_PythonToCpp_GroupParam_PTR_Convertible,
        GroupParam_PTR_CppToPython_GroupParam);

    Shiboken::Conversions::registerConverterName(converter, "GroupParam");
    Shiboken::Conversions::registerConverterName(converter, "GroupParam*");
    Shiboken::Conversions::registerConverterName(converter, "GroupParam&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::GroupParam).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::GroupParamWrapper).name());


    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(Sbk_GroupParam_TypeF(), &Sbk_GroupParam_typeDiscovery);

    GroupParamWrapper::pysideInitQtMetaTypes();
}
