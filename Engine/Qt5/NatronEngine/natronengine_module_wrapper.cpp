
#include <sbkpython.h>
#include "Global/Macros.h"
CLANG_DIAG_OFF(mismatched-tags)
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_OFF(missing-field-initializers)
GCC_DIAG_OFF(missing-declarations)
GCC_DIAG_OFF(uninitialized)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <pysidesignal.h>
#include <shiboken.h> // produces many warnings
#include <algorithm>
#include <signature.h>
#ifndef QT_NO_VERSION_TAGGING
#  define QT_NO_VERSION_TAGGING
#endif
#include <QDebug>
#include <pyside.h>
#include <pysideqenum.h>
#include <feature_select.h>
#include <qapp_macro.h>
#include "natronengine_python.h"



// Extra includes
NATRON_NAMESPACE_USING NATRON_PYTHON_NAMESPACE_USING

// Current module's type array.
PyTypeObject **SbkNatronEngineTypes = nullptr;
// Current module's PyObject pointer.
PyObject *SbkNatronEngineModuleObject = nullptr;
// Current module's converter array.
SbkConverter **SbkNatronEngineTypeConverters = nullptr;
void cleanTypesAttributes(void) {
    if (PY_VERSION_HEX >= 0x03000000 && PY_VERSION_HEX < 0x03060000)
        return; // PYSIDE-953: testbinding crashes in Python 3.5 when hasattr touches types!
    for (int i = 0, imax = SBK_NatronEngine_IDX_COUNT; i < imax; i++) {
        PyObject *pyType = reinterpret_cast<PyObject *>(SbkNatronEngineTypes[i]);
        Shiboken::AutoDecRef attrName(Py_BuildValue("s", "staticMetaObject"));
        if (pyType && PyObject_HasAttr(pyType, attrName))
            PyObject_SetAttr(pyType, attrName, Py_None);
    }
}
// Global functions ------------------------------------------------------------

static PyMethodDef NatronEngine_methods[] = {
    {0} // Sentinel
};

// Classes initialization functions ------------------------------------------------------------
void init_UserParamHolder(PyObject *module);
void init_Tracker(PyObject *module);
void init_Track(PyObject *module);
void init_Roto(PyObject *module);
void init_RectI(PyObject *module);
void init_RectD(PyObject *module);
void init_PyCoreApplication(PyObject *module);
void init_Param(PyObject *module);
void init_SeparatorParam(PyObject *module);
void init_ParametricParam(PyObject *module);
void init_PageParam(PyObject *module);
void init_NodeCreationProperty(PyObject *module);
void init_StringNodeCreationProperty(PyObject *module);
void init_NatronEngineNATRON_ENUM(PyObject *module);
void init_ItemBase(PyObject *module);
void init_Layer(PyObject *module);
void init_IntNodeCreationProperty(PyObject *module);
void init_Int3DTuple(PyObject *module);
void init_Int2DTuple(PyObject *module);
void init_ImageLayer(PyObject *module);
void init_GroupParam(PyObject *module);
void init_Group(PyObject *module);
void init_FloatNodeCreationProperty(PyObject *module);
void init_ExprUtils(PyObject *module);
void init_Effect(PyObject *module);
void init_Double3DTuple(PyObject *module);
void init_Double2DTuple(PyObject *module);
void init_ColorTuple(PyObject *module);
void init_ButtonParam(PyObject *module);
void init_BoolNodeCreationProperty(PyObject *module);
void init_BezierCurve(PyObject *module);
void init_AppSettings(PyObject *module);
void init_App(PyObject *module);
void init_AnimatedParam(PyObject *module);
void init_DoubleParam(PyObject *module);
void init_Double2DParam(PyObject *module);
void init_Double3DParam(PyObject *module);
void init_StringParamBase(PyObject *module);
void init_OutputFileParam(PyObject *module);
void init_StringParam(PyObject *module);
void init_PathParam(PyObject *module);
void init_FileParam(PyObject *module);
void init_IntParam(PyObject *module);
void init_Int2DParam(PyObject *module);
void init_Int3DParam(PyObject *module);
void init_ColorParam(PyObject *module);
void init_ChoiceParam(PyObject *module);
void init_BooleanParam(PyObject *module);

// Required modules' type and converter arrays.
PyTypeObject **SbkPySide2_QtCoreTypes;
SbkConverter **SbkPySide2_QtCoreTypeConverters;

// Module initialization ------------------------------------------------------------

// Primitive Type converters.

// C++ to Python conversion for type 'std::size_t'.
static PyObject *std_size_t_CppToPython_std_size_t(const void *cppIn) {
    auto &cppInRef = *reinterpret_cast<::std::size_t *>(const_cast<void *>(cppIn));
    return PyLong_FromSize_t(cppInRef);

}
// Python to C++ conversions for type 'std::size_t'.
static void PyLong_PythonToCpp_std_size_t(PyObject *pyIn, void *cppOut) {
    *reinterpret_cast<::std::size_t *>(cppOut) = std::size_t(PyLong_AsSsize_t(pyIn));

}
static PythonToCppFunc is_PyLong_PythonToCpp_std_size_t_Convertible(PyObject *pyIn) {
    if (PyLong_Check(pyIn))
        return PyLong_PythonToCpp_std_size_t;
    return {};
}


// Container Type converters.

// C++ to Python conversion for type 'const std::map<QString,NodeCreationProperty* > &'.
static PyObject *conststd_map_QString_NodeCreationPropertyPTR_REF_CppToPython_conststd_map_QString_NodeCreationPropertyPTR_REF(const void *cppIn) {
    auto &cppInRef = *reinterpret_cast<::std::map<QString,NodeCreationProperty* > *>(const_cast<void *>(cppIn));
    // TEMPLATE - stdMapToPyDict - START
    PyObject* pyOut = PyDict_New();
    ::std::map<QString,NodeCreationProperty* >::const_iterator it = cppInRef.begin();
    for (; it != cppInRef.end(); ++it) {
    ::QString key = it->first;
    ::NodeCreationProperty* value = it->second;
    PyObject* pyKey = Shiboken::Conversions::copyToPython(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], &key);
    PyObject* pyValue = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_NODECREATIONPROPERTY_IDX]), value);
    PyDict_SetItem(pyOut, pyKey, pyValue);
    Py_DECREF(pyKey);
    Py_DECREF(pyValue);
    }
    return pyOut;
    // TEMPLATE - stdMapToPyDict - END

}
static void conststd_map_QString_NodeCreationPropertyPTR_REF_PythonToCpp_conststd_map_QString_NodeCreationPropertyPTR_REF(PyObject *pyIn, void *cppOut) {
    auto &cppOutRef = *reinterpret_cast<::std::map<QString,NodeCreationProperty* > *>(cppOut);
    // TEMPLATE - pyDictToStdMap - START
    PyObject* key;
    PyObject* value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(pyIn, &pos, &key, &value)) {
    ::QString cppKey;
        Shiboken::Conversions::pythonToCppCopy(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], key, &(cppKey));
    ::NodeCreationProperty* cppValue{nullptr};
        Shiboken::Conversions::pythonToCppPointer(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_NODECREATIONPROPERTY_IDX]), value, &(cppValue));
    cppOutRef.insert(std::make_pair(cppKey, cppValue));
    }
    // TEMPLATE - pyDictToStdMap - END

}
static PythonToCppFunc is_conststd_map_QString_NodeCreationPropertyPTR_REF_PythonToCpp_conststd_map_QString_NodeCreationPropertyPTR_REF_Convertible(PyObject *pyIn) {
    if (Shiboken::Conversions::convertibleDictTypes(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], false, *PepType_SGTP(SbkNatronEngineTypes[SBK_NODECREATIONPROPERTY_IDX])->converter, true, pyIn))
        return conststd_map_QString_NodeCreationPropertyPTR_REF_PythonToCpp_conststd_map_QString_NodeCreationPropertyPTR_REF;
    return {};
}

