
//workaround to access protected functions
#define protected public

// default includes
#include <shiboken.h>
#include <typeresolver.h>
#include <typeinfo>
#include "natronengine_python.h"

#include "double2dtuple_wrapper.h"

// Extra includes



// Target ---------------------------------------------------------

extern "C" {
static int
Sbk_Double2DTuple_Init(PyObject* self, PyObject* args, PyObject* kwds)
{
    SbkObject* sbkSelf = reinterpret_cast<SbkObject*>(self);
    if (Shiboken::Object::isUserType(self) && !Shiboken::ObjectType::canCallConstructor(self->ob_type, Shiboken::SbkType< ::Double2DTuple >()))
        return -1;

    ::Double2DTuple* cptr = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "Double2DTuple", 0, 1, &(pyArgs[0])))
        return -1;


    // Overloaded function decisor
    // 0: Double2DTuple()
    // 1: Double2DTuple(Double2DTuple)
    if (numArgs == 0) {
        overloadId = 0; // Double2DTuple()
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppReferenceConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_DOUBLE2DTUPLE_IDX], (pyArgs[0])))) {
        overloadId = 1; // Double2DTuple(Double2DTuple)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_Double2DTuple_Init_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // Double2DTuple()
        {

            if (!PyErr_Occurred()) {
                // Double2DTuple()
                PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
                cptr = new ::Double2DTuple();
                PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            }
            break;
        }
        case 1: // Double2DTuple(const Double2DTuple & Double2DTuple)
        {
            if (!Shiboken::Object::isValid(pyArgs[0]))
                return -1;
            ::Double2DTuple cppArg0_local = ::Double2DTuple();
            ::Double2DTuple* cppArg0 = &cppArg0_local;
            if (Shiboken::Conversions::isImplicitConversion((SbkObjectType*)SbkNatronEngineTypes[SBK_DOUBLE2DTUPLE_IDX], pythonToCpp[0]))
                pythonToCpp[0](pyArgs[0], &cppArg0_local);
            else
                pythonToCpp[0](pyArgs[0], &cppArg0);


            if (!PyErr_Occurred()) {
                // Double2DTuple(Double2DTuple)
                PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
                cptr = new ::Double2DTuple(*cppArg0);
                PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            }
            break;
        }
    }

    if (PyErr_Occurred() || !Shiboken::Object::setCppPointer(sbkSelf, Shiboken::SbkType< ::Double2DTuple >(), cptr)) {
        delete cptr;
        return -1;
    }
    if (!cptr) goto Sbk_Double2DTuple_Init_TypeError;

    Shiboken::Object::setValidCpp(sbkSelf, true);
    Shiboken::BindingManager::instance().registerWrapper(sbkSelf, cptr);


    return 1;

    Sbk_Double2DTuple_Init_TypeError:
        const char* overloads[] = {"", "NatronEngine.Double2DTuple", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Double2DTuple", overloads);
        return -1;
}

static PyObject* Sbk_Double2DTuple___copy__(PyObject* self)
{
    if (!Shiboken::Object::isValid(self))
        return 0;
    ::Double2DTuple& cppSelf = *(((::Double2DTuple*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_DOUBLE2DTUPLE_IDX], (SbkObject*)self)));
    PyObject* pyResult = Shiboken::Conversions::copyToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_DOUBLE2DTUPLE_IDX], &cppSelf);
    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyMethodDef Sbk_Double2DTuple_methods[] = {

    {"__copy__", (PyCFunction)Sbk_Double2DTuple___copy__, METH_NOARGS},
    {0} // Sentinel
};

static PyObject* Sbk_Double2DTuple_get_x(PyObject* self, void*)
{
    ::Double2DTuple* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Double2DTuple*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_DOUBLE2DTUPLE_IDX], (SbkObject*)self));
    PyObject* pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppSelf->x);
    return pyOut;
}
static int Sbk_Double2DTuple_set_x(PyObject* self, PyObject* pyIn, void*)
{
    ::Double2DTuple* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Double2DTuple*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_DOUBLE2DTUPLE_IDX], (SbkObject*)self));
    if (pyIn == 0) {
        PyErr_SetString(PyExc_TypeError, "'x' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp;
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'x', 'double' or convertible type expected");
        return -1;
    }

    double& cppOut_ptr = cppSelf->x;
    pythonToCpp(pyIn, &cppOut_ptr);

    return 0;
}

