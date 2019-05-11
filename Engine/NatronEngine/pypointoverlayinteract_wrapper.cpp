
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

#include "pypointoverlayinteract_wrapper.h"

// Extra includes
NATRON_NAMESPACE_USING NATRON_PYTHON_NAMESPACE_USING
#include <PyNode.h>
#include <PyOverlayInteract.h>
#include <PyParameter.h>
#include <map>


// Native ---------------------------------------------------------

void PyPointOverlayInteractWrapper::pysideInitQtMetaTypes()
{
}

PyPointOverlayInteractWrapper::PyPointOverlayInteractWrapper() : PyPointOverlayInteract() {
    // ... middle
}

std::map<QString, PyOverlayParamDesc > PyPointOverlayInteractWrapper::describeParameters() const
{
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return ::std::map<QString, PyOverlayParamDesc >();
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, "describeParameters"));
    if (pyOverride.isNull()) {
        gil.release();
        return this->::PyOverlayInteract::describeParameters();
    }

    Shiboken::AutoDecRef pyArgs(PyTuple_New(0));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, NULL));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return ::std::map<QString, PyOverlayParamDesc >();
    }
    // Check return type
    PythonToCppFunc pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_MAP_QSTRING_PYOVERLAYPARAMDESC_IDX], pyResult);
    if (!pythonToCpp) {
        Shiboken::warning(PyExc_RuntimeWarning, 2, "Invalid return value in function %s, expected %s, got %s.", "PyPointOverlayInteract.describeParameters", "map", pyResult->ob_type->tp_name);
        return ::std::map<QString, PyOverlayParamDesc >();
    }
    ::std::map<QString, PyOverlayParamDesc > cppResult;
    pythonToCpp(pyResult, &cppResult);
    return cppResult;
}

void PyPointOverlayInteractWrapper::draw(double time, const Double2DTuple & renderScale, QString view)
{
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return ;
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, "draw"));
    if (pyOverride.isNull()) {
        gil.release();
        return this->::PyOverlayInteract::draw(time, renderScale, view);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(dNN)",
        time,
        Shiboken::Conversions::referenceToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_DOUBLE2DTUPLE_IDX], &renderScale),
        Shiboken::Conversions::copyToPython(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], &view)
    ));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, NULL));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return ;
    }
}

void PyPointOverlayInteractWrapper::fetchParameters(const std::map<QString, QString > & params)
{
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return ;
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, "fetchParameters"));
    if (pyOverride.isNull()) {
        gil.release();
        return this->::PyOverlayInteract::fetchParameters(params);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(N)",
        Shiboken::Conversions::copyToPython(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_MAP_QSTRING_QSTRING_IDX], &params)
    ));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, NULL));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return ;
    }
}

PyPointOverlayInteractWrapper::~PyPointOverlayInteractWrapper()
{
    SbkObject* wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static int
Sbk_PyPointOverlayInteract_Init(PyObject* self, PyObject* args, PyObject* kwds)
{
    SbkObject* sbkSelf = reinterpret_cast<SbkObject*>(self);
    if (Shiboken::Object::isUserType(self) && !Shiboken::ObjectType::canCallConstructor(self->ob_type, Shiboken::SbkType< ::PyPointOverlayInteract >()))
        return -1;

    ::PyPointOverlayInteractWrapper* cptr = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // PyPointOverlayInteract()
            cptr = new ::PyPointOverlayInteractWrapper();
        }
    }

    if (PyErr_Occurred() || !Shiboken::Object::setCppPointer(sbkSelf, Shiboken::SbkType< ::PyPointOverlayInteract >(), cptr)) {
        delete cptr;
        return -1;
    }
    Shiboken::Object::setValidCpp(sbkSelf, true);
    Shiboken::Object::setHasCppWrapper(sbkSelf, true);
    Shiboken::BindingManager::instance().registerWrapper(sbkSelf, cptr);


    return 1;
}

static PyMethodDef Sbk_PyPointOverlayInteract_methods[] = {

    {0} // Sentinel
};

} // extern "C"

static int Sbk_PyPointOverlayInteract_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_PyPointOverlayInteract_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_PyPointOverlayInteract_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.PyPointOverlayInteract",
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
    /*tp_traverse*/         Sbk_PyPointOverlayInteract_traverse,
    /*tp_clear*/            Sbk_PyPointOverlayInteract_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_PyPointOverlayInteract_methods,
    /*tp_members*/          0,
    /*tp_getset*/           0,
    /*tp_base*/             0,
    /*tp_dict*/             0,
    /*tp_descr_get*/        0,
    /*tp_descr_set*/        0,
    /*tp_dictoffset*/       0,
    /*tp_init*/             Sbk_PyPointOverlayInteract_Init,
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

static void* Sbk_PyPointOverlayInteract_typeDiscovery(void* cptr, SbkObjectType* instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType*>(Shiboken::SbkType< ::PyOverlayInteract >()))
        return dynamic_cast< ::PyPointOverlayInteract*>(reinterpret_cast< ::PyOverlayInteract*>(cptr));
    return 0;
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void PyPointOverlayInteract_PythonToCpp_PyPointOverlayInteract_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_PyPointOverlayInteract_Type, pyIn, cppOut);
}
static PythonToCppFunc is_PyPointOverlayInteract_PythonToCpp_PyPointOverlayInteract_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_PyPointOverlayInteract_Type))
        return PyPointOverlayInteract_PythonToCpp_PyPointOverlayInteract_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* PyPointOverlayInteract_PTR_CppToPython_PyPointOverlayInteract(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::PyPointOverlayInteract*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_PyPointOverlayInteract_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_PyPointOverlayInteract(PyObject* module)
{
    SbkNatronEngineTypes[SBK_PYPOINTOVERLAYINTERACT_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_PyPointOverlayInteract_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "PyPointOverlayInteract", "PyPointOverlayInteract*",
        &Sbk_PyPointOverlayInteract_Type, &Shiboken::callCppDestructor< ::PyPointOverlayInteract >, (SbkObjectType*)SbkNatronEngineTypes[SBK_PYOVERLAYINTERACT_IDX])) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_PyPointOverlayInteract_Type,
        PyPointOverlayInteract_PythonToCpp_PyPointOverlayInteract_PTR,
        is_PyPointOverlayInteract_PythonToCpp_PyPointOverlayInteract_PTR_Convertible,
        PyPointOverlayInteract_PTR_CppToPython_PyPointOverlayInteract);

    Shiboken::Conversions::registerConverterName(converter, "PyPointOverlayInteract");
    Shiboken::Conversions::registerConverterName(converter, "PyPointOverlayInteract*");
    Shiboken::Conversions::registerConverterName(converter, "PyPointOverlayInteract&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::PyPointOverlayInteract).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::PyPointOverlayInteractWrapper).name());


    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(&Sbk_PyPointOverlayInteract_Type, &Sbk_PyPointOverlayInteract_typeDiscovery);


    PyPointOverlayInteractWrapper::pysideInitQtMetaTypes();
}