// C++ to Python conversion for type 'std::list<Effect* >'.
static PyObject *std_list_EffectPTR__CppToPython_std_list_EffectPTR_(const void *cppIn) {
    auto &cppInRef = *reinterpret_cast<::std::list<Effect* > *>(const_cast<void *>(cppIn));
    // TEMPLATE - stdListToPyList - START
    PyObject* pyOut = PyList_New((int) cppInRef.size());
    ::std::list<Effect* >::const_iterator it = cppInRef.begin();
    for (int idx = 0; it != cppInRef.end(); ++it, ++idx) {
    ::Effect* cppItem(*it);
    PyList_SET_ITEM(pyOut, idx, Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_EFFECT_IDX]), cppItem));
    }
    return pyOut;
    // TEMPLATE - stdListToPyList - END

}
static void std_list_EffectPTR__PythonToCpp_std_list_EffectPTR_(PyObject *pyIn, void *cppOut) {
    auto &cppOutRef = *reinterpret_cast<::std::list<Effect* > *>(cppOut);
    // TEMPLATE - pyListToStdList - START
    for (int i = 0; i < PySequence_Size(pyIn); i++) {
    Shiboken::AutoDecRef pyItem(PySequence_GetItem(pyIn, i));
    ::Effect* cppItem{nullptr};
        Shiboken::Conversions::pythonToCppPointer(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_EFFECT_IDX]), pyItem, &(cppItem));
    cppOutRef.push_back(cppItem);
    }
    // TEMPLATE - pyListToStdList - END

}
static PythonToCppFunc is_std_list_EffectPTR__PythonToCpp_std_list_EffectPTR__Convertible(PyObject *pyIn) {
    if (Shiboken::Conversions::checkSequenceTypes(SbkNatronEngineTypes[SBK_EFFECT_IDX], pyIn))
        return std_list_EffectPTR__PythonToCpp_std_list_EffectPTR_;
    return {};
}

// C++ to Python conversion for type 'std::list<QString >'.
static PyObject *std_list_QString__CppToPython_std_list_QString_(const void *cppIn) {
    auto &cppInRef = *reinterpret_cast<::std::list<QString > *>(const_cast<void *>(cppIn));
    // TEMPLATE - stdListToPyList - START
    PyObject* pyOut = PyList_New((int) cppInRef.size());
    ::std::list<QString >::const_iterator it = cppInRef.begin();
    for (int idx = 0; it != cppInRef.end(); ++it, ++idx) {
    ::QString cppItem(*it);
    PyList_SET_ITEM(pyOut, idx, Shiboken::Conversions::copyToPython(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppItem));
    }
    return pyOut;
    // TEMPLATE - stdListToPyList - END

}
static void std_list_QString__PythonToCpp_std_list_QString_(PyObject *pyIn, void *cppOut) {
    auto &cppOutRef = *reinterpret_cast<::std::list<QString > *>(cppOut);
    // TEMPLATE - pyListToStdList - START
    for (int i = 0; i < PySequence_Size(pyIn); i++) {
    Shiboken::AutoDecRef pyItem(PySequence_GetItem(pyIn, i));
    ::QString cppItem;
        Shiboken::Conversions::pythonToCppCopy(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], pyItem, &(cppItem));
    cppOutRef.push_back(cppItem);
    }
    // TEMPLATE - pyListToStdList - END

}
static PythonToCppFunc is_std_list_QString__PythonToCpp_std_list_QString__Convertible(PyObject *pyIn) {
    if (Shiboken::Conversions::convertibleSequenceTypes(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], pyIn))
        return std_list_QString__PythonToCpp_std_list_QString_;
    return {};
}

// C++ to Python conversion for type 'const std::list<int > &'.
static PyObject *conststd_list_int_REF_CppToPython_conststd_list_int_REF(const void *cppIn) {
    auto &cppInRef = *reinterpret_cast<::std::list<int > *>(const_cast<void *>(cppIn));
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
static void conststd_list_int_REF_PythonToCpp_conststd_list_int_REF(PyObject *pyIn, void *cppOut) {
    auto &cppOutRef = *reinterpret_cast<::std::list<int > *>(cppOut);
    // TEMPLATE - pyListToStdList - START
    for (int i = 0; i < PySequence_Size(pyIn); i++) {
    Shiboken::AutoDecRef pyItem(PySequence_GetItem(pyIn, i));
    int cppItem;
        Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<int>(), pyItem, &(cppItem));
    cppOutRef.push_back(cppItem);
    }
    // TEMPLATE - pyListToStdList - END

}
static PythonToCppFunc is_conststd_list_int_REF_PythonToCpp_conststd_list_int_REF_Convertible(PyObject *pyIn) {
    if (Shiboken::Conversions::convertibleSequenceTypes(Shiboken::Conversions::PrimitiveTypeConverter<int>(), pyIn))
        return conststd_list_int_REF_PythonToCpp_conststd_list_int_REF;
    return {};
}

// C++ to Python conversion for type 'std::list<Param* >'.
static PyObject *std_list_ParamPTR__CppToPython_std_list_ParamPTR_(const void *cppIn) {
    auto &cppInRef = *reinterpret_cast<::std::list<Param* > *>(const_cast<void *>(cppIn));
    // TEMPLATE - stdListToPyList - START
    PyObject* pyOut = PyList_New((int) cppInRef.size());
    ::std::list<Param* >::const_iterator it = cppInRef.begin();
    for (int idx = 0; it != cppInRef.end(); ++it, ++idx) {
    ::Param* cppItem(*it);
    PyList_SET_ITEM(pyOut, idx, Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_PARAM_IDX]), cppItem));
    }
    return pyOut;
    // TEMPLATE - stdListToPyList - END

}
static void std_list_ParamPTR__PythonToCpp_std_list_ParamPTR_(PyObject *pyIn, void *cppOut) {
    auto &cppOutRef = *reinterpret_cast<::std::list<Param* > *>(cppOut);
    // TEMPLATE - pyListToStdList - START
    for (int i = 0; i < PySequence_Size(pyIn); i++) {
    Shiboken::AutoDecRef pyItem(PySequence_GetItem(pyIn, i));
    ::Param* cppItem{nullptr};
        Shiboken::Conversions::pythonToCppPointer(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_PARAM_IDX]), pyItem, &(cppItem));
    cppOutRef.push_back(cppItem);
    }
    // TEMPLATE - pyListToStdList - END

}
static PythonToCppFunc is_std_list_ParamPTR__PythonToCpp_std_list_ParamPTR__Convertible(PyObject *pyIn) {
    if (Shiboken::Conversions::checkSequenceTypes(SbkNatronEngineTypes[SBK_PARAM_IDX], pyIn))
        return std_list_ParamPTR__PythonToCpp_std_list_ParamPTR_;
    return {};
}

