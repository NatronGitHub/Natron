
#include <sbkpython.h>
#include "Global/Macros.h"
CLANG_DIAG_OFF(mismatched-tags)
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_OFF(missing-field-initializers)
GCC_DIAG_OFF(missing-declarations)
#include <shiboken.h> // produces many warnings
#include <algorithm>
#include <pyside.h>
#include "natrongui_python.h"



// Extra includes

// Current module's type array.
PyTypeObject** SbkNatronGuiTypes;
// Current module's converter array.
SbkConverter** SbkNatronGuiTypeConverters;
void cleanGuiTypesAttributes(void) {
    for (int i = 0, imax = SBK_NatronGui_IDX_COUNT; i < imax; i++) {
        PyObject *pyType = reinterpret_cast<PyObject*>(SbkNatronGuiTypes[i]);
        if (pyType && PyObject_HasAttrString(pyType, "staticMetaObject"))
            PyObject_SetAttrString(pyType, "staticMetaObject", Py_None);
    }
}
// Global functions ------------------------------------------------------------

static PyMethodDef NatronGui_methods[] = {
    {0} // Sentinel
};

// Classes initialization functions ------------------------------------------------------------
void init_GuiApp(PyObject* module);
void init_PyGuiApplication(PyObject* module);
void init_PyViewer(PyObject* module);
void init_PyTabWidget(PyObject* module);
void init_PyModalDialog(PyObject* module);
void init_PyPanel(PyObject* module);

// Required modules' type and converter arrays.
PyTypeObject** SbkPySide_QtGuiTypes;
SbkConverter** SbkPySide_QtGuiTypeConverters;

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

// C++ to Python conversion for type 'const std::vector<std::string > &'.
static PyObject* _conststd_vector_std_string_REF_CppToPython__conststd_vector_std_string_REF(const void* cppIn) {
    ::std::vector<std::string >& cppInRef = *((::std::vector<std::string >*)cppIn);

                    // TEMPLATE - stdVectorToPyList - START
            ::std::vector<std::string >::size_type vectorSize = cppInRef.size();
            PyObject* pyOut = PyList_New((int) vectorSize);
            for (::std::vector<std::string >::size_type idx = 0; idx < vectorSize; ++idx) {
            ::std::string cppItem(cppInRef[idx]);
            PyList_SET_ITEM(pyOut, idx, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), &cppItem));
            }
            return pyOut;
        // TEMPLATE - stdVectorToPyList - END

}
static void _conststd_vector_std_string_REF_PythonToCpp__conststd_vector_std_string_REF(PyObject* pyIn, void* cppOut) {
    ::std::vector<std::string >& cppOutRef = *((::std::vector<std::string >*)cppOut);

                    // TEMPLATE - pySeqToStdVector - START
        int vectorSize = PySequence_Size(pyIn);
        cppOutRef.reserve(vectorSize);
        for (int idx = 0; idx < vectorSize; ++idx) {
        Shiboken::AutoDecRef pyItem(PySequence_GetItem(pyIn, idx));
        ::std::string cppItem;
        Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), pyItem, &(cppItem));
        cppOutRef.push_back(cppItem);
        }
    // TEMPLATE - pySeqToStdVector - END

}
static PythonToCppFunc is__conststd_vector_std_string_REF_PythonToCpp__conststd_vector_std_string_REF_Convertible(PyObject* pyIn) {
    if (Shiboken::Conversions::convertibleSequenceTypes(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), pyIn))
        return _conststd_vector_std_string_REF_PythonToCpp__conststd_vector_std_string_REF;
    return 0;
}

