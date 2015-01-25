
#include <sbkpython.h>
#include <shiboken.h>
#include <algorithm>
#include "natrongui_python.h"



// Extra includes

// Current module's type array.
PyTypeObject** SbkNatronGuiTypes;
// Current module's converter array.
SbkConverter** SbkNatronGuiTypeConverters;
// Global functions ------------------------------------------------------------

static PyMethodDef NatronGui_methods[] = {
    {0} // Sentinel
};

// Classes initialization functions ------------------------------------------------------------
void init_GuiApp(PyObject* module);
void init_PyGuiApplication(PyObject* module);
void init_PyModalDialog(PyObject* module);

// Required modules' type and converter arrays.
PyTypeObject** SbkPySide_QtGuiTypes;
SbkConverter** SbkPySide_QtGuiTypeConverters;
/*PyTypeObject** SbkPySide_QtCoreTypes;
SbkConverter** SbkPySide_QtCoreTypeConverters;
PyTypeObject** SbkNatronEngineTypes;
SbkConverter** SbkNatronEngineTypeConverters;*/

// Module initialization ------------------------------------------------------------
// Container Type converters.

// C++ to Python conversion for type 'std::list<std::string >'.
static PyObject* _std_list_std_string__CppToPython__std_list_std_string_(const void* cppIn) {
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
static void _std_list_std_string__PythonToCpp__std_list_std_string_(PyObject* pyIn, void* cppOut) {
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
static PythonToCppFunc is__std_list_std_string__PythonToCpp__std_list_std_string__Convertible(PyObject* pyIn) {
    if (Shiboken::Conversions::convertibleSequenceTypes(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), pyIn))
        return _std_list_std_string__PythonToCpp__std_list_std_string_;
    return 0;
}

// C++ to Python conversion for type 'std::list<Effect * >'.
static PyObject* _std_list_EffectPTR__CppToPython__std_list_EffectPTR_(const void* cppIn) {
    ::std::list<Effect * >& cppInRef = *((::std::list<Effect * >*)cppIn);

                    // TEMPLATE - stdListToPyList - START
            PyObject* pyOut = PyList_New((int) cppInRef.size());
            ::std::list<Effect * >::const_iterator it = cppInRef.begin();
            for (int idx = 0; it != cppInRef.end(); ++it, ++idx) {
            ::Effect* cppItem(*it);
            PyList_SET_ITEM(pyOut, idx, Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_EFFECT_IDX], cppItem));
            }
            return pyOut;
        // TEMPLATE - stdListToPyList - END

}
static void _std_list_EffectPTR__PythonToCpp__std_list_EffectPTR_(PyObject* pyIn, void* cppOut) {
    ::std::list<Effect * >& cppOutRef = *((::std::list<Effect * >*)cppOut);

                    // TEMPLATE - pyListToStdList - START
        for (int i = 0; i < PySequence_Size(pyIn); i++) {
        Shiboken::AutoDecRef pyItem(PySequence_GetItem(pyIn, i));
        ::Effect* cppItem = ((::Effect*)0);
        Shiboken::Conversions::pythonToCppPointer((SbkObjectType*)SbkNatronEngineTypes[SBK_EFFECT_IDX], pyItem, &(cppItem));
        cppOutRef.push_back(cppItem);
        }
    // TEMPLATE - pyListToStdList - END

}
static PythonToCppFunc is__std_list_EffectPTR__PythonToCpp__std_list_EffectPTR__Convertible(PyObject* pyIn) {
    if (Shiboken::Conversions::checkSequenceTypes(SbkNatronEngineTypes[SBK_EFFECT_IDX], pyIn))
        return _std_list_EffectPTR__PythonToCpp__std_list_EffectPTR_;
    return 0;
}