// C++ to Python conversion for type 'std::list<double > *'.
static PyObject *std_list_double_PTR_CppToPython_std_list_double_PTR(const void *cppIn) {
    auto &cppInRef = *reinterpret_cast<::std::list<double > *>(const_cast<void *>(cppIn));
    // TEMPLATE - stdListToPyList - START
    PyObject* pyOut = PyList_New((int) cppInRef.size());
    ::std::list<double >::const_iterator it = cppInRef.begin();
    for (int idx = 0; it != cppInRef.end(); ++it, ++idx) {
    double cppItem(*it);
    PyList_SET_ITEM(pyOut, idx, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppItem));
    }
    return pyOut;
    // TEMPLATE - stdListToPyList - END

}
static void std_list_double_PTR_PythonToCpp_std_list_double_PTR(PyObject *pyIn, void *cppOut) {
    auto &cppOutRef = *reinterpret_cast<::std::list<double > *>(cppOut);
    // TEMPLATE - pyListToStdList - START
    for (int i = 0; i < PySequence_Size(pyIn); i++) {
    Shiboken::AutoDecRef pyItem(PySequence_GetItem(pyIn, i));
    double cppItem;
        Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), pyItem, &(cppItem));
    cppOutRef.push_back(cppItem);
    }
    // TEMPLATE - pyListToStdList - END

}
static PythonToCppFunc is_std_list_double_PTR_PythonToCpp_std_list_double_PTR_Convertible(PyObject *pyIn) {
    if (Shiboken::Conversions::convertibleSequenceTypes(Shiboken::Conversions::PrimitiveTypeConverter<double>(), pyIn))
        return std_list_double_PTR_PythonToCpp_std_list_double_PTR;
    return {};
}

// C++ to Python conversion for type 'const std::vector<bool > &'.
static PyObject *conststd_vector_bool_REF_CppToPython_conststd_vector_bool_REF(const void *cppIn) {
    auto &cppInRef = *reinterpret_cast<::std::vector<bool > *>(const_cast<void *>(cppIn));
    // TEMPLATE - stdVectorToPyList - START
    ::std::vector<bool >::size_type vectorSize = cppInRef.size();
    PyObject* pyOut = PyList_New((int) vectorSize);
    for (::std::vector<bool >::size_type idx = 0; idx < vectorSize; ++idx) {
    bool cppItem(cppInRef[idx]);
    PyList_SET_ITEM(pyOut, idx, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppItem));
    }
    return pyOut;
    // TEMPLATE - stdVectorToPyList - END

}
static void conststd_vector_bool_REF_PythonToCpp_conststd_vector_bool_REF(PyObject *pyIn, void *cppOut) {
    auto &cppOutRef = *reinterpret_cast<::std::vector<bool > *>(cppOut);
    // TEMPLATE - pySeqToStdVector - START
    int vectorSize = PySequence_Size(pyIn);
    cppOutRef.reserve(vectorSize);
    for (int idx = 0; idx < vectorSize; ++idx) {
    Shiboken::AutoDecRef pyItem(PySequence_GetItem(pyIn, idx));
    bool cppItem;
        Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), pyItem, &(cppItem));
    cppOutRef.push_back(cppItem);
    }
    // TEMPLATE - pySeqToStdVector - END

}
static PythonToCppFunc is_conststd_vector_bool_REF_PythonToCpp_conststd_vector_bool_REF_Convertible(PyObject *pyIn) {
    if (Shiboken::Conversions::convertibleSequenceTypes(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), pyIn))
        return conststd_vector_bool_REF_PythonToCpp_conststd_vector_bool_REF;
    return {};
}

// C++ to Python conversion for type 'std::pair<QString,QString >'.
static PyObject *std_pair_QString_QString__CppToPython_std_pair_QString_QString_(const void *cppIn) {
    auto &cppInRef = *reinterpret_cast<::std::pair<QString,QString > *>(const_cast<void *>(cppIn));
    PyObject* pyOut = PyTuple_New(2);
    PyTuple_SET_ITEM(pyOut, 0, Shiboken::Conversions::copyToPython(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppInRef.first));
    PyTuple_SET_ITEM(pyOut, 1, Shiboken::Conversions::copyToPython(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppInRef.second));
    return pyOut;

}
static void std_pair_QString_QString__PythonToCpp_std_pair_QString_QString_(PyObject *pyIn, void *cppOut) {
    auto &cppOutRef = *reinterpret_cast<::std::pair<QString,QString > *>(cppOut);
    Shiboken::Conversions::pythonToCppCopy(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], PySequence_Fast_GET_ITEM(pyIn, 0), &(cppOutRef.first));
    Shiboken::Conversions::pythonToCppCopy(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], PySequence_Fast_GET_ITEM(pyIn, 1), &(cppOutRef.second));

}
static PythonToCppFunc is_std_pair_QString_QString__PythonToCpp_std_pair_QString_QString__Convertible(PyObject *pyIn) {
    if (Shiboken::Conversions::convertiblePairTypes(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], false, SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], false, pyIn))
        return std_pair_QString_QString__PythonToCpp_std_pair_QString_QString_;
    return {};
}

// C++ to Python conversion for type 'const std::list<std::pair< QString,QString > > &'.
static PyObject *conststd_list_std_pair_QString_QString__REF_CppToPython_conststd_list_std_pair_QString_QString__REF(const void *cppIn) {
    auto &cppInRef = *reinterpret_cast<::std::list<std::pair< QString,QString > > *>(const_cast<void *>(cppIn));
    // TEMPLATE - stdListToPyList - START
    PyObject* pyOut = PyList_New((int) cppInRef.size());
    ::std::list<std::pair< QString,QString > >::const_iterator it = cppInRef.begin();
    for (int idx = 0; it != cppInRef.end(); ++it, ++idx) {
    ::std::pair<QString,QString > cppItem(*it);
    PyList_SET_ITEM(pyOut, idx, Shiboken::Conversions::copyToPython(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_PAIR_QSTRING_QSTRING_IDX], &cppItem));
    }
    return pyOut;
    // TEMPLATE - stdListToPyList - END

}
static void conststd_list_std_pair_QString_QString__REF_PythonToCpp_conststd_list_std_pair_QString_QString__REF(PyObject *pyIn, void *cppOut) {
    auto &cppOutRef = *reinterpret_cast<::std::list<std::pair< QString,QString > > *>(cppOut);
    // TEMPLATE - pyListToStdList - START
    for (int i = 0; i < PySequence_Size(pyIn); i++) {
    Shiboken::AutoDecRef pyItem(PySequence_GetItem(pyIn, i));
    ::std::pair<QString,QString > cppItem;
        Shiboken::Conversions::pythonToCppCopy(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_PAIR_QSTRING_QSTRING_IDX], pyItem, &(cppItem));
    cppOutRef.push_back(cppItem);
    }
    // TEMPLATE - pyListToStdList - END

}
static PythonToCppFunc is_conststd_list_std_pair_QString_QString__REF_PythonToCpp_conststd_list_std_pair_QString_QString__REF_Convertible(PyObject *pyIn) {
    if (Shiboken::Conversions::convertibleSequenceTypes(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_PAIR_QSTRING_QSTRING_IDX], pyIn))
        return conststd_list_std_pair_QString_QString__REF_PythonToCpp_conststd_list_std_pair_QString_QString__REF;
    return {};
}

