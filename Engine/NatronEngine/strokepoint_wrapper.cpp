
// default includes
#include "Global/Macros.h"
CLANG_DIAG_OFF(mismatched-tags)
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_OFF(missing-field-initializers)
GCC_DIAG_OFF(missing-declarations)
GCC_DIAG_OFF(uninitialized)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <shiboken.h> // produces many warnings
#include <pysidesignal.h>
#include <pysideproperty.h>
#include <pyside.h>
#include <typeresolver.h>
#include <typeinfo>
#include "natronengine_python.h"

#include "strokepoint_wrapper.h"

// Extra includes
NATRON_NAMESPACE_USING NATRON_PYTHON_NAMESPACE_USING



// Target ---------------------------------------------------------

extern "C" {
static int
Sbk_StrokePoint_Init(PyObject* self, PyObject* args, PyObject* kwds)
{
    SbkObject* sbkSelf = reinterpret_cast<SbkObject*>(self);
    if (Shiboken::Object::isUserType(self) && !Shiboken::ObjectType::canCallConstructor(self->ob_type, Shiboken::SbkType< ::StrokePoint >()))
        return -1;

    ::StrokePoint* cptr = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // StrokePoint()
            cptr = new ::StrokePoint();
        }
    }

    if (PyErr_Occurred() || !Shiboken::Object::setCppPointer(sbkSelf, Shiboken::SbkType< ::StrokePoint >(), cptr)) {
        delete cptr;
        return -1;
    }
    Shiboken::Object::setValidCpp(sbkSelf, true);
    Shiboken::BindingManager::instance().registerWrapper(sbkSelf, cptr);


    return 1;
}

static PyMethodDef Sbk_StrokePoint_methods[] = {

    {0} // Sentinel
};

static PyObject* Sbk_StrokePoint_get_x(PyObject* self, void*)
{
    ::StrokePoint* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::StrokePoint*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_STROKEPOINT_IDX], (SbkObject*)self));
    PyObject* pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppSelf->x);
    return pyOut;
}
static int Sbk_StrokePoint_set_x(PyObject* self, PyObject* pyIn, void*)
{
    ::StrokePoint* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::StrokePoint*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_STROKEPOINT_IDX], (SbkObject*)self));
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

static PyObject* Sbk_StrokePoint_get_y(PyObject* self, void*)
{
    ::StrokePoint* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::StrokePoint*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_STROKEPOINT_IDX], (SbkObject*)self));
    PyObject* pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppSelf->y);
    return pyOut;
}
static int Sbk_StrokePoint_set_y(PyObject* self, PyObject* pyIn, void*)
{
    ::StrokePoint* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::StrokePoint*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_STROKEPOINT_IDX], (SbkObject*)self));
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

static PyObject* Sbk_StrokePoint_get_timestamp(PyObject* self, void*)
{
    ::StrokePoint* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::StrokePoint*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_STROKEPOINT_IDX], (SbkObject*)self));
    PyObject* pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppSelf->timestamp);
    return pyOut;
}
static int Sbk_StrokePoint_set_timestamp(PyObject* self, PyObject* pyIn, void*)
{
    ::StrokePoint* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::StrokePoint*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_STROKEPOINT_IDX], (SbkObject*)self));
    if (pyIn == 0) {
        PyErr_SetString(PyExc_TypeError, "'timestamp' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp;
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'timestamp', 'double' or convertible type expected");
        return -1;
    }

    double& cppOut_ptr = cppSelf->timestamp;
    pythonToCpp(pyIn, &cppOut_ptr);

    return 0;
}

static PyObject* Sbk_StrokePoint_get_pressure(PyObject* self, void*)
{
    ::StrokePoint* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::StrokePoint*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_STROKEPOINT_IDX], (SbkObject*)self));
    PyObject* pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppSelf->pressure);
    return pyOut;
}
static int Sbk_StrokePoint_set_pressure(PyObject* self, PyObject* pyIn, void*)
{
    ::StrokePoint* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::StrokePoint*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_STROKEPOINT_IDX], (SbkObject*)self));
    if (pyIn == 0) {
        PyErr_SetString(PyExc_TypeError, "'pressure' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp;
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'pressure', 'double' or convertible type expected");
        return -1;
    }

    double& cppOut_ptr = cppSelf->pressure;
    pythonToCpp(pyIn, &cppOut_ptr);

    return 0;
}

// Getters and Setters for StrokePoint
static PyGetSetDef Sbk_StrokePoint_getsetlist[] = {
    {const_cast<char*>("x"), Sbk_StrokePoint_get_x, Sbk_StrokePoint_set_x},
    {const_cast<char*>("y"), Sbk_StrokePoint_get_y, Sbk_StrokePoint_set_y},
    {const_cast<char*>("timestamp"), Sbk_StrokePoint_get_timestamp, Sbk_StrokePoint_set_timestamp},
    {const_cast<char*>("pressure"), Sbk_StrokePoint_get_pressure, Sbk_StrokePoint_set_pressure},
    {0}  // Sentinel
};

} // extern "C"

static int Sbk_StrokePoint_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_StrokePoint_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_StrokePoint_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.StrokePoint",
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
    /*tp_traverse*/         Sbk_StrokePoint_traverse,
    /*tp_clear*/            Sbk_StrokePoint_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_StrokePoint_methods,
    /*tp_members*/          0,
    /*tp_getset*/           Sbk_StrokePoint_getsetlist,
    /*tp_base*/             reinterpret_cast<PyTypeObject*>(&SbkObject_Type),
    /*tp_dict*/             0,
    /*tp_descr_get*/        0,
    /*tp_descr_set*/        0,
    /*tp_dictoffset*/       0,
    /*tp_init*/             Sbk_StrokePoint_Init,
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
static void StrokePoint_PythonToCpp_StrokePoint_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_StrokePoint_Type, pyIn, cppOut);
}
static PythonToCppFunc is_StrokePoint_PythonToCpp_StrokePoint_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_StrokePoint_Type))
        return StrokePoint_PythonToCpp_StrokePoint_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* StrokePoint_PTR_CppToPython_StrokePoint(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::StrokePoint*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_StrokePoint_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_StrokePoint(PyObject* module)
{
    SbkNatronEngineTypes[SBK_STROKEPOINT_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_StrokePoint_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "StrokePoint", "StrokePoint*",
        &Sbk_StrokePoint_Type, &Shiboken::callCppDestructor< ::StrokePoint >)) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_StrokePoint_Type,
        StrokePoint_PythonToCpp_StrokePoint_PTR,
        is_StrokePoint_PythonToCpp_StrokePoint_PTR_Convertible,
        StrokePoint_PTR_CppToPython_StrokePoint);

    Shiboken::Conversions::registerConverterName(converter, "StrokePoint");
    Shiboken::Conversions::registerConverterName(converter, "StrokePoint*");
    Shiboken::Conversions::registerConverterName(converter, "StrokePoint&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::StrokePoint).name());



}
