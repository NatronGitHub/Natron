
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

#include "stringparambase_wrapper.h"

// Extra includes
NATRON_NAMESPACE_USING
#include <ParameterWrapper.h>


// Native ---------------------------------------------------------

void StringParamBaseWrapper::pysideInitQtMetaTypes()
{
}

StringParamBaseWrapper::~StringParamBaseWrapper()
{
    SbkObject* wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject* Sbk_StringParamBaseFunc_addAsDependencyOf(PyObject* self, PyObject* args)
{
    StringParamBaseWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (StringParamBaseWrapper*)((::StringParamBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_STRINGPARAMBASE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "addAsDependencyOf", 3, 3, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return 0;


    // Overloaded function decisor
    // 0: addAsDependencyOf(int,Param*,int)
    if (numArgs == 3
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppPointerConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_PARAM_IDX], (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2])))) {
        overloadId = 0; // addAsDependencyOf(int,Param*,int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_StringParamBaseFunc_addAsDependencyOf_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        if (!Shiboken::Object::isValid(pyArgs[1]))
            return 0;
        ::Param* cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        int cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // addAsDependencyOf(int,Param*,int)
            std::string cppResult = cppSelf->addAsDependencyOf(cppArg0, cppArg1, cppArg2);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_StringParamBaseFunc_addAsDependencyOf_TypeError:
        const char* overloads[] = {"int, NatronEngine.Param, int", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.StringParamBase.addAsDependencyOf", overloads);
        return 0;
}

static PyObject* Sbk_StringParamBaseFunc_get(PyObject* self, PyObject* args)
{
    StringParamBaseWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (StringParamBaseWrapper*)((::StringParamBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_STRINGPARAMBASE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "get", 0, 1, &(pyArgs[0])))
        return 0;


    // Overloaded function decisor
    // 0: get()const
    // 1: get(double)const
    if (numArgs == 0) {
        overloadId = 0; // get()const
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))) {
        overloadId = 1; // get(double)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_StringParamBaseFunc_get_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // get() const
        {

            if (!PyErr_Occurred()) {
                // get()const
                std::string cppResult = const_cast<const ::StringParamBaseWrapper*>(cppSelf)->get();
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), &cppResult);
            }
            break;
        }
        case 1: // get(double frame) const
        {
            double cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // get(double)const
                std::string cppResult = const_cast<const ::StringParamBaseWrapper*>(cppSelf)->get(cppArg0);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), &cppResult);
            }
            break;
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_StringParamBaseFunc_get_TypeError:
        const char* overloads[] = {"", "float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.StringParamBase.get", overloads);
        return 0;
}

