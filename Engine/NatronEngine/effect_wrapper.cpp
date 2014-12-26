
//workaround to access protected functions
#define protected public

// default includes
#include <shiboken.h>
#include <typeresolver.h>
#include <typeinfo>
#include "natronengine_python.h"

#include "effect_wrapper.h"

// Extra includes
#include <NodeWrapper.h>
#include <ParameterWrapper.h>
#include <list>



// Target ---------------------------------------------------------

extern "C" {
static PyObject* Sbk_EffectFunc_canSetInput(PyObject* self, PyObject* args)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "canSetInput", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: canSetInput(int,const Effect*)const
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppPointerConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_EFFECT_IDX], (pyArgs[1])))) {
        overloadId = 0; // canSetInput(int,const Effect*)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_canSetInput_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        if (!Shiboken::Object::isValid(pyArgs[1]))
            return 0;
        ::Effect* cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // canSetInput(int,const Effect*)const
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            bool cppResult = const_cast<const ::Effect*>(cppSelf)->canSetInput(cppArg0, cppArg1);
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_EffectFunc_canSetInput_TypeError:
        const char* overloads[] = {"int, NatronEngine.Effect", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Effect.canSetInput", overloads);
        return 0;
}

static PyObject* Sbk_EffectFunc_connectInput(PyObject* self, PyObject* args)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "connectInput", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: connectInput(int,const Effect*)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppPointerConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_EFFECT_IDX], (pyArgs[1])))) {
        overloadId = 0; // connectInput(int,const Effect*)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_connectInput_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        if (!Shiboken::Object::isValid(pyArgs[1]))
            return 0;
        ::Effect* cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // connectInput(int,const Effect*)
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            bool cppResult = cppSelf->connectInput(cppArg0, cppArg1);
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_EffectFunc_connectInput_TypeError:
        const char* overloads[] = {"int, NatronEngine.Effect", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Effect.connectInput", overloads);
        return 0;
}

static PyObject* Sbk_EffectFunc_disconnectInput(PyObject* self, PyObject* pyArg)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: disconnectInput(int)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // disconnectInput(int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_disconnectInput_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // disconnectInput(int)
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            cppSelf->disconnectInput(cppArg0);
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_EffectFunc_disconnectInput_TypeError:
        const char* overloads[] = {"int", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Effect.disconnectInput", overloads);
        return 0;
}

static PyObject* Sbk_EffectFunc_getInput(PyObject* self, PyObject* pyArg)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: getInput(int)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // getInput(int)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_getInput_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getInput(int)const
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            Effect * cppResult = const_cast<const ::Effect*>(cppSelf)->getInput(cppArg0);
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_EFFECT_IDX], cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_EffectFunc_getInput_TypeError:
        const char* overloads[] = {"int", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Effect.getInput", overloads);
        return 0;
}

static PyObject* Sbk_EffectFunc_getMaxInputCount(PyObject* self)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getMaxInputCount()const
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            int cppResult = const_cast<const ::Effect*>(cppSelf)->getMaxInputCount();
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_EffectFunc_getName(PyObject* self)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getName()const
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            std::string cppResult = const_cast<const ::Effect*>(cppSelf)->getName();
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_EffectFunc_getParam(PyObject* self, PyObject* pyArg)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: getParam(std::string)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArg)))) {
        overloadId = 0; // getParam(std::string)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_getParam_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getParam(std::string)const
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            Param * cppResult = const_cast<const ::Effect*>(cppSelf)->getParam(cppArg0);
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_PARAM_IDX], cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_EffectFunc_getParam_TypeError:
        const char* overloads[] = {"std::string", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Effect.getParam", overloads);
        return 0;
}

static PyObject* Sbk_EffectFunc_getParams(PyObject* self)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getParams()const
            // Begin code injection

            std::list<Param*> params = cppSelf->getParams();
            PyObject* ret = PyList_New((int) params.size());
            int idx = 0;
            for (std::list<Param*>::iterator it = params.begin(); it!=params.end(); ++it,++idx) {
                PyObject* item = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_PARAM_IDX], *it);
                // Ownership transferences.
                Shiboken::Object::getOwnership(item);
                PyList_SET_ITEM(ret, idx, item);
            }
            return ret;

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_EffectFunc_getPluginID(PyObject* self)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getPluginID()const
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            std::string cppResult = const_cast<const ::Effect*>(cppSelf)->getPluginID();
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_EffectFunc_isNull(PyObject* self)
{
    ::Effect* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Effect*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // isNull()const
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            bool cppResult = const_cast<const ::Effect*>(cppSelf)->isNull();
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyMethodDef Sbk_Effect_methods[] = {
    {"canSetInput", (PyCFunction)Sbk_EffectFunc_canSetInput, METH_VARARGS},
    {"connectInput", (PyCFunction)Sbk_EffectFunc_connectInput, METH_VARARGS},
    {"disconnectInput", (PyCFunction)Sbk_EffectFunc_disconnectInput, METH_O},
    {"getInput", (PyCFunction)Sbk_EffectFunc_getInput, METH_O},
    {"getMaxInputCount", (PyCFunction)Sbk_EffectFunc_getMaxInputCount, METH_NOARGS},
    {"getName", (PyCFunction)Sbk_EffectFunc_getName, METH_NOARGS},
    {"getParam", (PyCFunction)Sbk_EffectFunc_getParam, METH_O},
    {"getParams", (PyCFunction)Sbk_EffectFunc_getParams, METH_NOARGS},
    {"getPluginID", (PyCFunction)Sbk_EffectFunc_getPluginID, METH_NOARGS},
    {"isNull", (PyCFunction)Sbk_EffectFunc_isNull, METH_NOARGS},

    {0} // Sentinel
};

} // extern "C"

static int Sbk_Effect_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_Effect_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_Effect_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.Effect",
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
    /*tp_traverse*/         Sbk_Effect_traverse,
    /*tp_clear*/            Sbk_Effect_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_Effect_methods,
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
static void Effect_PythonToCpp_Effect_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_Effect_Type, pyIn, cppOut);
}
static PythonToCppFunc is_Effect_PythonToCpp_Effect_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_Effect_Type))
        return Effect_PythonToCpp_Effect_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* Effect_PTR_CppToPython_Effect(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::Effect*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_Effect_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_Effect(PyObject* module)
{
    SbkNatronEngineTypes[SBK_EFFECT_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_Effect_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "Effect", "Effect*",
        &Sbk_Effect_Type, &Shiboken::callCppDestructor< ::Effect >)) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_Effect_Type,
        Effect_PythonToCpp_Effect_PTR,
        is_Effect_PythonToCpp_Effect_PTR_Convertible,
        Effect_PTR_CppToPython_Effect);

    Shiboken::Conversions::registerConverterName(converter, "Effect");
    Shiboken::Conversions::registerConverterName(converter, "Effect*");
    Shiboken::Conversions::registerConverterName(converter, "Effect&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::Effect).name());



}