// C++ to Python conversion for type 'const std::list<RenderTask > &'.
static PyObject* _conststd_list_RenderTask_REF_CppToPython__conststd_list_RenderTask_REF(const void* cppIn) {
    ::std::list<RenderTask >& cppInRef = *((::std::list<RenderTask >*)cppIn);

                    // TEMPLATE - stdListToPyList - START
            PyObject* pyOut = PyList_New((int) cppInRef.size());
            ::std::list<RenderTask >::const_iterator it = cppInRef.begin();
            for (int idx = 0; it != cppInRef.end(); ++it, ++idx) {
            ::RenderTask cppItem(*it);
            PyList_SET_ITEM(pyOut, idx, Shiboken::Conversions::copyToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_RENDERTASK_IDX], &cppItem));
            }
            return pyOut;
        // TEMPLATE - stdListToPyList - END

}
static void _conststd_list_RenderTask_REF_PythonToCpp__conststd_list_RenderTask_REF(PyObject* pyIn, void* cppOut) {
    ::std::list<RenderTask >& cppOutRef = *((::std::list<RenderTask >*)cppOut);

                    // TEMPLATE - pyListToStdList - START
        for (int i = 0; i < PySequence_Size(pyIn); i++) {
        Shiboken::AutoDecRef pyItem(PySequence_GetItem(pyIn, i));
        ::RenderTask cppItem = ::RenderTask();
        Shiboken::Conversions::pythonToCppCopy((SbkObjectType*)SbkNatronEngineTypes[SBK_RENDERTASK_IDX], pyItem, &(cppItem));
        cppOutRef.push_back(cppItem);
        }
    // TEMPLATE - pyListToStdList - END

}
static PythonToCppFunc is__conststd_list_RenderTask_REF_PythonToCpp__conststd_list_RenderTask_REF_Convertible(PyObject* pyIn) {
    if (Shiboken::Conversions::convertibleSequenceTypes((SbkObjectType*)SbkNatronEngineTypes[SBK_RENDERTASK_IDX], pyIn))
        return _conststd_list_RenderTask_REF_PythonToCpp__conststd_list_RenderTask_REF;
    return 0;
}

// C++ to Python conversion for type 'QList<QAction * >'.
static PyObject* _QList_QActionPTR__CppToPython__QList_QActionPTR_(const void* cppIn) {
    ::QList<QAction * >& cppInRef = *((::QList<QAction * >*)cppIn);

                // TEMPLATE - cpplist_to_pylist_conversion - START
        PyObject* pyOut = PyList_New((int) cppInRef.size());
        ::QList<QAction * >::const_iterator it = cppInRef.begin();
        for (int idx = 0; it != cppInRef.end(); ++it, ++idx) {
            ::QAction* cppItem(*it);
            PyList_SET_ITEM(pyOut, idx, Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkPySide_QtGuiTypes[SBK_QACTION_IDX], cppItem));
        }
        return pyOut;
        // TEMPLATE - cpplist_to_pylist_conversion - END

}
static void _QList_QActionPTR__PythonToCpp__QList_QActionPTR_(PyObject* pyIn, void* cppOut) {
    ::QList<QAction * >& cppOutRef = *((::QList<QAction * >*)cppOut);

                // TEMPLATE - pyseq_to_cpplist_conversion - START
    for (int i = 0; i < PySequence_Size(pyIn); i++) {
        Shiboken::AutoDecRef pyItem(PySequence_GetItem(pyIn, i));
        ::QAction* cppItem = ((::QAction*)0);
        Shiboken::Conversions::pythonToCppPointer((SbkObjectType*)SbkPySide_QtGuiTypes[SBK_QACTION_IDX], pyItem, &(cppItem));
        cppOutRef << cppItem;
    }
    // TEMPLATE - pyseq_to_cpplist_conversion - END

}
static PythonToCppFunc is__QList_QActionPTR__PythonToCpp__QList_QActionPTR__Convertible(PyObject* pyIn) {
    if (Shiboken::Conversions::checkSequenceTypes(SbkPySide_QtGuiTypes[SBK_QACTION_IDX], pyIn))
        return _QList_QActionPTR__PythonToCpp__QList_QActionPTR_;
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
    /* m_name     */ "NatronGui",
    /* m_doc      */ 0,
    /* m_size     */ -1,
    /* m_methods  */ NatronGui_methods,
    /* m_reload   */ 0,
    /* m_traverse */ 0,
    /* m_clear    */ 0,
    /* m_free     */ 0
};

