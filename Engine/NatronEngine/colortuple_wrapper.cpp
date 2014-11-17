
//workaround to access protected functions
#define protected public

// default includes
#include <shiboken.h>
#include <typeresolver.h>
#include <typeinfo>
#include "natronengine_python.h"

#include "colortuple_wrapper.h"

// Extra includes



// Target ---------------------------------------------------------

extern "C" {
static int
Sbk_ColorTuple_Init(PyObject* self, PyObject* args, PyObject* kwds)
{
    SbkObject* sbkSelf = reinterpret_cast<SbkObject*>(self);
    if (Shiboken::Object::isUserType(self) && !Shiboken::ObjectType::canCallConstructor(self->ob_type, Shiboken::SbkType< ::ColorTuple >()))
        return -1;

    ::ColorTuple* cptr = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "ColorTuple", 0, 1, &(pyArgs[0])))
        return -1;


    // Overloaded function decisor
    // 0: ColorTuple()
    // 1: ColorTuple(ColorTuple)
    if (numArgs == 0) {
        overloadId = 0; // ColorTuple()
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppReferenceConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_COLORTUPLE_IDX], (pyArgs[0])))) {
        overloadId = 1; // ColorTuple(ColorTuple)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ColorTuple_Init_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // ColorTuple()
        {

            if (!PyErr_Occurred()) {
                // ColorTuple()
                PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
                cptr = new ::ColorTuple();
                PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            }
            break;
        }
        case 1: // ColorTuple(const ColorTuple & ColorTuple)
        {
            if (!Shiboken::Object::isValid(pyArgs[0]))
                return -1;
            ::ColorTuple cppArg0_local = ::ColorTuple();
            ::ColorTuple* cppArg0 = &cppArg0_local;
            if (Shiboken::Conversions::isImplicitConversion((SbkObjectType*)SbkNatronEngineTypes[SBK_COLORTUPLE_IDX], pythonToCpp[0]))
                pythonToCpp[0](pyArgs[0], &cppArg0_local);
            else
                pythonToCpp[0](pyArgs[0], &cppArg0);


            if (!PyErr_Occurred()) {
                // ColorTuple(ColorTuple)
                PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
                cptr = new ::ColorTuple(*cppArg0);
                PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            }
            break;
        }
    }

    if (PyErr_Occurred() || !Shiboken::Object::setCppPointer(sbkSelf, Shiboken::SbkType< ::ColorTuple >(), cptr)) {
        delete cptr;
        return -1;
    }
    if (!cptr) goto Sbk_ColorTuple_Init_TypeError;

    Shiboken::Object::setValidCpp(sbkSelf, true);
    Shiboken::BindingManager::instance().registerWrapper(sbkSelf, cptr);


    return 1;

    Sbk_ColorTuple_Init_TypeError:
        const char* overloads[] = {"", "NatronEngine.ColorTuple", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ColorTuple", overloads);
        return -1;
}

static PyObject* Sbk_ColorTuple___copy__(PyObject* self)
{
    if (!Shiboken::Object::isValid(self))
        return 0;
    ::ColorTuple& cppSelf = *(((::ColorTuple*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_COLORTUPLE_IDX], (SbkObject*)self)));
    PyObject* pyResult = Shiboken::Conversions::copyToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_COLORTUPLE_IDX], &cppSelf);
    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyMethodDef Sbk_ColorTuple_methods[] = {

    {"__copy__", (PyCFunction)Sbk_ColorTuple___copy__, METH_NOARGS},
    {0} // Sentinel
};

static PyObject* Sbk_ColorTuple_get_g(PyObject* self, void*)
{
    ::ColorTuple* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ColorTuple*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_COLORTUPLE_IDX], (SbkObject*)self));
    PyObject* pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppSelf->g);
    return pyOut;
}
static int Sbk_ColorTuple_set_g(PyObject* self, PyObject* pyIn, void*)
{
    ::ColorTuple* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ColorTuple*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_COLORTUPLE_IDX], (SbkObject*)self));
    if (pyIn == 0) {
        PyErr_SetString(PyExc_TypeError, "'g' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp;
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'g', 'double' or convertible type expected");
        return -1;
    }

    double& cppOut_ptr = cppSelf->g;
    pythonToCpp(pyIn, &cppOut_ptr);

    return 0;
}

static PyObject* Sbk_ColorTuple_get_r(PyObject* self, void*)
{
    ::ColorTuple* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ColorTuple*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_COLORTUPLE_IDX], (SbkObject*)self));
    PyObject* pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppSelf->r);
    return pyOut;
}
static int Sbk_ColorTuple_set_r(PyObject* self, PyObject* pyIn, void*)
{
    ::ColorTuple* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ColorTuple*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_COLORTUPLE_IDX], (SbkObject*)self));
    if (pyIn == 0) {
        PyErr_SetString(PyExc_TypeError, "'r' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp;
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'r', 'double' or convertible type expected");
        return -1;
    }

    double& cppOut_ptr = cppSelf->r;
    pythonToCpp(pyIn, &cppOut_ptr);

    return 0;
}

