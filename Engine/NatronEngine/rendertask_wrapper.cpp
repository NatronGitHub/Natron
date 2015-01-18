
//workaround to access protected functions
#define protected public

// default includes
#include <shiboken.h>
#include <typeresolver.h>
#include <typeinfo>
#include "natronengine_python.h"

#include "rendertask_wrapper.h"

// Extra includes
#include <NodeWrapper.h>



// Target ---------------------------------------------------------

extern "C" {
static int
Sbk_RenderTask_Init(PyObject* self, PyObject* args, PyObject* kwds)
{
    SbkObject* sbkSelf = reinterpret_cast<SbkObject*>(self);
    if (Shiboken::Object::isUserType(self) && !Shiboken::ObjectType::canCallConstructor(self->ob_type, Shiboken::SbkType< ::RenderTask >()))
        return -1;

    ::RenderTask* cptr = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0};

    // invalid argument lengths
    if (numArgs == 1 || numArgs == 2)
        goto Sbk_RenderTask_Init_TypeError;

    if (!PyArg_UnpackTuple(args, "RenderTask", 0, 3, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return -1;


    // Overloaded function decisor
    // 0: RenderTask()
    // 1: RenderTask(Effect*,int,int)
    if (numArgs == 0) {
        overloadId = 0; // RenderTask()
    } else if (numArgs == 3
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppPointerConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_EFFECT_IDX], (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2])))) {
        overloadId = 1; // RenderTask(Effect*,int,int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RenderTask_Init_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // RenderTask()
        {

            if (!PyErr_Occurred()) {
                // RenderTask()
                PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
                cptr = new ::RenderTask();
                PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            }
            break;
        }
        case 1: // RenderTask(Effect * writeNode, int firstFrame, int lastFrame)
        {
            if (!Shiboken::Object::isValid(pyArgs[0]))
                return -1;
            ::Effect* cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            int cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);
            int cppArg2;
            pythonToCpp[2](pyArgs[2], &cppArg2);

            if (!PyErr_Occurred()) {
                // RenderTask(Effect*,int,int)
                PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
                cptr = new ::RenderTask(cppArg0, cppArg1, cppArg2);
                PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            }
            break;
        }
    }

    if (PyErr_Occurred() || !Shiboken::Object::setCppPointer(sbkSelf, Shiboken::SbkType< ::RenderTask >(), cptr)) {
        delete cptr;
        return -1;
    }
    if (!cptr) goto Sbk_RenderTask_Init_TypeError;

    Shiboken::Object::setValidCpp(sbkSelf, true);
    Shiboken::BindingManager::instance().registerWrapper(sbkSelf, cptr);


    return 1;

    Sbk_RenderTask_Init_TypeError:
        const char* overloads[] = {"", "NatronEngine.Effect, int, int", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.RenderTask", overloads);
        return -1;
}

static PyMethodDef Sbk_RenderTask_methods[] = {

    {0} // Sentinel
};

static PyObject* Sbk_RenderTask_get_writeNode(PyObject* self, void*)
{
    ::RenderTask* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RenderTask*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RENDERTASK_IDX], (SbkObject*)self));
    PyObject* pyOut = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_EFFECT_IDX], cppSelf->writeNode);
    return pyOut;
}
static int Sbk_RenderTask_set_writeNode(PyObject* self, PyObject* pyIn, void*)
{
    ::RenderTask* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RenderTask*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RENDERTASK_IDX], (SbkObject*)self));
    if (pyIn == 0) {
        PyErr_SetString(PyExc_TypeError, "'writeNode' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp;
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppPointerConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_EFFECT_IDX], (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'writeNode', 'Effect' or convertible type expected");
        return -1;
    }

    ::Effect*& cppOut_ptr = cppSelf->writeNode;
    pythonToCpp(pyIn, &cppOut_ptr);

    Shiboken::Object::keepReference(reinterpret_cast<SbkObject*>(self), "writeNode", pyIn);
    return 0;
}

