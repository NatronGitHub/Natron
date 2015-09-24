
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

#include "outputfileparam_wrapper.h"

// Extra includes
#include <ParameterWrapper.h>


// Native ---------------------------------------------------------

void OutputFileParamWrapper::pysideInitQtMetaTypes()
{
}

OutputFileParamWrapper::~OutputFileParamWrapper()
{
    SbkObject* wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject* Sbk_OutputFileParamFunc_openFile(PyObject* self)
{
    OutputFileParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (OutputFileParamWrapper*)((::OutputFileParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_OUTPUTFILEPARAM_IDX], (SbkObject*)self));

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // openFile()
            cppSelf->openFile();
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;
}

static PyObject* Sbk_OutputFileParamFunc_setSequenceEnabled(PyObject* self, PyObject* pyArg)
{
    OutputFileParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (OutputFileParamWrapper*)((::OutputFileParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_OUTPUTFILEPARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setSequenceEnabled(bool)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArg)))) {
        overloadId = 0; // setSequenceEnabled(bool)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_OutputFileParamFunc_setSequenceEnabled_TypeError;

    // Call function/method
    {
        bool cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setSequenceEnabled(bool)
            cppSelf->setSequenceEnabled(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_OutputFileParamFunc_setSequenceEnabled_TypeError:
        const char* overloads[] = {"bool", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.OutputFileParam.setSequenceEnabled", overloads);
        return 0;
}

static PyMethodDef Sbk_OutputFileParam_methods[] = {
    {"openFile", (PyCFunction)Sbk_OutputFileParamFunc_openFile, METH_NOARGS},
    {"setSequenceEnabled", (PyCFunction)Sbk_OutputFileParamFunc_setSequenceEnabled, METH_O},

    {0} // Sentinel
};

} // extern "C"

static int Sbk_OutputFileParam_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_OutputFileParam_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_OutputFileParam_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.OutputFileParam",
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
    /*tp_traverse*/         Sbk_OutputFileParam_traverse,
    /*tp_clear*/            Sbk_OutputFileParam_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_OutputFileParam_methods,
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

static void* Sbk_OutputFileParam_typeDiscovery(void* cptr, SbkObjectType* instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType*>(Shiboken::SbkType< ::Param >()))
        return dynamic_cast< ::OutputFileParam*>(reinterpret_cast< ::Param*>(cptr));
    return 0;
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void OutputFileParam_PythonToCpp_OutputFileParam_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_OutputFileParam_Type, pyIn, cppOut);
}
static PythonToCppFunc is_OutputFileParam_PythonToCpp_OutputFileParam_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_OutputFileParam_Type))
        return OutputFileParam_PythonToCpp_OutputFileParam_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* OutputFileParam_PTR_CppToPython_OutputFileParam(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::OutputFileParam*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_OutputFileParam_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_OutputFileParam(PyObject* module)
{
    SbkNatronEngineTypes[SBK_OUTPUTFILEPARAM_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_OutputFileParam_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "OutputFileParam", "OutputFileParam*",
        &Sbk_OutputFileParam_Type, &Shiboken::callCppDestructor< ::OutputFileParam >, (SbkObjectType*)SbkNatronEngineTypes[SBK_STRINGPARAMBASE_IDX])) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_OutputFileParam_Type,
        OutputFileParam_PythonToCpp_OutputFileParam_PTR,
        is_OutputFileParam_PythonToCpp_OutputFileParam_PTR_Convertible,
        OutputFileParam_PTR_CppToPython_OutputFileParam);

    Shiboken::Conversions::registerConverterName(converter, "OutputFileParam");
    Shiboken::Conversions::registerConverterName(converter, "OutputFileParam*");
    Shiboken::Conversions::registerConverterName(converter, "OutputFileParam&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::OutputFileParam).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::OutputFileParamWrapper).name());


    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(&Sbk_OutputFileParam_Type, &Sbk_OutputFileParam_typeDiscovery);


    OutputFileParamWrapper::pysideInitQtMetaTypes();
}
