
#include <sbkpython.h>
#include "Global/Macros.h"
CLANG_DIAG_OFF(mismatched-tags)
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_OFF(missing-field-initializers)
#include <shiboken.h> // produces many warnings
#include <algorithm>
#include <pyside.h>
#include "natronengine_python.h"



// Extra includes

// Current module's type array.
PyTypeObject** SbkNatronEngineTypes;
// Current module's converter array.
SbkConverter** SbkNatronEngineTypeConverters;
void cleanTypesAttributes(void) {
    for (int i = 0, imax = SBK_NatronEngine_IDX_COUNT; i < imax; i++) {
        PyObject *pyType = reinterpret_cast<PyObject*>(SbkNatronEngineTypes[i]);
        if (pyType && PyObject_HasAttrString(pyType, "staticMetaObject"))
            PyObject_SetAttrString(pyType, "staticMetaObject", Py_None);
    }
}
// Global functions ------------------------------------------------------------

static PyMethodDef NatronEngine_methods[] = {
    {0} // Sentinel
};

// Classes initialization functions ------------------------------------------------------------
void init_Roto(PyObject* module);
void init_UserParamHolder(PyObject* module);
void init_Param(PyObject* module);
void init_AnimatedParam(PyObject* module);
void init_IntParam(PyObject* module);
void init_Int2DParam(PyObject* module);
void init_Int3DParam(PyObject* module);
void init_DoubleParam(PyObject* module);
void init_Double2DParam(PyObject* module);
void init_Double3DParam(PyObject* module);
void init_ColorParam(PyObject* module);
void init_ChoiceParam(PyObject* module);
void init_BooleanParam(PyObject* module);
void init_StringParamBase(PyObject* module);
void init_StringParam(PyObject* module);
void init_FileParam(PyObject* module);
void init_OutputFileParam(PyObject* module);
void init_PathParam(PyObject* module);
void init_PageParam(PyObject* module);
void init_ButtonParam(PyObject* module);
void init_ParametricParam(PyObject* module);
void init_GroupParam(PyObject* module);
void init_Int2DTuple(PyObject* module);
void init_Int3DTuple(PyObject* module);
void init_Double2DTuple(PyObject* module);
void init_Double3DTuple(PyObject* module);
void init_ColorTuple(PyObject* module);
void init_PyCoreApplication(PyObject* module);
void init_Group(PyObject* module);
void init_App(PyObject* module);
void init_Effect(PyObject* module);
void init_AppSettings(PyObject* module);
void init_ItemBase(PyObject* module);
void init_Layer(PyObject* module);
void init_BezierCurve(PyObject* module);
void init_RectI(PyObject* module);
void init_RectD(PyObject* module);
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

// C++ to Python conversion for type 'std::vector<RectI >'.
static PyObject* std_vector_RectI__CppToPython_std_vector_RectI_(const void* cppIn) {
    ::std::vector<RectI >& cppInRef = *((::std::vector<RectI >*)cppIn);

                    // TEMPLATE - stdVectorToPyList - START
            ::std::vector<RectI >::size_type vectorSize = cppInRef.size();
            PyObject* pyOut = PyList_New((int) vectorSize);
            for (::std::vector<RectI >::size_type idx = 0; idx < vectorSize; ++idx) {
            ::RectI cppItem(cppInRef[idx]);
            PyList_SET_ITEM(pyOut, idx, Shiboken::Conversions::copyToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_RECTI_IDX], &cppItem));
            }
            return pyOut;
        // TEMPLATE - stdVectorToPyList - END

}
static void std_vector_RectI__PythonToCpp_std_vector_RectI_(PyObject* pyIn, void* cppOut) {
    ::std::vector<RectI >& cppOutRef = *((::std::vector<RectI >*)cppOut);

                    // TEMPLATE - pySeqToStdVector - START
        int vectorSize = PySequence_Size(pyIn);
        cppOutRef.reserve(vectorSize);
        for (int idx = 0; idx < vectorSize; ++idx) {
        Shiboken::AutoDecRef pyItem(PySequence_GetItem(pyIn, idx));
        ::RectI cppItem = ::RectI();
        Shiboken::Conversions::pythonToCppCopy((SbkObjectType*)SbkNatronEngineTypes[SBK_RECTI_IDX], pyItem, &(cppItem));
        cppOutRef.push_back(cppItem);
        }
    // TEMPLATE - pySeqToStdVector - END

}
static PythonToCppFunc is_std_vector_RectI__PythonToCpp_std_vector_RectI__Convertible(PyObject* pyIn) {
    if (Shiboken::Conversions::convertibleSequenceTypes((SbkObjectType*)SbkNatronEngineTypes[SBK_RECTI_IDX], pyIn))
        return std_vector_RectI__PythonToCpp_std_vector_RectI_;
    return 0;
}