static PyObject* Sbk_RenderTask_get_firstFrame(PyObject* self, void*)
{
    ::RenderTask* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RenderTask*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RENDERTASK_IDX], (SbkObject*)self));
    int cppOut_local = cppSelf->firstFrame;
    PyObject* pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppOut_local);
    return pyOut;
}
static int Sbk_RenderTask_set_firstFrame(PyObject* self, PyObject* pyIn, void*)
{
    ::RenderTask* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RenderTask*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RENDERTASK_IDX], (SbkObject*)self));
    if (pyIn == 0) {
        PyErr_SetString(PyExc_TypeError, "'firstFrame' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp;
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'firstFrame', 'int' or convertible type expected");
        return -1;
    }

    int cppOut_local = cppSelf->firstFrame;
    pythonToCpp(pyIn, &cppOut_local);
    cppSelf->firstFrame = cppOut_local;

    return 0;
}

static PyObject* Sbk_RenderTask_get_lastFrame(PyObject* self, void*)
{
    ::RenderTask* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RenderTask*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RENDERTASK_IDX], (SbkObject*)self));
    int cppOut_local = cppSelf->lastFrame;
    PyObject* pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppOut_local);
    return pyOut;
}
static int Sbk_RenderTask_set_lastFrame(PyObject* self, PyObject* pyIn, void*)
{
    ::RenderTask* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::RenderTask*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RENDERTASK_IDX], (SbkObject*)self));
    if (pyIn == 0) {
        PyErr_SetString(PyExc_TypeError, "'lastFrame' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp;
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'lastFrame', 'int' or convertible type expected");
        return -1;
    }

    int cppOut_local = cppSelf->lastFrame;
    pythonToCpp(pyIn, &cppOut_local);
    cppSelf->lastFrame = cppOut_local;

    return 0;
}

// Getters and Setters for RenderTask
static PyGetSetDef Sbk_RenderTask_getsetlist[] = {
    {const_cast<char*>("writeNode"), Sbk_RenderTask_get_writeNode, Sbk_RenderTask_set_writeNode},
    {const_cast<char*>("firstFrame"), Sbk_RenderTask_get_firstFrame, Sbk_RenderTask_set_firstFrame},
    {const_cast<char*>("lastFrame"), Sbk_RenderTask_get_lastFrame, Sbk_RenderTask_set_lastFrame},
    {0}  // Sentinel
};

} // extern "C"

static int Sbk_RenderTask_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_RenderTask_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_RenderTask_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.RenderTask",
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
    /*tp_traverse*/         Sbk_RenderTask_traverse,
    /*tp_clear*/            Sbk_RenderTask_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_RenderTask_methods,
    /*tp_members*/          0,
    /*tp_getset*/           Sbk_RenderTask_getsetlist,
    /*tp_base*/             reinterpret_cast<PyTypeObject*>(&SbkObject_Type),
    /*tp_dict*/             0,
    /*tp_descr_get*/        0,
    /*tp_descr_set*/        0,
    /*tp_dictoffset*/       0,
    /*tp_init*/             Sbk_RenderTask_Init,
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
static void RenderTask_PythonToCpp_RenderTask_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_RenderTask_Type, pyIn, cppOut);
}
static PythonToCppFunc is_RenderTask_PythonToCpp_RenderTask_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_RenderTask_Type))
        return RenderTask_PythonToCpp_RenderTask_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* RenderTask_PTR_CppToPython_RenderTask(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::RenderTask*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_RenderTask_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_RenderTask(PyObject* module)
{
    SbkNatronEngineTypes[SBK_RENDERTASK_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_RenderTask_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "RenderTask", "RenderTask*",
        &Sbk_RenderTask_Type, &Shiboken::callCppDestructor< ::RenderTask >)) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_RenderTask_Type,
        RenderTask_PythonToCpp_RenderTask_PTR,
        is_RenderTask_PythonToCpp_RenderTask_PTR_Convertible,
        RenderTask_PTR_CppToPython_RenderTask);

    Shiboken::Conversions::registerConverterName(converter, "RenderTask");
    Shiboken::Conversions::registerConverterName(converter, "RenderTask*");
    Shiboken::Conversions::registerConverterName(converter, "RenderTask&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::RenderTask).name());



}
