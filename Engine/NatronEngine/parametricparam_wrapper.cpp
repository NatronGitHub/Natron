
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

#include "parametricparam_wrapper.h"

// Extra includes
NATRON_NAMESPACE_USING
#include <ParameterWrapper.h>


// Native ---------------------------------------------------------

void ParametricParamWrapper::pysideInitQtMetaTypes()
{
}

ParametricParamWrapper::~ParametricParamWrapper()
{
    SbkObject* wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject* Sbk_ParametricParamFunc_getCurveColor(PyObject* self, PyObject* pyArg)
{
    ParametricParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ParametricParamWrapper*)((::ParametricParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAMETRICPARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: getCurveColor(int,ColorTuple&)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // getCurveColor(int,ColorTuple&)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ParametricParamFunc_getCurveColor_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getCurveColor(int,ColorTuple&)const
            // Begin code injection

            ColorTuple t;
            cppSelf->getCurveColor(cppArg0,t);
            pyResult = Shiboken::Conversions::copyToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_COLORTUPLE_IDX], &t);
            return pyResult;

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ParametricParamFunc_getCurveColor_TypeError:
        const char* overloads[] = {"int, NatronEngine.ColorTuple", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ParametricParam.getCurveColor", overloads);
        return 0;
}

static PyObject* Sbk_ParametricParamFunc_getNControlPoints(PyObject* self, PyObject* pyArg)
{
    ParametricParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ParametricParamWrapper*)((::ParametricParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAMETRICPARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: getNControlPoints(int)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // getNControlPoints(int)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ParametricParamFunc_getNControlPoints_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getNControlPoints(int)const
            int cppResult = const_cast<const ::ParametricParamWrapper*>(cppSelf)->getNControlPoints(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ParametricParamFunc_getNControlPoints_TypeError:
        const char* overloads[] = {"int", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ParametricParam.getNControlPoints", overloads);
        return 0;
}

static PyObject* Sbk_ParametricParamFunc_getNthControlPoint(PyObject* self, PyObject* args)
{
    ParametricParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ParametricParamWrapper*)((::ParametricParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAMETRICPARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "getNthControlPoint", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: getNthControlPoint(int,int,double*,double*,double*,double*)const
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
        overloadId = 0; // getNthControlPoint(int,int,double*,double*,double*,double*)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ParametricParamFunc_getNthControlPoint_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // getNthControlPoint(int,int,double*,double*,double*,double*)const
            // Begin code injection

            double key,value,left,right;
            NATRON_NAMESPACE::StatusEnum cppResult = cppSelf->getNthControlPoint(cppArg0, cppArg1,&key,&value, &left, &right);
            pyResult = PyTuple_New(5);
            PyTuple_SET_ITEM(pyResult, 0, Shiboken::Conversions::copyToPython(SBK_CONVERTER(SbkNatronEngineTypes[SBK_NATRON_NAMESPACE_STATUSENUM_IDX]), &cppResult));
            PyTuple_SET_ITEM(pyResult, 1, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &key));
            PyTuple_SET_ITEM(pyResult, 2, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &value));
            PyTuple_SET_ITEM(pyResult, 3, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &left));
            PyTuple_SET_ITEM(pyResult, 4, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &right));
            return pyResult;

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ParametricParamFunc_getNthControlPoint_TypeError:
        const char* overloads[] = {"int, int, PySide.QtCore.double, PySide.QtCore.double, PySide.QtCore.double, PySide.QtCore.double", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ParametricParam.getNthControlPoint", overloads);
        return 0;
}

static PyObject* Sbk_ParametricParamFunc_getValue(PyObject* self, PyObject* args)
{
    ParametricParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ParametricParamWrapper*)((::ParametricParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAMETRICPARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "getValue", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: getValue(int,double)const
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
        overloadId = 0; // getValue(int,double)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ParametricParamFunc_getValue_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // getValue(int,double)const
            double cppResult = const_cast<const ::ParametricParamWrapper*>(cppSelf)->getValue(cppArg0, cppArg1);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ParametricParamFunc_getValue_TypeError:
        const char* overloads[] = {"int, float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ParametricParam.getValue", overloads);
        return 0;
}

static PyObject* Sbk_ParametricParamFunc_setCurveColor(PyObject* self, PyObject* args)
{
    ParametricParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ParametricParamWrapper*)((::ParametricParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAMETRICPARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "setCurveColor", 4, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return 0;


    // Overloaded function decisor
    // 0: setCurveColor(int,double,double,double)
    if (numArgs == 4
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))) {
        overloadId = 0; // setCurveColor(int,double,double,double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ParametricParamFunc_setCurveColor_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        double cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);
        double cppArg3;
        pythonToCpp[3](pyArgs[3], &cppArg3);

        if (!PyErr_Occurred()) {
            // setCurveColor(int,double,double,double)
            cppSelf->setCurveColor(cppArg0, cppArg1, cppArg2, cppArg3);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_ParametricParamFunc_setCurveColor_TypeError:
        const char* overloads[] = {"int, float, float, float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ParametricParam.setCurveColor", overloads);
        return 0;
}

static PyMethodDef Sbk_ParametricParam_methods[] = {
    {"getCurveColor", (PyCFunction)Sbk_ParametricParamFunc_getCurveColor, METH_O},
    {"getNControlPoints", (PyCFunction)Sbk_ParametricParamFunc_getNControlPoints, METH_O},
    {"getNthControlPoint", (PyCFunction)Sbk_ParametricParamFunc_getNthControlPoint, METH_VARARGS},
    {"getValue", (PyCFunction)Sbk_ParametricParamFunc_getValue, METH_VARARGS},
    {"setCurveColor", (PyCFunction)Sbk_ParametricParamFunc_setCurveColor, METH_VARARGS},

    {0} // Sentinel
};

} // extern "C"

static int Sbk_ParametricParam_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_ParametricParam_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_ParametricParam_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.ParametricParam",
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
    /*tp_traverse*/         Sbk_ParametricParam_traverse,
    /*tp_clear*/            Sbk_ParametricParam_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_ParametricParam_methods,
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

static void* Sbk_ParametricParam_typeDiscovery(void* cptr, SbkObjectType* instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType*>(Shiboken::SbkType< ::Param >()))
        return dynamic_cast< ::ParametricParam*>(reinterpret_cast< ::Param*>(cptr));
    return 0;
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void ParametricParam_PythonToCpp_ParametricParam_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_ParametricParam_Type, pyIn, cppOut);
}
static PythonToCppFunc is_ParametricParam_PythonToCpp_ParametricParam_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_ParametricParam_Type))
        return ParametricParam_PythonToCpp_ParametricParam_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* ParametricParam_PTR_CppToPython_ParametricParam(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::ParametricParam*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_ParametricParam_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_ParametricParam(PyObject* module)
{
    SbkNatronEngineTypes[SBK_PARAMETRICPARAM_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_ParametricParam_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "ParametricParam", "ParametricParam*",
        &Sbk_ParametricParam_Type, &Shiboken::callCppDestructor< ::ParametricParam >, (SbkObjectType*)SbkNatronEngineTypes[SBK_PARAM_IDX])) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_ParametricParam_Type,
        ParametricParam_PythonToCpp_ParametricParam_PTR,
        is_ParametricParam_PythonToCpp_ParametricParam_PTR_Convertible,
        ParametricParam_PTR_CppToPython_ParametricParam);

    Shiboken::Conversions::registerConverterName(converter, "ParametricParam");
    Shiboken::Conversions::registerConverterName(converter, "ParametricParam*");
    Shiboken::Conversions::registerConverterName(converter, "ParametricParam&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::ParametricParam).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::ParametricParamWrapper).name());


    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(&Sbk_ParametricParam_Type, &Sbk_ParametricParam_typeDiscovery);


    ParametricParamWrapper::pysideInitQtMetaTypes();
}