static PyObject* Sbk_Double2DTuple_get_y(PyObject* self, void*)
{
    ::Double2DTuple* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Double2DTuple*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_DOUBLE2DTUPLE_IDX], (SbkObject*)self));
    PyObject* pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppSelf->y);
    return pyOut;
}
static int Sbk_Double2DTuple_set_y(PyObject* self, PyObject* pyIn, void*)
{
    ::Double2DTuple* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Double2DTuple*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_DOUBLE2DTUPLE_IDX], (SbkObject*)self));
    if (pyIn == 0) {
        PyErr_SetString(PyExc_TypeError, "'y' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp;
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'y', 'double' or convertible type expected");
        return -1;
    }

    double& cppOut_ptr = cppSelf->y;
    pythonToCpp(pyIn, &cppOut_ptr);

    return 0;
}

// Getters and Setters for Double2DTuple
static PyGetSetDef Sbk_Double2DTuple_getsetlist[] = {
    {const_cast<char*>("x"), Sbk_Double2DTuple_get_x, Sbk_Double2DTuple_set_x},
    {const_cast<char*>("y"), Sbk_Double2DTuple_get_y, Sbk_Double2DTuple_set_y},
    {0}  // Sentinel
};

} // extern "C"

static int Sbk_Double2DTuple_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_Double2DTuple_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_Double2DTuple_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.Double2DTuple",
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
    /*tp_traverse*/         Sbk_Double2DTuple_traverse,
    /*tp_clear*/            Sbk_Double2DTuple_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_Double2DTuple_methods,
    /*tp_members*/          0,
    /*tp_getset*/           Sbk_Double2DTuple_getsetlist,
    /*tp_base*/             reinterpret_cast<PyTypeObject*>(&SbkObject_Type),
    /*tp_dict*/             0,
    /*tp_descr_get*/        0,
    /*tp_descr_set*/        0,
    /*tp_dictoffset*/       0,
    /*tp_init*/             Sbk_Double2DTuple_Init,
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
static void Double2DTuple_PythonToCpp_Double2DTuple_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_Double2DTuple_Type, pyIn, cppOut);
}
static PythonToCppFunc is_Double2DTuple_PythonToCpp_Double2DTuple_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_Double2DTuple_Type))
        return Double2DTuple_PythonToCpp_Double2DTuple_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* Double2DTuple_PTR_CppToPython_Double2DTuple(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::Double2DTuple*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_Double2DTuple_Type, const_cast<void*>(cppIn), false, false, typeName);
}

// C++ to Python copy conversion.
static PyObject* Double2DTuple_COPY_CppToPython_Double2DTuple(const void* cppIn) {
    return Shiboken::Object::newObject(&Sbk_Double2DTuple_Type, new ::Double2DTuple(*((::Double2DTuple*)cppIn)), true, true);
}

// Python to C++ copy conversion.
static void Double2DTuple_PythonToCpp_Double2DTuple_COPY(PyObject* pyIn, void* cppOut) {
    *((::Double2DTuple*)cppOut) = *((::Double2DTuple*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_DOUBLE2DTUPLE_IDX], (SbkObject*)pyIn));
}
static PythonToCppFunc is_Double2DTuple_PythonToCpp_Double2DTuple_COPY_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_Double2DTuple_Type))
        return Double2DTuple_PythonToCpp_Double2DTuple_COPY;
    return 0;
}

void init_Double2DTuple(PyObject* module)
{
    SbkNatronEngineTypes[SBK_DOUBLE2DTUPLE_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_Double2DTuple_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "Double2DTuple", "Double2DTuple",
        &Sbk_Double2DTuple_Type, &Shiboken::callCppDestructor< ::Double2DTuple >)) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_Double2DTuple_Type,
        Double2DTuple_PythonToCpp_Double2DTuple_PTR,
        is_Double2DTuple_PythonToCpp_Double2DTuple_PTR_Convertible,
        Double2DTuple_PTR_CppToPython_Double2DTuple,
        Double2DTuple_COPY_CppToPython_Double2DTuple);

    Shiboken::Conversions::registerConverterName(converter, "Double2DTuple");
    Shiboken::Conversions::registerConverterName(converter, "Double2DTuple*");
    Shiboken::Conversions::registerConverterName(converter, "Double2DTuple&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::Double2DTuple).name());

    // Add Python to C++ copy (value, not pointer neither reference) conversion to type converter.
    Shiboken::Conversions::addPythonToCppValueConversion(converter,
        Double2DTuple_PythonToCpp_Double2DTuple_COPY,
        is_Double2DTuple_PythonToCpp_Double2DTuple_COPY_Convertible);


}
