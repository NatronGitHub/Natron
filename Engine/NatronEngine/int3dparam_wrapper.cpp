
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

#include "int3dparam_wrapper.h"

// Extra includes
#include <ParameterWrapper.h>


// Native ---------------------------------------------------------

void Int3DParamWrapper::pysideInitQtMetaTypes()
{
}

Int3DParamWrapper::~Int3DParamWrapper()
{
    SbkObject* wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject* Sbk_Int3DParamFunc_get(PyObject* self, PyObject* args)
{
    Int3DParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (Int3DParamWrapper*)((::Int3DParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INT3DPARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "get", 0, 1, &(pyArgs[0])))
        return 0;


    // Overloaded function decisor
    // 0: get()const
    // 1: get(int)const
    if (numArgs == 0) {
        overloadId = 0; // get()const
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))) {
        overloadId = 1; // get(int)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_Int3DParamFunc_get_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // get() const
        {

            if (!PyErr_Occurred()) {
                // get()const
                Int3DTuple* cppResult = new Int3DTuple(const_cast<const ::Int3DParamWrapper*>(cppSelf)->get());
                pyResult = Shiboken::Object::newObject((SbkObjectType*)SbkNatronEngineTypes[SBK_INT3DTUPLE_IDX], cppResult, true, true);
            }
            break;
        }
        case 1: // get(int frame) const
        {
            int cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // get(int)const
                Int3DTuple* cppResult = new Int3DTuple(const_cast<const ::Int3DParamWrapper*>(cppSelf)->get(cppArg0));
                pyResult = Shiboken::Object::newObject((SbkObjectType*)SbkNatronEngineTypes[SBK_INT3DTUPLE_IDX], cppResult, true, true);
            }
            break;
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_Int3DParamFunc_get_TypeError:
        const char* overloads[] = {"", "int", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Int3DParam.get", overloads);
        return 0;
}

static PyObject* Sbk_Int3DParamFunc_set(PyObject* self, PyObject* args)
{
    Int3DParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (Int3DParamWrapper*)((::Int3DParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INT3DPARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "set", 3, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return 0;


    // Overloaded function decisor
    // 0: set(int,int,int)
    // 1: set(int,int,int,int)
    if (numArgs >= 3
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2])))) {
        if (numArgs == 3) {
            overloadId = 0; // set(int,int,int)
        } else if (numArgs == 4
            && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[3])))) {
            overloadId = 1; // set(int,int,int,int)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_Int3DParamFunc_set_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // set(int x, int y, int z)
        {
            int cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            int cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);
            int cppArg2;
            pythonToCpp[2](pyArgs[2], &cppArg2);

            if (!PyErr_Occurred()) {
                // set(int,int,int)
                // Begin code injection

                cppSelf->set(cppArg0,cppArg1,cppArg2);

                // End of code injection


            }
            break;
        }
        case 1: // set(int x, int y, int z, int frame)
        {
            int cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            int cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);
            int cppArg2;
            pythonToCpp[2](pyArgs[2], &cppArg2);
            int cppArg3;
            pythonToCpp[3](pyArgs[3], &cppArg3);

            if (!PyErr_Occurred()) {
                // set(int,int,int,int)
                // Begin code injection

                cppSelf->set(cppArg0,cppArg1,cppArg2,cppArg3);

                // End of code injection


            }
            break;
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_Int3DParamFunc_set_TypeError:
        const char* overloads[] = {"int, int, int", "int, int, int, int", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Int3DParam.set", overloads);
        return 0;
}

static PyMethodDef Sbk_Int3DParam_methods[] = {
    {"get", (PyCFunction)Sbk_Int3DParamFunc_get, METH_VARARGS},
    {"set", (PyCFunction)Sbk_Int3DParamFunc_set, METH_VARARGS},

    {0} // Sentinel
};

} // extern "C"

static int Sbk_Int3DParam_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_Int3DParam_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_Int3DParam_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.Int3DParam",
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
    /*tp_traverse*/         Sbk_Int3DParam_traverse,
    /*tp_clear*/            Sbk_Int3DParam_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_Int3DParam_methods,
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

static void* Sbk_Int3DParam_typeDiscovery(void* cptr, SbkObjectType* instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType*>(Shiboken::SbkType< ::Param >()))
        return dynamic_cast< ::Int3DParam*>(reinterpret_cast< ::Param*>(cptr));
    return 0;
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void Int3DParam_PythonToCpp_Int3DParam_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_Int3DParam_Type, pyIn, cppOut);
}
static PythonToCppFunc is_Int3DParam_PythonToCpp_Int3DParam_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_Int3DParam_Type))
        return Int3DParam_PythonToCpp_Int3DParam_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* Int3DParam_PTR_CppToPython_Int3DParam(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::Int3DParam*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_Int3DParam_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_Int3DParam(PyObject* module)
{
    SbkNatronEngineTypes[SBK_INT3DPARAM_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_Int3DParam_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "Int3DParam", "Int3DParam*",
        &Sbk_Int3DParam_Type, &Shiboken::callCppDestructor< ::Int3DParam >, (SbkObjectType*)SbkNatronEngineTypes[SBK_INT2DPARAM_IDX])) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_Int3DParam_Type,
        Int3DParam_PythonToCpp_Int3DParam_PTR,
        is_Int3DParam_PythonToCpp_Int3DParam_PTR_Convertible,
        Int3DParam_PTR_CppToPython_Int3DParam);

    Shiboken::Conversions::registerConverterName(converter, "Int3DParam");
    Shiboken::Conversions::registerConverterName(converter, "Int3DParam*");
    Shiboken::Conversions::registerConverterName(converter, "Int3DParam&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::Int3DParam).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::Int3DParamWrapper).name());


    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(&Sbk_Int3DParam_Type, &Sbk_Int3DParam_typeDiscovery);


    Int3DParamWrapper::pysideInitQtMetaTypes();
}
