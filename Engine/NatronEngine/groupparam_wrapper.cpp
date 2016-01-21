
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

#include "groupparam_wrapper.h"

// Extra includes
NATRON_NAMESPACE_USING
#include <ParameterWrapper.h>


// Native ---------------------------------------------------------

void GroupParamWrapper::pysideInitQtMetaTypes()
{
}

GroupParamWrapper::~GroupParamWrapper()
{
    SbkObject* wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject* Sbk_GroupParamFunc_addParam(PyObject* self, PyObject* pyArg)
{
    GroupParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (GroupParamWrapper*)((::GroupParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_GROUPPARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: addParam(const Param*)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppPointerConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_PARAM_IDX], (pyArg)))) {
        overloadId = 0; // addParam(const Param*)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_GroupParamFunc_addParam_TypeError;

    // Call function/method
    {
        if (!Shiboken::Object::isValid(pyArg))
            return 0;
        ::Param* cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // addParam(const Param*)
            cppSelf->addParam(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_GroupParamFunc_addParam_TypeError:
        const char* overloads[] = {"NatronEngine.Param", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.GroupParam.addParam", overloads);
        return 0;
}

static PyObject* Sbk_GroupParamFunc_getIsOpened(PyObject* self)
{
    GroupParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (GroupParamWrapper*)((::GroupParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_GROUPPARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getIsOpened()const
            bool cppResult = const_cast<const ::GroupParamWrapper*>(cppSelf)->getIsOpened();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_GroupParamFunc_setAsTab(PyObject* self)
{
    GroupParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (GroupParamWrapper*)((::GroupParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_GROUPPARAM_IDX], (SbkObject*)self));

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // setAsTab()
            cppSelf->setAsTab();
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;
}

static PyObject* Sbk_GroupParamFunc_setOpened(PyObject* self, PyObject* pyArg)
{
    GroupParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (GroupParamWrapper*)((::GroupParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_GROUPPARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setOpened(bool)
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
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_GroupParamFunc_setOpened_TypeError:
        const char* overloads[] = {"bool", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.GroupParam.setOpened", overloads);
        return 0;
}

static PyMethodDef Sbk_GroupParam_methods[] = {
    {"addParam", (PyCFunction)Sbk_GroupParamFunc_addParam, METH_O},
    {"getIsOpened", (PyCFunction)Sbk_GroupParamFunc_getIsOpened, METH_NOARGS},
    {"setAsTab", (PyCFunction)Sbk_GroupParamFunc_setAsTab, METH_NOARGS},
    {"setOpened", (PyCFunction)Sbk_GroupParamFunc_setOpened, METH_O},

    {0} // Sentinel
};

} // extern "C"

static int Sbk_GroupParam_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_GroupParam_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_GroupParam_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.GroupParam",
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
    /*tp_traverse*/         Sbk_GroupParam_traverse,
    /*tp_clear*/            Sbk_GroupParam_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_GroupParam_methods,
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

static void* Sbk_GroupParam_typeDiscovery(void* cptr, SbkObjectType* instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType*>(Shiboken::SbkType< ::Param >()))
        return dynamic_cast< ::GroupParam*>(reinterpret_cast< ::Param*>(cptr));
    return 0;
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void GroupParam_PythonToCpp_GroupParam_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_GroupParam_Type, pyIn, cppOut);
}
static PythonToCppFunc is_GroupParam_PythonToCpp_GroupParam_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_GroupParam_Type))
        return GroupParam_PythonToCpp_GroupParam_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* GroupParam_PTR_CppToPython_GroupParam(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::GroupParam*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_GroupParam_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_GroupParam(PyObject* module)
{
    SbkNatronEngineTypes[SBK_GROUPPARAM_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_GroupParam_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "GroupParam", "GroupParam*",
        &Sbk_GroupParam_Type, &Shiboken::callCppDestructor< ::GroupParam >, (SbkObjectType*)SbkNatronEngineTypes[SBK_PARAM_IDX])) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_GroupParam_Type,
        GroupParam_PythonToCpp_GroupParam_PTR,
        is_GroupParam_PythonToCpp_GroupParam_PTR_Convertible,
        GroupParam_PTR_CppToPython_GroupParam);

    Shiboken::Conversions::registerConverterName(converter, "GroupParam");
    Shiboken::Conversions::registerConverterName(converter, "GroupParam*");
    Shiboken::Conversions::registerConverterName(converter, "GroupParam&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::GroupParam).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::GroupParamWrapper).name());


    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(&Sbk_GroupParam_Type, &Sbk_GroupParam_typeDiscovery);


    GroupParamWrapper::pysideInitQtMetaTypes();
}