// C++ to Python conversion for type 'std::list<ImageLayer >'.
static PyObject *std_list_ImageLayer__CppToPython_std_list_ImageLayer_(const void *cppIn) {
    auto &cppInRef = *reinterpret_cast<::std::list<ImageLayer > *>(const_cast<void *>(cppIn));
    // TEMPLATE - stdListToPyList - START
    PyObject* pyOut = PyList_New((int) cppInRef.size());
    ::std::list<ImageLayer >::const_iterator it = cppInRef.begin();
    for (int idx = 0; it != cppInRef.end(); ++it, ++idx) {
    ::ImageLayer cppItem(*it);
    PyList_SET_ITEM(pyOut, idx, Shiboken::Conversions::copyToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX]), &cppItem));
    }
    return pyOut;
    // TEMPLATE - stdListToPyList - END

}
static void std_list_ImageLayer__PythonToCpp_std_list_ImageLayer_(PyObject *pyIn, void *cppOut) {
    auto &cppOutRef = *reinterpret_cast<::std::list<ImageLayer > *>(cppOut);
    // TEMPLATE - pyListToStdList - START
    for (int i = 0; i < PySequence_Size(pyIn); i++) {
    Shiboken::AutoDecRef pyItem(PySequence_GetItem(pyIn, i));
    ::ImageLayer cppItem = ::ImageLayer(::QString(), ::QString(), ::QStringList());
        Shiboken::Conversions::pythonToCppCopy(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX]), pyItem, &(cppItem));
    cppOutRef.push_back(cppItem);
    }
    // TEMPLATE - pyListToStdList - END

}
static PythonToCppFunc is_std_list_ImageLayer__PythonToCpp_std_list_ImageLayer__Convertible(PyObject *pyIn) {
    if (Shiboken::Conversions::convertibleSequenceTypes(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX]), pyIn))
        return std_list_ImageLayer__PythonToCpp_std_list_ImageLayer_;
    return {};
}

// C++ to Python conversion for type 'const std::vector<double > &'.
static PyObject *conststd_vector_double_REF_CppToPython_conststd_vector_double_REF(const void *cppIn) {
    auto &cppInRef = *reinterpret_cast<::std::vector<double > *>(const_cast<void *>(cppIn));
    // TEMPLATE - stdVectorToPyList - START
    ::std::vector<double >::size_type vectorSize = cppInRef.size();
    PyObject* pyOut = PyList_New((int) vectorSize);
    for (::std::vector<double >::size_type idx = 0; idx < vectorSize; ++idx) {
    double cppItem(cppInRef[idx]);
    PyList_SET_ITEM(pyOut, idx, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppItem));
    }
    return pyOut;
    // TEMPLATE - stdVectorToPyList - END

}
static void conststd_vector_double_REF_PythonToCpp_conststd_vector_double_REF(PyObject *pyIn, void *cppOut) {
    auto &cppOutRef = *reinterpret_cast<::std::vector<double > *>(cppOut);
    // TEMPLATE - pySeqToStdVector - START
    int vectorSize = PySequence_Size(pyIn);
    cppOutRef.reserve(vectorSize);
    for (int idx = 0; idx < vectorSize; ++idx) {
    Shiboken::AutoDecRef pyItem(PySequence_GetItem(pyIn, idx));
    double cppItem;
        Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<double>(), pyItem, &(cppItem));
    cppOutRef.push_back(cppItem);
    }
    // TEMPLATE - pySeqToStdVector - END

}
static PythonToCppFunc is_conststd_vector_double_REF_PythonToCpp_conststd_vector_double_REF_Convertible(PyObject *pyIn) {
    if (Shiboken::Conversions::convertibleSequenceTypes(Shiboken::Conversions::PrimitiveTypeConverter<double>(), pyIn))
        return conststd_vector_double_REF_PythonToCpp_conststd_vector_double_REF;
    return {};
}

// C++ to Python conversion for type 'const std::vector<int > &'.
static PyObject *conststd_vector_int_REF_CppToPython_conststd_vector_int_REF(const void *cppIn) {
    auto &cppInRef = *reinterpret_cast<::std::vector<int > *>(const_cast<void *>(cppIn));
    // TEMPLATE - stdVectorToPyList - START
    ::std::vector<int >::size_type vectorSize = cppInRef.size();
    PyObject* pyOut = PyList_New((int) vectorSize);
    for (::std::vector<int >::size_type idx = 0; idx < vectorSize; ++idx) {
    int cppItem(cppInRef[idx]);
    PyList_SET_ITEM(pyOut, idx, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppItem));
    }
    return pyOut;
    // TEMPLATE - stdVectorToPyList - END

}
static void conststd_vector_int_REF_PythonToCpp_conststd_vector_int_REF(PyObject *pyIn, void *cppOut) {
    auto &cppOutRef = *reinterpret_cast<::std::vector<int > *>(cppOut);
    // TEMPLATE - pySeqToStdVector - START
    int vectorSize = PySequence_Size(pyIn);
    cppOutRef.reserve(vectorSize);
    for (int idx = 0; idx < vectorSize; ++idx) {
    Shiboken::AutoDecRef pyItem(PySequence_GetItem(pyIn, idx));
    int cppItem;
        Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<int>(), pyItem, &(cppItem));
    cppOutRef.push_back(cppItem);
    }
    // TEMPLATE - pySeqToStdVector - END

}
static PythonToCppFunc is_conststd_vector_int_REF_PythonToCpp_conststd_vector_int_REF_Convertible(PyObject *pyIn) {
    if (Shiboken::Conversions::convertibleSequenceTypes(Shiboken::Conversions::PrimitiveTypeConverter<int>(), pyIn))
        return conststd_vector_int_REF_PythonToCpp_conststd_vector_int_REF;
    return {};
}

