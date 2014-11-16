
//workaround to access protected functions
#define protected public

// default includes
#include <shiboken.h>
#include <typeresolver.h>
#include <typeinfo>
#include "natronengine_python.h"

#include "int2dparam_wrapper.h"

// Extra includes
#include <ParameterWrapper.h>


// Native ---------------------------------------------------------

Int2DParamWrapper::~Int2DParamWrapper()
{
    SbkObject* wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject* Sbk_Int2DParamFunc_get(PyObject* self)
{
    ::Int2DParam* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Int2DParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INT2DPARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // get(Int2DTuple&)const
            // Begin code injection

            Int2DTuple t;
            cppSelf->get(t);
            pyResult = Shiboken::Conversions::copyToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_INT2DTUPLE_IDX], &t);
            return pyResult;

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_Int2DParamFunc_getAt(PyObject* self, PyObject* pyArg)
{
    ::Int2DParam* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Int2DParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INT2DPARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: getAt(int,Int2DTuple&)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // getAt(int,Int2DTuple&)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_Int2DParamFunc_getAt_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getAt(int,Int2DTuple&)const
            // Begin code injection

            Int2DTuple t;
            cppSelf->getAt(cppArg0,t);
            pyResult = Shiboken::Conversions::copyToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_INT2DTUPLE_IDX], &t);
            return pyResult;

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_Int2DParamFunc_getAt_TypeError:
        const char* overloads[] = {"int, NatronEngine.Int2DTuple", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Int2DParam.getAt", overloads);
        return 0;
}

static PyObject* Sbk_Int2DParamFunc_set(PyObject* self, PyObject* args)
{
    ::Int2DParam* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Int2DParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INT2DPARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "set", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: set(int,int)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
        overloadId = 0; // set(int,int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_Int2DParamFunc_set_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // set(int,int)
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            cppSelf->set(cppArg0, cppArg1);
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_Int2DParamFunc_set_TypeError:
        const char* overloads[] = {"int, int", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Int2DParam.set", overloads);
        return 0;
}

static PyObject* Sbk_Int2DParamFunc_setAt(PyObject* self, PyObject* args)
{
    ::Int2DParam* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Int2DParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INT2DPARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "setAt", 3, 3, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return 0;


    // Overloaded function decisor
    // 0: setAt(int,int,int)
    if (numArgs == 3
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2])))) {
        overloadId = 0; // setAt(int,int,int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_Int2DParamFunc_setAt_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        int cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // setAt(int,int,int)
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            cppSelf->setAt(cppArg0, cppArg1, cppArg2);
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_Int2DParamFunc_setAt_TypeError:
        const char* overloads[] = {"int, int, int", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Int2DParam.setAt", overloads);
        return 0;
}

static PyMethodDef Sbk_Int2DParam_methods[] = {
    {"get", (PyCFunction)Sbk_Int2DParamFunc_get, METH_NOARGS},
    {"getAt", (PyCFunction)Sbk_Int2DParamFunc_getAt, METH_O},
    {"set", (PyCFunction)Sbk_Int2DParamFunc_set, METH_VARARGS},
    {"setAt", (PyCFunction)Sbk_Int2DParamFunc_setAt, METH_VARARGS},

    {0} // Sentinel
};

} // extern "C"

static int Sbk_Int2DParam_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_Int2DParam_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_Int2DParam_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.Int2DParam",
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
    /*tp_traverse*/         Sbk_Int2DParam_traverse,
    /*tp_clear*/            Sbk_Int2DParam_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_Int2DParam_methods,
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

static void* Sbk_Int2DParam_typeDiscovery(void* cptr, SbkObjectType* instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType*>(Shiboken::SbkType< ::Param >()))
        return dynamic_cast< ::Int2DParam*>(reinterpret_cast< ::Param*>(cptr));
    return 0;
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void Int2DParam_PythonToCpp_Int2DParam_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_Int2DParam_Type, pyIn, cppOut);
}
static PythonToCppFunc is_Int2DParam_PythonToCpp_Int2DParam_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_Int2DParam_Type))
        return Int2DParam_PythonToCpp_Int2DParam_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* Int2DParam_PTR_CppToPython_Int2DParam(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::Int2DParam*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_Int2DParam_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_Int2DParam(PyObject* module)
{
    SbkNatronEngineTypes[SBK_INT2DPARAM_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_Int2DParam_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "Int2DParam", "Int2DParam*",
        &Sbk_Int2DParam_Type, &Shiboken::callCppDestructor< ::Int2DParam >, (SbkObjectType*)SbkNatronEngineTypes[SBK_INTPARAM_IDX])) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_Int2DParam_Type,
        Int2DParam_PythonToCpp_Int2DParam_PTR,
        is_Int2DParam_PythonToCpp_Int2DParam_PTR_Convertible,
        Int2DParam_PTR_CppToPython_Int2DParam);

    Shiboken::Conversions::registerConverterName(converter, "Int2DParam");
    Shiboken::Conversions::registerConverterName(converter, "Int2DParam*");
    Shiboken::Conversions::registerConverterName(converter, "Int2DParam&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::Int2DParam).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::Int2DParamWrapper).name());


    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(&Sbk_Int2DParam_Type, &Sbk_Int2DParam_typeDiscovery);


}