// C++ to Python conversion for type 'std::list<ItemBase * >'.
static PyObject* std_list_ItemBasePTR__CppToPython_std_list_ItemBasePTR_(const void* cppIn) {
    ::std::list<ItemBase * >& cppInRef = *((::std::list<ItemBase * >*)cppIn);

                    // TEMPLATE - stdListToPyList - START
            PyObject* pyOut = PyList_New((int) cppInRef.size());
            ::std::list<ItemBase * >::const_iterator it = cppInRef.begin();
            for (int idx = 0; it != cppInRef.end(); ++it, ++idx) {
            ::ItemBase* cppItem(*it);
            PyList_SET_ITEM(pyOut, idx, Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_ITEMBASE_IDX], cppItem));
            }
            return pyOut;
        // TEMPLATE - stdListToPyList - END

}
static void std_list_ItemBasePTR__PythonToCpp_std_list_ItemBasePTR_(PyObject* pyIn, void* cppOut) {
    ::std::list<ItemBase * >& cppOutRef = *((::std::list<ItemBase * >*)cppOut);

                    // TEMPLATE - pyListToStdList - START
        for (int i = 0; i < PySequence_Size(pyIn); i++) {
        Shiboken::AutoDecRef pyItem(PySequence_GetItem(pyIn, i));
        ::ItemBase* cppItem = ((::ItemBase*)0);
        Shiboken::Conversions::pythonToCppPointer((SbkObjectType*)SbkNatronEngineTypes[SBK_ITEMBASE_IDX], pyItem, &(cppItem));
        cppOutRef.push_back(cppItem);
        }
    // TEMPLATE - pyListToStdList - END

}
static PythonToCppFunc is_std_list_ItemBasePTR__PythonToCpp_std_list_ItemBasePTR__Convertible(PyObject* pyIn) {
    if (Shiboken::Conversions::checkSequenceTypes(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], pyIn))
        return std_list_ItemBasePTR__PythonToCpp_std_list_ItemBasePTR_;
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

// C++ to Python conversion for type 'std::list<Effect * >'.
static PyObject* std_list_EffectPTR__CppToPython_std_list_EffectPTR_(const void* cppIn) {
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
static void std_list_EffectPTR__PythonToCpp_std_list_EffectPTR_(PyObject* pyIn, void* cppOut) {
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
static PythonToCppFunc is_std_list_EffectPTR__PythonToCpp_std_list_EffectPTR__Convertible(PyObject* pyIn) {
    if (Shiboken::Conversions::checkSequenceTypes(SbkNatronEngineTypes[SBK_EFFECT_IDX], pyIn))
        return std_list_EffectPTR__PythonToCpp_std_list_EffectPTR_;
    return 0;
}

// C++ to Python conversion for type 'const std::list<int > &'.
static PyObject* conststd_list_int_REF_CppToPython_conststd_list_int_REF(const void* cppIn) {
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
static void conststd_list_int_REF_PythonToCpp_conststd_list_int_REF(PyObject* pyIn, void* cppOut) {
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
static PythonToCppFunc is_conststd_list_int_REF_PythonToCpp_conststd_list_int_REF_Convertible(PyObject* pyIn) {
    if (Shiboken::Conversions::convertibleSequenceTypes(Shiboken::Conversions::PrimitiveTypeConverter<int>(), pyIn))
        return conststd_list_int_REF_PythonToCpp_conststd_list_int_REF;
    return 0;
}

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

// C++ to Python conversion for type 'std::vector<std::string >'.
static PyObject* std_vector_std_string__CppToPython_std_vector_std_string_(const void* cppIn) {
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
static void std_vector_std_string__PythonToCpp_std_vector_std_string_(PyObject* pyIn, void* cppOut) {
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
static PythonToCppFunc is_std_vector_std_string__PythonToCpp_std_vector_std_string__Convertible(PyObject* pyIn) {
    if (Shiboken::Conversions::convertibleSequenceTypes(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), pyIn))
        return std_vector_std_string__PythonToCpp_std_vector_std_string_;
    return 0;
}

// C++ to Python conversion for type 'std::pair<std::string, std::string >'.
static PyObject* std_pair_std_string_std_string__CppToPython_std_pair_std_string_std_string_(const void* cppIn) {
    ::std::pair<std::string, std::string >& cppInRef = *((::std::pair<std::string, std::string >*)cppIn);

                    PyObject* pyOut = PyTuple_New(2);
                    PyTuple_SET_ITEM(pyOut, 0, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), &cppInRef.first));
                    PyTuple_SET_ITEM(pyOut, 1, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), &cppInRef.second));
                    return pyOut;

}
static void std_pair_std_string_std_string__PythonToCpp_std_pair_std_string_std_string_(PyObject* pyIn, void* cppOut) {
    ::std::pair<std::string, std::string >& cppOutRef = *((::std::pair<std::string, std::string >*)cppOut);

                    Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), PySequence_Fast_GET_ITEM(pyIn, 0), &(cppOutRef.first));
                    Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), PySequence_Fast_GET_ITEM(pyIn, 1), &(cppOutRef.second));

}
static PythonToCppFunc is_std_pair_std_string_std_string__PythonToCpp_std_pair_std_string_std_string__Convertible(PyObject* pyIn) {
    if (Shiboken::Conversions::convertiblePairTypes(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), false, Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), false, pyIn))
        return std_pair_std_string_std_string__PythonToCpp_std_pair_std_string_std_string_;
    return 0;
}

