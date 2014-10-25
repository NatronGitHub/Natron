
//workaround to access protected functions
#define protected public

// default includes
#include <shiboken.h>
#include <typeresolver.h>
#include <typeinfo>
#include "natronengine_python.h"

#include "param_wrapper.h"

// Extra includes


// Native ---------------------------------------------------------

ParamWrapper::~ParamWrapper()
{
    SbkObject* wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject* Sbk_ParamFunc_getNumDimensions(PyObject* self)
{
    ::Param* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Param*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getNumDimensions()const
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            int cppResult = const_cast<const ::Param*>(cppSelf)->getNumDimensions();
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ParamFunc_getScriptName(PyObject* self)
{
    ::Param* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Param*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getScriptName()const
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            std::string cppResult = const_cast<const ::Param*>(cppSelf)->getScriptName();
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyMethodDef Sbk_Param_methods[] = {
    {"getNumDimensions", (PyCFunction)Sbk_ParamFunc_getNumDimensions, METH_NOARGS},
    {"getScriptName", (PyCFunction)Sbk_ParamFunc_getScriptName, METH_NOARGS},

    {0} // Sentinel
};

} // extern "C"

static int Sbk_Param_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_Param_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_Param_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.Param",
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
    /*tp_traverse*/         Sbk_Param_traverse,
    /*tp_clear*/            Sbk_Param_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_Param_methods,
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
static void Param_PythonToCpp_Param_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_Param_Type, pyIn, cppOut);
}
static PythonToCppFunc is_Param_PythonToCpp_Param_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_Param_Type))
        return Param_PythonToCpp_Param_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* Param_PTR_CppToPython_Param(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::Param*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_Param_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_Param(PyObject* module)
{
    SbkNatronEngineTypes[SBK_PARAM_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_Param_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "Param", "Param*",
        &Sbk_Param_Type, &Shiboken::callCppDestructor< ::Param >)) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_Param_Type,
        Param_PythonToCpp_Param_PTR,
        is_Param_PythonToCpp_Param_PTR_Convertible,
        Param_PTR_CppToPython_Param);

    Shiboken::Conversions::registerConverterName(converter, "Param");
    Shiboken::Conversions::registerConverterName(converter, "Param*");
    Shiboken::Conversions::registerConverterName(converter, "Param&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::Param).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::ParamWrapper).name());



}
