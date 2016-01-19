
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

#include "double2dparam_wrapper.h"

// Extra includes
NATRON_NAMESPACE_USING
#include <ParameterWrapper.h>


// Native ---------------------------------------------------------

void Double2DParamWrapper::pysideInitQtMetaTypes()
{
}

Double2DParamWrapper::~Double2DParamWrapper()
{
    SbkObject* wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject* Sbk_Double2DParamFunc_get(PyObject* self, PyObject* args)
{
    Double2DParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (Double2DParamWrapper*)((::Double2DParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_DOUBLE2DPARAM_IDX], (SbkObject*)self));
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
    // 1: get(double)const
    if (numArgs == 0) {
        overloadId = 0; // get()const
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))) {
        overloadId = 1; // get(double)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_Double2DParamFunc_get_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // get() const
        {

            if (!PyErr_Occurred()) {
                // get()const
                Double2DTuple* cppResult = new Double2DTuple(const_cast<const ::Double2DParamWrapper*>(cppSelf)->get());
                pyResult = Shiboken::Object::newObject((SbkObjectType*)SbkNatronEngineTypes[SBK_DOUBLE2DTUPLE_IDX], cppResult, true, true);
            }
            break;
        }
        case 1: // get(double frame) const
        {
            double cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // get(double)const
                Double2DTuple* cppResult = new Double2DTuple(const_cast<const ::Double2DParamWrapper*>(cppSelf)->get(cppArg0));
                pyResult = Shiboken::Object::newObject((SbkObjectType*)SbkNatronEngineTypes[SBK_DOUBLE2DTUPLE_IDX], cppResult, true, true);
            }
            break;
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_Double2DParamFunc_get_TypeError:
        const char* overloads[] = {"", "float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Double2DParam.get", overloads);
        return 0;
}

static PyObject* Sbk_Double2DParamFunc_set(PyObject* self, PyObject* args)
{
    Double2DParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (Double2DParamWrapper*)((::Double2DParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_DOUBLE2DPARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "set", 2, 3, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return 0;


    // Overloaded function decisor
    // 0: set(double,double)
    // 1: set(double,double,double)
    if (numArgs >= 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
        if (numArgs == 2) {
            overloadId = 0; // set(double,double)
        } else if (numArgs == 3
            && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))) {
            overloadId = 1; // set(double,double,double)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_Double2DParamFunc_set_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // set(double x, double y)
        {
            double cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            double cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);

            if (!PyErr_Occurred()) {
                // set(double,double)
                cppSelf->set(cppArg0, cppArg1);
            }
            break;
        }
        case 1: // set(double x, double y, double frame)
        {
            double cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            double cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);
            double cppArg2;
            pythonToCpp[2](pyArgs[2], &cppArg2);

            if (!PyErr_Occurred()) {
                // set(double,double,double)
                cppSelf->set(cppArg0, cppArg1, cppArg2);
            }
            break;
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_Double2DParamFunc_set_TypeError:
        const char* overloads[] = {"float, float", "float, float, float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Double2DParam.set", overloads);
        return 0;
}

static PyObject* Sbk_Double2DParamFunc_setUsePointInteract(PyObject* self, PyObject* pyArg)
{
    Double2DParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (Double2DParamWrapper*)((::Double2DParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_DOUBLE2DPARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setUsePointInteract(bool)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArg)))) {
        overloadId = 0; // setUsePointInteract(bool)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_Double2DParamFunc_setUsePointInteract_TypeError;

    // Call function/method
    {
        bool cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setUsePointInteract(bool)
            cppSelf->setUsePointInteract(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_Double2DParamFunc_setUsePointInteract_TypeError:
        const char* overloads[] = {"bool", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Double2DParam.setUsePointInteract", overloads);
        return 0;
}

static PyMethodDef Sbk_Double2DParam_methods[] = {
    {"get", (PyCFunction)Sbk_Double2DParamFunc_get, METH_VARARGS},
    {"set", (PyCFunction)Sbk_Double2DParamFunc_set, METH_VARARGS},
    {"setUsePointInteract", (PyCFunction)Sbk_Double2DParamFunc_setUsePointInteract, METH_O},

    {0} // Sentinel
};

} // extern "C"

static int Sbk_Double2DParam_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_Double2DParam_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_Double2DParam_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.Double2DParam",
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
    /*tp_traverse*/         Sbk_Double2DParam_traverse,
    /*tp_clear*/            Sbk_Double2DParam_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_Double2DParam_methods,
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

static void* Sbk_Double2DParam_typeDiscovery(void* cptr, SbkObjectType* instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType*>(Shiboken::SbkType< ::Param >()))
        return dynamic_cast< ::Double2DParam*>(reinterpret_cast< ::Param*>(cptr));
    return 0;
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void Double2DParam_PythonToCpp_Double2DParam_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_Double2DParam_Type, pyIn, cppOut);
}
static PythonToCppFunc is_Double2DParam_PythonToCpp_Double2DParam_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_Double2DParam_Type))
        return Double2DParam_PythonToCpp_Double2DParam_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* Double2DParam_PTR_CppToPython_Double2DParam(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::Double2DParam*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_Double2DParam_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_Double2DParam(PyObject* module)
{
    SbkNatronEngineTypes[SBK_DOUBLE2DPARAM_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_Double2DParam_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "Double2DParam", "Double2DParam*",
        &Sbk_Double2DParam_Type, &Shiboken::callCppDestructor< ::Double2DParam >, (SbkObjectType*)SbkNatronEngineTypes[SBK_DOUBLEPARAM_IDX])) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_Double2DParam_Type,
        Double2DParam_PythonToCpp_Double2DParam_PTR,
        is_Double2DParam_PythonToCpp_Double2DParam_PTR_Convertible,
        Double2DParam_PTR_CppToPython_Double2DParam);

    Shiboken::Conversions::registerConverterName(converter, "Double2DParam");
    Shiboken::Conversions::registerConverterName(converter, "Double2DParam*");
    Shiboken::Conversions::registerConverterName(converter, "Double2DParam&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::Double2DParam).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::Double2DParamWrapper).name());


    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(&Sbk_Double2DParam_Type, &Sbk_Double2DParam_typeDiscovery);


    Double2DParamWrapper::pysideInitQtMetaTypes();
}
