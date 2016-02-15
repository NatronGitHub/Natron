
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

#include "buttonparam_wrapper.h"

// Extra includes
NATRON_NAMESPACE_USING
#include <PyParameter.h>


// Native ---------------------------------------------------------

void ButtonParamWrapper::pysideInitQtMetaTypes()
{
}

ButtonParamWrapper::~ButtonParamWrapper()
{
    SbkObject* wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject* Sbk_ButtonParamFunc_setIconFilePath(PyObject* self, PyObject* pyArg)
{
    ButtonParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ButtonParamWrapper*)((::ButtonParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BUTTONPARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setIconFilePath(std::string)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArg)))) {
        overloadId = 0; // setIconFilePath(std::string)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ButtonParamFunc_setIconFilePath_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setIconFilePath(std::string)
            cppSelf->setIconFilePath(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_ButtonParamFunc_setIconFilePath_TypeError:
        const char* overloads[] = {"std::string", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ButtonParam.setIconFilePath", overloads);
        return 0;
}

static PyObject* Sbk_ButtonParamFunc_trigger(PyObject* self)
{
    ButtonParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ButtonParamWrapper*)((::ButtonParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_BUTTONPARAM_IDX], (SbkObject*)self));

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // trigger()
            cppSelf->trigger();
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;
}

static PyMethodDef Sbk_ButtonParam_methods[] = {
    {"setIconFilePath", (PyCFunction)Sbk_ButtonParamFunc_setIconFilePath, METH_O},
    {"trigger", (PyCFunction)Sbk_ButtonParamFunc_trigger, METH_NOARGS},

    {0} // Sentinel
};

} // extern "C"

static int Sbk_ButtonParam_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_ButtonParam_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_ButtonParam_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.ButtonParam",
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
    /*tp_traverse*/         Sbk_ButtonParam_traverse,
    /*tp_clear*/            Sbk_ButtonParam_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_ButtonParam_methods,
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

static void* Sbk_ButtonParam_typeDiscovery(void* cptr, SbkObjectType* instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType*>(Shiboken::SbkType< ::Param >()))
        return dynamic_cast< ::ButtonParam*>(reinterpret_cast< ::Param*>(cptr));
    return 0;
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void ButtonParam_PythonToCpp_ButtonParam_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_ButtonParam_Type, pyIn, cppOut);
}
static PythonToCppFunc is_ButtonParam_PythonToCpp_ButtonParam_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_ButtonParam_Type))
        return ButtonParam_PythonToCpp_ButtonParam_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* ButtonParam_PTR_CppToPython_ButtonParam(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::ButtonParam*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_ButtonParam_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_ButtonParam(PyObject* module)
{
    SbkNatronEngineTypes[SBK_BUTTONPARAM_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_ButtonParam_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "ButtonParam", "ButtonParam*",
        &Sbk_ButtonParam_Type, &Shiboken::callCppDestructor< ::ButtonParam >, (SbkObjectType*)SbkNatronEngineTypes[SBK_PARAM_IDX])) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_ButtonParam_Type,
        ButtonParam_PythonToCpp_ButtonParam_PTR,
        is_ButtonParam_PythonToCpp_ButtonParam_PTR_Convertible,
        ButtonParam_PTR_CppToPython_ButtonParam);

    Shiboken::Conversions::registerConverterName(converter, "ButtonParam");
    Shiboken::Conversions::registerConverterName(converter, "ButtonParam*");
    Shiboken::Conversions::registerConverterName(converter, "ButtonParam&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::ButtonParam).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::ButtonParamWrapper).name());


    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(&Sbk_ButtonParam_Type, &Sbk_ButtonParam_typeDiscovery);


    ButtonParamWrapper::pysideInitQtMetaTypes();
}