#endif
SBK_MODULE_INIT_FUNCTION_BEGIN(NatronGui)
    {
        Shiboken::AutoDecRef requiredModule(Shiboken::Module::import("PySide.QtGui"));
        if (requiredModule.isNull())
            return SBK_MODULE_INIT_ERROR;
        SbkPySide_QtGuiTypes = Shiboken::Module::getTypes(requiredModule);
        SbkPySide_QtGuiTypeConverters = Shiboken::Module::getTypeConverters(requiredModule);
    }

    {
        Shiboken::AutoDecRef requiredModule(Shiboken::Module::import("PySide.QtCore"));
        if (requiredModule.isNull())
            return SBK_MODULE_INIT_ERROR;
        SbkPySide_QtCoreTypes = Shiboken::Module::getTypes(requiredModule);
        SbkPySide_QtCoreTypeConverters = Shiboken::Module::getTypeConverters(requiredModule);
    }

    {
        Shiboken::AutoDecRef requiredModule(Shiboken::Module::import("NatronEngine"));
        if (requiredModule.isNull())
            return SBK_MODULE_INIT_ERROR;
        SbkNatronEngineTypes = Shiboken::Module::getTypes(requiredModule);
        SbkNatronEngineTypeConverters = Shiboken::Module::getTypeConverters(requiredModule);
    }

    // Create an array of wrapper types for the current module.
    static PyTypeObject* cppApi[SBK_NatronGui_IDX_COUNT];
    SbkNatronGuiTypes = cppApi;

    // Create an array of primitive type converters for the current module.
    static SbkConverter* sbkConverters[SBK_NatronGui_CONVERTERS_IDX_COUNT];
    SbkNatronGuiTypeConverters = sbkConverters;

#ifdef IS_PY3K
    PyObject* module = Shiboken::Module::create("NatronGui", &moduledef);
#else
    PyObject* module = Shiboken::Module::create("NatronGui", NatronGui_methods);