// C++ to Python conversion for type 'const std::list<int > &'.
static PyObject* _conststd_list_int_REF_CppToPython__conststd_list_int_REF(const void* cppIn) {
    ::std::list<int >& cppInRef = *((::std::list<int >*)cppIn);

                    // TEMPLATE - stdListToPyList - START
            PyObject* pyOut = PyList_New((int) cppInRef.size());
            ::std::list<int >::const_iterator it = cppInRef.begin();
            for (int idx = 0; it != cppInRef.end(); ++it, ++idx) {
            int cppItem(*it);
            PyList_SET_ITEM(pyOut, idx, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppItem));
            }
            return pyOut;
        // TEMPLATE - stdListToPyList - END

}
static void _conststd_list_int_REF_PythonToCpp__conststd_list_int_REF(PyObject* pyIn, void* cppOut) {
    ::std::list<int >& cppOutRef = *((::std::list<int >*)cppOut);

                    // TEMPLATE - pyListToStdList - START
        for (int i = 0; i < PySequence_Size(pyIn); i++) {
        Shiboken::AutoDecRef pyItem(PySequence_GetItem(pyIn, i));
        int cppItem;
        Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<int>(), pyItem, &(cppItem));
        cppOutRef.push_back(cppItem);
        }
    // TEMPLATE - pyListToStdList - END

}
static PythonToCppFunc is__conststd_list_int_REF_PythonToCpp__conststd_list_int_REF_Convertible(PyObject* pyIn) {
    if (Shiboken::Conversions::convertibleSequenceTypes(Shiboken::Conversions::PrimitiveTypeConverter<int>(), pyIn))
        return _conststd_list_int_REF_PythonToCpp__conststd_list_int_REF;
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

// C++ to Python conversion for type 'const QList<QObject * > &'.
static PyObject* _constQList_QObjectPTR_REF_CppToPython__constQList_QObjectPTR_REF(const void* cppIn) {
    ::QList<QObject * >& cppInRef = *((::QList<QObject * >*)cppIn);

                // TEMPLATE - cpplist_to_pylist_conversion - START
        PyObject* pyOut = PyList_New((int) cppInRef.size());
        ::QList<QObject * >::const_iterator it = cppInRef.begin();
        for (int idx = 0; it != cppInRef.end(); ++it, ++idx) {
            ::QObject* cppItem(*it);
            PyList_SET_ITEM(pyOut, idx, Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkPySide_QtCoreTypes[SBK_QOBJECT_IDX], cppItem));
        }
        return pyOut;
        // TEMPLATE - cpplist_to_pylist_conversion - END

}
static void _constQList_QObjectPTR_REF_PythonToCpp__constQList_QObjectPTR_REF(PyObject* pyIn, void* cppOut) {
    ::QList<QObject * >& cppOutRef = *((::QList<QObject * >*)cppOut);

                // TEMPLATE - pyseq_to_cpplist_conversion - START
    for (int i = 0; i < PySequence_Size(pyIn); i++) {
        Shiboken::AutoDecRef pyItem(PySequence_GetItem(pyIn, i));
        ::QObject* cppItem = ((::QObject*)0);
        Shiboken::Conversions::pythonToCppPointer((SbkObjectType*)SbkPySide_QtCoreTypes[SBK_QOBJECT_IDX], pyItem, &(cppItem));
        cppOutRef << cppItem;
    }
    // TEMPLATE - pyseq_to_cpplist_conversion - END

}
static PythonToCppFunc is__constQList_QObjectPTR_REF_PythonToCpp__constQList_QObjectPTR_REF_Convertible(PyObject* pyIn) {
    if (Shiboken::Conversions::checkSequenceTypes(SbkPySide_QtCoreTypes[SBK_QOBJECT_IDX], pyIn))
        return _constQList_QObjectPTR_REF_PythonToCpp__constQList_QObjectPTR_REF;
    return 0;
}

