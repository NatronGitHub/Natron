
// default includes
#include "Global/Macros.h"
CLANG_DIAG_OFF(mismatched-tags)
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_OFF(missing-field-initializers)
GCC_DIAG_OFF(missing-declarations)
#include <shiboken.h> // produces many warnings
#include <pysidesignal.h>
#include <pysideproperty.h>
#include <pyside.h>
#include <typeresolver.h>
#include <typeinfo>
#include "natronengine_python.h"

#include "itembase_wrapper.h"

// Extra includes
NATRON_NAMESPACE_USING
#include <ParameterWrapper.h>
#include <RotoWrapper.h>


// Native ---------------------------------------------------------

void ItemBaseWrapper::pysideInitQtMetaTypes()
{
}

ItemBaseWrapper::~ItemBaseWrapper()
{
    SbkObject* wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject* Sbk_ItemBaseFunc_getLabel(PyObject* self)
{
    ::ItemBase* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ItemBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getLabel()const
            std::string cppResult = const_cast<const ::ItemBase*>(cppSelf)->getLabel();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ItemBaseFunc_getLocked(PyObject* self)
{
    ::ItemBase* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ItemBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getLocked()const
            bool cppResult = const_cast<const ::ItemBase*>(cppSelf)->getLocked();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ItemBaseFunc_getLockedRecursive(PyObject* self)
{
    ::ItemBase* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ItemBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getLockedRecursive()const
            bool cppResult = const_cast<const ::ItemBase*>(cppSelf)->getLockedRecursive();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ItemBaseFunc_getParam(PyObject* self, PyObject* pyArg)
{
    ::ItemBase* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ItemBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: getParam(std::string)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArg)))) {
        overloadId = 0; // getParam(std::string)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ItemBaseFunc_getParam_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getParam(std::string)const
            Param * cppResult = const_cast<const ::ItemBase*>(cppSelf)->getParam(cppArg0);
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_PARAM_IDX], cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ItemBaseFunc_getParam_TypeError:
        const char* overloads[] = {"std::string", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ItemBase.getParam", overloads);
        return 0;
}

static PyObject* Sbk_ItemBaseFunc_getParentLayer(PyObject* self)
{
    ::ItemBase* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ItemBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getParentLayer()const
            Layer * cppResult = const_cast<const ::ItemBase*>(cppSelf)->getParentLayer();
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_LAYER_IDX], cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ItemBaseFunc_getScriptName(PyObject* self)
{
    ::ItemBase* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ItemBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getScriptName()const
            std::string cppResult = const_cast<const ::ItemBase*>(cppSelf)->getScriptName();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ItemBaseFunc_getVisible(PyObject* self)
{
    ::ItemBase* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ItemBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getVisible()const
            bool cppResult = const_cast<const ::ItemBase*>(cppSelf)->getVisible();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ItemBaseFunc_setLabel(PyObject* self, PyObject* pyArg)
{
    ::ItemBase* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ItemBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setLabel(std::string)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArg)))) {
        overloadId = 0; // setLabel(std::string)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ItemBaseFunc_setLabel_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setLabel(std::string)
            // Begin code injection

            cppSelf->setLabel(cppArg0);

            // End of code injection


        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_ItemBaseFunc_setLabel_TypeError:
        const char* overloads[] = {"std::string", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ItemBase.setLabel", overloads);
        return 0;
}

static PyObject* Sbk_ItemBaseFunc_setLocked(PyObject* self, PyObject* pyArg)
{
    ::ItemBase* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ItemBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setLocked(bool)
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
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_ItemBaseFunc_setLocked_TypeError:
        const char* overloads[] = {"bool", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ItemBase.setLocked", overloads);
        return 0;
}

static PyObject* Sbk_ItemBaseFunc_setScriptName(PyObject* self, PyObject* pyArg)
{
    ::ItemBase* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ItemBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setScriptName(std::string)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArg)))) {
        overloadId = 0; // setScriptName(std::string)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ItemBaseFunc_setScriptName_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setScriptName(std::string)
            // Begin code injection

            bool cppResult = cppSelf->setScriptName(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ItemBaseFunc_setScriptName_TypeError:
        const char* overloads[] = {"std::string", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ItemBase.setScriptName", overloads);
        return 0;
}

static PyObject* Sbk_ItemBaseFunc_setVisible(PyObject* self, PyObject* pyArg)
{
    ::ItemBase* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ItemBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setVisible(bool)
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
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_ItemBaseFunc_setVisible_TypeError:
        const char* overloads[] = {"bool", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ItemBase.setVisible", overloads);
        return 0;
}

static PyMethodDef Sbk_ItemBase_methods[] = {
    {"getLabel", (PyCFunction)Sbk_ItemBaseFunc_getLabel, METH_NOARGS},
    {"getLocked", (PyCFunction)Sbk_ItemBaseFunc_getLocked, METH_NOARGS},
    {"getLockedRecursive", (PyCFunction)Sbk_ItemBaseFunc_getLockedRecursive, METH_NOARGS},
    {"getParam", (PyCFunction)Sbk_ItemBaseFunc_getParam, METH_O},
    {"getParentLayer", (PyCFunction)Sbk_ItemBaseFunc_getParentLayer, METH_NOARGS},
    {"getScriptName", (PyCFunction)Sbk_ItemBaseFunc_getScriptName, METH_NOARGS},
    {"getVisible", (PyCFunction)Sbk_ItemBaseFunc_getVisible, METH_NOARGS},
    {"setLabel", (PyCFunction)Sbk_ItemBaseFunc_setLabel, METH_O},
    {"setLocked", (PyCFunction)Sbk_ItemBaseFunc_setLocked, METH_O},
    {"setScriptName", (PyCFunction)Sbk_ItemBaseFunc_setScriptName, METH_O},
    {"setVisible", (PyCFunction)Sbk_ItemBaseFunc_setVisible, METH_O},

    {0} // Sentinel
};

} // extern "C"

static int Sbk_ItemBase_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_ItemBase_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_ItemBase_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.ItemBase",
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
    /*tp_traverse*/         Sbk_ItemBase_traverse,
    /*tp_clear*/            Sbk_ItemBase_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_ItemBase_methods,
    /*tp_members*/          0,
    /*tp_getset*/           0,
    /*tp_base*/             reinterpret_cast<PyTypeObject*>(&SbkObject_Type),
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


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void ItemBase_PythonToCpp_ItemBase_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_ItemBase_Type, pyIn, cppOut);
}
static PythonToCppFunc is_ItemBase_PythonToCpp_ItemBase_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_ItemBase_Type))
        return ItemBase_PythonToCpp_ItemBase_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* ItemBase_PTR_CppToPython_ItemBase(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::ItemBase*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_ItemBase_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_ItemBase(PyObject* module)
{
    SbkNatronEngineTypes[SBK_ITEMBASE_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_ItemBase_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "ItemBase", "ItemBase*",
        &Sbk_ItemBase_Type, &Shiboken::callCppDestructor< ::ItemBase >)) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_ItemBase_Type,
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
