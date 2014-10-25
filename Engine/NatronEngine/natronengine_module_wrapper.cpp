
#include <sbkpython.h>
#include <shiboken.h>
#include <algorithm>
#include "natronengine_python.h"

#include <GlobalFunctionsWrapper.h>


// Extra includes

// Current module's type array.
PyTypeObject** SbkNatronEngineTypes;
// Current module's converter array.
SbkConverter** SbkNatronEngineTypeConverters;
// Global functions ------------------------------------------------------------
static PyObject* SbkNatronEngineModule_getInstance(PyObject* self, PyObject* pyArg)
{
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: getInstance(int)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // getInstance(int)
    }

    // Function signature not found.
    if (overloadId == -1) goto SbkNatronEngineModule_getInstance_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getInstance(int)
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            App* cppResult = new App(getInstance(cppArg0));
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            pyResult = Shiboken::Object::newObject((SbkObjectType*)SbkNatronEngineTypes[SBK_APP_IDX], cppResult, true, true);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    SbkNatronEngineModule_getInstance_TypeError:
        const char* overloads[] = {"int", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "getInstance", overloads);
        return 0;
}

static PyObject* SbkNatronEngineModule_getNumInstances(PyObject* self)
{
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getNumInstances()
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            int cppResult = getNumInstances();
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

static PyObject* SbkNatronEngineModule_getPluginIDs(PyObject* self)
{
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getPluginIDs()
            PyThreadState* _save = PyEval_SaveThread(); // Py_BEGIN_ALLOW_THREADS
            std::list<std::string > cppResult = getPluginIDs();
            PyEval_RestoreThread(_save); // Py_END_ALLOW_THREADS
            pyResult = Shiboken::Conversions::copyToPython(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_STD_STRING_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}


static PyMethodDef NatronEngine_methods[] = {
    {"getInstance", (PyCFunction)SbkNatronEngineModule_getInstance, METH_O},
    {"getNumInstances", (PyCFunction)SbkNatronEngineModule_getNumInstances, METH_NOARGS},
    {"getPluginIDs", (PyCFunction)SbkNatronEngineModule_getPluginIDs, METH_NOARGS},
    {0} // Sentinel
};

// Classes initialization functions ------------------------------------------------------------
void init_App(PyObject* module);
void init_Effect(PyObject* module);
void init_Param(PyObject* module);
void init_IntParam(PyObject* module);
void init_Natron(PyObject* module);

// Required modules' type and converter arrays.
PyTypeObject** SbkPySide_QtCoreTypes;
SbkConverter** SbkPySide_QtCoreTypeConverters;

// Module initialization ------------------------------------------------------------

// Primitive Type converters.

// C++ to Python conversion for type 'std::size_t'.
static PyObject* std_size_t_CppToPython_std_size_t(const void* cppIn) {
    ::std::size_t& cppInRef = *((::std::size_t*)cppIn);

                    return PyLong_FromSize_t(cppInRef);

}
// Python to C++ conversions for type 'std::size_t'.
static void PyLong_PythonToCpp_std_size_t(PyObject* pyIn, void* cppOut) {

    *((::std::size_t*)cppOut) = std::size_t(PyLong_AsSsize_t(pyIn));

}
static PythonToCppFunc is_PyLong_PythonToCpp_std_size_t_Convertible(PyObject* pyIn) {
    if (PyLong_Check(pyIn))
        return PyLong_PythonToCpp_std_size_t;
    return 0;
}


// Container Type converters.

// C++ to Python conversion for type 'std::list<std::string >'.
static PyObject* std_list_std_string__CppToPython_std_list_std_string_(const void* cppIn) {
    ::std::list<std::string >& cppInRef = *((::std::list<std::string >*)cppIn);

                    // TEMPLATE - stdListToPyList - START
            PyObject* pyOut = PyList_New((int) cppInRef.size());
            ::std::list<std::string >::const_iterator it = cppInRef.begin();
            for (int idx = 0; it != cppInRef.end(); ++it, ++idx) {
            ::std::string cppItem(*it);
            PyList_SET_ITEM(pyOut, idx, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), &cppItem));
            }
            return pyOut;
        // TEMPLATE - stdListToPyList - END

}
static void std_list_std_string__PythonToCpp_std_list_std_string_(PyObject* pyIn, void* cppOut) {
    ::std::list<std::string >& cppOutRef = *((::std::list<std::string >*)cppOut);

                    // TEMPLATE - pyListToStdList - START
        for (int i = 0; i < PySequence_Size(pyIn); i++) {
        Shiboken::AutoDecRef pyItem(PySequence_GetItem(pyIn, i));
        ::std::string cppItem;
        Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), pyItem, &(cppItem));
        cppOutRef.push_back(cppItem);
        }
    // TEMPLATE - pyListToStdList - END

}
static PythonToCppFunc is_std_list_std_string__PythonToCpp_std_list_std_string__Convertible(PyObject* pyIn) {
    if (Shiboken::Conversions::convertibleSequenceTypes(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), pyIn))
        return std_list_std_string__PythonToCpp_std_list_std_string_;
    return 0;
}

