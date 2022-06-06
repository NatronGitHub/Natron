
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
#include "itembase_wrapper.h"

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

void ItemBaseWrapper::pysideInitQtMetaTypes()
{
}

void ItemBaseWrapper::resetPyMethodCache()
{
    std::fill_n(m_PyMethodCache, sizeof(m_PyMethodCache) / sizeof(m_PyMethodCache[0]), false);
}

ItemBaseWrapper::~ItemBaseWrapper()
{
    SbkObject *wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject *Sbk_ItemBaseFunc_getLabel(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::ItemBase *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.ItemBase.getLabel";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getLabel()const
            QString cppResult = const_cast<const ::ItemBase *>(cppSelf)->getLabel();
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_ItemBaseFunc_getLocked(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::ItemBase *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.ItemBase.getLocked";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getLocked()const
            bool cppResult = const_cast<const ::ItemBase *>(cppSelf)->getLocked();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_ItemBaseFunc_getLockedRecursive(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::ItemBase *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.ItemBase.getLockedRecursive";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getLockedRecursive()const
            bool cppResult = const_cast<const ::ItemBase *>(cppSelf)->getLockedRecursive();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_ItemBaseFunc_getParam(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::ItemBase *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.ItemBase.getParam";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: ItemBase::getParam(QString)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // getParam(QString)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ItemBaseFunc_getParam_TypeError;

    // Call function/method
    {
        ::QString cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getParam(QString)const
            Param * cppResult = const_cast<const ::ItemBase *>(cppSelf)->getParam(cppArg0);
            pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_PARAM_IDX]), cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_ItemBaseFunc_getParam_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_ItemBaseFunc_getParentLayer(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::ItemBase *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.ItemBase.getParentLayer";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getParentLayer()const
            Layer * cppResult = const_cast<const ::ItemBase *>(cppSelf)->getParentLayer();
            pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_LAYER_IDX]), cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_ItemBaseFunc_getScriptName(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::ItemBase *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.ItemBase.getScriptName";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getScriptName()const
            QString cppResult = const_cast<const ::ItemBase *>(cppSelf)->getScriptName();
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_ItemBaseFunc_getVisible(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::ItemBase *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.ItemBase.getVisible";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getVisible()const
            bool cppResult = const_cast<const ::ItemBase *>(cppSelf)->getVisible();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_ItemBaseFunc_setLabel(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::ItemBase *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.ItemBase.setLabel";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: ItemBase::setLabel(QString)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // setLabel(QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ItemBaseFunc_setLabel_TypeError;

    // Call function/method
    {
        ::QString cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setLabel(QString)
            // Begin code injection
            cppSelf->setLabel(cppArg0);

            // End of code injection

        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_ItemBaseFunc_setLabel_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_ItemBaseFunc_setLocked(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::ItemBase *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.ItemBase.setLocked";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: ItemBase::setLocked(bool)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArg)))) {
        overloadId = 0; // setLocked(bool)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ItemBaseFunc_setLocked_TypeError;

    // Call function/method
    {
        bool cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setLocked(bool)
            cppSelf->setLocked(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_ItemBaseFunc_setLocked_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_ItemBaseFunc_setScriptName(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::ItemBase *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.ItemBase.setScriptName";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: ItemBase::setScriptName(QString)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // setScriptName(QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ItemBaseFunc_setScriptName_TypeError;

    // Call function/method
    {
        ::QString cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setScriptName(QString)
            // Begin code injection
            bool cppResult = cppSelf->setScriptName(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);

            // End of code injection

        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_ItemBaseFunc_setScriptName_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_ItemBaseFunc_setVisible(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::ItemBase *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.ItemBase.setVisible";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: ItemBase::setVisible(bool)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArg)))) {
        overloadId = 0; // setVisible(bool)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ItemBaseFunc_setVisible_TypeError;

    // Call function/method
    {
        bool cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setVisible(bool)
            cppSelf->setVisible(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_ItemBaseFunc_setVisible_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}


static const char *Sbk_ItemBase_PropertyStrings[] = {
    nullptr // Sentinel
};

static PyMethodDef Sbk_ItemBase_methods[] = {
    {"getLabel", reinterpret_cast<PyCFunction>(Sbk_ItemBaseFunc_getLabel), METH_NOARGS},
    {"getLocked", reinterpret_cast<PyCFunction>(Sbk_ItemBaseFunc_getLocked), METH_NOARGS},
    {"getLockedRecursive", reinterpret_cast<PyCFunction>(Sbk_ItemBaseFunc_getLockedRecursive), METH_NOARGS},
    {"getParam", reinterpret_cast<PyCFunction>(Sbk_ItemBaseFunc_getParam), METH_O},
    {"getParentLayer", reinterpret_cast<PyCFunction>(Sbk_ItemBaseFunc_getParentLayer), METH_NOARGS},
    {"getScriptName", reinterpret_cast<PyCFunction>(Sbk_ItemBaseFunc_getScriptName), METH_NOARGS},
    {"getVisible", reinterpret_cast<PyCFunction>(Sbk_ItemBaseFunc_getVisible), METH_NOARGS},
    {"setLabel", reinterpret_cast<PyCFunction>(Sbk_ItemBaseFunc_setLabel), METH_O},
    {"setLocked", reinterpret_cast<PyCFunction>(Sbk_ItemBaseFunc_setLocked), METH_O},
    {"setScriptName", reinterpret_cast<PyCFunction>(Sbk_ItemBaseFunc_setScriptName), METH_O},
    {"setVisible", reinterpret_cast<PyCFunction>(Sbk_ItemBaseFunc_setVisible), METH_O},

    {nullptr, nullptr} // Sentinel
};

static int Sbk_ItemBase_setattro(PyObject *self, PyObject *name, PyObject *value)
{
    PySide::Feature::Select(self);
    if (value && PyCallable_Check(value)) {
        auto plain_inst = reinterpret_cast< ::ItemBase *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], reinterpret_cast<SbkObject *>(self)));
        auto inst = dynamic_cast<ItemBaseWrapper *>(plain_inst);
        if (inst)
            inst->resetPyMethodCache();
    }
    return PyObject_GenericSetAttr(self, name, value);
}

} // extern "C"

static int Sbk_ItemBase_traverse(PyObject *self, visitproc visit, void *arg)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_traverse(self, visit, arg);
}
static int Sbk_ItemBase_clear(PyObject *self)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType *_Sbk_ItemBase_Type = nullptr;
static SbkObjectType *Sbk_ItemBase_TypeF(void)
{
    return _Sbk_ItemBase_Type;
}

static PyType_Slot Sbk_ItemBase_slots[] = {
    {Py_tp_base,        nullptr}, // inserted by introduceWrapperType
    {Py_tp_dealloc,     reinterpret_cast<void *>(&SbkDeallocWrapper)},
    {Py_tp_repr,        nullptr},
    {Py_tp_hash,        nullptr},
    {Py_tp_call,        nullptr},
    {Py_tp_str,         nullptr},
    {Py_tp_getattro,    nullptr},
    {Py_tp_setattro,    reinterpret_cast<void *>(Sbk_ItemBase_setattro)},
    {Py_tp_traverse,    reinterpret_cast<void *>(Sbk_ItemBase_traverse)},
    {Py_tp_clear,       reinterpret_cast<void *>(Sbk_ItemBase_clear)},
    {Py_tp_richcompare, nullptr},
    {Py_tp_iter,        nullptr},
    {Py_tp_iternext,    nullptr},
    {Py_tp_methods,     reinterpret_cast<void *>(Sbk_ItemBase_methods)},
    {Py_tp_getset,      nullptr},
    {Py_tp_init,        nullptr},
    {Py_tp_new,         reinterpret_cast<void *>(SbkDummyNew /* PYSIDE-595: Prevent replacement of "0" with base->tp_new. */)},
    {0, nullptr}
};
static PyType_Spec Sbk_ItemBase_spec = {
    "1:NatronEngine.ItemBase",
    sizeof(SbkObject),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    Sbk_ItemBase_slots
};

} //extern "C"


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void ItemBase_PythonToCpp_ItemBase_PTR(PyObject *pyIn, void *cppOut) {
    Shiboken::Conversions::pythonToCppPointer(Sbk_ItemBase_TypeF(), pyIn, cppOut);
}
static PythonToCppFunc is_ItemBase_PythonToCpp_ItemBase_PTR_Convertible(PyObject *pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, reinterpret_cast<PyTypeObject *>(Sbk_ItemBase_TypeF())))
        return ItemBase_PythonToCpp_ItemBase_PTR;
    return {};
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject *ItemBase_PTR_CppToPython_ItemBase(const void *cppIn) {
    auto pyOut = reinterpret_cast<PyObject *>(Shiboken::BindingManager::instance().retrieveWrapper(cppIn));
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    bool changedTypeName = false;
    auto tCppIn = reinterpret_cast<const ::ItemBase *>(cppIn);
    const char *typeName = typeid(*tCppIn).name();
    auto sbkType = Shiboken::ObjectType::typeForTypeName(typeName);
    if (sbkType && Shiboken::ObjectType::hasSpecialCastFunction(sbkType)) {
        typeName = typeNameOf(tCppIn);
        changedTypeName = true;
    }
    PyObject *result = Shiboken::Object::newObject(Sbk_ItemBase_TypeF(), const_cast<void *>(cppIn), false, /* exactType */ changedTypeName, typeName);
    if (changedTypeName)
        delete [] typeName;
    return result;
}

// The signatures string for the functions.
// Multiple signatures have their index "n:" in front.
static const char *ItemBase_SignatureStrings[] = {
    "NatronEngine.ItemBase.getLabel(self)->QString",
    "NatronEngine.ItemBase.getLocked(self)->bool",
    "NatronEngine.ItemBase.getLockedRecursive(self)->bool",
    "NatronEngine.ItemBase.getParam(self,name:QString)->NatronEngine.Param",
    "NatronEngine.ItemBase.getParentLayer(self)->NatronEngine.Layer",
    "NatronEngine.ItemBase.getScriptName(self)->QString",
    "NatronEngine.ItemBase.getVisible(self)->bool",
    "NatronEngine.ItemBase.setLabel(self,name:QString)",
    "NatronEngine.ItemBase.setLocked(self,locked:bool)",
    "NatronEngine.ItemBase.setScriptName(self,name:QString)->bool",
    "NatronEngine.ItemBase.setVisible(self,activated:bool)",
    nullptr}; // Sentinel

void init_ItemBase(PyObject *module)
{
    _Sbk_ItemBase_Type = Shiboken::ObjectType::introduceWrapperType(
        module,
        "ItemBase",
        "ItemBase*",
        &Sbk_ItemBase_spec,
        &Shiboken::callCppDestructor< ::ItemBase >,
        0,
        0,
        0    );
    
    auto pyType = reinterpret_cast<PyTypeObject *>(_Sbk_ItemBase_Type);
    InitSignatureStrings(pyType, ItemBase_SignatureStrings);
    SbkObjectType_SetPropertyStrings(reinterpret_cast<PyTypeObject *>(_Sbk_ItemBase_Type), Sbk_ItemBase_PropertyStrings);
    SbkNatronEngineTypes[SBK_ITEMBASE_IDX]
        = reinterpret_cast<PyTypeObject *>(Sbk_ItemBase_TypeF());

    // Register Converter
    SbkConverter *converter = Shiboken::Conversions::createConverter(Sbk_ItemBase_TypeF(),
        ItemBase_PythonToCpp_ItemBase_PTR,
        is_ItemBase_PythonToCpp_ItemBase_PTR_Convertible,
        ItemBase_PTR_CppToPython_ItemBase);

    Shiboken::Conversions::registerConverterName(converter, "ItemBase");
    Shiboken::Conversions::registerConverterName(converter, "ItemBase*");
    Shiboken::Conversions::registerConverterName(converter, "ItemBase&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::ItemBase).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::ItemBaseWrapper).name());


    ItemBaseWrapper::pysideInitQtMetaTypes();
}
