
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

#include "appsettings_wrapper.h"

// Extra includes
NATRON_NAMESPACE_USING
#include <ParameterWrapper.h>
#include <list>



// Target ---------------------------------------------------------

extern "C" {
static PyObject* Sbk_AppSettingsFunc_getParam(PyObject* self, PyObject* pyArg)
{
    ::AppSettings* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::AppSettings*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_APPSETTINGS_IDX], (SbkObject*)self));
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
    if (overloadId == -1) goto Sbk_AppSettingsFunc_getParam_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getParam(std::string)const
            Param * cppResult = const_cast<const ::AppSettings*>(cppSelf)->getParam(cppArg0);
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

    Sbk_AppSettingsFunc_getParam_TypeError:
        const char* overloads[] = {"std::string", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.AppSettings.getParam", overloads);
        return 0;
}

static PyObject* Sbk_AppSettingsFunc_getParams(PyObject* self)
{
    ::AppSettings* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::AppSettings*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_APPSETTINGS_IDX], (SbkObject*)self));
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

static PyObject* Sbk_AppSettingsFunc_restoreDefaultSettings(PyObject* self)
{
    ::AppSettings* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::AppSettings*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_APPSETTINGS_IDX], (SbkObject*)self));

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // restoreDefaultSettings()
            cppSelf->restoreDefaultSettings();
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;
}

static PyObject* Sbk_AppSettingsFunc_saveSettings(PyObject* self)
{
    ::AppSettings* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::AppSettings*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_APPSETTINGS_IDX], (SbkObject*)self));

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // saveSettings()
            cppSelf->saveSettings();
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;
}

static PyMethodDef Sbk_AppSettings_methods[] = {
    {"getParam", (PyCFunction)Sbk_AppSettingsFunc_getParam, METH_O},
    {"getParams", (PyCFunction)Sbk_AppSettingsFunc_getParams, METH_NOARGS},
    {"restoreDefaultSettings", (PyCFunction)Sbk_AppSettingsFunc_restoreDefaultSettings, METH_NOARGS},
    {"saveSettings", (PyCFunction)Sbk_AppSettingsFunc_saveSettings, METH_NOARGS},

    {0} // Sentinel
};

} // extern "C"

static int Sbk_AppSettings_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_AppSettings_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_AppSettings_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.AppSettings",
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
    /*tp_traverse*/         Sbk_AppSettings_traverse,
    /*tp_clear*/            Sbk_AppSettings_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_AppSettings_methods,
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
static void AppSettings_PythonToCpp_AppSettings_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_AppSettings_Type, pyIn, cppOut);
}
static PythonToCppFunc is_AppSettings_PythonToCpp_AppSettings_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_AppSettings_Type))
        return AppSettings_PythonToCpp_AppSettings_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* AppSettings_PTR_CppToPython_AppSettings(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::AppSettings*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_AppSettings_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_AppSettings(PyObject* module)
{
    SbkNatronEngineTypes[SBK_APPSETTINGS_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_AppSettings_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "AppSettings", "AppSettings*",
        &Sbk_AppSettings_Type, &Shiboken::callCppDestructor< ::AppSettings >)) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_AppSettings_Type,
        AppSettings_PythonToCpp_AppSettings_PTR,
        is_AppSettings_PythonToCpp_AppSettings_PTR_Convertible,
        AppSettings_PTR_CppToPython_AppSettings);

    Shiboken::Conversions::registerConverterName(converter, "AppSettings");
    Shiboken::Conversions::registerConverterName(converter, "AppSettings*");
    Shiboken::Conversions::registerConverterName(converter, "AppSettings&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::AppSettings).name());



}