// C++ to Python conversion for type 'std::list<Param * >'.
static PyObject* std_list_ParamPTR__CppToPython_std_list_ParamPTR_(const void* cppIn) {
    ::std::list<Param * >& cppInRef = *((::std::list<Param * >*)cppIn);

                    // TEMPLATE - stdListToPyList - START
            PyObject* pyOut = PyList_New((int) cppInRef.size());
            ::std::list<Param * >::const_iterator it = cppInRef.begin();
            for (int idx = 0; it != cppInRef.end(); ++it, ++idx) {
            ::Param* cppItem(*it);
            PyList_SET_ITEM(pyOut, idx, Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_PARAM_IDX], cppItem));
            }
            return pyOut;
        // TEMPLATE - stdListToPyList - END

}
static void std_list_ParamPTR__PythonToCpp_std_list_ParamPTR_(PyObject* pyIn, void* cppOut) {
    ::std::list<Param * >& cppOutRef = *((::std::list<Param * >*)cppOut);

                    // TEMPLATE - pyListToStdList - START
        for (int i = 0; i < PySequence_Size(pyIn); i++) {
        Shiboken::AutoDecRef pyItem(PySequence_GetItem(pyIn, i));
        ::Param* cppItem = ((::Param*)0);
        Shiboken::Conversions::pythonToCppPointer((SbkObjectType*)SbkNatronEngineTypes[SBK_PARAM_IDX], pyItem, &(cppItem));
        cppOutRef.push_back(cppItem);
        }
    // TEMPLATE - pyListToStdList - END

}
static PythonToCppFunc is_std_list_ParamPTR__PythonToCpp_std_list_ParamPTR__Convertible(PyObject* pyIn) {
    if (Shiboken::Conversions::checkSequenceTypes(SbkNatronEngineTypes[SBK_PARAM_IDX], pyIn))
        return std_list_ParamPTR__PythonToCpp_std_list_ParamPTR_;
    return 0;
}

// C++ to Python conversion for type 'QList<QVariant >'.
static PyObject* _QList_QVariant__CppToPython__QList_QVariant_(const void* cppIn) {
    ::QList<QVariant >& cppInRef = *((::QList<QVariant >*)cppIn);

                // TEMPLATE - cpplist_to_pylist_conversion - START
        PyObject* pyOut = PyList_New((int) cppInRef.size());
        ::QList<QVariant >::const_iterator it = cppInRef.begin();
        for (int idx = 0; it != cppInRef.end(); ++it, ++idx) {
            ::QVariant cppItem(*it);
            PyList_SET_ITEM(pyOut, idx, Shiboken::Conversions::copyToPython(SbkPySide_QtCoreTypeConverters[SBK_QVARIANT_IDX], &cppItem));
        }
        return pyOut;
        // TEMPLATE - cpplist_to_pylist_conversion - END

}
static void _QList_QVariant__PythonToCpp__QList_QVariant_(PyObject* pyIn, void* cppOut) {
    ::QList<QVariant >& cppOutRef = *((::QList<QVariant >*)cppOut);

                // TEMPLATE - pyseq_to_cpplist_conversion - START
    for (int i = 0; i < PySequence_Size(pyIn); i++) {
        Shiboken::AutoDecRef pyItem(PySequence_GetItem(pyIn, i));
        ::QVariant cppItem = ::QVariant();
        Shiboken::Conversions::pythonToCppCopy(SbkPySide_QtCoreTypeConverters[SBK_QVARIANT_IDX], pyItem, &(cppItem));
        cppOutRef << cppItem;
    }
    // TEMPLATE - pyseq_to_cpplist_conversion - END

}
static PythonToCppFunc is__QList_QVariant__PythonToCpp__QList_QVariant__Convertible(PyObject* pyIn) {
    if (Shiboken::Conversions::convertibleSequenceTypes(SbkPySide_QtCoreTypeConverters[SBK_QVARIANT_IDX], pyIn))
        return _QList_QVariant__PythonToCpp__QList_QVariant_;
    return 0;
}