static PyObject* Sbk_StringParamBaseFunc_getDefaultValue(PyObject* self)
{
    StringParamBaseWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (StringParamBaseWrapper*)((::StringParamBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_STRINGPARAMBASE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getDefaultValue()const
            std::string cppResult = const_cast<const ::StringParamBaseWrapper*>(cppSelf)->getDefaultValue();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_StringParamBaseFunc_getValue(PyObject* self)
{
    StringParamBaseWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (StringParamBaseWrapper*)((::StringParamBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_STRINGPARAMBASE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getValue()const
            std::string cppResult = const_cast<const ::StringParamBaseWrapper*>(cppSelf)->getValue();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_StringParamBaseFunc_getValueAtTime(PyObject* self, PyObject* pyArg)
{
    StringParamBaseWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (StringParamBaseWrapper*)((::StringParamBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_STRINGPARAMBASE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: getValueAtTime(double)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArg)))) {
        overloadId = 0; // getValueAtTime(double)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_StringParamBaseFunc_getValueAtTime_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getValueAtTime(double)const
            std::string cppResult = const_cast<const ::StringParamBaseWrapper*>(cppSelf)->getValueAtTime(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_StringParamBaseFunc_getValueAtTime_TypeError:
        const char* overloads[] = {"float", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.StringParamBase.getValueAtTime", overloads);
        return 0;
}

static PyObject* Sbk_StringParamBaseFunc_restoreDefaultValue(PyObject* self)
{
    StringParamBaseWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (StringParamBaseWrapper*)((::StringParamBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_STRINGPARAMBASE_IDX], (SbkObject*)self));

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // restoreDefaultValue()
            cppSelf->restoreDefaultValue();
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;
}

static PyObject* Sbk_StringParamBaseFunc_set(PyObject* self, PyObject* args)
{
    StringParamBaseWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (StringParamBaseWrapper*)((::StringParamBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_STRINGPARAMBASE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "set", 1, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: set(std::string)
    // 1: set(std::string,double)
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // set(std::string)
        } else if (numArgs == 2
            && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
            overloadId = 1; // set(std::string,double)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_StringParamBaseFunc_set_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // set(const std::string & x)
        {
            ::std::string cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // set(std::string)
                cppSelf->set(cppArg0);
            }
            break;
        }
        case 1: // set(const std::string & x, double frame)
        {
            ::std::string cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            double cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);

            if (!PyErr_Occurred()) {
                // set(std::string,double)
                cppSelf->set(cppArg0, cppArg1);
            }
            break;
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_StringParamBaseFunc_set_TypeError:
        const char* overloads[] = {"std::string", "std::string, float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.StringParamBase.set", overloads);
        return 0;
}

static PyObject* Sbk_StringParamBaseFunc_setDefaultValue(PyObject* self, PyObject* pyArg)
{
    StringParamBaseWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (StringParamBaseWrapper*)((::StringParamBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_STRINGPARAMBASE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setDefaultValue(std::string)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArg)))) {
        overloadId = 0; // setDefaultValue(std::string)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_StringParamBaseFunc_setDefaultValue_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setDefaultValue(std::string)
            cppSelf->setDefaultValue(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_StringParamBaseFunc_setDefaultValue_TypeError:
        const char* overloads[] = {"std::string", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.StringParamBase.setDefaultValue", overloads);
        return 0;
}

static PyObject* Sbk_StringParamBaseFunc_setValue(PyObject* self, PyObject* pyArg)
{
    StringParamBaseWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (StringParamBaseWrapper*)((::StringParamBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_STRINGPARAMBASE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setValue(std::string)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArg)))) {
        overloadId = 0; // setValue(std::string)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_StringParamBaseFunc_setValue_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setValue(std::string)
            cppSelf->setValue(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_StringParamBaseFunc_setValue_TypeError:
        const char* overloads[] = {"std::string", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.StringParamBase.setValue", overloads);
        return 0;
}

static PyObject* Sbk_StringParamBaseFunc_setValueAtTime(PyObject* self, PyObject* args)
{
    StringParamBaseWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (StringParamBaseWrapper*)((::StringParamBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_STRINGPARAMBASE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "setValueAtTime", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: setValueAtTime(std::string,double)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
        overloadId = 0; // setValueAtTime(std::string,double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_StringParamBaseFunc_setValueAtTime_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // setValueAtTime(std::string,double)
            cppSelf->setValueAtTime(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_StringParamBaseFunc_setValueAtTime_TypeError:
        const char* overloads[] = {"std::string, float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.StringParamBase.setValueAtTime", overloads);
        return 0;
}

static PyMethodDef Sbk_StringParamBase_methods[] = {
    {"addAsDependencyOf", (PyCFunction)Sbk_StringParamBaseFunc_addAsDependencyOf, METH_VARARGS},
    {"get", (PyCFunction)Sbk_StringParamBaseFunc_get, METH_VARARGS},
    {"getDefaultValue", (PyCFunction)Sbk_StringParamBaseFunc_getDefaultValue, METH_NOARGS},
    {"getValue", (PyCFunction)Sbk_StringParamBaseFunc_getValue, METH_NOARGS},
    {"getValueAtTime", (PyCFunction)Sbk_StringParamBaseFunc_getValueAtTime, METH_O},
    {"restoreDefaultValue", (PyCFunction)Sbk_StringParamBaseFunc_restoreDefaultValue, METH_NOARGS},
    {"set", (PyCFunction)Sbk_StringParamBaseFunc_set, METH_VARARGS},
    {"setDefaultValue", (PyCFunction)Sbk_StringParamBaseFunc_setDefaultValue, METH_O},
    {"setValue", (PyCFunction)Sbk_StringParamBaseFunc_setValue, METH_O},
    {"setValueAtTime", (PyCFunction)Sbk_StringParamBaseFunc_setValueAtTime, METH_VARARGS},

    {0} // Sentinel
};

} // extern "C"

static int Sbk_StringParamBase_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_StringParamBase_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_StringParamBase_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.StringParamBase",
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
    /*tp_traverse*/         Sbk_StringParamBase_traverse,
    /*tp_clear*/            Sbk_StringParamBase_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_StringParamBase_methods,
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

static void* Sbk_StringParamBase_typeDiscovery(void* cptr, SbkObjectType* instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType*>(Shiboken::SbkType< ::Param >()))
        return dynamic_cast< ::StringParamBase*>(reinterpret_cast< ::Param*>(cptr));
    return 0;
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void StringParamBase_PythonToCpp_StringParamBase_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_StringParamBase_Type, pyIn, cppOut);
}
static PythonToCppFunc is_StringParamBase_PythonToCpp_StringParamBase_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_StringParamBase_Type))
        return StringParamBase_PythonToCpp_StringParamBase_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* StringParamBase_PTR_CppToPython_StringParamBase(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::StringParamBase*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_StringParamBase_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_StringParamBase(PyObject* module)
{
    SbkNatronEngineTypes[SBK_STRINGPARAMBASE_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_StringParamBase_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "StringParamBase", "StringParamBase*",
        &Sbk_StringParamBase_Type, &Shiboken::callCppDestructor< ::StringParamBase >, (SbkObjectType*)SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX])) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_StringParamBase_Type,
        StringParamBase_PythonToCpp_StringParamBase_PTR,
        is_StringParamBase_PythonToCpp_StringParamBase_PTR_Convertible,
        StringParamBase_PTR_CppToPython_StringParamBase);

    Shiboken::Conversions::registerConverterName(converter, "StringParamBase");
    Shiboken::Conversions::registerConverterName(converter, "StringParamBase*");
    Shiboken::Conversions::registerConverterName(converter, "StringParamBase&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::StringParamBase).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::StringParamBaseWrapper).name());


    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(&Sbk_StringParamBase_Type, &Sbk_StringParamBase_typeDiscovery);


    StringParamBaseWrapper::pysideInitQtMetaTypes();
}
