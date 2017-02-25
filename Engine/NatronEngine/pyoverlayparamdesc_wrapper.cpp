
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

#include "pyoverlayparamdesc_wrapper.h"

// Extra includes
NATRON_NAMESPACE_USING NATRON_PYTHON_NAMESPACE_USING



// Target ---------------------------------------------------------

extern "C" {
static int
Sbk_PyOverlayParamDesc_Init(PyObject* self, PyObject* args, PyObject* kwds)
{
    SbkObject* sbkSelf = reinterpret_cast<SbkObject*>(self);
    if (Shiboken::Object::isUserType(self) && !Shiboken::ObjectType::canCallConstructor(self->ob_type, Shiboken::SbkType< ::PyOverlayParamDesc >()))
        return -1;

    ::PyOverlayParamDesc* cptr = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // PyOverlayParamDesc()
            cptr = new ::PyOverlayParamDesc();
        }
    }

    if (PyErr_Occurred() || !Shiboken::Object::setCppPointer(sbkSelf, Shiboken::SbkType< ::PyOverlayParamDesc >(), cptr)) {
        delete cptr;
        return -1;
    }
    Shiboken::Object::setValidCpp(sbkSelf, true);
    Shiboken::BindingManager::instance().registerWrapper(sbkSelf, cptr);


    return 1;
}

static PyMethodDef Sbk_PyOverlayParamDesc_methods[] = {

    {0} // Sentinel
};

static PyObject* Sbk_PyOverlayParamDesc_get_nDims(PyObject* self, void*)
{
    ::PyOverlayParamDesc* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyOverlayParamDesc*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYOVERLAYPARAMDESC_IDX], (SbkObject*)self));
    int cppOut_local = cppSelf->nDims;
    PyObject* pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppOut_local);
    return pyOut;
}
static int Sbk_PyOverlayParamDesc_set_nDims(PyObject* self, PyObject* pyIn, void*)
{
    ::PyOverlayParamDesc* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyOverlayParamDesc*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYOVERLAYPARAMDESC_IDX], (SbkObject*)self));
    if (pyIn == 0) {
        PyErr_SetString(PyExc_TypeError, "'nDims' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp;
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'nDims', 'int' or convertible type expected");
        return -1;
    }

    int cppOut_local = cppSelf->nDims;
    pythonToCpp(pyIn, &cppOut_local);
    cppSelf->nDims = cppOut_local;

    return 0;
}

static PyObject* Sbk_PyOverlayParamDesc_get_optional(PyObject* self, void*)
{
    ::PyOverlayParamDesc* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyOverlayParamDesc*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYOVERLAYPARAMDESC_IDX], (SbkObject*)self));
    bool cppOut_local = cppSelf->optional;
    PyObject* pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppOut_local);
    return pyOut;
}
static int Sbk_PyOverlayParamDesc_set_optional(PyObject* self, PyObject* pyIn, void*)
{
    ::PyOverlayParamDesc* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyOverlayParamDesc*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYOVERLAYPARAMDESC_IDX], (SbkObject*)self));
    if (pyIn == 0) {
        PyErr_SetString(PyExc_TypeError, "'optional' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp;
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'optional', 'bool' or convertible type expected");
        return -1;
    }

    bool cppOut_local = cppSelf->optional;
    pythonToCpp(pyIn, &cppOut_local);
    cppSelf->optional = cppOut_local;

    return 0;
}

static PyObject* Sbk_PyOverlayParamDesc_get_type(PyObject* self, void*)
{
    ::PyOverlayParamDesc* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyOverlayParamDesc*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYOVERLAYPARAMDESC_IDX], (SbkObject*)self));
    PyObject* pyOut = Shiboken::Conversions::copyToPython(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppSelf->type);
    return pyOut;
}
static int Sbk_PyOverlayParamDesc_set_type(PyObject* self, PyObject* pyIn, void*)
{
    ::PyOverlayParamDesc* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyOverlayParamDesc*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYOVERLAYPARAMDESC_IDX], (SbkObject*)self));
    if (pyIn == 0) {
        PyErr_SetString(PyExc_TypeError, "'type' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp;
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'type', 'QString' or convertible type expected");
        return -1;
    }

    ::QString& cppOut_ptr = cppSelf->type;
    pythonToCpp(pyIn, &cppOut_ptr);

    return 0;
}

// Getters and Setters for PyOverlayParamDesc
static PyGetSetDef Sbk_PyOverlayParamDesc_getsetlist[] = {
    {const_cast<char*>("nDims"), Sbk_PyOverlayParamDesc_get_nDims, Sbk_PyOverlayParamDesc_set_nDims},
    {const_cast<char*>("optional"), Sbk_PyOverlayParamDesc_get_optional, Sbk_PyOverlayParamDesc_set_optional},
    {const_cast<char*>("type"), Sbk_PyOverlayParamDesc_get_type, Sbk_PyOverlayParamDesc_set_type},
    {0}  // Sentinel
};

} // extern "C"

static int Sbk_PyOverlayParamDesc_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_PyOverlayParamDesc_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_PyOverlayParamDesc_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.PyOverlayParamDesc",
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
    /*tp_traverse*/         Sbk_PyOverlayParamDesc_traverse,
    /*tp_clear*/            Sbk_PyOverlayParamDesc_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_PyOverlayParamDesc_methods,
    /*tp_members*/          0,
    /*tp_getset*/           Sbk_PyOverlayParamDesc_getsetlist,
    /*tp_base*/             reinterpret_cast<PyTypeObject*>(&SbkObject_Type),
    /*tp_dict*/             0,
    /*tp_descr_get*/        0,
    /*tp_descr_set*/        0,
    /*tp_dictoffset*/       0,
    /*tp_init*/             Sbk_PyOverlayParamDesc_Init,
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
static void PyOverlayParamDesc_PythonToCpp_PyOverlayParamDesc_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_PyOverlayParamDesc_Type, pyIn, cppOut);
}
static PythonToCppFunc is_PyOverlayParamDesc_PythonToCpp_PyOverlayParamDesc_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_PyOverlayParamDesc_Type))
        return PyOverlayParamDesc_PythonToCpp_PyOverlayParamDesc_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* PyOverlayParamDesc_PTR_CppToPython_PyOverlayParamDesc(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::PyOverlayParamDesc*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_PyOverlayParamDesc_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_PyOverlayParamDesc(PyObject* module)
{
    SbkNatronEngineTypes[SBK_PYOVERLAYPARAMDESC_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_PyOverlayParamDesc_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "PyOverlayParamDesc", "PyOverlayParamDesc*",
        &Sbk_PyOverlayParamDesc_Type, &Shiboken::callCppDestructor< ::PyOverlayParamDesc >)) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_PyOverlayParamDesc_Type,
        PyOverlayParamDesc_PythonToCpp_PyOverlayParamDesc_PTR,
        is_PyOverlayParamDesc_PythonToCpp_PyOverlayParamDesc_PTR_Convertible,
        PyOverlayParamDesc_PTR_CppToPython_PyOverlayParamDesc);

    Shiboken::Conversions::registerConverterName(converter, "PyOverlayParamDesc");
    Shiboken::Conversions::registerConverterName(converter, "PyOverlayParamDesc*");
    Shiboken::Conversions::registerConverterName(converter, "PyOverlayParamDesc&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::PyOverlayParamDesc).name());



}