// C++ to Python conversion for type 'const std::list<std::pair<std::string, std::string > > &'.
static PyObject* conststd_list_std_pair_std_string_std_string__REF_CppToPython_conststd_list_std_pair_std_string_std_string__REF(const void* cppIn) {
    ::std::list<std::pair<std::string, std::string > >& cppInRef = *((::std::list<std::pair<std::string, std::string > >*)cppIn);

                    // TEMPLATE - stdListToPyList - START
            PyObject* pyOut = PyList_New((int) cppInRef.size());
            ::std::list<std::pair<std::string, std::string > >::const_iterator it = cppInRef.begin();
            for (int idx = 0; it != cppInRef.end(); ++it, ++idx) {
            ::std::pair<std::string, std::string > cppItem(*it);
            PyList_SET_ITEM(pyOut, idx, Shiboken::Conversions::copyToPython(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_PAIR_STD_STRING_STD_STRING_IDX], &cppItem));
            }
            return pyOut;
        // TEMPLATE - stdListToPyList - END

}
static void conststd_list_std_pair_std_string_std_string__REF_PythonToCpp_conststd_list_std_pair_std_string_std_string__REF(PyObject* pyIn, void* cppOut) {
    ::std::list<std::pair<std::string, std::string > >& cppOutRef = *((::std::list<std::pair<std::string, std::string > >*)cppOut);

                    // TEMPLATE - pyListToStdList - START
        for (int i = 0; i < PySequence_Size(pyIn); i++) {
        Shiboken::AutoDecRef pyItem(PySequence_GetItem(pyIn, i));
        ::std::pair<std::string, std::string > cppItem = ::std::pair<std::string, std::string >();
        Shiboken::Conversions::pythonToCppCopy(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_PAIR_STD_STRING_STD_STRING_IDX], pyItem, &(cppItem));
        cppOutRef.push_back(cppItem);
        }
    // TEMPLATE - pyListToStdList - END

}
static PythonToCppFunc is_conststd_list_std_pair_std_string_std_string__REF_PythonToCpp_conststd_list_std_pair_std_string_std_string__REF_Convertible(PyObject* pyIn) {
    if (Shiboken::Conversions::convertibleSequenceTypes(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_PAIR_STD_STRING_STD_STRING_IDX], pyIn))
        return conststd_list_std_pair_std_string_std_string__REF_PythonToCpp_conststd_list_std_pair_std_string_std_string__REF;
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
    init_Roto(module);
    init_UserParamHolder(module);
    init_Param(module);
    init_AnimatedParam(module);
    init_IntParam(module);
    init_Int2DParam(module);
    init_Int3DParam(module);
    init_DoubleParam(module);
    init_Double2DParam(module);
    init_Double3DParam(module);
    init_ColorParam(module);
    init_ChoiceParam(module);
    init_BooleanParam(module);
    init_StringParamBase(module);
    init_StringParam(module);
    init_FileParam(module);
    init_OutputFileParam(module);
    init_PathParam(module);
    init_PageParam(module);
    init_ButtonParam(module);
    init_ParametricParam(module);
    init_GroupParam(module);
    init_Int2DTuple(module);
    init_Int3DTuple(module);
    init_Double2DTuple(module);
    init_Double3DTuple(module);
    init_ColorTuple(module);
    init_PyCoreApplication(module);
    init_Group(module);
    init_App(module);
    init_Effect(module);
    init_AppSettings(module);
    init_ItemBase(module);
    init_Layer(module);
    init_BezierCurve(module);
    init_RectI(module);
    init_RectD(module);
    init_Natron(module);

    // Register converter for type 'NatronEngine.std::size_t'.
    SbkNatronEngineTypeConverters[SBK_STD_SIZE_T_IDX] = Shiboken::Conversions::createConverter(&PyLong_Type, std_size_t_CppToPython_std_size_t);
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_STD_SIZE_T_IDX], "std::size_t");
    // Add user defined implicit conversions to type converter.
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronEngineTypeConverters[SBK_STD_SIZE_T_IDX],
        PyLong_PythonToCpp_std_size_t,
        is_PyLong_PythonToCpp_std_size_t_Convertible);


    // Register converter for type 'std::vector<RectI>'.
    SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_VECTOR_RECTI_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, std_vector_RectI__CppToPython_std_vector_RectI_);
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_VECTOR_RECTI_IDX], "std::vector<RectI>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_VECTOR_RECTI_IDX],
        std_vector_RectI__PythonToCpp_std_vector_RectI_,
        is_std_vector_RectI__PythonToCpp_std_vector_RectI__Convertible);

    // Register converter for type 'std::list<ItemBase*>'.
    SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_ITEMBASEPTR_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, std_list_ItemBasePTR__CppToPython_std_list_ItemBasePTR_);
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_ITEMBASEPTR_IDX], "std::list<ItemBase*>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_ITEMBASEPTR_IDX],
        std_list_ItemBasePTR__PythonToCpp_std_list_ItemBasePTR_,
        is_std_list_ItemBasePTR__PythonToCpp_std_list_ItemBasePTR__Convertible);

    // Register converter for type 'std::list<Param*>'.
    SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_PARAMPTR_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, std_list_ParamPTR__CppToPython_std_list_ParamPTR_);
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_PARAMPTR_IDX], "std::list<Param*>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_PARAMPTR_IDX],
        std_list_ParamPTR__PythonToCpp_std_list_ParamPTR_,
        is_std_list_ParamPTR__PythonToCpp_std_list_ParamPTR__Convertible);

    // Register converter for type 'std::list<Effect*>'.
    SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_EFFECTPTR_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, std_list_EffectPTR__CppToPython_std_list_EffectPTR_);
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_EFFECTPTR_IDX], "std::list<Effect*>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_EFFECTPTR_IDX],
        std_list_EffectPTR__PythonToCpp_std_list_EffectPTR_,
        is_std_list_EffectPTR__PythonToCpp_std_list_EffectPTR__Convertible);

    // Register converter for type 'const std::list<int>&'.
    SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_INT_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, conststd_list_int_REF_CppToPython_conststd_list_int_REF);
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_INT_IDX], "const std::list<int>&");
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_INT_IDX], "std::list<int>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_INT_IDX],
        conststd_list_int_REF_PythonToCpp_conststd_list_int_REF,
        is_conststd_list_int_REF_PythonToCpp_conststd_list_int_REF_Convertible);

    // Register converter for type 'std::list<std::string>'.
    SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_STD_STRING_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, std_list_std_string__CppToPython_std_list_std_string_);
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_STD_STRING_IDX], "std::list<std::string>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_STD_STRING_IDX],
        std_list_std_string__PythonToCpp_std_list_std_string_,
        is_std_list_std_string__PythonToCpp_std_list_std_string__Convertible);

    // Register converter for type 'std::vector<std::string>'.
    SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_VECTOR_STD_STRING_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, std_vector_std_string__CppToPython_std_vector_std_string_);
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_VECTOR_STD_STRING_IDX], "std::vector<std::string>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_VECTOR_STD_STRING_IDX],
        std_vector_std_string__PythonToCpp_std_vector_std_string_,
        is_std_vector_std_string__PythonToCpp_std_vector_std_string__Convertible);

    // Register converter for type 'std::pair<std::string,std::string>'.
    SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_PAIR_STD_STRING_STD_STRING_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, std_pair_std_string_std_string__CppToPython_std_pair_std_string_std_string_);
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_PAIR_STD_STRING_STD_STRING_IDX], "std::pair<std::string,std::string>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_PAIR_STD_STRING_STD_STRING_IDX],
        std_pair_std_string_std_string__PythonToCpp_std_pair_std_string_std_string_,
        is_std_pair_std_string_std_string__PythonToCpp_std_pair_std_string_std_string__Convertible);

    // Register converter for type 'const std::list<std::pair<std::string,std::string>>&'.
    SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_STD_PAIR_STD_STRING_STD_STRING_IDX] = Shiboken::Conversions::createConverter(&PyList_Type, conststd_list_std_pair_std_string_std_string__REF_CppToPython_conststd_list_std_pair_std_string_std_string__REF);
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_STD_PAIR_STD_STRING_STD_STRING_IDX], "const std::list<std::pair<std::string,std::string>>&");
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_STD_PAIR_STD_STRING_STD_STRING_IDX], "std::list<std::pair<std::string,std::string>>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_STD_PAIR_STD_STRING_STD_STRING_IDX],
        conststd_list_std_pair_std_string_std_string__REF_PythonToCpp_conststd_list_std_pair_std_string_std_string__REF,
        is_conststd_list_std_pair_std_string_std_string__REF_PythonToCpp_conststd_list_std_pair_std_string_std_string__REF_Convertible);

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
    PySide::registerCleanupFunction(cleanTypesAttributes);
SBK_MODULE_INIT_FUNCTION_END
