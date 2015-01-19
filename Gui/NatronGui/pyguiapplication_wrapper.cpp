
//workaround to access protected functions
#define protected public

// default includes
#include <shiboken.h>
#include <typeresolver.h>
#include <typeinfo>
#include "natrongui_python.h"

#include "pyguiapplication_wrapper.h"

// Extra includes
#include <AppInstanceWrapper.h>
#include <GuiAppWrapper.h>
#include <list>


// Native ---------------------------------------------------------

PyGuiApplicationWrapper::PyGuiApplicationWrapper() : PyGuiApplication() {
    // ... middle
}

PyGuiApplicationWrapper::~PyGuiApplicationWrapper()
{
    SbkObject* wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static int
Sbk_PyGuiApplication_Init(PyObject* self, PyObject* args, PyObject* kwds)
{
    SbkObject* sbkSelf = reinterpret_cast<SbkObject*>(self);
    if (Shiboken::Object::isUserType(self) && !Shiboken::ObjectType::canCallConstructor(self->ob_type, Shiboken::SbkType< ::PyGuiApplication >()))
        return -1;

    ::PyGuiApplicationWrapper* cptr = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // PyGuiApplication()
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            cptr = new ::PyGuiApplicationWrapper();
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
        }
    }

    if (PyErr_Occurred() || !Shiboken::Object::setCppPointer(sbkSelf, Shiboken::SbkType< ::PyGuiApplication >(), cptr)) {
        delete cptr;
        return -1;
    }
    Shiboken::Object::setValidCpp(sbkSelf, true);
    Shiboken::Object::setHasCppWrapper(sbkSelf, true);
    Shiboken::BindingManager::instance().registerWrapper(sbkSelf, cptr);


    return 1;
}

static PyObject* Sbk_PyGuiApplicationFunc_getGuiInstance(PyObject* self, PyObject* pyArg)
{
    ::PyGuiApplication* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyGuiApplication*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYGUIAPPLICATION_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: getGuiInstance(int)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // getGuiInstance(int)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyGuiApplicationFunc_getGuiInstance_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getGuiInstance(int)const
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            GuiApp * cppResult = const_cast<const ::PyGuiApplication*>(cppSelf)->getGuiInstance(cppArg0);
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronGuiTypes[SBK_GUIAPP_IDX], cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_PyGuiApplicationFunc_getGuiInstance_TypeError:
        const char* overloads[] = {"int", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronGui.PyGuiApplication.getGuiInstance", overloads);
        return 0;
}

static PyMethodDef Sbk_PyGuiApplication_methods[] = {
    {"getGuiInstance", (PyCFunction)Sbk_PyGuiApplicationFunc_getGuiInstance, METH_O},

    {0} // Sentinel
};

} // extern "C"

static int Sbk_PyGuiApplication_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_PyGuiApplication_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_PyGuiApplication_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronGui.PyGuiApplication",
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
    /*tp_traverse*/         Sbk_PyGuiApplication_traverse,
    /*tp_clear*/            Sbk_PyGuiApplication_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_PyGuiApplication_methods,
    /*tp_members*/          0,
    /*tp_getset*/           0,
    /*tp_base*/             0,
    /*tp_dict*/             0,
    /*tp_descr_get*/        0,
    /*tp_descr_set*/        0,
    /*tp_dictoffset*/       0,
    /*tp_init*/             Sbk_PyGuiApplication_Init,
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

static void* Sbk_PyGuiApplication_typeDiscovery(void* cptr, SbkObjectType* instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType*>(Shiboken::SbkType< ::PyCoreApplication >()))
        return dynamic_cast< ::PyGuiApplication*>(reinterpret_cast< ::PyCoreApplication*>(cptr));
    return 0;
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void PyGuiApplication_PythonToCpp_PyGuiApplication_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_PyGuiApplication_Type, pyIn, cppOut);
}
static PythonToCppFunc is_PyGuiApplication_PythonToCpp_PyGuiApplication_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_PyGuiApplication_Type))
        return PyGuiApplication_PythonToCpp_PyGuiApplication_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* PyGuiApplication_PTR_CppToPython_PyGuiApplication(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::PyGuiApplication*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_PyGuiApplication_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_PyGuiApplication(PyObject* module)
{
    SbkNatronGuiTypes[SBK_PYGUIAPPLICATION_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_PyGuiApplication_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "PyGuiApplication", "PyGuiApplication*",
        &Sbk_PyGuiApplication_Type, &Shiboken::callCppDestructor< ::PyGuiApplication >, (SbkObjectType*)SbkNatronEngineTypes[SBK_PYCOREAPPLICATION_IDX])) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_PyGuiApplication_Type,
        PyGuiApplication_PythonToCpp_PyGuiApplication_PTR,
        is_PyGuiApplication_PythonToCpp_PyGuiApplication_PTR_Convertible,
        PyGuiApplication_PTR_CppToPython_PyGuiApplication);

    Shiboken::Conversions::registerConverterName(converter, "PyGuiApplication");
    Shiboken::Conversions::registerConverterName(converter, "PyGuiApplication*");
    Shiboken::Conversions::registerConverterName(converter, "PyGuiApplication&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::PyGuiApplication).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::PyGuiApplicationWrapper).name());


    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(&Sbk_PyGuiApplication_Type, &Sbk_PyGuiApplication_typeDiscovery);


}
