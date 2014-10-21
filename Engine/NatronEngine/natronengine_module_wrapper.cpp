
#include <sbkpython.h>
#include <shiboken.h>
#include <algorithm>
#include "natronengine_python.h"



// Extra includes

// Current module's type array.
PyTypeObject** SbkNatronEngineTypes;
// Current module's converter array.
SbkConverter** SbkNatronEngineTypeConverters;
// Global functions ------------------------------------------------------------

static PyMethodDef NatronEngine_methods[] = {
    {0} // Sentinel
};

// Classes initialization functions ------------------------------------------------------------

// Required modules' type and converter arrays.
PyTypeObject** SbkPySide_QtCoreTypes;
SbkConverter** SbkPySide_QtCoreTypeConverters;

// Module initialization ------------------------------------------------------------
// Container Type converters.

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
