
//workaround to access protected functions
#define protected public

// default includes
#include <shiboken.h>
#include <typeresolver.h>
#include <typeinfo>
#include "natronengine_python.h"

#include "intparam_wrapper.h"

// Extra includes


// Native ---------------------------------------------------------

IntParamWrapper::~IntParamWrapper()
{
    SbkObject* wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject* Sbk_IntParamFunc_getValue(PyObject* self, PyObject* args, PyObject* kwds)
{
    ::IntParam* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::IntParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INTPARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.IntParam.getValue(): too many arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|O:getValue", &(pyArgs[0])))
        return 0;


    // Overloaded function decisor
    // 0: getValue(int)const
    if (numArgs == 0) {
        overloadId = 0; // getValue(int)const
    } else if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))) {
        overloadId = 0; // getValue(int)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_IntParamFunc_getValue_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "dimension");
            if (value && pyArgs[0]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.IntParam.getValue(): got multiple values for keyword argument 'dimension'.");
                return 0;
            } else if (value) {
                pyArgs[0] = value;
                if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0]))))
                    goto Sbk_IntParamFunc_getValue_TypeError;
            }
        }
        int cppArg0 = 0;
        if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);

        if (!PyErr_Occurred()) {
            // getValue(int)const
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            int cppResult = const_cast<const ::IntParam*>(cppSelf)->getValue(cppArg0);
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_IntParamFunc_getValue_TypeError:
        const char* overloads[] = {"int = 0", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.IntParam.getValue", overloads);
        return 0;
}

static PyMethodDef Sbk_IntParam_methods[] = {
    {"getValue", (PyCFunction)Sbk_IntParamFunc_getValue, METH_VARARGS|METH_KEYWORDS},

    {0} // Sentinel
};

} // extern "C"

static int Sbk_IntParam_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_IntParam_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_IntParam_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.IntParam",
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
    /*tp_traverse*/         Sbk_IntParam_traverse,
    /*tp_clear*/            Sbk_IntParam_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_IntParam_methods,
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
static void IntParam_PythonToCpp_IntParam_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_IntParam_Type, pyIn, cppOut);
}
static PythonToCppFunc is_IntParam_PythonToCpp_IntParam_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_IntParam_Type))
        return IntParam_PythonToCpp_IntParam_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* IntParam_PTR_CppToPython_IntParam(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::IntParam*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_IntParam_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_IntParam(PyObject* module)
{
    SbkNatronEngineTypes[SBK_INTPARAM_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_IntParam_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "IntParam", "IntParam*",
        &Sbk_IntParam_Type, &Shiboken::callCppDestructor< ::IntParam >)) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_IntParam_Type,
        IntParam_PythonToCpp_IntParam_PTR,
        is_IntParam_PythonToCpp_IntParam_PTR_Convertible,
        IntParam_PTR_CppToPython_IntParam);

    Shiboken::Conversions::registerConverterName(converter, "IntParam");
    Shiboken::Conversions::registerConverterName(converter, "IntParam*");
    Shiboken::Conversions::registerConverterName(converter, "IntParam&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::IntParam).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::IntParamWrapper).name());



}