// C++ to Python conversion for type 'QList<QString >'.
static PyObject* _QList_QString__CppToPython__QList_QString_(const void* cppIn) {
    ::QList<QString >& cppInRef = *((::QList<QString >*)cppIn);

                // TEMPLATE - cpplist_to_pylist_conversion - START
        PyObject* pyOut = PyList_New((int) cppInRef.size());
        ::QList<QString >::const_iterator it = cppInRef.begin();
        for (int idx = 0; it != cppInRef.end(); ++it, ++idx) {
            ::QString cppItem(*it);
            PyList_SET_ITEM(pyOut, idx, Shiboken::Conversions::copyToPython(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppItem));
        }
        return pyOut;
        // TEMPLATE - cpplist_to_pylist_conversion - END

}
static void _QList_QString__PythonToCpp__QList_QString_(PyObject* pyIn, void* cppOut) {
    ::QList<QString >& cppOutRef = *((::QList<QString >*)cppOut);

                // TEMPLATE - pyseq_to_cpplist_conversion - START
    for (int i = 0; i < PySequence_Size(pyIn); i++) {
        Shiboken::AutoDecRef pyItem(PySequence_GetItem(pyIn, i));
        ::QString cppItem = ::QString();
        Shiboken::Conversions::pythonToCppCopy(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], pyItem, &(cppItem));
        cppOutRef << cppItem;
    }
    // TEMPLATE - pyseq_to_cpplist_conversion - END

}
static PythonToCppFunc is__QList_QString__PythonToCpp__QList_QString__Convertible(PyObject* pyIn) {
    if (Shiboken::Conversions::convertibleSequenceTypes(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], pyIn))
        return _QList_QString__PythonToCpp__QList_QString_;
    return 0;
}

// C++ to Python conversion for type 'QMap<QString, QVariant >'.
static PyObject* _QMap_QString_QVariant__CppToPython__QMap_QString_QVariant_(const void* cppIn) {
    ::QMap<QString, QVariant >& cppInRef = *((::QMap<QString, QVariant >*)cppIn);

                // TEMPLATE - cppmap_to_pymap_conversion - START
        PyObject* pyOut = PyDict_New();
        ::QMap<QString, QVariant >::const_iterator it = cppInRef.begin();
        for (; it != cppInRef.end(); ++it) {
            ::QString key = it.key();
            ::QVariant value = it.value();
            PyObject* pyKey = Shiboken::Conversions::copyToPython(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], &key);
            PyObject* pyValue = Shiboken::Conversions::copyToPython(SbkPySide_QtCoreTypeConverters[SBK_QVARIANT_IDX], &value);
            PyDict_SetItem(pyOut, pyKey, pyValue);
            Py_DECREF(pyKey);
            Py_DECREF(pyValue);
        }
        return pyOut;
      // TEMPLATE - cppmap_to_pymap_conversion - END

}
static void _QMap_QString_QVariant__PythonToCpp__QMap_QString_QVariant_(PyObject* pyIn, void* cppOut) {
    ::QMap<QString, QVariant >& cppOutRef = *((::QMap<QString, QVariant >*)cppOut);

                // TEMPLATE - pydict_to_cppmap_conversion - START
    PyObject* key;
    PyObject* value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(pyIn, &pos, &key, &value)) {
        ::QString cppKey = ::QString();
        Shiboken::Conversions::pythonToCppCopy(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], key, &(cppKey));
        ::QVariant cppValue = ::QVariant();
        Shiboken::Conversions::pythonToCppCopy(SbkPySide_QtCoreTypeConverters[SBK_QVARIANT_IDX], value, &(cppValue));
        cppOutRef.insert(cppKey, cppValue);
    }
    // TEMPLATE - pydict_to_cppmap_conversion - END

}
static PythonToCppFunc is__QMap_QString_QVariant__PythonToCpp__QMap_QString_QVariant__Convertible(PyObject* pyIn) {
    if (Shiboken::Conversions::convertibleDictTypes(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], false, SbkPySide_QtCoreTypeConverters[SBK_QVARIANT_IDX], false, pyIn))
        return _QMap_QString_QVariant__PythonToCpp__QMap_QString_QVariant_;
    return 0;
}


#if defined _WIN32 || defined __CYGWIN__
    #define SBK_EXPORT_MODULE __declspec(dllexport)
#elif __GNUC__ >= 4
    #define SBK_EXPORT_MODULE __attribute__ ((visibility("default")))
#else
    #define SBK_EXPORT_MODULE
#endif

#ifdef IS_PY3K
static struct PyModuleDef moduledef = {
    /* m_base     */ PyModuleDef_HEAD_INIT,
    /* m_name     */ "NatronEngine",
    /* m_doc      */ 0,
    /* m_size     */ -1,
    /* m_methods  */ NatronEngine_methods,
    /* m_reload   */ 0,
    /* m_traverse */ 0,
    /* m_clear    */ 0,
    /* m_free     */ 0
};