// C++ to Python conversion for type 'QList<QByteArray >'.
static PyObject* _QList_QByteArray__CppToPython__QList_QByteArray_(const void* cppIn) {
    ::QList<QByteArray >& cppInRef = *((::QList<QByteArray >*)cppIn);

                // TEMPLATE - cpplist_to_pylist_conversion - START
        PyObject* pyOut = PyList_New((int) cppInRef.size());
        ::QList<QByteArray >::const_iterator it = cppInRef.begin();
        for (int idx = 0; it != cppInRef.end(); ++it, ++idx) {
            ::QByteArray cppItem(*it);
            PyList_SET_ITEM(pyOut, idx, Shiboken::Conversions::copyToPython((SbkObjectType*)SbkPySide_QtCoreTypes[SBK_QBYTEARRAY_IDX], &cppItem));
        }
        return pyOut;
        // TEMPLATE - cpplist_to_pylist_conversion - END

}
static void _QList_QByteArray__PythonToCpp__QList_QByteArray_(PyObject* pyIn, void* cppOut) {
    ::QList<QByteArray >& cppOutRef = *((::QList<QByteArray >*)cppOut);

                // TEMPLATE - pyseq_to_cpplist_conversion - START
    for (int i = 0; i < PySequence_Size(pyIn); i++) {
        Shiboken::AutoDecRef pyItem(PySequence_GetItem(pyIn, i));
        ::QByteArray cppItem = ::QByteArray();
        Shiboken::Conversions::pythonToCppCopy((SbkObjectType*)SbkPySide_QtCoreTypes[SBK_QBYTEARRAY_IDX], pyItem, &(cppItem));
        cppOutRef << cppItem;
    }
    // TEMPLATE - pyseq_to_cpplist_conversion - END

}
static PythonToCppFunc is__QList_QByteArray__PythonToCpp__QList_QByteArray__Convertible(PyObject* pyIn) {
    if (Shiboken::Conversions::convertibleSequenceTypes((SbkObjectType*)SbkPySide_QtCoreTypes[SBK_QBYTEARRAY_IDX], pyIn))
        return _QList_QByteArray__PythonToCpp__QList_QByteArray_;
    return 0;
}

