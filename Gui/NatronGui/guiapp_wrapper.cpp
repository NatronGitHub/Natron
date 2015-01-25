
//workaround to access protected functions
#define protected public

// default includes
#include <shiboken.h>
#include <typeresolver.h>
#include <typeinfo>
#include "natrongui_python.h"

#include "guiapp_wrapper.h"

// Extra includes
#include <AppInstanceWrapper.h>
#include <NodeGroupWrapper.h>
#include <NodeWrapper.h>
#include <ParameterWrapper.h>
#include <PythonPanels.h>
#include <list>


// Native ---------------------------------------------------------

GuiAppWrapper::~GuiAppWrapper()
{
    SbkObject* wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject* Sbk_GuiAppFunc_createModalDialog(PyObject* self)
{
    ::GuiApp* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::GuiApp*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_GUIAPP_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // createModalDialog()
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            PyModalDialog * cppResult = cppSelf->createModalDialog();
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronGuiTypes[SBK_PYMODALDIALOG_IDX], cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyMethodDef Sbk_GuiApp_methods[] = {
    {"createModalDialog", (PyCFunction)Sbk_GuiAppFunc_createModalDialog, METH_NOARGS},

    {0} // Sentinel
};

} // extern "C"

static int Sbk_GuiApp_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_GuiApp_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_GuiApp_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronGui.GuiApp",
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
    /*tp_traverse*/         Sbk_GuiApp_traverse,
    /*tp_clear*/            Sbk_GuiApp_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_GuiApp_methods,
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

static void* Sbk_GuiApp_typeDiscovery(void* cptr, SbkObjectType* instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType*>(Shiboken::SbkType< ::Group >()))
        return dynamic_cast< ::GuiApp*>(reinterpret_cast< ::Group*>(cptr));
    return 0;
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void GuiApp_PythonToCpp_GuiApp_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_GuiApp_Type, pyIn, cppOut);
}
static PythonToCppFunc is_GuiApp_PythonToCpp_GuiApp_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_GuiApp_Type))
        return GuiApp_PythonToCpp_GuiApp_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* GuiApp_PTR_CppToPython_GuiApp(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::GuiApp*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_GuiApp_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_GuiApp(PyObject* module)
{
    SbkNatronGuiTypes[SBK_GUIAPP_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_GuiApp_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "GuiApp", "GuiApp*",
        &Sbk_GuiApp_Type, &Shiboken::callCppDestructor< ::GuiApp >, (SbkObjectType*)SbkNatronEngineTypes[SBK_APP_IDX])) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_GuiApp_Type,
        GuiApp_PythonToCpp_GuiApp_PTR,
        is_GuiApp_PythonToCpp_GuiApp_PTR_Convertible,
        GuiApp_PTR_CppToPython_GuiApp);

    Shiboken::Conversions::registerConverterName(converter, "GuiApp");
    Shiboken::Conversions::registerConverterName(converter, "GuiApp*");
    Shiboken::Conversions::registerConverterName(converter, "GuiApp&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::GuiApp).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::GuiAppWrapper).name());


    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(&Sbk_GuiApp_Type, &Sbk_GuiApp_typeDiscovery);


}