static PyObject* Sbk_ColorTuple_get_a(PyObject* self, void*)
{
    ::ColorTuple* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ColorTuple*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_COLORTUPLE_IDX], (SbkObject*)self));
    PyObject* pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppSelf->a);
    return pyOut;
}
static int Sbk_ColorTuple_set_a(PyObject* self, PyObject* pyIn, void*)
{
    ::ColorTuple* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ColorTuple*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_COLORTUPLE_IDX], (SbkObject*)self));
    if (pyIn == 0) {
        PyErr_SetString(PyExc_TypeError, "'a' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp;
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'a', 'double' or convertible type expected");
        return -1;
    }

    double& cppOut_ptr = cppSelf->a;
    pythonToCpp(pyIn, &cppOut_ptr);

    return 0;
}

static PyObject* Sbk_ColorTuple_get_b(PyObject* self, void*)
{
    ::ColorTuple* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ColorTuple*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_COLORTUPLE_IDX], (SbkObject*)self));
    PyObject* pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppSelf->b);
    return pyOut;
}
static int Sbk_ColorTuple_set_b(PyObject* self, PyObject* pyIn, void*)
{
    ::ColorTuple* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ColorTuple*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_COLORTUPLE_IDX], (SbkObject*)self));
    if (pyIn == 0) {
        PyErr_SetString(PyExc_TypeError, "'b' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp;
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'b', 'double' or convertible type expected");
        return -1;
    }

    double& cppOut_ptr = cppSelf->b;
    pythonToCpp(pyIn, &cppOut_ptr);

    return 0;
}

// Getters and Setters for ColorTuple
static PyGetSetDef Sbk_ColorTuple_getsetlist[] = {
    {const_cast<char*>("g"), Sbk_ColorTuple_get_g, Sbk_ColorTuple_set_g},
    {const_cast<char*>("r"), Sbk_ColorTuple_get_r, Sbk_ColorTuple_set_r},
    {const_cast<char*>("a"), Sbk_ColorTuple_get_a, Sbk_ColorTuple_set_a},
    {const_cast<char*>("b"), Sbk_ColorTuple_get_b, Sbk_ColorTuple_set_b},
    {0}  // Sentinel
};

} // extern "C"

static int Sbk_ColorTuple_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_ColorTuple_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_ColorTuple_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.ColorTuple",
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
    /*tp_flags*/            Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    /*tp_doc*/              0,
    /*tp_traverse*/         Sbk_ColorTuple_traverse,
    /*tp_clear*/            Sbk_ColorTuple_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_ColorTuple_methods,
    /*tp_members*/          0,
    /*tp_getset*/           Sbk_ColorTuple_getsetlist,
    /*tp_base*/             reinterpret_cast<PyTypeObject*>(&SbkObject_Type),
    /*tp_dict*/             0,
    /*tp_descr_get*/        0,
    /*tp_descr_set*/        0,
    /*tp_dictoffset*/       0,
    /*tp_init*/             Sbk_ColorTuple_Init,
    /*tp_alloc*/            0,
    /*tp_new*/              SbkObjectTpNew,
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
static void ColorTuple_PythonToCpp_ColorTuple_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_ColorTuple_Type, pyIn, cppOut);
}
static PythonToCppFunc is_ColorTuple_PythonToCpp_ColorTuple_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_ColorTuple_Type))
        return ColorTuple_PythonToCpp_ColorTuple_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* ColorTuple_PTR_CppToPython_ColorTuple(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::ColorTuple*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_ColorTuple_Type, const_cast<void*>(cppIn), false, false, typeName);
}

// C++ to Python copy conversion.
static PyObject* ColorTuple_COPY_CppToPython_ColorTuple(const void* cppIn) {
    return Shiboken::Object::newObject(&Sbk_ColorTuple_Type, new ::ColorTuple(*((::ColorTuple*)cppIn)), true, true);
}

// Python to C++ copy conversion.
static void ColorTuple_PythonToCpp_ColorTuple_COPY(PyObject* pyIn, void* cppOut) {
    *((::ColorTuple*)cppOut) = *((::ColorTuple*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_COLORTUPLE_IDX], (SbkObject*)pyIn));
}
static PythonToCppFunc is_ColorTuple_PythonToCpp_ColorTuple_COPY_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_ColorTuple_Type))
        return ColorTuple_PythonToCpp_ColorTuple_COPY;
    return 0;
}

void init_ColorTuple(PyObject* module)
{
    SbkNatronEngineTypes[SBK_COLORTUPLE_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_ColorTuple_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "ColorTuple", "ColorTuple",
        &Sbk_ColorTuple_Type, &Shiboken::callCppDestructor< ::ColorTuple >)) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_ColorTuple_Type,
        ColorTuple_PythonToCpp_ColorTuple_PTR,
        is_ColorTuple_PythonToCpp_ColorTuple_PTR_Convertible,
        ColorTuple_PTR_CppToPython_ColorTuple,
        ColorTuple_COPY_CppToPython_ColorTuple);

    Shiboken::Conversions::registerConverterName(converter, "ColorTuple");
    Shiboken::Conversions::registerConverterName(converter, "ColorTuple*");
    Shiboken::Conversions::registerConverterName(converter, "ColorTuple&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::ColorTuple).name());

    // Add Python to C++ copy (value, not pointer neither reference) conversion to type converter.
    Shiboken::Conversions::addPythonToCppValueConversion(converter,
        ColorTuple_PythonToCpp_ColorTuple_COPY,
        is_ColorTuple_PythonToCpp_ColorTuple_COPY_Convertible);


}