// C++ to Python conversion for type 'std::list<ItemBase* >'.
static PyObject *std_list_ItemBasePTR__CppToPython_std_list_ItemBasePTR_(const void *cppIn) {
    auto &cppInRef = *reinterpret_cast<::std::list<ItemBase* > *>(const_cast<void *>(cppIn));
    // TEMPLATE - stdListToPyList - START
    PyObject* pyOut = PyList_New((int) cppInRef.size());
    ::std::list<ItemBase* >::const_iterator it = cppInRef.begin();
    for (int idx = 0; it != cppInRef.end(); ++it, ++idx) {
    ::ItemBase* cppItem(*it);
    PyList_SET_ITEM(pyOut, idx, Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_ITEMBASE_IDX]), cppItem));
    }
    return pyOut;
    // TEMPLATE - stdListToPyList - END

}
static void std_list_ItemBasePTR__PythonToCpp_std_list_ItemBasePTR_(PyObject *pyIn, void *cppOut) {
    auto &cppOutRef = *reinterpret_cast<::std::list<ItemBase* > *>(cppOut);
    // TEMPLATE - pyListToStdList - START
    for (int i = 0; i < PySequence_Size(pyIn); i++) {
    Shiboken::AutoDecRef pyItem(PySequence_GetItem(pyIn, i));
    ::ItemBase* cppItem{nullptr};
        Shiboken::Conversions::pythonToCppPointer(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_ITEMBASE_IDX]), pyItem, &(cppItem));
    cppOutRef.push_back(cppItem);
    }
    // TEMPLATE - pyListToStdList - END

}
static PythonToCppFunc is_std_list_ItemBasePTR__PythonToCpp_std_list_ItemBasePTR__Convertible(PyObject *pyIn) {
    if (Shiboken::Conversions::checkSequenceTypes(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], pyIn))
        return std_list_ItemBasePTR__PythonToCpp_std_list_ItemBasePTR_;
    return {};
}

// C++ to Python conversion for type 'std::vector<std::string >'.
static PyObject *std_vector_std_string__CppToPython_std_vector_std_string_(const void *cppIn) {
    auto &cppInRef = *reinterpret_cast<::std::vector<std::string > *>(const_cast<void *>(cppIn));
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
static void std_vector_std_string__PythonToCpp_std_vector_std_string_(PyObject *pyIn, void *cppOut) {
    auto &cppOutRef = *reinterpret_cast<::std::vector<std::string > *>(cppOut);
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
static PythonToCppFunc is_std_vector_std_string__PythonToCpp_std_vector_std_string__Convertible(PyObject *pyIn) {
    if (Shiboken::Conversions::convertibleSequenceTypes(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), pyIn))
        return std_vector_std_string__PythonToCpp_std_vector_std_string_;
    return {};
}

// C++ to Python conversion for type 'std::list<std::vector< std::string > > *'.
static PyObject *std_list_std_vector_std_string__PTR_CppToPython_std_list_std_vector_std_string__PTR(const void *cppIn) {
    auto &cppInRef = *reinterpret_cast<::std::list<std::vector< std::string > > *>(const_cast<void *>(cppIn));
    // TEMPLATE - stdListToPyList - START
    PyObject* pyOut = PyList_New((int) cppInRef.size());
    ::std::list<std::vector< std::string > >::const_iterator it = cppInRef.begin();
    for (int idx = 0; it != cppInRef.end(); ++it, ++idx) {
    ::std::vector<std::string > cppItem(*it);
    PyList_SET_ITEM(pyOut, idx, Shiboken::Conversions::copyToPython(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_VECTOR_STD_STRING_IDX], &cppItem));
    }
    return pyOut;
    // TEMPLATE - stdListToPyList - END

}
static void std_list_std_vector_std_string__PTR_PythonToCpp_std_list_std_vector_std_string__PTR(PyObject *pyIn, void *cppOut) {
    auto &cppOutRef = *reinterpret_cast<::std::list<std::vector< std::string > > *>(cppOut);
    // TEMPLATE - pyListToStdList - START
    for (int i = 0; i < PySequence_Size(pyIn); i++) {
    Shiboken::AutoDecRef pyItem(PySequence_GetItem(pyIn, i));
    ::std::vector<std::string > cppItem;
        Shiboken::Conversions::pythonToCppCopy(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_VECTOR_STD_STRING_IDX], pyItem, &(cppItem));
    cppOutRef.push_back(cppItem);
    }
    // TEMPLATE - pyListToStdList - END

}
static PythonToCppFunc is_std_list_std_vector_std_string__PTR_PythonToCpp_std_list_std_vector_std_string__PTR_Convertible(PyObject *pyIn) {
    if (Shiboken::Conversions::convertibleSequenceTypes(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_VECTOR_STD_STRING_IDX], pyIn))
        return std_list_std_vector_std_string__PTR_PythonToCpp_std_list_std_vector_std_string__PTR;
    return {};
}

// C++ to Python conversion for type 'std::vector<RectI >'.
static PyObject *std_vector_RectI__CppToPython_std_vector_RectI_(const void *cppIn) {
    auto &cppInRef = *reinterpret_cast<::std::vector<RectI > *>(const_cast<void *>(cppIn));
    // TEMPLATE - stdVectorToPyList - START
    ::std::vector<RectI >::size_type vectorSize = cppInRef.size();
    PyObject* pyOut = PyList_New((int) vectorSize);
    for (::std::vector<RectI >::size_type idx = 0; idx < vectorSize; ++idx) {
    ::RectI cppItem(cppInRef[idx]);
    PyList_SET_ITEM(pyOut, idx, Shiboken::Conversions::copyToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_RECTI_IDX]), &cppItem));
    }
    return pyOut;
    // TEMPLATE - stdVectorToPyList - END

}
static void std_vector_RectI__PythonToCpp_std_vector_RectI_(PyObject *pyIn, void *cppOut) {
    auto &cppOutRef = *reinterpret_cast<::std::vector<RectI > *>(cppOut);
    // TEMPLATE - pySeqToStdVector - START
    int vectorSize = PySequence_Size(pyIn);
    cppOutRef.reserve(vectorSize);
    for (int idx = 0; idx < vectorSize; ++idx) {
    Shiboken::AutoDecRef pyItem(PySequence_GetItem(pyIn, idx));
    ::RectI cppItem;
        Shiboken::Conversions::pythonToCppCopy(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_RECTI_IDX]), pyItem, &(cppItem));
    cppOutRef.push_back(cppItem);
    }
    // TEMPLATE - pySeqToStdVector - END

}
static PythonToCppFunc is_std_vector_RectI__PythonToCpp_std_vector_RectI__Convertible(PyObject *pyIn) {
    if (Shiboken::Conversions::convertibleSequenceTypes(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_RECTI_IDX]), pyIn))
        return std_vector_RectI__PythonToCpp_std_vector_RectI_;
    return {};
}

// C++ to Python conversion for type 'std::list<Track* > *'.
static PyObject *std_list_TrackPTR_PTR_CppToPython_std_list_TrackPTR_PTR(const void *cppIn) {
    auto &cppInRef = *reinterpret_cast<::std::list<Track* > *>(const_cast<void *>(cppIn));
    // TEMPLATE - stdListToPyList - START
    PyObject* pyOut = PyList_New((int) cppInRef.size());
    ::std::list<Track* >::const_iterator it = cppInRef.begin();
    for (int idx = 0; it != cppInRef.end(); ++it, ++idx) {
    ::Track* cppItem(*it);
    PyList_SET_ITEM(pyOut, idx, Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_TRACK_IDX]), cppItem));
    }
    return pyOut;
    // TEMPLATE - stdListToPyList - END

}
static void std_list_TrackPTR_PTR_PythonToCpp_std_list_TrackPTR_PTR(PyObject *pyIn, void *cppOut) {
    auto &cppOutRef = *reinterpret_cast<::std::list<Track* > *>(cppOut);
    // TEMPLATE - pyListToStdList - START
    for (int i = 0; i < PySequence_Size(pyIn); i++) {
    Shiboken::AutoDecRef pyItem(PySequence_GetItem(pyIn, i));
    ::Track* cppItem{nullptr};
        Shiboken::Conversions::pythonToCppPointer(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_TRACK_IDX]), pyItem, &(cppItem));
    cppOutRef.push_back(cppItem);
    }
    // TEMPLATE - pyListToStdList - END

}
static PythonToCppFunc is_std_list_TrackPTR_PTR_PythonToCpp_std_list_TrackPTR_PTR_Convertible(PyObject *pyIn) {
    if (Shiboken::Conversions::checkSequenceTypes(SbkNatronEngineTypes[SBK_TRACK_IDX], pyIn))
        return std_list_TrackPTR_PTR_PythonToCpp_std_list_TrackPTR_PTR;
    return {};
}