#endif

    // Initialize classes in the type system
    init_GuiApp(module);
    init_PyGuiApplication(module);
    init_PyModalDialog(module);

    // Register converter for type 'std::list<std::string>'.
    SbkNatronGuiTypeConverters[SBK_NATRONGUI_STD_LIST_STD_STRING_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, _std_list_std_string__CppToPython__std_list_std_string_);
    Shiboken::Conversions::registerConverterName(SbkNatronGuiTypeConverters[SBK_NATRONGUI_STD_LIST_STD_STRING_IDX], "std::list<std::string>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronGuiTypeConverters[SBK_NATRONGUI_STD_LIST_STD_STRING_IDX],
        _std_list_std_string__PythonToCpp__std_list_std_string_,
        is__std_list_std_string__PythonToCpp__std_list_std_string__Convertible);

    // Register converter for type 'std::list<Effect*>'.
    SbkNatronGuiTypeConverters[SBK_NATRONGUI_STD_LIST_EFFECTPTR_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, _std_list_EffectPTR__CppToPython__std_list_EffectPTR_);
    Shiboken::Conversions::registerConverterName(SbkNatronGuiTypeConverters[SBK_NATRONGUI_STD_LIST_EFFECTPTR_IDX], "std::list<Effect*>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronGuiTypeConverters[SBK_NATRONGUI_STD_LIST_EFFECTPTR_IDX],
        _std_list_EffectPTR__PythonToCpp__std_list_EffectPTR_,
        is__std_list_EffectPTR__PythonToCpp__std_list_EffectPTR__Convertible);

    // Register converter for type 'const std::list<RenderTask>&'.
    SbkNatronGuiTypeConverters[SBK_NATRONGUI_STD_LIST_RENDERTASK_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, _conststd_list_RenderTask_REF_CppToPython__conststd_list_RenderTask_REF);
    Shiboken::Conversions::registerConverterName(SbkNatronGuiTypeConverters[SBK_NATRONGUI_STD_LIST_RENDERTASK_IDX], "const std::list<RenderTask>&");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronGuiTypeConverters[SBK_NATRONGUI_STD_LIST_RENDERTASK_IDX],
        _conststd_list_RenderTask_REF_PythonToCpp__conststd_list_RenderTask_REF,
        is__conststd_list_RenderTask_REF_PythonToCpp__conststd_list_RenderTask_REF_Convertible);

    // Register converter for type 'QList<QAction*>'.
    SbkNatronGuiTypeConverters[SBK_NATRONGUI_QLIST_QACTIONPTR_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, _QList_QActionPTR__CppToPython__QList_QActionPTR_);
    Shiboken::Conversions::registerConverterName(SbkNatronGuiTypeConverters[SBK_NATRONGUI_QLIST_QACTIONPTR_IDX], "QList<QAction*>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronGuiTypeConverters[SBK_NATRONGUI_QLIST_QACTIONPTR_IDX],
        _QList_QActionPTR__PythonToCpp__QList_QActionPTR_,
        is__QList_QActionPTR__PythonToCpp__QList_QActionPTR__Convertible);

    // Register converter for type 'QList<QVariant>'.
    SbkNatronGuiTypeConverters[SBK_NATRONGUI_QLIST_QVARIANT_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, _QList_QVariant__CppToPython__QList_QVariant_);
    Shiboken::Conversions::registerConverterName(SbkNatronGuiTypeConverters[SBK_NATRONGUI_QLIST_QVARIANT_IDX], "QList<QVariant>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronGuiTypeConverters[SBK_NATRONGUI_QLIST_QVARIANT_IDX],
        _QList_QVariant__PythonToCpp__QList_QVariant_,
        is__QList_QVariant__PythonToCpp__QList_QVariant__Convertible);

    // Register converter for type 'QList<QString>'.
    SbkNatronGuiTypeConverters[SBK_NATRONGUI_QLIST_QSTRING_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, _QList_QString__CppToPython__QList_QString_);
    Shiboken::Conversions::registerConverterName(SbkNatronGuiTypeConverters[SBK_NATRONGUI_QLIST_QSTRING_IDX], "QList<QString>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronGuiTypeConverters[SBK_NATRONGUI_QLIST_QSTRING_IDX],
        _QList_QString__PythonToCpp__QList_QString_,
        is__QList_QString__PythonToCpp__QList_QString__Convertible);

    // Register converter for type 'QMap<QString,QVariant>'.
    SbkNatronGuiTypeConverters[SBK_NATRONGUI_QMAP_QSTRING_QVARIANT_IDX] = Shiboken::Conversions::createConverter(&PyDict_Type, _QMap_QString_QVariant__CppToPython__QMap_QString_QVariant_);
    Shiboken::Conversions::registerConverterName(SbkNatronGuiTypeConverters[SBK_NATRONGUI_QMAP_QSTRING_QVARIANT_IDX], "QMap<QString,QVariant>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronGuiTypeConverters[SBK_NATRONGUI_QMAP_QSTRING_QVARIANT_IDX],
        _QMap_QString_QVariant__PythonToCpp__QMap_QString_QVariant_,
        is__QMap_QString_QVariant__PythonToCpp__QMap_QString_QVariant__Convertible);

    // Register primitive types converters.

    Shiboken::Module::registerTypes(module, SbkNatronGuiTypes);
    Shiboken::Module::registerTypeConverters(module, SbkNatronGuiTypeConverters);

    if (PyErr_Occurred()) {
        PyErr_Print();
        Py_FatalError("can't initialize module NatronGui");
    }
SBK_MODULE_INIT_FUNCTION_END
