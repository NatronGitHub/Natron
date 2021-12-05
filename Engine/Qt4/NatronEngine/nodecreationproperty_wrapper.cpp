
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

#include "nodecreationproperty_wrapper.h"

// Extra includes
NATRON_NAMESPACE_USING NATRON_PYTHON_NAMESPACE_USING


// Native ---------------------------------------------------------

void NodeCreationPropertyWrapper::pysideInitQtMetaTypes()
{
}

NodeCreationPropertyWrapper::NodeCreationPropertyWrapper() : NodeCreationProperty() {
    // ... middle
}

NodeCreationPropertyWrapper::~NodeCreationPropertyWrapper()
{
    SbkObject* wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static int
Sbk_NodeCreationProperty_Init(PyObject* self, PyObject* args, PyObject* kwds)
{
    SbkObject* sbkSelf = reinterpret_cast<SbkObject*>(self);
    if (Shiboken::Object::isUserType(self) && !Shiboken::ObjectType::canCallConstructor(self->ob_type, Shiboken::SbkType< ::NodeCreationProperty >()))
        return -1;

    ::NodeCreationPropertyWrapper* cptr = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // NodeCreationProperty()
            cptr = new ::NodeCreationPropertyWrapper();
        }
    }

    if (PyErr_Occurred() || !Shiboken::Object::setCppPointer(sbkSelf, Shiboken::SbkType< ::NodeCreationProperty >(), cptr)) {
        delete cptr;
        return -1;
    }
    Shiboken::Object::setValidCpp(sbkSelf, true);
    Shiboken::Object::setHasCppWrapper(sbkSelf, true);
    Shiboken::BindingManager::instance().registerWrapper(sbkSelf, cptr);


    return 1;
}

static PyMethodDef Sbk_NodeCreationProperty_methods[] = {

    {0} // Sentinel
};

} // extern "C"

static int Sbk_NodeCreationProperty_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_NodeCreationProperty_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_NodeCreationProperty_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.NodeCreationProperty",
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
    /*tp_traverse*/         Sbk_NodeCreationProperty_traverse,
    /*tp_clear*/            Sbk_NodeCreationProperty_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_NodeCreationProperty_methods,
    /*tp_members*/          0,
    /*tp_getset*/           0,
    /*tp_base*/             reinterpret_cast<PyTypeObject*>(&SbkObject_Type),
    /*tp_dict*/             0,
    /*tp_descr_get*/        0,
    /*tp_descr_set*/        0,
    /*tp_dictoffset*/       0,
    /*tp_init*/             Sbk_NodeCreationProperty_Init,
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
static void NodeCreationProperty_PythonToCpp_NodeCreationProperty_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_NodeCreationProperty_Type, pyIn, cppOut);
}
static PythonToCppFunc is_NodeCreationProperty_PythonToCpp_NodeCreationProperty_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_NodeCreationProperty_Type))
        return NodeCreationProperty_PythonToCpp_NodeCreationProperty_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* NodeCreationProperty_PTR_CppToPython_NodeCreationProperty(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::NodeCreationProperty*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_NodeCreationProperty_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_NodeCreationProperty(PyObject* module)
{
    SbkNatronEngineTypes[SBK_NODECREATIONPROPERTY_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_NodeCreationProperty_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "NodeCreationProperty", "NodeCreationProperty*",
        &Sbk_NodeCreationProperty_Type, &Shiboken::callCppDestructor< ::NodeCreationProperty >)) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_NodeCreationProperty_Type,
        NodeCreationProperty_PythonToCpp_NodeCreationProperty_PTR,
        is_NodeCreationProperty_PythonToCpp_NodeCreationProperty_PTR_Convertible,
        NodeCreationProperty_PTR_CppToPython_NodeCreationProperty);

    Shiboken::Conversions::registerConverterName(converter, "NodeCreationProperty");
    Shiboken::Conversions::registerConverterName(converter, "NodeCreationProperty*");
    Shiboken::Conversions::registerConverterName(converter, "NodeCreationProperty&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::NodeCreationProperty).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::NodeCreationPropertyWrapper).name());



    NodeCreationPropertyWrapper::pysideInitQtMetaTypes();
}