// C++ to Python conversion for type 'QList<QVariant >'.
static PyObject *_QList_QVariant__CppToPython__QList_QVariant_(const void *cppIn) {
    auto &cppInRef = *reinterpret_cast<::QList<QVariant > *>(const_cast<void *>(cppIn));
    // TEMPLATE - cpplist_to_pylist_conversion - START
    PyObject* pyOut = PyList_New((int) cppInRef.size());
    ::QList<QVariant >::const_iterator it = cppInRef.begin();
    for (int idx = 0; it != cppInRef.end(); ++it, ++idx) {
        ::QVariant cppItem(*it);
        PyList_SET_ITEM(pyOut, idx, Shiboken::Conversions::copyToPython(SbkPySide2_QtCoreTypeConverters[SBK_QVARIANT_IDX], &cppItem));
    }
    return pyOut;
    // TEMPLATE - cpplist_to_pylist_conversion - END

}
static void _QList_QVariant__PythonToCpp__QList_QVariant_(PyObject *pyIn, void *cppOut) {
    auto &cppOutRef = *reinterpret_cast<::QList<QVariant > *>(cppOut);
    // TEMPLATE - pyseq_to_cpplist_conversion - START
    // PYSIDE-795: Turn all sequences into iterables.
    Shiboken::AutoDecRef it(PyObject_GetIter(pyIn));
    PyObject *(*iternext)(PyObject *) = *Py_TYPE(it)->tp_iternext;
    for (;;) {
        Shiboken::AutoDecRef pyItem(iternext(it));
        if (pyItem.isNull()) {
            if (PyErr_Occurred() && PyErr_ExceptionMatches(PyExc_StopIteration))
                PyErr_Clear();
            break;
        }
        ::QVariant cppItem;
        Shiboken::Conversions::pythonToCppCopy(SbkPySide2_QtCoreTypeConverters[SBK_QVARIANT_IDX], pyItem, &(cppItem));
        cppOutRef << cppItem;
    }
    // TEMPLATE - pyseq_to_cpplist_conversion - END

}
static PythonToCppFunc is__QList_QVariant__PythonToCpp__QList_QVariant__Convertible(PyObject *pyIn) {
    if (Shiboken::Conversions::convertibleSequenceTypes(SbkPySide2_QtCoreTypeConverters[SBK_QVARIANT_IDX], pyIn))
        return _QList_QVariant__PythonToCpp__QList_QVariant_;
    return {};
}

// C++ to Python conversion for type 'QList<QString >'.
static PyObject *_QList_QString__CppToPython__QList_QString_(const void *cppIn) {
    auto &cppInRef = *reinterpret_cast<::QList<QString > *>(const_cast<void *>(cppIn));
    // TEMPLATE - cpplist_to_pylist_conversion - START
    PyObject* pyOut = PyList_New((int) cppInRef.size());
    ::QList<QString >::const_iterator it = cppInRef.begin();
    for (int idx = 0; it != cppInRef.end(); ++it, ++idx) {
        ::QString cppItem(*it);
        PyList_SET_ITEM(pyOut, idx, Shiboken::Conversions::copyToPython(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppItem));
    }
    return pyOut;
    // TEMPLATE - cpplist_to_pylist_conversion - END

}
static void _QList_QString__PythonToCpp__QList_QString_(PyObject *pyIn, void *cppOut) {
    auto &cppOutRef = *reinterpret_cast<::QList<QString > *>(cppOut);
    // TEMPLATE - pyseq_to_cpplist_conversion - START
    // PYSIDE-795: Turn all sequences into iterables.
    Shiboken::AutoDecRef it(PyObject_GetIter(pyIn));
    PyObject *(*iternext)(PyObject *) = *Py_TYPE(it)->tp_iternext;
    for (;;) {
        Shiboken::AutoDecRef pyItem(iternext(it));
        if (pyItem.isNull()) {
            if (PyErr_Occurred() && PyErr_ExceptionMatches(PyExc_StopIteration))
                PyErr_Clear();
            break;
        }
        ::QString cppItem;
        Shiboken::Conversions::pythonToCppCopy(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], pyItem, &(cppItem));
        cppOutRef << cppItem;
    }
    // TEMPLATE - pyseq_to_cpplist_conversion - END

}
static PythonToCppFunc is__QList_QString__PythonToCpp__QList_QString__Convertible(PyObject *pyIn) {
    if (Shiboken::Conversions::convertibleSequenceTypes(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], pyIn))
        return _QList_QString__PythonToCpp__QList_QString_;
    return {};
}

// C++ to Python conversion for type 'QMap<QString,QVariant >'.
static PyObject *_QMap_QString_QVariant__CppToPython__QMap_QString_QVariant_(const void *cppIn) {
    auto &cppInRef = *reinterpret_cast<::QMap<QString,QVariant > *>(const_cast<void *>(cppIn));
    // TEMPLATE - cppmap_to_pymap_conversion - START
    PyObject *pyOut = PyDict_New();
    for (::QMap<QString,QVariant >::const_iterator it = cppInRef.begin(), end = cppInRef.end(); it != end; ++it) {
        ::QString key = it.key();
        ::QVariant value = it.value();
        PyObject *pyKey = Shiboken::Conversions::copyToPython(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], &key);
        PyObject *pyValue = Shiboken::Conversions::copyToPython(SbkPySide2_QtCoreTypeConverters[SBK_QVARIANT_IDX], &value);
        PyDict_SetItem(pyOut, pyKey, pyValue);
        Py_DECREF(pyKey);
        Py_DECREF(pyValue);
    }
    return pyOut;
    // TEMPLATE - cppmap_to_pymap_conversion - END

}
static void _QMap_QString_QVariant__PythonToCpp__QMap_QString_QVariant_(PyObject *pyIn, void *cppOut) {
    auto &cppOutRef = *reinterpret_cast<::QMap<QString,QVariant > *>(cppOut);
    // TEMPLATE - pydict_to_cppmap_conversion - START
    PyObject *key;
    PyObject *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(pyIn, &pos, &key, &value)) {
        ::QString cppKey;
        Shiboken::Conversions::pythonToCppCopy(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], key, &(cppKey));
        ::QVariant cppValue;
        Shiboken::Conversions::pythonToCppCopy(SbkPySide2_QtCoreTypeConverters[SBK_QVARIANT_IDX], value, &(cppValue));
        cppOutRef.insert(cppKey, cppValue);
    }
    // TEMPLATE - pydict_to_cppmap_conversion - END

}
static PythonToCppFunc is__QMap_QString_QVariant__PythonToCpp__QMap_QString_QVariant__Convertible(PyObject *pyIn) {
    if (Shiboken::Conversions::convertibleDictTypes(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], false, SbkPySide2_QtCoreTypeConverters[SBK_QVARIANT_IDX], false, pyIn))
        return _QMap_QString_QVariant__PythonToCpp__QMap_QString_QVariant_;
    return {};
}