#endif
SBK_MODULE_INIT_FUNCTION_BEGIN(NatronEngine)
    {
        Shiboken::AutoDecRef requiredModule(Shiboken::Module::import("PySide.QtCore"));
        if (requiredModule.isNull())
            return SBK_MODULE_INIT_ERROR;
        SbkPySide_QtCoreTypes = Shiboken::Module::getTypes(requiredModule);
        SbkPySide_QtCoreTypeConverters = Shiboken::Module::getTypeConverters(requiredModule);
    }

    // Create an array of wrapper types for the current module.
    static PyTypeObject* cppApi[SBK_NatronEngine_IDX_COUNT];
    SbkNatronEngineTypes = cppApi;

    // Create an array of primitive type converters for the current module.
    static SbkConverter* sbkConverters[SBK_NatronEngine_CONVERTERS_IDX_COUNT];
    SbkNatronEngineTypeConverters = sbkConverters;

#ifdef IS_PY3K
    PyObject* module = Shiboken::Module::create("NatronEngine", &moduledef);
#else
    PyObject* module = Shiboken::Module::create("NatronEngine", NatronEngine_methods);
#endif

    // Initialize classes in the type system
    init_App(module);
    init_Effect(module);
    init_Param(module);
    init_IntParam(module);
    init_Natron(module);

    // Register converter for type 'NatronEngine.std::size_t'.
    SbkNatronEngineTypeConverters[SBK_STD_SIZE_T_IDX] = Shiboken::Conversions::createConverter(&PyLong_Type, std_size_t_CppToPython_std_size_t);
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_STD_SIZE_T_IDX], "std::size_t");
    // Add user defined implicit conversions to type converter.
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronEngineTypeConverters[SBK_STD_SIZE_T_IDX],
        PyLong_PythonToCpp_std_size_t,
        is_PyLong_PythonToCpp_std_size_t_Convertible);


    // Register converter for type 'std::list<std::string>'.
    SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_STD_STRING_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, std_list_std_string__CppToPython_std_list_std_string_);
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_STD_STRING_IDX], "std::list<std::string>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_STD_STRING_IDX],
        std_list_std_string__PythonToCpp_std_list_std_string_,
        is_std_list_std_string__PythonToCpp_std_list_std_string__Convertible);

    // Register converter for type 'std::list<Param*>'.
    SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_PARAMPTR_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, std_list_ParamPTR__CppToPython_std_list_ParamPTR_);
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_PARAMPTR_IDX], "std::list<Param*>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_PARAMPTR_IDX],
        std_list_ParamPTR__PythonToCpp_std_list_ParamPTR_,
        is_std_list_ParamPTR__PythonToCpp_std_list_ParamPTR__Convertible);

    // Register converter for type 'QList<QVariant>'.
    SbkNatronEngineTypeConverters[SBK_NATRONENGINE_QLIST_QVARIANT_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, _QList_QVariant__CppToPython__QList_QVariant_);
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_QLIST_QVARIANT_IDX], "QList<QVariant>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_QLIST_QVARIANT_IDX],
        _QList_QVariant__PythonToCpp__QList_QVariant_,
        is__QList_QVariant__PythonToCpp__QList_QVariant__Convertible);

    // Register converter for type 'QList<QString>'.
    SbkNatronEngineTypeConverters[SBK_NATRONENGINE_QLIST_QSTRING_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, _QList_QString__CppToPython__QList_QString_);
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_QLIST_QSTRING_IDX], "QList<QString>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_QLIST_QSTRING_IDX],
        _QList_QString__PythonToCpp__QList_QString_,
        is__QList_QString__PythonToCpp__QList_QString__Convertible);

    // Register converter for type 'QMap<QString,QVariant>'.
    SbkNatronEngineTypeConverters[SBK_NATRONENGINE_QMAP_QSTRING_QVARIANT_IDX] = Shiboken::Conversions::createConverter(&PyDict_Type, _QMap_QString_QVariant__CppToPython__QMap_QString_QVariant_);
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_QMAP_QSTRING_QVARIANT_IDX], "QMap<QString,QVariant>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_QMAP_QSTRING_QVARIANT_IDX],
        _QMap_QString_QVariant__PythonToCpp__QMap_QString_QVariant_,
        is__QMap_QString_QVariant__PythonToCpp__QMap_QString_QVariant__Convertible);

    // Register primitive types converters.

    Shiboken::Module::registerTypes(module, SbkNatronEngineTypes);
    Shiboken::Module::registerTypeConverters(module, SbkNatronEngineTypeConverters);

    if (PyErr_Occurred()) {
        PyErr_Print();
        Py_FatalError("can't initialize module NatronEngine");
    }
SBK_MODULE_INIT_FUNCTION_END
