
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

#include "separatorparam_wrapper.h"

// Extra includes
NATRON_NAMESPACE_USING
#include <ParameterWrapper.h>


// Native ---------------------------------------------------------

void SeparatorParamWrapper::pysideInitQtMetaTypes()
{
}

SeparatorParamWrapper::~SeparatorParamWrapper()
{
    SbkObject* wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyMethodDef Sbk_SeparatorParam_methods[] = {

    {0} // Sentinel
};

} // extern "C"

static int Sbk_SeparatorParam_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_SeparatorParam_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_SeparatorParam_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.SeparatorParam",
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
    /*tp_traverse*/         Sbk_SeparatorParam_traverse,
    /*tp_clear*/            Sbk_SeparatorParam_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_SeparatorParam_methods,
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

static void* Sbk_SeparatorParam_typeDiscovery(void* cptr, SbkObjectType* instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType*>(Shiboken::SbkType< ::Param >()))
        return dynamic_cast< ::SeparatorParam*>(reinterpret_cast< ::Param*>(cptr));
    return 0;
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void SeparatorParam_PythonToCpp_SeparatorParam_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_SeparatorParam_Type, pyIn, cppOut);
}
static PythonToCppFunc is_SeparatorParam_PythonToCpp_SeparatorParam_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_SeparatorParam_Type))
        return SeparatorParam_PythonToCpp_SeparatorParam_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* SeparatorParam_PTR_CppToPython_SeparatorParam(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::SeparatorParam*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_SeparatorParam_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_SeparatorParam(PyObject* module)
{
    SbkNatronEngineTypes[SBK_SEPARATORPARAM_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_SeparatorParam_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "SeparatorParam", "SeparatorParam*",
        &Sbk_SeparatorParam_Type, &Shiboken::callCppDestructor< ::SeparatorParam >, (SbkObjectType*)SbkNatronEngineTypes[SBK_PARAM_IDX])) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_SeparatorParam_Type,
        SeparatorParam_PythonToCpp_SeparatorParam_PTR,
        is_SeparatorParam_PythonToCpp_SeparatorParam_PTR_Convertible,
        SeparatorParam_PTR_CppToPython_SeparatorParam);

    Shiboken::Conversions::registerConverterName(converter, "SeparatorParam");
    Shiboken::Conversions::registerConverterName(converter, "SeparatorParam*");
    Shiboken::Conversions::registerConverterName(converter, "SeparatorParam&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::SeparatorParam).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::SeparatorParamWrapper).name());


    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(&Sbk_SeparatorParam_Type, &Sbk_SeparatorParam_typeDiscovery);


    SeparatorParamWrapper::pysideInitQtMetaTypes();
}