#ifdef IS_PY3K
static struct PyModuleDef moduledef = {
    /* m_base     */ PyModuleDef_HEAD_INIT,
    /* m_name     */ "NatronEngine",
    /* m_doc      */ nullptr,
    /* m_size     */ -1,
    /* m_methods  */ NatronEngine_methods,
    /* m_reload   */ nullptr,
    /* m_traverse */ nullptr,
    /* m_clear    */ nullptr,
    /* m_free     */ nullptr
};

#endif

// The signatures string for the global functions.
// Multiple signatures have their index "n:" in front.
static const char *NatronEngine_SignatureStrings[] = {
    nullptr}; // Sentinel

SBK_MODULE_INIT_FUNCTION_BEGIN(NatronEngine)
    {
        Shiboken::AutoDecRef requiredModule(Shiboken::Module::import("PySide2.QtCore"));
        if (requiredModule.isNull())
            return SBK_MODULE_INIT_ERROR;
        SbkPySide2_QtCoreTypes = Shiboken::Module::getTypes(requiredModule);
        SbkPySide2_QtCoreTypeConverters = Shiboken::Module::getTypeConverters(requiredModule);
    }

    // Create an array of wrapper types for the current module.
    static PyTypeObject *cppApi[SBK_NatronEngine_IDX_COUNT];
    SbkNatronEngineTypes = cppApi;

    // Create an array of primitive type converters for the current module.
    static SbkConverter *sbkConverters[SBK_NatronEngine_CONVERTERS_IDX_COUNT];
    SbkNatronEngineTypeConverters = sbkConverters;

#ifdef IS_PY3K
    PyObject *module = Shiboken::Module::create("NatronEngine", &moduledef);
#else
    PyObject *module = Shiboken::Module::create("NatronEngine", NatronEngine_methods);