// C++ to Python conversion for type 'std::list<Param * >'.
static PyObject* _std_list_ParamPTR__CppToPython__std_list_ParamPTR_(const void* cppIn) {
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
static void _std_list_ParamPTR__PythonToCpp__std_list_ParamPTR_(PyObject* pyIn, void* cppOut) {
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
static PythonToCppFunc is__std_list_ParamPTR__PythonToCpp__std_list_ParamPTR__Convertible(PyObject* pyIn) {
    if (Shiboken::Conversions::checkSequenceTypes(SbkNatronEngineTypes[SBK_PARAM_IDX], pyIn))
        return _std_list_ParamPTR__PythonToCpp__std_list_ParamPTR_;
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
    init_PyViewer(module);
    init_PyTabWidget(module);
    init_PyModalDialog(module);
    init_PyPanel(module);

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

    // Register converter for type 'const std::vector<std::string>&'.
    SbkNatronGuiTypeConverters[SBK_NATRONGUI_STD_VECTOR_STD_STRING_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, _conststd_vector_std_string_REF_CppToPython__conststd_vector_std_string_REF);
    Shiboken::Conversions::registerConverterName(SbkNatronGuiTypeConverters[SBK_NATRONGUI_STD_VECTOR_STD_STRING_IDX], "const std::vector<std::string>&");
    Shiboken::Conversions::registerConverterName(SbkNatronGuiTypeConverters[SBK_NATRONGUI_STD_VECTOR_STD_STRING_IDX], "std::vector<std::string>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronGuiTypeConverters[SBK_NATRONGUI_STD_VECTOR_STD_STRING_IDX],
        _conststd_vector_std_string_REF_PythonToCpp__conststd_vector_std_string_REF,
        is__conststd_vector_std_string_REF_PythonToCpp__conststd_vector_std_string_REF_Convertible);

    // Register converter for type 'const std::list<int>&'.
    SbkNatronGuiTypeConverters[SBK_NATRONGUI_STD_LIST_INT_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, _conststd_list_int_REF_CppToPython__conststd_list_int_REF);
    Shiboken::Conversions::registerConverterName(SbkNatronGuiTypeConverters[SBK_NATRONGUI_STD_LIST_INT_IDX], "const std::list<int>&");
    Shiboken::Conversions::registerConverterName(SbkNatronGuiTypeConverters[SBK_NATRONGUI_STD_LIST_INT_IDX], "std::list<int>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronGuiTypeConverters[SBK_NATRONGUI_STD_LIST_INT_IDX],
        _conststd_list_int_REF_PythonToCpp__conststd_list_int_REF,
        is__conststd_list_int_REF_PythonToCpp__conststd_list_int_REF_Convertible);

    // Register converter for type 'QList<QAction*>'.
    SbkNatronGuiTypeConverters[SBK_NATRONGUI_QLIST_QACTIONPTR_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, _QList_QActionPTR__CppToPython__QList_QActionPTR_);
    Shiboken::Conversions::registerConverterName(SbkNatronGuiTypeConverters[SBK_NATRONGUI_QLIST_QACTIONPTR_IDX], "QList<QAction*>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronGuiTypeConverters[SBK_NATRONGUI_QLIST_QACTIONPTR_IDX],
        _QList_QActionPTR__PythonToCpp__QList_QActionPTR_,
        is__QList_QActionPTR__PythonToCpp__QList_QActionPTR__Convertible);

    // Register converter for type 'const QList<QObject*>&'.
    SbkNatronGuiTypeConverters[SBK_NATRONGUI_QLIST_QOBJECTPTR_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, _constQList_QObjectPTR_REF_CppToPython__constQList_QObjectPTR_REF);
    Shiboken::Conversions::registerConverterName(SbkNatronGuiTypeConverters[SBK_NATRONGUI_QLIST_QOBJECTPTR_IDX], "const QList<QObject*>&");
    Shiboken::Conversions::registerConverterName(SbkNatronGuiTypeConverters[SBK_NATRONGUI_QLIST_QOBJECTPTR_IDX], "QList<QObject*>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronGuiTypeConverters[SBK_NATRONGUI_QLIST_QOBJECTPTR_IDX],
        _constQList_QObjectPTR_REF_PythonToCpp__constQList_QObjectPTR_REF,
        is__constQList_QObjectPTR_REF_PythonToCpp__constQList_QObjectPTR_REF_Convertible);

    // Register converter for type 'QList<QByteArray>'.
    SbkNatronGuiTypeConverters[SBK_NATRONGUI_QLIST_QBYTEARRAY_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, _QList_QByteArray__CppToPython__QList_QByteArray_);
    Shiboken::Conversions::registerConverterName(SbkNatronGuiTypeConverters[SBK_NATRONGUI_QLIST_QBYTEARRAY_IDX], "QList<QByteArray>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronGuiTypeConverters[SBK_NATRONGUI_QLIST_QBYTEARRAY_IDX],
        _QList_QByteArray__PythonToCpp__QList_QByteArray_,
        is__QList_QByteArray__PythonToCpp__QList_QByteArray__Convertible);

    // Register converter for type 'std::list<Param*>'.
    SbkNatronGuiTypeConverters[SBK_NATRONGUI_STD_LIST_PARAMPTR_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, _std_list_ParamPTR__CppToPython__std_list_ParamPTR_);
    Shiboken::Conversions::registerConverterName(SbkNatronGuiTypeConverters[SBK_NATRONGUI_STD_LIST_PARAMPTR_IDX], "std::list<Param*>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronGuiTypeConverters[SBK_NATRONGUI_STD_LIST_PARAMPTR_IDX],
        _std_list_ParamPTR__PythonToCpp__std_list_ParamPTR_,
        is__std_list_ParamPTR__PythonToCpp__std_list_ParamPTR__Convertible);

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
    PySide::registerCleanupFunction(cleanGuiTypesAttributes);
SBK_MODULE_INIT_FUNCTION_END
