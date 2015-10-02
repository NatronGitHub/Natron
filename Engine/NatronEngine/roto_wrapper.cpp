
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

#include "roto_wrapper.h"

// Extra includes
#include <RotoWrapper.h>



// Target ---------------------------------------------------------

extern "C" {
static PyObject* Sbk_RotoFunc_createBezier(PyObject* self, PyObject* args)
{
    ::Roto* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Roto*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ROTO_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "createBezier", 3, 3, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return 0;


    // Overloaded function decisor
    // 0: createBezier(double,double,int)
    if (numArgs == 3
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2])))) {
        overloadId = 0; // createBezier(double,double,int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RotoFunc_createBezier_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        int cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // createBezier(double,double,int)
            // Begin code injection

            BezierCurve * cppResult = cppSelf->createBezier(cppArg0,cppArg1,cppArg2);
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], cppResult);

            // End of code injection



            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_RotoFunc_createBezier_TypeError:
        const char* overloads[] = {"float, float, int", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Roto.createBezier", overloads);
        return 0;
}

static PyObject* Sbk_RotoFunc_createEllipse(PyObject* self, PyObject* args)
{
    ::Roto* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Roto*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ROTO_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "createEllipse", 5, 5, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3]), &(pyArgs[4])))
        return 0;


    // Overloaded function decisor
    // 0: createEllipse(double,double,double,bool,int)
    if (numArgs == 5
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArgs[3])))
        && (pythonToCpp[4] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[4])))) {
        overloadId = 0; // createEllipse(double,double,double,bool,int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RotoFunc_createEllipse_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        double cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);
        bool cppArg3;
        pythonToCpp[3](pyArgs[3], &cppArg3);
        int cppArg4;
        pythonToCpp[4](pyArgs[4], &cppArg4);

        if (!PyErr_Occurred()) {
            // createEllipse(double,double,double,bool,int)
            // Begin code injection

            BezierCurve * cppResult = cppSelf->createEllipse(cppArg0,cppArg1,cppArg2,cppArg3,cppArg4);
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], cppResult);

            // End of code injection



            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_RotoFunc_createEllipse_TypeError:
        const char* overloads[] = {"float, float, float, bool, int", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Roto.createEllipse", overloads);
        return 0;
}

static PyObject* Sbk_RotoFunc_createLayer(PyObject* self)
{
    ::Roto* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Roto*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ROTO_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // createLayer()
            // Begin code injection

            Layer * cppResult = cppSelf->createLayer();
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_LAYER_IDX], cppResult);

            // End of code injection



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

static PyObject* Sbk_RotoFunc_createRectangle(PyObject* self, PyObject* args)
{
    ::Roto* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Roto*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ROTO_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "createRectangle", 4, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return 0;


    // Overloaded function decisor
    // 0: createRectangle(double,double,double,int)
    if (numArgs == 4
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[3])))) {
        overloadId = 0; // createRectangle(double,double,double,int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RotoFunc_createRectangle_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        double cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);
        int cppArg3;
        pythonToCpp[3](pyArgs[3], &cppArg3);

        if (!PyErr_Occurred()) {
            // createRectangle(double,double,double,int)
            // Begin code injection

            BezierCurve * cppResult = cppSelf->createRectangle(cppArg0,cppArg1,cppArg2,cppArg3);
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX], cppResult);

            // End of code injection



            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_RotoFunc_createRectangle_TypeError:
        const char* overloads[] = {"float, float, float, int", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Roto.createRectangle", overloads);
        return 0;
}

static PyObject* Sbk_RotoFunc_getBaseLayer(PyObject* self)
{
    ::Roto* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Roto*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ROTO_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getBaseLayer()const
            Layer * cppResult = const_cast<const ::Roto*>(cppSelf)->getBaseLayer();
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

static PyObject* Sbk_RotoFunc_getItemByName(PyObject* self, PyObject* pyArg)
{
    ::Roto* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Roto*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ROTO_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: getItemByName(std::string)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArg)))) {
        overloadId = 0; // getItemByName(std::string)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RotoFunc_getItemByName_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getItemByName(std::string)const
            ItemBase * cppResult = const_cast<const ::Roto*>(cppSelf)->getItemByName(cppArg0);
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_ITEMBASE_IDX], cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_RotoFunc_getItemByName_TypeError:
        const char* overloads[] = {"std::string", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Roto.getItemByName", overloads);
        return 0;
}

static PyMethodDef Sbk_Roto_methods[] = {
    {"createBezier", (PyCFunction)Sbk_RotoFunc_createBezier, METH_VARARGS},
    {"createEllipse", (PyCFunction)Sbk_RotoFunc_createEllipse, METH_VARARGS},
    {"createLayer", (PyCFunction)Sbk_RotoFunc_createLayer, METH_NOARGS},
    {"createRectangle", (PyCFunction)Sbk_RotoFunc_createRectangle, METH_VARARGS},
    {"getBaseLayer", (PyCFunction)Sbk_RotoFunc_getBaseLayer, METH_NOARGS},
    {"getItemByName", (PyCFunction)Sbk_RotoFunc_getItemByName, METH_O},

    {0} // Sentinel
};

} // extern "C"

static int Sbk_Roto_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_Roto_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_Roto_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.Roto",
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
    /*tp_traverse*/         Sbk_Roto_traverse,
    /*tp_clear*/            Sbk_Roto_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_Roto_methods,
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
static void Roto_PythonToCpp_Roto_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_Roto_Type, pyIn, cppOut);
}
static PythonToCppFunc is_Roto_PythonToCpp_Roto_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_Roto_Type))
        return Roto_PythonToCpp_Roto_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* Roto_PTR_CppToPython_Roto(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::Roto*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_Roto_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_Roto(PyObject* module)
{
    SbkNatronEngineTypes[SBK_ROTO_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_Roto_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "Roto", "Roto*",
        &Sbk_Roto_Type, &Shiboken::callCppDestructor< ::Roto >)) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_Roto_Type,
        Roto_PythonToCpp_Roto_PTR,
        is_Roto_PythonToCpp_Roto_PTR_Convertible,
        Roto_PTR_CppToPython_Roto);

    Shiboken::Conversions::registerConverterName(converter, "Roto");
    Shiboken::Conversions::registerConverterName(converter, "Roto*");
    Shiboken::Conversions::registerConverterName(converter, "Roto&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::Roto).name());



}