#endif

    // Make module available from global scope
    SbkNatronEngineModuleObject = module;

    // Initialize classes in the type system
    init_UserParamHolder(module);
    init_Tracker(module);
    init_Track(module);
    init_Roto(module);
    init_RectI(module);
    init_RectD(module);
    init_PyCoreApplication(module);
    init_Param(module);
    init_SeparatorParam(module);
    init_ParametricParam(module);
    init_PageParam(module);
    init_NodeCreationProperty(module);
    init_StringNodeCreationProperty(module);
    init_NatronEngineNATRON_ENUM(module);
    init_ItemBase(module);
    init_Layer(module);
    init_IntNodeCreationProperty(module);
    init_Int3DTuple(module);
    init_Int2DTuple(module);
    init_ImageLayer(module);
    init_GroupParam(module);
    init_Group(module);
    init_FloatNodeCreationProperty(module);
    init_ExprUtils(module);
    init_Effect(module);
    init_Double3DTuple(module);
    init_Double2DTuple(module);
    init_ColorTuple(module);
    init_ButtonParam(module);
    init_BoolNodeCreationProperty(module);
    init_BezierCurve(module);
    init_AppSettings(module);
    init_App(module);
    init_AnimatedParam(module);
    init_DoubleParam(module);
    init_Double2DParam(module);
    init_Double3DParam(module);
    init_StringParamBase(module);
    init_OutputFileParam(module);
    init_StringParam(module);
    init_PathParam(module);
    init_FileParam(module);
    init_IntParam(module);
    init_Int2DParam(module);
    init_Int3DParam(module);
    init_ColorParam(module);
    init_ChoiceParam(module);
    init_BooleanParam(module);

    // Register converter for type 'NatronEngine.std::size_t'.
    SbkNatronEngineTypeConverters[SBK_STD_SIZE_T_IDX] = Shiboken::Conversions::createConverter(&PyLong_Type, std_size_t_CppToPython_std_size_t);
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_STD_SIZE_T_IDX], "std::size_t");
    // Add user defined implicit conversions to type converter.
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronEngineTypeConverters[SBK_STD_SIZE_T_IDX],
        PyLong_PythonToCpp_std_size_t,
        is_PyLong_PythonToCpp_std_size_t_Convertible);


    // Register converter for type 'const std::map<QString,NodeCreationProperty*>&'.
    SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_MAP_QSTRING_NODECREATIONPROPERTYPTR_IDX] = Shiboken::Conversions::createConverter(&PyDict_Type, conststd_map_QString_NodeCreationPropertyPTR_REF_CppToPython_conststd_map_QString_NodeCreationPropertyPTR_REF);
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_MAP_QSTRING_NODECREATIONPROPERTYPTR_IDX], "const std::map<QString,NodeCreationProperty*>&");
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_MAP_QSTRING_NODECREATIONPROPERTYPTR_IDX], "std::map<QString,NodeCreationProperty*>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_MAP_QSTRING_NODECREATIONPROPERTYPTR_IDX],
        conststd_map_QString_NodeCreationPropertyPTR_REF_PythonToCpp_conststd_map_QString_NodeCreationPropertyPTR_REF,
        is_conststd_map_QString_NodeCreationPropertyPTR_REF_PythonToCpp_conststd_map_QString_NodeCreationPropertyPTR_REF_Convertible);

    // Register converter for type 'std::list<Effect*>'.
    SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_EFFECTPTR_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, std_list_EffectPTR__CppToPython_std_list_EffectPTR_);
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_EFFECTPTR_IDX], "std::list<Effect*>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_EFFECTPTR_IDX],
        std_list_EffectPTR__PythonToCpp_std_list_EffectPTR_,
        is_std_list_EffectPTR__PythonToCpp_std_list_EffectPTR__Convertible);

    // Register converter for type 'std::list<QString>'.
    SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_QSTRING_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, std_list_QString__CppToPython_std_list_QString_);
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_QSTRING_IDX], "std::list<QString>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_QSTRING_IDX],
        std_list_QString__PythonToCpp_std_list_QString_,
        is_std_list_QString__PythonToCpp_std_list_QString__Convertible);

    // Register converter for type 'const std::list<int>&'.
    SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_INT_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, conststd_list_int_REF_CppToPython_conststd_list_int_REF);
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_INT_IDX], "const std::list<int>&");
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_INT_IDX], "std::list<int>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_INT_IDX],
        conststd_list_int_REF_PythonToCpp_conststd_list_int_REF,
        is_conststd_list_int_REF_PythonToCpp_conststd_list_int_REF_Convertible);

    // Register converter for type 'std::list<Param*>'.
    SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_PARAMPTR_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, std_list_ParamPTR__CppToPython_std_list_ParamPTR_);
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_PARAMPTR_IDX], "std::list<Param*>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_PARAMPTR_IDX],
        std_list_ParamPTR__PythonToCpp_std_list_ParamPTR_,
        is_std_list_ParamPTR__PythonToCpp_std_list_ParamPTR__Convertible);

    // Register converter for type 'std::list<double>*'.
    SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_DOUBLE_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, std_list_double_PTR_CppToPython_std_list_double_PTR);
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_DOUBLE_IDX], "std::list<double>*");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_DOUBLE_IDX],
        std_list_double_PTR_PythonToCpp_std_list_double_PTR,
        is_std_list_double_PTR_PythonToCpp_std_list_double_PTR_Convertible);

    // Register converter for type 'const std::vector<bool>&'.
    SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_VECTOR_BOOL_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, conststd_vector_bool_REF_CppToPython_conststd_vector_bool_REF);
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_VECTOR_BOOL_IDX], "const std::vector<bool>&");
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_VECTOR_BOOL_IDX], "std::vector<bool>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_VECTOR_BOOL_IDX],
        conststd_vector_bool_REF_PythonToCpp_conststd_vector_bool_REF,
        is_conststd_vector_bool_REF_PythonToCpp_conststd_vector_bool_REF_Convertible);

    // Register converter for type 'std::pair<QString,QString>'.
    SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_PAIR_QSTRING_QSTRING_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, std_pair_QString_QString__CppToPython_std_pair_QString_QString_);
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_PAIR_QSTRING_QSTRING_IDX], "std::pair<QString,QString>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_PAIR_QSTRING_QSTRING_IDX],
        std_pair_QString_QString__PythonToCpp_std_pair_QString_QString_,
        is_std_pair_QString_QString__PythonToCpp_std_pair_QString_QString__Convertible);

    // Register converter for type 'const std::list<std::pair<QString,QString>>&'.
    SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_STD_PAIR_QSTRING_QSTRING_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, conststd_list_std_pair_QString_QString__REF_CppToPython_conststd_list_std_pair_QString_QString__REF);
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_STD_PAIR_QSTRING_QSTRING_IDX], "const std::list<std::pair<QString,QString>>&");
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_STD_PAIR_QSTRING_QSTRING_IDX], "std::list<std::pair<QString,QString>>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_STD_PAIR_QSTRING_QSTRING_IDX],
        conststd_list_std_pair_QString_QString__REF_PythonToCpp_conststd_list_std_pair_QString_QString__REF,
        is_conststd_list_std_pair_QString_QString__REF_PythonToCpp_conststd_list_std_pair_QString_QString__REF_Convertible);

    // Register converter for type 'std::list<ImageLayer>'.
    SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_IMAGELAYER_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, std_list_ImageLayer__CppToPython_std_list_ImageLayer_);
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_IMAGELAYER_IDX], "std::list<ImageLayer>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_IMAGELAYER_IDX],
        std_list_ImageLayer__PythonToCpp_std_list_ImageLayer_,
        is_std_list_ImageLayer__PythonToCpp_std_list_ImageLayer__Convertible);

    // Register converter for type 'const std::vector<double>&'.
    SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_VECTOR_DOUBLE_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, conststd_vector_double_REF_CppToPython_conststd_vector_double_REF);
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_VECTOR_DOUBLE_IDX], "const std::vector<double>&");
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_VECTOR_DOUBLE_IDX], "std::vector<double>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_VECTOR_DOUBLE_IDX],
        conststd_vector_double_REF_PythonToCpp_conststd_vector_double_REF,
        is_conststd_vector_double_REF_PythonToCpp_conststd_vector_double_REF_Convertible);

    // Register converter for type 'const std::vector<int>&'.
    SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_VECTOR_INT_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, conststd_vector_int_REF_CppToPython_conststd_vector_int_REF);
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_VECTOR_INT_IDX], "const std::vector<int>&");
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_VECTOR_INT_IDX], "std::vector<int>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_VECTOR_INT_IDX],
        conststd_vector_int_REF_PythonToCpp_conststd_vector_int_REF,
        is_conststd_vector_int_REF_PythonToCpp_conststd_vector_int_REF_Convertible);

    // Register converter for type 'std::list<ItemBase*>'.
    SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_ITEMBASEPTR_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, std_list_ItemBasePTR__CppToPython_std_list_ItemBasePTR_);
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_ITEMBASEPTR_IDX], "std::list<ItemBase*>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_ITEMBASEPTR_IDX],
        std_list_ItemBasePTR__PythonToCpp_std_list_ItemBasePTR_,
        is_std_list_ItemBasePTR__PythonToCpp_std_list_ItemBasePTR__Convertible);

    // Register converter for type 'std::vector<std::string>'.
    SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_VECTOR_STD_STRING_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, std_vector_std_string__CppToPython_std_vector_std_string_);
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_VECTOR_STD_STRING_IDX], "std::vector<std::string>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_VECTOR_STD_STRING_IDX],
        std_vector_std_string__PythonToCpp_std_vector_std_string_,
        is_std_vector_std_string__PythonToCpp_std_vector_std_string__Convertible);

    // Register converter for type 'std::list<std::vector<std::string>>*'.
    SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_STD_VECTOR_STD_STRING_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, std_list_std_vector_std_string__PTR_CppToPython_std_list_std_vector_std_string__PTR);
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_STD_VECTOR_STD_STRING_IDX], "std::list<std::vector<std::string>>*");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_STD_VECTOR_STD_STRING_IDX],
        std_list_std_vector_std_string__PTR_PythonToCpp_std_list_std_vector_std_string__PTR,
        is_std_list_std_vector_std_string__PTR_PythonToCpp_std_list_std_vector_std_string__PTR_Convertible);

    // Register converter for type 'std::vector<RectI>'.
    SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_VECTOR_RECTI_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, std_vector_RectI__CppToPython_std_vector_RectI_);
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_VECTOR_RECTI_IDX], "std::vector<RectI>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_VECTOR_RECTI_IDX],
        std_vector_RectI__PythonToCpp_std_vector_RectI_,
        is_std_vector_RectI__PythonToCpp_std_vector_RectI__Convertible);

    // Register converter for type 'std::list<Track*>*'.
    SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_TRACKPTR_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, std_list_TrackPTR_PTR_CppToPython_std_list_TrackPTR_PTR);
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_TRACKPTR_IDX], "std::list<Track*>*");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_TRACKPTR_IDX],
        std_list_TrackPTR_PTR_PythonToCpp_std_list_TrackPTR_PTR,
        is_std_list_TrackPTR_PTR_PythonToCpp_std_list_TrackPTR_PTR_Convertible);

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
    Shiboken::Conversions::registerConverterName(Shiboken::Conversions::PrimitiveTypeConverter<unsigned short>(), "ushort");

    Shiboken::Module::registerTypes(module, SbkNatronEngineTypes);
    Shiboken::Module::registerTypeConverters(module, SbkNatronEngineTypeConverters);

    if (PyErr_Occurred()) {
        PyErr_Print();
        Py_FatalError("can't initialize module NatronEngine");
    }
    PySide::registerCleanupFunction(cleanTypesAttributes);

    FinishSignatureInitialization(module, NatronEngine_SignatureStrings);

SBK_MODULE_INIT_FUNCTION_END
