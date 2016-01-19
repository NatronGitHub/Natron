
#include <sbkpython.h>
#include "Global/Macros.h"
CLANG_DIAG_OFF(mismatched-tags)
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_OFF(missing-field-initializers)
GCC_DIAG_OFF(missing-declarations)
#include <shiboken.h> // produces many warnings
#include <algorithm>
#include <pyside.h>
#include "natronengine_python.h"



// Extra includes
NATRON_NAMESPACE_USING

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
void init_ImageLayer(PyObject* module);
void init_UserParamHolder(PyObject* module);
void init_Param(PyObject* module);
void init_ParametricParam(PyObject* module);
void init_SeparatorParam(PyObject* module);
void init_GroupParam(PyObject* module);
void init_AnimatedParam(PyObject* module);
void init_DoubleParam(PyObject* module);
void init_Double2DParam(PyObject* module);
void init_Double3DParam(PyObject* module);
void init_ColorParam(PyObject* module);
void init_ChoiceParam(PyObject* module);
void init_IntParam(PyObject* module);
void init_Int2DParam(PyObject* module);
void init_Int3DParam(PyObject* module);
void init_BooleanParam(PyObject* module);
void init_StringParamBase(PyObject* module);
void init_StringParam(PyObject* module);
void init_FileParam(PyObject* module);
void init_OutputFileParam(PyObject* module);
void init_PathParam(PyObject* module);
void init_PageParam(PyObject* module);
void init_ButtonParam(PyObject* module);
void init_PyCoreApplication(PyObject* module);
void init_Group(PyObject* module);
void init_App(PyObject* module);
void init_Effect(PyObject* module);
void init_AppSettings(PyObject* module);
void init_ItemBase(PyObject* module);
void init_Layer(PyObject* module);
void init_BezierCurve(PyObject* module);
void init_Int2DTuple(PyObject* module);
void init_RectI(PyObject* module);
void init_Int3DTuple(PyObject* module);
void init_Double2DTuple(PyObject* module);
void init_Double3DTuple(PyObject* module);
void init_ColorTuple(PyObject* module);
void init_RectD(PyObject* module);

// Enum definitions ------------------------------------------------------------
static void AnimationLevelEnum_PythonToCpp_AnimationLevelEnum(PyObject* pyIn, void* cppOut) {
    *((::AnimationLevelEnum*)cppOut) = (::AnimationLevelEnum) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_AnimationLevelEnum_PythonToCpp_AnimationLevelEnum_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_ANIMATIONLEVELENUM_IDX]))
        return AnimationLevelEnum_PythonToCpp_AnimationLevelEnum;
    return 0;
}
static PyObject* AnimationLevelEnum_CppToPython_AnimationLevelEnum(const void* cppIn) {
    int castCppIn = *((::AnimationLevelEnum*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_ANIMATIONLEVELENUM_IDX], castCppIn);

}


static void KeyframeTypeEnum_PythonToCpp_KeyframeTypeEnum(PyObject* pyIn, void* cppOut) {
    *((::KeyframeTypeEnum*)cppOut) = (::KeyframeTypeEnum) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_KeyframeTypeEnum_PythonToCpp_KeyframeTypeEnum_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_KEYFRAMETYPEENUM_IDX]))
        return KeyframeTypeEnum_PythonToCpp_KeyframeTypeEnum;
    return 0;
}
static PyObject* KeyframeTypeEnum_CppToPython_KeyframeTypeEnum(const void* cppIn) {
    int castCppIn = *((::KeyframeTypeEnum*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_KEYFRAMETYPEENUM_IDX], castCppIn);

}


static void PixmapEnum_PythonToCpp_PixmapEnum(PyObject* pyIn, void* cppOut) {
    *((::PixmapEnum*)cppOut) = (::PixmapEnum) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_PixmapEnum_PythonToCpp_PixmapEnum_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX]))
        return PixmapEnum_PythonToCpp_PixmapEnum;
    return 0;
}
static PyObject* PixmapEnum_CppToPython_PixmapEnum(const void* cppIn) {
    int castCppIn = *((::PixmapEnum*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX], castCppIn);

}


static void ImagePremultiplicationEnum_PythonToCpp_ImagePremultiplicationEnum(PyObject* pyIn, void* cppOut) {
    *((::ImagePremultiplicationEnum*)cppOut) = (::ImagePremultiplicationEnum) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_ImagePremultiplicationEnum_PythonToCpp_ImagePremultiplicationEnum_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_IMAGEPREMULTIPLICATIONENUM_IDX]))
        return ImagePremultiplicationEnum_PythonToCpp_ImagePremultiplicationEnum;
    return 0;
}
static PyObject* ImagePremultiplicationEnum_CppToPython_ImagePremultiplicationEnum(const void* cppIn) {
    int castCppIn = *((::ImagePremultiplicationEnum*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_IMAGEPREMULTIPLICATIONENUM_IDX], castCppIn);

}


static void ImageBitDepthEnum_PythonToCpp_ImageBitDepthEnum(PyObject* pyIn, void* cppOut) {
    *((::ImageBitDepthEnum*)cppOut) = (::ImageBitDepthEnum) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_ImageBitDepthEnum_PythonToCpp_ImageBitDepthEnum_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_IMAGEBITDEPTHENUM_IDX]))
        return ImageBitDepthEnum_PythonToCpp_ImageBitDepthEnum;
    return 0;
}
static PyObject* ImageBitDepthEnum_CppToPython_ImageBitDepthEnum(const void* cppIn) {
    int castCppIn = *((::ImageBitDepthEnum*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_IMAGEBITDEPTHENUM_IDX], castCppIn);

}


static void ViewerCompositingOperatorEnum_PythonToCpp_ViewerCompositingOperatorEnum(PyObject* pyIn, void* cppOut) {
    *((::ViewerCompositingOperatorEnum*)cppOut) = (::ViewerCompositingOperatorEnum) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_ViewerCompositingOperatorEnum_PythonToCpp_ViewerCompositingOperatorEnum_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_VIEWERCOMPOSITINGOPERATORENUM_IDX]))
        return ViewerCompositingOperatorEnum_PythonToCpp_ViewerCompositingOperatorEnum;
    return 0;
}
static PyObject* ViewerCompositingOperatorEnum_CppToPython_ViewerCompositingOperatorEnum(const void* cppIn) {
    int castCppIn = *((::ViewerCompositingOperatorEnum*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_VIEWERCOMPOSITINGOPERATORENUM_IDX], castCppIn);

}


static void PlaybackModeEnum_PythonToCpp_PlaybackModeEnum(PyObject* pyIn, void* cppOut) {
    *((::PlaybackModeEnum*)cppOut) = (::PlaybackModeEnum) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_PlaybackModeEnum_PythonToCpp_PlaybackModeEnum_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_PLAYBACKMODEENUM_IDX]))
        return PlaybackModeEnum_PythonToCpp_PlaybackModeEnum;
    return 0;
}
static PyObject* PlaybackModeEnum_CppToPython_PlaybackModeEnum(const void* cppIn) {
    int castCppIn = *((::PlaybackModeEnum*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_PLAYBACKMODEENUM_IDX], castCppIn);

}


static void DisplayChannelsEnum_PythonToCpp_DisplayChannelsEnum(PyObject* pyIn, void* cppOut) {
    *((::DisplayChannelsEnum*)cppOut) = (::DisplayChannelsEnum) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_DisplayChannelsEnum_PythonToCpp_DisplayChannelsEnum_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_DISPLAYCHANNELSENUM_IDX]))
        return DisplayChannelsEnum_PythonToCpp_DisplayChannelsEnum;
    return 0;
}
static PyObject* DisplayChannelsEnum_CppToPython_DisplayChannelsEnum(const void* cppIn) {
    int castCppIn = *((::DisplayChannelsEnum*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_DISPLAYCHANNELSENUM_IDX], castCppIn);

}


static void OrientationEnum_PythonToCpp_OrientationEnum(PyObject* pyIn, void* cppOut) {
    *((::OrientationEnum*)cppOut) = (::OrientationEnum) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_OrientationEnum_PythonToCpp_OrientationEnum_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_ORIENTATIONENUM_IDX]))
        return OrientationEnum_PythonToCpp_OrientationEnum;
    return 0;
}
static PyObject* OrientationEnum_CppToPython_OrientationEnum(const void* cppIn) {
    int castCppIn = *((::OrientationEnum*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_ORIENTATIONENUM_IDX], castCppIn);

}


static void StandardButtonEnum_PythonToCpp_StandardButtonEnum(PyObject* pyIn, void* cppOut) {
    *((::StandardButtonEnum*)cppOut) = (::StandardButtonEnum) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_StandardButtonEnum_PythonToCpp_StandardButtonEnum_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_STANDARDBUTTONENUM_IDX]))
        return StandardButtonEnum_PythonToCpp_StandardButtonEnum;
    return 0;
}
static PyObject* StandardButtonEnum_CppToPython_StandardButtonEnum(const void* cppIn) {
    int castCppIn = *((::StandardButtonEnum*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_STANDARDBUTTONENUM_IDX], castCppIn);

}

static void QFlags_StandardButtonEnum__PythonToCpp_QFlags_StandardButtonEnum_(PyObject* pyIn, void* cppOut) {
    *((::QFlags<StandardButtonEnum>*)cppOut) = ::QFlags<StandardButtonEnum>(QFlag(PySide::QFlags::getValue(reinterpret_cast<PySideQFlagsObject*>(pyIn))));

}
static PythonToCppFunc is_QFlags_StandardButtonEnum__PythonToCpp_QFlags_StandardButtonEnum__Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_QFLAGS_STANDARDBUTTONENUM__IDX]))
        return QFlags_StandardButtonEnum__PythonToCpp_QFlags_StandardButtonEnum_;
    return 0;
}
static PyObject* QFlags_StandardButtonEnum__CppToPython_QFlags_StandardButtonEnum_(const void* cppIn) {
    int castCppIn = *((::QFlags<StandardButtonEnum>*)cppIn);
    return reinterpret_cast<PyObject*>(PySide::QFlags::newObject(castCppIn, SbkNatronEngineTypes[SBK_QFLAGS_STANDARDBUTTONENUM__IDX]));

}

static void StandardButtonEnum_PythonToCpp_QFlags_StandardButtonEnum_(PyObject* pyIn, void* cppOut) {
    *((::QFlags<StandardButtonEnum>*)cppOut) = ::QFlags<StandardButtonEnum>(QFlag(Shiboken::Enum::getValue(pyIn)));

}
static PythonToCppFunc is_StandardButtonEnum_PythonToCpp_QFlags_StandardButtonEnum__Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_STANDARDBUTTONENUM_IDX]))
        return StandardButtonEnum_PythonToCpp_QFlags_StandardButtonEnum_;
    return 0;
}
static void number_PythonToCpp_QFlags_StandardButtonEnum_(PyObject* pyIn, void* cppOut) {
    Shiboken::AutoDecRef pyLong(PyNumber_Long(pyIn));
    *((::QFlags<StandardButtonEnum>*)cppOut) = ::QFlags<StandardButtonEnum>(QFlag(PyLong_AsLong(pyLong.object())));

}
static PythonToCppFunc is_number_PythonToCpp_QFlags_StandardButtonEnum__Convertible(PyObject* pyIn) {
    if (PyNumber_Check(pyIn))
        return number_PythonToCpp_QFlags_StandardButtonEnum_;
    return 0;
}

static void ImageComponentsEnum_PythonToCpp_ImageComponentsEnum(PyObject* pyIn, void* cppOut) {
    *((::ImageComponentsEnum*)cppOut) = (::ImageComponentsEnum) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_ImageComponentsEnum_PythonToCpp_ImageComponentsEnum_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_IMAGECOMPONENTSENUM_IDX]))
        return ImageComponentsEnum_PythonToCpp_ImageComponentsEnum;
    return 0;
}
static PyObject* ImageComponentsEnum_CppToPython_ImageComponentsEnum(const void* cppIn) {
    int castCppIn = *((::ImageComponentsEnum*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_IMAGECOMPONENTSENUM_IDX], castCppIn);

}


static void ValueChangedReasonEnum_PythonToCpp_ValueChangedReasonEnum(PyObject* pyIn, void* cppOut) {
    *((::ValueChangedReasonEnum*)cppOut) = (::ValueChangedReasonEnum) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_ValueChangedReasonEnum_PythonToCpp_ValueChangedReasonEnum_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_VALUECHANGEDREASONENUM_IDX]))
        return ValueChangedReasonEnum_PythonToCpp_ValueChangedReasonEnum;
    return 0;
}
static PyObject* ValueChangedReasonEnum_CppToPython_ValueChangedReasonEnum(const void* cppIn) {
    int castCppIn = *((::ValueChangedReasonEnum*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_VALUECHANGEDREASONENUM_IDX], castCppIn);

}


static void StatusEnum_PythonToCpp_StatusEnum(PyObject* pyIn, void* cppOut) {
    *((::StatusEnum*)cppOut) = (::StatusEnum) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_StatusEnum_PythonToCpp_StatusEnum_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_STATUSENUM_IDX]))
        return StatusEnum_PythonToCpp_StatusEnum;
    return 0;
}
static PyObject* StatusEnum_CppToPython_StatusEnum(const void* cppIn) {
    int castCppIn = *((::StatusEnum*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_STATUSENUM_IDX], castCppIn);

}


static void ViewerColorSpaceEnum_PythonToCpp_ViewerColorSpaceEnum(PyObject* pyIn, void* cppOut) {
    *((::ViewerColorSpaceEnum*)cppOut) = (::ViewerColorSpaceEnum) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_ViewerColorSpaceEnum_PythonToCpp_ViewerColorSpaceEnum_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_VIEWERCOLORSPACEENUM_IDX]))
        return ViewerColorSpaceEnum_PythonToCpp_ViewerColorSpaceEnum;
    return 0;
}
static PyObject* ViewerColorSpaceEnum_CppToPython_ViewerColorSpaceEnum(const void* cppIn) {
    int castCppIn = *((::ViewerColorSpaceEnum*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_VIEWERCOLORSPACEENUM_IDX], castCppIn);

}


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

// C++ to Python conversion for type 'std::map<ImageLayer, Effect * >'.
static PyObject* std_map_ImageLayer_EffectPTR__CppToPython_std_map_ImageLayer_EffectPTR_(const void* cppIn) {
    ::std::map<ImageLayer, Effect * >& cppInRef = *((::std::map<ImageLayer, Effect * >*)cppIn);

                    // TEMPLATE - stdMapToPyDict - START
            PyObject* pyOut = PyDict_New();
            ::std::map<ImageLayer, Effect * >::const_iterator it = cppInRef.begin();
            for (; it != cppInRef.end(); ++it) {
            ::ImageLayer key = it->first;
            ::Effect* value = it->second;
            PyObject* pyKey = Shiboken::Conversions::copyToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], &key);
            PyObject* pyValue = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_EFFECT_IDX], value);
            PyDict_SetItem(pyOut, pyKey, pyValue);
            Py_DECREF(pyKey);
            Py_DECREF(pyValue);
            }
            return pyOut;
        // TEMPLATE - stdMapToPyDict - END

}
static void std_map_ImageLayer_EffectPTR__PythonToCpp_std_map_ImageLayer_EffectPTR_(PyObject* pyIn, void* cppOut) {
    ::std::map<ImageLayer, Effect * >& cppOutRef = *((::std::map<ImageLayer, Effect * >*)cppOut);

                    // TEMPLATE - pyDictToStdMap - START
        PyObject* key;
        PyObject* value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(pyIn, &pos, &key, &value)) {
        ::ImageLayer cppKey = ::ImageLayer(::std::string(), ::std::string(), ::std::vector<std::string >());
        Shiboken::Conversions::pythonToCppCopy((SbkObjectType*)SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], key, &(cppKey));
        ::Effect* cppValue = ((::Effect*)0);
        Shiboken::Conversions::pythonToCppPointer((SbkObjectType*)SbkNatronEngineTypes[SBK_EFFECT_IDX], value, &(cppValue));
        cppOutRef.insert(std::make_pair(cppKey, cppValue));
        }
    // TEMPLATE - pyDictToStdMap - END

}
static PythonToCppFunc is_std_map_ImageLayer_EffectPTR__PythonToCpp_std_map_ImageLayer_EffectPTR__Convertible(PyObject* pyIn) {
    if (Shiboken::Conversions::convertibleDictTypes(SBK_CONVERTER(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX]), false, SBK_CONVERTER(SbkNatronEngineTypes[SBK_EFFECT_IDX]), true, pyIn))
        return std_map_ImageLayer_EffectPTR__PythonToCpp_std_map_ImageLayer_EffectPTR_;
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
    init_ImageLayer(module);
    init_UserParamHolder(module);
    init_Param(module);
    init_ParametricParam(module);
    init_SeparatorParam(module);
    init_GroupParam(module);
    init_AnimatedParam(module);
    init_DoubleParam(module);
    init_Double2DParam(module);
    init_Double3DParam(module);
    init_ColorParam(module);
    init_ChoiceParam(module);
    init_IntParam(module);
    init_Int2DParam(module);
    init_Int3DParam(module);
    init_BooleanParam(module);
    init_StringParamBase(module);
    init_StringParam(module);
    init_FileParam(module);
    init_OutputFileParam(module);
    init_PathParam(module);
    init_PageParam(module);
    init_ButtonParam(module);
    init_PyCoreApplication(module);
    init_Group(module);
    init_App(module);
    init_Effect(module);
    init_AppSettings(module);
    init_ItemBase(module);
    init_Layer(module);
    init_BezierCurve(module);
    init_Int2DTuple(module);
    init_RectI(module);
    init_Int3DTuple(module);
    init_Double2DTuple(module);
    init_Double3DTuple(module);
    init_ColorTuple(module);
    init_RectD(module);

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

    // Register converter for type 'std::map<ImageLayer,Effect*>'.
    SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_MAP_IMAGELAYER_EFFECTPTR_IDX] = Shiboken::Conversions::createConverter(&PyDict_Type, std_map_ImageLayer_EffectPTR__CppToPython_std_map_ImageLayer_EffectPTR_);
    Shiboken::Conversions::registerConverterName(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_MAP_IMAGELAYER_EFFECTPTR_IDX], "std::map<ImageLayer,Effect*>");
    Shiboken::Conversions::addPythonToCppValueConversion(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_MAP_IMAGELAYER_EFFECTPTR_IDX],
        std_map_ImageLayer_EffectPTR__PythonToCpp_std_map_ImageLayer_EffectPTR_,
        is_std_map_ImageLayer_EffectPTR__PythonToCpp_std_map_ImageLayer_EffectPTR__Convertible);

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

    // Initialization of enums.

    // Initialization of enum 'AnimationLevelEnum'.
    SbkNatronEngineTypes[SBK_ANIMATIONLEVELENUM_IDX] = Shiboken::Enum::createGlobalEnum(module,
        "AnimationLevelEnum",
        "NatronEngine.AnimationLevelEnum",
        "AnimationLevelEnum");
    if (!SbkNatronEngineTypes[SBK_ANIMATIONLEVELENUM_IDX])
        return SBK_MODULE_INIT_ERROR;

    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_ANIMATIONLEVELENUM_IDX],
        module, "eAnimationLevelNone", (long) eAnimationLevelNone))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_ANIMATIONLEVELENUM_IDX],
        module, "eAnimationLevelInterpolatedValue", (long) eAnimationLevelInterpolatedValue))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_ANIMATIONLEVELENUM_IDX],
        module, "eAnimationLevelOnKeyframe", (long) eAnimationLevelOnKeyframe))
        return SBK_MODULE_INIT_ERROR;
    // Register converter for enum 'AnimationLevelEnum'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_ANIMATIONLEVELENUM_IDX],
            AnimationLevelEnum_CppToPython_AnimationLevelEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            AnimationLevelEnum_PythonToCpp_AnimationLevelEnum,
            is_AnimationLevelEnum_PythonToCpp_AnimationLevelEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_ANIMATIONLEVELENUM_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_ANIMATIONLEVELENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "AnimationLevelEnum");
    }
    // End of 'AnimationLevelEnum' enum.

    // Initialization of enum 'KeyframeTypeEnum'.
    SbkNatronEngineTypes[SBK_KEYFRAMETYPEENUM_IDX] = Shiboken::Enum::createGlobalEnum(module,
        "KeyframeTypeEnum",
        "NatronEngine.KeyframeTypeEnum",
        "KeyframeTypeEnum");
    if (!SbkNatronEngineTypes[SBK_KEYFRAMETYPEENUM_IDX])
        return SBK_MODULE_INIT_ERROR;

    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_KEYFRAMETYPEENUM_IDX],
        module, "eKeyframeTypeConstant", (long) eKeyframeTypeConstant))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_KEYFRAMETYPEENUM_IDX],
        module, "eKeyframeTypeLinear", (long) eKeyframeTypeLinear))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_KEYFRAMETYPEENUM_IDX],
        module, "eKeyframeTypeSmooth", (long) eKeyframeTypeSmooth))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_KEYFRAMETYPEENUM_IDX],
        module, "eKeyframeTypeCatmullRom", (long) eKeyframeTypeCatmullRom))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_KEYFRAMETYPEENUM_IDX],
        module, "eKeyframeTypeCubic", (long) eKeyframeTypeCubic))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_KEYFRAMETYPEENUM_IDX],
        module, "eKeyframeTypeHorizontal", (long) eKeyframeTypeHorizontal))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_KEYFRAMETYPEENUM_IDX],
        module, "eKeyframeTypeFree", (long) eKeyframeTypeFree))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_KEYFRAMETYPEENUM_IDX],
        module, "eKeyframeTypeBroken", (long) eKeyframeTypeBroken))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_KEYFRAMETYPEENUM_IDX],
        module, "eKeyframeTypeNone", (long) eKeyframeTypeNone))
        return SBK_MODULE_INIT_ERROR;
    // Register converter for enum 'KeyframeTypeEnum'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_KEYFRAMETYPEENUM_IDX],
            KeyframeTypeEnum_CppToPython_KeyframeTypeEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            KeyframeTypeEnum_PythonToCpp_KeyframeTypeEnum,
            is_KeyframeTypeEnum_PythonToCpp_KeyframeTypeEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_KEYFRAMETYPEENUM_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_KEYFRAMETYPEENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "KeyframeTypeEnum");
    }
    // End of 'KeyframeTypeEnum' enum.

    // Initialization of enum 'PixmapEnum'.
    SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX] = Shiboken::Enum::createGlobalEnum(module,
        "PixmapEnum",
        "NatronEngine.PixmapEnum",
        "PixmapEnum");
    if (!SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX])
        return SBK_MODULE_INIT_ERROR;

    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_PLAYER_PREVIOUS", (long) NATRON_PIXMAP_PLAYER_PREVIOUS))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_PLAYER_FIRST_FRAME", (long) NATRON_PIXMAP_PLAYER_FIRST_FRAME))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_PLAYER_NEXT", (long) NATRON_PIXMAP_PLAYER_NEXT))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_PLAYER_LAST_FRAME", (long) NATRON_PIXMAP_PLAYER_LAST_FRAME))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_PLAYER_NEXT_INCR", (long) NATRON_PIXMAP_PLAYER_NEXT_INCR))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_PLAYER_NEXT_KEY", (long) NATRON_PIXMAP_PLAYER_NEXT_KEY))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_PLAYER_PREVIOUS_INCR", (long) NATRON_PIXMAP_PLAYER_PREVIOUS_INCR))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_PLAYER_PREVIOUS_KEY", (long) NATRON_PIXMAP_PLAYER_PREVIOUS_KEY))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_PLAYER_REWIND_ENABLED", (long) NATRON_PIXMAP_PLAYER_REWIND_ENABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_PLAYER_REWIND_DISABLED", (long) NATRON_PIXMAP_PLAYER_REWIND_DISABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_PLAYER_PLAY_ENABLED", (long) NATRON_PIXMAP_PLAYER_PLAY_ENABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_PLAYER_PLAY_DISABLED", (long) NATRON_PIXMAP_PLAYER_PLAY_DISABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_PLAYER_STOP", (long) NATRON_PIXMAP_PLAYER_STOP))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_PLAYER_LOOP_MODE", (long) NATRON_PIXMAP_PLAYER_LOOP_MODE))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_PLAYER_BOUNCE", (long) NATRON_PIXMAP_PLAYER_BOUNCE))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_PLAYER_PLAY_ONCE", (long) NATRON_PIXMAP_PLAYER_PLAY_ONCE))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MAXIMIZE_WIDGET", (long) NATRON_PIXMAP_MAXIMIZE_WIDGET))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MINIMIZE_WIDGET", (long) NATRON_PIXMAP_MINIMIZE_WIDGET))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_CLOSE_WIDGET", (long) NATRON_PIXMAP_CLOSE_WIDGET))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_HELP_WIDGET", (long) NATRON_PIXMAP_HELP_WIDGET))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_CLOSE_PANEL", (long) NATRON_PIXMAP_CLOSE_PANEL))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_HIDE_UNMODIFIED", (long) NATRON_PIXMAP_HIDE_UNMODIFIED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_UNHIDE_UNMODIFIED", (long) NATRON_PIXMAP_UNHIDE_UNMODIFIED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_GROUPBOX_FOLDED", (long) NATRON_PIXMAP_GROUPBOX_FOLDED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_GROUPBOX_UNFOLDED", (long) NATRON_PIXMAP_GROUPBOX_UNFOLDED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_UNDO", (long) NATRON_PIXMAP_UNDO))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_REDO", (long) NATRON_PIXMAP_REDO))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_UNDO_GRAYSCALE", (long) NATRON_PIXMAP_UNDO_GRAYSCALE))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_REDO_GRAYSCALE", (long) NATRON_PIXMAP_REDO_GRAYSCALE))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_RESTORE_DEFAULTS_ENABLED", (long) NATRON_PIXMAP_RESTORE_DEFAULTS_ENABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_RESTORE_DEFAULTS_DISABLED", (long) NATRON_PIXMAP_RESTORE_DEFAULTS_DISABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_TAB_WIDGET_LAYOUT_BUTTON", (long) NATRON_PIXMAP_TAB_WIDGET_LAYOUT_BUTTON))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_TAB_WIDGET_LAYOUT_BUTTON_ANCHOR", (long) NATRON_PIXMAP_TAB_WIDGET_LAYOUT_BUTTON_ANCHOR))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_TAB_WIDGET_SPLIT_HORIZONTALLY", (long) NATRON_PIXMAP_TAB_WIDGET_SPLIT_HORIZONTALLY))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_TAB_WIDGET_SPLIT_VERTICALLY", (long) NATRON_PIXMAP_TAB_WIDGET_SPLIT_VERTICALLY))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_VIEWER_CENTER", (long) NATRON_PIXMAP_VIEWER_CENTER))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_VIEWER_CLIP_TO_PROJECT_ENABLED", (long) NATRON_PIXMAP_VIEWER_CLIP_TO_PROJECT_ENABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_VIEWER_CLIP_TO_PROJECT_DISABLED", (long) NATRON_PIXMAP_VIEWER_CLIP_TO_PROJECT_DISABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_VIEWER_REFRESH", (long) NATRON_PIXMAP_VIEWER_REFRESH))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_VIEWER_REFRESH_ACTIVE", (long) NATRON_PIXMAP_VIEWER_REFRESH_ACTIVE))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_VIEWER_ROI_ENABLED", (long) NATRON_PIXMAP_VIEWER_ROI_ENABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_VIEWER_ROI_DISABLED", (long) NATRON_PIXMAP_VIEWER_ROI_DISABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_VIEWER_RENDER_SCALE", (long) NATRON_PIXMAP_VIEWER_RENDER_SCALE))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_VIEWER_RENDER_SCALE_CHECKED", (long) NATRON_PIXMAP_VIEWER_RENDER_SCALE_CHECKED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_VIEWER_AUTOCONTRAST_ENABLED", (long) NATRON_PIXMAP_VIEWER_AUTOCONTRAST_ENABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_VIEWER_AUTOCONTRAST_DISABLED", (long) NATRON_PIXMAP_VIEWER_AUTOCONTRAST_DISABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_OPEN_FILE", (long) NATRON_PIXMAP_OPEN_FILE))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_COLORWHEEL", (long) NATRON_PIXMAP_COLORWHEEL))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_OVERLAY", (long) NATRON_PIXMAP_OVERLAY))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_ROTO_MERGE", (long) NATRON_PIXMAP_ROTO_MERGE))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_COLOR_PICKER", (long) NATRON_PIXMAP_COLOR_PICKER))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_IO_GROUPING", (long) NATRON_PIXMAP_IO_GROUPING))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_3D_GROUPING", (long) NATRON_PIXMAP_3D_GROUPING))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_CHANNEL_GROUPING", (long) NATRON_PIXMAP_CHANNEL_GROUPING))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_GROUPING", (long) NATRON_PIXMAP_MERGE_GROUPING))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_COLOR_GROUPING", (long) NATRON_PIXMAP_COLOR_GROUPING))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_TRANSFORM_GROUPING", (long) NATRON_PIXMAP_TRANSFORM_GROUPING))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_DEEP_GROUPING", (long) NATRON_PIXMAP_DEEP_GROUPING))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_FILTER_GROUPING", (long) NATRON_PIXMAP_FILTER_GROUPING))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MULTIVIEW_GROUPING", (long) NATRON_PIXMAP_MULTIVIEW_GROUPING))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_TOOLSETS_GROUPING", (long) NATRON_PIXMAP_TOOLSETS_GROUPING))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MISC_GROUPING", (long) NATRON_PIXMAP_MISC_GROUPING))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_OPEN_EFFECTS_GROUPING", (long) NATRON_PIXMAP_OPEN_EFFECTS_GROUPING))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_TIME_GROUPING", (long) NATRON_PIXMAP_TIME_GROUPING))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_PAINT_GROUPING", (long) NATRON_PIXMAP_PAINT_GROUPING))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_KEYER_GROUPING", (long) NATRON_PIXMAP_KEYER_GROUPING))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_OTHER_PLUGINS", (long) NATRON_PIXMAP_OTHER_PLUGINS))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_READ_IMAGE", (long) NATRON_PIXMAP_READ_IMAGE))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_WRITE_IMAGE", (long) NATRON_PIXMAP_WRITE_IMAGE))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_COMBOBOX", (long) NATRON_PIXMAP_COMBOBOX))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_COMBOBOX_PRESSED", (long) NATRON_PIXMAP_COMBOBOX_PRESSED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_ADD_KEYFRAME", (long) NATRON_PIXMAP_ADD_KEYFRAME))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_REMOVE_KEYFRAME", (long) NATRON_PIXMAP_REMOVE_KEYFRAME))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_INVERTED", (long) NATRON_PIXMAP_INVERTED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_UNINVERTED", (long) NATRON_PIXMAP_UNINVERTED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_VISIBLE", (long) NATRON_PIXMAP_VISIBLE))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_UNVISIBLE", (long) NATRON_PIXMAP_UNVISIBLE))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_LOCKED", (long) NATRON_PIXMAP_LOCKED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_UNLOCKED", (long) NATRON_PIXMAP_UNLOCKED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_LAYER", (long) NATRON_PIXMAP_LAYER))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_BEZIER", (long) NATRON_PIXMAP_BEZIER))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_PENCIL", (long) NATRON_PIXMAP_PENCIL))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_CURVE", (long) NATRON_PIXMAP_CURVE))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_BEZIER_32", (long) NATRON_PIXMAP_BEZIER_32))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_ELLIPSE", (long) NATRON_PIXMAP_ELLIPSE))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_RECTANGLE", (long) NATRON_PIXMAP_RECTANGLE))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_ADD_POINTS", (long) NATRON_PIXMAP_ADD_POINTS))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_REMOVE_POINTS", (long) NATRON_PIXMAP_REMOVE_POINTS))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_CUSP_POINTS", (long) NATRON_PIXMAP_CUSP_POINTS))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_SMOOTH_POINTS", (long) NATRON_PIXMAP_SMOOTH_POINTS))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_REMOVE_FEATHER", (long) NATRON_PIXMAP_REMOVE_FEATHER))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_OPEN_CLOSE_CURVE", (long) NATRON_PIXMAP_OPEN_CLOSE_CURVE))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_SELECT_ALL", (long) NATRON_PIXMAP_SELECT_ALL))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_SELECT_POINTS", (long) NATRON_PIXMAP_SELECT_POINTS))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_SELECT_FEATHER", (long) NATRON_PIXMAP_SELECT_FEATHER))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_SELECT_CURVES", (long) NATRON_PIXMAP_SELECT_CURVES))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_AUTO_KEYING_ENABLED", (long) NATRON_PIXMAP_AUTO_KEYING_ENABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_AUTO_KEYING_DISABLED", (long) NATRON_PIXMAP_AUTO_KEYING_DISABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_STICKY_SELECTION_ENABLED", (long) NATRON_PIXMAP_STICKY_SELECTION_ENABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_STICKY_SELECTION_DISABLED", (long) NATRON_PIXMAP_STICKY_SELECTION_DISABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_FEATHER_LINK_ENABLED", (long) NATRON_PIXMAP_FEATHER_LINK_ENABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_FEATHER_LINK_DISABLED", (long) NATRON_PIXMAP_FEATHER_LINK_DISABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_FEATHER_VISIBLE", (long) NATRON_PIXMAP_FEATHER_VISIBLE))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_FEATHER_UNVISIBLE", (long) NATRON_PIXMAP_FEATHER_UNVISIBLE))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_RIPPLE_EDIT_ENABLED", (long) NATRON_PIXMAP_RIPPLE_EDIT_ENABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_RIPPLE_EDIT_DISABLED", (long) NATRON_PIXMAP_RIPPLE_EDIT_DISABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_ROTOPAINT_BLUR", (long) NATRON_PIXMAP_ROTOPAINT_BLUR))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_ROTOPAINT_BUILDUP_ENABLED", (long) NATRON_PIXMAP_ROTOPAINT_BUILDUP_ENABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_ROTOPAINT_BUILDUP_DISABLED", (long) NATRON_PIXMAP_ROTOPAINT_BUILDUP_DISABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_ROTOPAINT_BURN", (long) NATRON_PIXMAP_ROTOPAINT_BURN))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_ROTOPAINT_CLONE", (long) NATRON_PIXMAP_ROTOPAINT_CLONE))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_ROTOPAINT_DODGE", (long) NATRON_PIXMAP_ROTOPAINT_DODGE))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_ROTOPAINT_ERASER", (long) NATRON_PIXMAP_ROTOPAINT_ERASER))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_ROTOPAINT_PRESSURE_ENABLED", (long) NATRON_PIXMAP_ROTOPAINT_PRESSURE_ENABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_ROTOPAINT_PRESSURE_DISABLED", (long) NATRON_PIXMAP_ROTOPAINT_PRESSURE_DISABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_ROTOPAINT_REVEAL", (long) NATRON_PIXMAP_ROTOPAINT_REVEAL))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_ROTOPAINT_SHARPEN", (long) NATRON_PIXMAP_ROTOPAINT_SHARPEN))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_ROTOPAINT_SMEAR", (long) NATRON_PIXMAP_ROTOPAINT_SMEAR))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_ROTOPAINT_SOLID", (long) NATRON_PIXMAP_ROTOPAINT_SOLID))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_BOLD_CHECKED", (long) NATRON_PIXMAP_BOLD_CHECKED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_BOLD_UNCHECKED", (long) NATRON_PIXMAP_BOLD_UNCHECKED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_ITALIC_CHECKED", (long) NATRON_PIXMAP_ITALIC_CHECKED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_ITALIC_UNCHECKED", (long) NATRON_PIXMAP_ITALIC_UNCHECKED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_CLEAR_ALL_ANIMATION", (long) NATRON_PIXMAP_CLEAR_ALL_ANIMATION))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_CLEAR_BACKWARD_ANIMATION", (long) NATRON_PIXMAP_CLEAR_BACKWARD_ANIMATION))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_CLEAR_FORWARD_ANIMATION", (long) NATRON_PIXMAP_CLEAR_FORWARD_ANIMATION))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_UPDATE_VIEWER_ENABLED", (long) NATRON_PIXMAP_UPDATE_VIEWER_ENABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_UPDATE_VIEWER_DISABLED", (long) NATRON_PIXMAP_UPDATE_VIEWER_DISABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_ADD_TRACK", (long) NATRON_PIXMAP_ADD_TRACK))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_ENTER_GROUP", (long) NATRON_PIXMAP_ENTER_GROUP))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_SETTINGS", (long) NATRON_PIXMAP_SETTINGS))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_FREEZE_ENABLED", (long) NATRON_PIXMAP_FREEZE_ENABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_FREEZE_DISABLED", (long) NATRON_PIXMAP_FREEZE_DISABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_VIEWER_ICON", (long) NATRON_PIXMAP_VIEWER_ICON))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_VIEWER_CHECKERBOARD_ENABLED", (long) NATRON_PIXMAP_VIEWER_CHECKERBOARD_ENABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_VIEWER_CHECKERBOARD_DISABLED", (long) NATRON_PIXMAP_VIEWER_CHECKERBOARD_DISABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_VIEWER_ZEBRA_ENABLED", (long) NATRON_PIXMAP_VIEWER_ZEBRA_ENABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_VIEWER_ZEBRA_DISABLED", (long) NATRON_PIXMAP_VIEWER_ZEBRA_DISABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_VIEWER_GAMMA_ENABLED", (long) NATRON_PIXMAP_VIEWER_GAMMA_ENABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_VIEWER_GAMMA_DISABLED", (long) NATRON_PIXMAP_VIEWER_GAMMA_DISABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_VIEWER_GAIN_ENABLED", (long) NATRON_PIXMAP_VIEWER_GAIN_ENABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_VIEWER_GAIN_DISABLED", (long) NATRON_PIXMAP_VIEWER_GAIN_DISABLED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_SCRIPT_CLEAR_OUTPUT", (long) NATRON_PIXMAP_SCRIPT_CLEAR_OUTPUT))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_SCRIPT_EXEC_SCRIPT", (long) NATRON_PIXMAP_SCRIPT_EXEC_SCRIPT))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_SCRIPT_LOAD_EXEC_SCRIPT", (long) NATRON_PIXMAP_SCRIPT_LOAD_EXEC_SCRIPT))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_SCRIPT_LOAD_SCRIPT", (long) NATRON_PIXMAP_SCRIPT_LOAD_SCRIPT))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_SCRIPT_NEXT_SCRIPT", (long) NATRON_PIXMAP_SCRIPT_NEXT_SCRIPT))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_SCRIPT_OUTPUT_PANE_ACTIVATED", (long) NATRON_PIXMAP_SCRIPT_OUTPUT_PANE_ACTIVATED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_SCRIPT_OUTPUT_PANE_DEACTIVATED", (long) NATRON_PIXMAP_SCRIPT_OUTPUT_PANE_DEACTIVATED))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_SCRIPT_PREVIOUS_SCRIPT", (long) NATRON_PIXMAP_SCRIPT_PREVIOUS_SCRIPT))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_SCRIPT_SAVE_SCRIPT", (long) NATRON_PIXMAP_SCRIPT_SAVE_SCRIPT))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_ATOP", (long) NATRON_PIXMAP_MERGE_ATOP))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_AVERAGE", (long) NATRON_PIXMAP_MERGE_AVERAGE))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_COLOR", (long) NATRON_PIXMAP_MERGE_COLOR))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_COLOR_BURN", (long) NATRON_PIXMAP_MERGE_COLOR_BURN))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_COLOR_DODGE", (long) NATRON_PIXMAP_MERGE_COLOR_DODGE))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_CONJOINT_OVER", (long) NATRON_PIXMAP_MERGE_CONJOINT_OVER))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_COPY", (long) NATRON_PIXMAP_MERGE_COPY))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_DIFFERENCE", (long) NATRON_PIXMAP_MERGE_DIFFERENCE))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_DISJOINT_OVER", (long) NATRON_PIXMAP_MERGE_DISJOINT_OVER))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_DIVIDE", (long) NATRON_PIXMAP_MERGE_DIVIDE))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_EXCLUSION", (long) NATRON_PIXMAP_MERGE_EXCLUSION))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_FREEZE", (long) NATRON_PIXMAP_MERGE_FREEZE))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_FROM", (long) NATRON_PIXMAP_MERGE_FROM))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_GEOMETRIC", (long) NATRON_PIXMAP_MERGE_GEOMETRIC))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_GRAIN_EXTRACT", (long) NATRON_PIXMAP_MERGE_GRAIN_EXTRACT))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_GRAIN_MERGE", (long) NATRON_PIXMAP_MERGE_GRAIN_MERGE))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_HARD_LIGHT", (long) NATRON_PIXMAP_MERGE_HARD_LIGHT))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_HUE", (long) NATRON_PIXMAP_MERGE_HUE))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_HYPOT", (long) NATRON_PIXMAP_MERGE_HYPOT))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_IN", (long) NATRON_PIXMAP_MERGE_IN))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_LUMINOSITY", (long) NATRON_PIXMAP_MERGE_LUMINOSITY))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_MASK", (long) NATRON_PIXMAP_MERGE_MASK))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_MATTE", (long) NATRON_PIXMAP_MERGE_MATTE))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_MAX", (long) NATRON_PIXMAP_MERGE_MAX))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_MIN", (long) NATRON_PIXMAP_MERGE_MIN))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_MINUS", (long) NATRON_PIXMAP_MERGE_MINUS))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_MULTIPLY", (long) NATRON_PIXMAP_MERGE_MULTIPLY))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_OUT", (long) NATRON_PIXMAP_MERGE_OUT))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_OVER", (long) NATRON_PIXMAP_MERGE_OVER))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_OVERLAY", (long) NATRON_PIXMAP_MERGE_OVERLAY))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_PINLIGHT", (long) NATRON_PIXMAP_MERGE_PINLIGHT))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_PLUS", (long) NATRON_PIXMAP_MERGE_PLUS))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_REFLECT", (long) NATRON_PIXMAP_MERGE_REFLECT))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_SATURATION", (long) NATRON_PIXMAP_MERGE_SATURATION))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_SCREEN", (long) NATRON_PIXMAP_MERGE_SCREEN))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_SOFT_LIGHT", (long) NATRON_PIXMAP_MERGE_SOFT_LIGHT))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_STENCIL", (long) NATRON_PIXMAP_MERGE_STENCIL))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_UNDER", (long) NATRON_PIXMAP_MERGE_UNDER))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_MERGE_XOR", (long) NATRON_PIXMAP_MERGE_XOR))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_ROTO_NODE_ICON", (long) NATRON_PIXMAP_ROTO_NODE_ICON))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_LINK_CURSOR", (long) NATRON_PIXMAP_LINK_CURSOR))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_LINK_MULT_CURSOR", (long) NATRON_PIXMAP_LINK_MULT_CURSOR))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_APP_ICON", (long) NATRON_PIXMAP_APP_ICON))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_INTERP_LINEAR", (long) NATRON_PIXMAP_INTERP_LINEAR))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_INTERP_CURVE", (long) NATRON_PIXMAP_INTERP_CURVE))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_INTERP_CONSTANT", (long) NATRON_PIXMAP_INTERP_CONSTANT))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_INTERP_BREAK", (long) NATRON_PIXMAP_INTERP_BREAK))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_INTERP_CURVE_C", (long) NATRON_PIXMAP_INTERP_CURVE_C))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_INTERP_CURVE_H", (long) NATRON_PIXMAP_INTERP_CURVE_H))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_INTERP_CURVE_R", (long) NATRON_PIXMAP_INTERP_CURVE_R))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
        module, "NATRON_PIXMAP_INTERP_CURVE_Z", (long) NATRON_PIXMAP_INTERP_CURVE_Z))
        return SBK_MODULE_INIT_ERROR;
    // Register converter for enum 'PixmapEnum'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX],
            PixmapEnum_CppToPython_PixmapEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            PixmapEnum_PythonToCpp_PixmapEnum,
            is_PixmapEnum_PythonToCpp_PixmapEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_PIXMAPENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "PixmapEnum");
    }
    // End of 'PixmapEnum' enum.

    // Initialization of enum 'ImagePremultiplicationEnum'.
    SbkNatronEngineTypes[SBK_IMAGEPREMULTIPLICATIONENUM_IDX] = Shiboken::Enum::createGlobalEnum(module,
        "ImagePremultiplicationEnum",
        "NatronEngine.ImagePremultiplicationEnum",
        "ImagePremultiplicationEnum");
    if (!SbkNatronEngineTypes[SBK_IMAGEPREMULTIPLICATIONENUM_IDX])
        return SBK_MODULE_INIT_ERROR;

    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_IMAGEPREMULTIPLICATIONENUM_IDX],
        module, "eImagePremultiplicationOpaque", (long) eImagePremultiplicationOpaque))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_IMAGEPREMULTIPLICATIONENUM_IDX],
        module, "eImagePremultiplicationPremultiplied", (long) eImagePremultiplicationPremultiplied))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_IMAGEPREMULTIPLICATIONENUM_IDX],
        module, "eImagePremultiplicationUnPremultiplied", (long) eImagePremultiplicationUnPremultiplied))
        return SBK_MODULE_INIT_ERROR;
    // Register converter for enum 'ImagePremultiplicationEnum'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_IMAGEPREMULTIPLICATIONENUM_IDX],
            ImagePremultiplicationEnum_CppToPython_ImagePremultiplicationEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            ImagePremultiplicationEnum_PythonToCpp_ImagePremultiplicationEnum,
            is_ImagePremultiplicationEnum_PythonToCpp_ImagePremultiplicationEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_IMAGEPREMULTIPLICATIONENUM_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_IMAGEPREMULTIPLICATIONENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "ImagePremultiplicationEnum");
    }
    // End of 'ImagePremultiplicationEnum' enum.

    // Initialization of enum 'ImageBitDepthEnum'.
    SbkNatronEngineTypes[SBK_IMAGEBITDEPTHENUM_IDX] = Shiboken::Enum::createGlobalEnum(module,
        "ImageBitDepthEnum",
        "NatronEngine.ImageBitDepthEnum",
        "ImageBitDepthEnum");
    if (!SbkNatronEngineTypes[SBK_IMAGEBITDEPTHENUM_IDX])
        return SBK_MODULE_INIT_ERROR;

    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_IMAGEBITDEPTHENUM_IDX],
        module, "eImageBitDepthNone", (long) eImageBitDepthNone))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_IMAGEBITDEPTHENUM_IDX],
        module, "eImageBitDepthByte", (long) eImageBitDepthByte))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_IMAGEBITDEPTHENUM_IDX],
        module, "eImageBitDepthShort", (long) eImageBitDepthShort))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_IMAGEBITDEPTHENUM_IDX],
        module, "eImageBitDepthHalf", (long) eImageBitDepthHalf))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_IMAGEBITDEPTHENUM_IDX],
        module, "eImageBitDepthFloat", (long) eImageBitDepthFloat))
        return SBK_MODULE_INIT_ERROR;
    // Register converter for enum 'ImageBitDepthEnum'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_IMAGEBITDEPTHENUM_IDX],
            ImageBitDepthEnum_CppToPython_ImageBitDepthEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            ImageBitDepthEnum_PythonToCpp_ImageBitDepthEnum,
            is_ImageBitDepthEnum_PythonToCpp_ImageBitDepthEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_IMAGEBITDEPTHENUM_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_IMAGEBITDEPTHENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "ImageBitDepthEnum");
    }
    // End of 'ImageBitDepthEnum' enum.

    // Initialization of enum 'ViewerCompositingOperatorEnum'.
    SbkNatronEngineTypes[SBK_VIEWERCOMPOSITINGOPERATORENUM_IDX] = Shiboken::Enum::createGlobalEnum(module,
        "ViewerCompositingOperatorEnum",
        "NatronEngine.ViewerCompositingOperatorEnum",
        "ViewerCompositingOperatorEnum");
    if (!SbkNatronEngineTypes[SBK_VIEWERCOMPOSITINGOPERATORENUM_IDX])
        return SBK_MODULE_INIT_ERROR;

    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_VIEWERCOMPOSITINGOPERATORENUM_IDX],
        module, "eViewerCompositingOperatorNone", (long) eViewerCompositingOperatorNone))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_VIEWERCOMPOSITINGOPERATORENUM_IDX],
        module, "eViewerCompositingOperatorOver", (long) eViewerCompositingOperatorOver))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_VIEWERCOMPOSITINGOPERATORENUM_IDX],
        module, "eViewerCompositingOperatorMinus", (long) eViewerCompositingOperatorMinus))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_VIEWERCOMPOSITINGOPERATORENUM_IDX],
        module, "eViewerCompositingOperatorUnder", (long) eViewerCompositingOperatorUnder))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_VIEWERCOMPOSITINGOPERATORENUM_IDX],
        module, "eViewerCompositingOperatorWipe", (long) eViewerCompositingOperatorWipe))
        return SBK_MODULE_INIT_ERROR;
    // Register converter for enum 'ViewerCompositingOperatorEnum'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_VIEWERCOMPOSITINGOPERATORENUM_IDX],
            ViewerCompositingOperatorEnum_CppToPython_ViewerCompositingOperatorEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            ViewerCompositingOperatorEnum_PythonToCpp_ViewerCompositingOperatorEnum,
            is_ViewerCompositingOperatorEnum_PythonToCpp_ViewerCompositingOperatorEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_VIEWERCOMPOSITINGOPERATORENUM_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_VIEWERCOMPOSITINGOPERATORENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "ViewerCompositingOperatorEnum");
    }
    // End of 'ViewerCompositingOperatorEnum' enum.

    // Initialization of enum 'PlaybackModeEnum'.
    SbkNatronEngineTypes[SBK_PLAYBACKMODEENUM_IDX] = Shiboken::Enum::createGlobalEnum(module,
        "PlaybackModeEnum",
        "NatronEngine.PlaybackModeEnum",
        "PlaybackModeEnum");
    if (!SbkNatronEngineTypes[SBK_PLAYBACKMODEENUM_IDX])
        return SBK_MODULE_INIT_ERROR;

    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PLAYBACKMODEENUM_IDX],
        module, "ePlaybackModeLoop", (long) ePlaybackModeLoop))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PLAYBACKMODEENUM_IDX],
        module, "ePlaybackModeBounce", (long) ePlaybackModeBounce))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_PLAYBACKMODEENUM_IDX],
        module, "ePlaybackModeOnce", (long) ePlaybackModeOnce))
        return SBK_MODULE_INIT_ERROR;
    // Register converter for enum 'PlaybackModeEnum'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_PLAYBACKMODEENUM_IDX],
            PlaybackModeEnum_CppToPython_PlaybackModeEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            PlaybackModeEnum_PythonToCpp_PlaybackModeEnum,
            is_PlaybackModeEnum_PythonToCpp_PlaybackModeEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_PLAYBACKMODEENUM_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_PLAYBACKMODEENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "PlaybackModeEnum");
    }
    // End of 'PlaybackModeEnum' enum.

    // Initialization of enum 'DisplayChannelsEnum'.
    SbkNatronEngineTypes[SBK_DISPLAYCHANNELSENUM_IDX] = Shiboken::Enum::createGlobalEnum(module,
        "DisplayChannelsEnum",
        "NatronEngine.DisplayChannelsEnum",
        "DisplayChannelsEnum");
    if (!SbkNatronEngineTypes[SBK_DISPLAYCHANNELSENUM_IDX])
        return SBK_MODULE_INIT_ERROR;

    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_DISPLAYCHANNELSENUM_IDX],
        module, "eDisplayChannelsRGB", (long) eDisplayChannelsRGB))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_DISPLAYCHANNELSENUM_IDX],
        module, "eDisplayChannelsR", (long) eDisplayChannelsR))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_DISPLAYCHANNELSENUM_IDX],
        module, "eDisplayChannelsG", (long) eDisplayChannelsG))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_DISPLAYCHANNELSENUM_IDX],
        module, "eDisplayChannelsB", (long) eDisplayChannelsB))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_DISPLAYCHANNELSENUM_IDX],
        module, "eDisplayChannelsA", (long) eDisplayChannelsA))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_DISPLAYCHANNELSENUM_IDX],
        module, "eDisplayChannelsY", (long) eDisplayChannelsY))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_DISPLAYCHANNELSENUM_IDX],
        module, "eDisplayChannelsMatte", (long) eDisplayChannelsMatte))
        return SBK_MODULE_INIT_ERROR;
    // Register converter for enum 'DisplayChannelsEnum'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_DISPLAYCHANNELSENUM_IDX],
            DisplayChannelsEnum_CppToPython_DisplayChannelsEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            DisplayChannelsEnum_PythonToCpp_DisplayChannelsEnum,
            is_DisplayChannelsEnum_PythonToCpp_DisplayChannelsEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_DISPLAYCHANNELSENUM_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_DISPLAYCHANNELSENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "DisplayChannelsEnum");
    }
    // End of 'DisplayChannelsEnum' enum.

    // Initialization of enum 'OrientationEnum'.
    SbkNatronEngineTypes[SBK_ORIENTATIONENUM_IDX] = Shiboken::Enum::createGlobalEnum(module,
        "OrientationEnum",
        "NatronEngine.OrientationEnum",
        "OrientationEnum");
    if (!SbkNatronEngineTypes[SBK_ORIENTATIONENUM_IDX])
        return SBK_MODULE_INIT_ERROR;

    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_ORIENTATIONENUM_IDX],
        module, "eOrientationHorizontal", (long) eOrientationHorizontal))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_ORIENTATIONENUM_IDX],
        module, "eOrientationVertical", (long) eOrientationVertical))
        return SBK_MODULE_INIT_ERROR;
    // Register converter for enum 'OrientationEnum'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_ORIENTATIONENUM_IDX],
            OrientationEnum_CppToPython_OrientationEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            OrientationEnum_PythonToCpp_OrientationEnum,
            is_OrientationEnum_PythonToCpp_OrientationEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_ORIENTATIONENUM_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_ORIENTATIONENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "OrientationEnum");
    }
    // End of 'OrientationEnum' enum.

    // Initialization of enum 'StandardButtonEnum'.
    SbkNatronEngineTypes[SBK_QFLAGS_STANDARDBUTTONENUM__IDX] = PySide::QFlags::create("StandardButtons", &SbkNatronEngine_StandardButtonEnum_as_number);
    SbkNatronEngineTypes[SBK_STANDARDBUTTONENUM_IDX] = Shiboken::Enum::createGlobalEnum(module,
        "StandardButtonEnum",
        "NatronEngine.StandardButtonEnum",
        "StandardButtonEnum",
        SbkNatronEngineTypes[SBK_QFLAGS_STANDARDBUTTONENUM__IDX]);
    if (!SbkNatronEngineTypes[SBK_STANDARDBUTTONENUM_IDX])
        return SBK_MODULE_INIT_ERROR;

    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_STANDARDBUTTONENUM_IDX],
        module, "eStandardButtonNoButton", (long) eStandardButtonNoButton))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_STANDARDBUTTONENUM_IDX],
        module, "eStandardButtonEscape", (long) eStandardButtonEscape))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_STANDARDBUTTONENUM_IDX],
        module, "eStandardButtonOk", (long) eStandardButtonOk))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_STANDARDBUTTONENUM_IDX],
        module, "eStandardButtonSave", (long) eStandardButtonSave))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_STANDARDBUTTONENUM_IDX],
        module, "eStandardButtonSaveAll", (long) eStandardButtonSaveAll))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_STANDARDBUTTONENUM_IDX],
        module, "eStandardButtonOpen", (long) eStandardButtonOpen))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_STANDARDBUTTONENUM_IDX],
        module, "eStandardButtonYes", (long) eStandardButtonYes))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_STANDARDBUTTONENUM_IDX],
        module, "eStandardButtonYesToAll", (long) eStandardButtonYesToAll))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_STANDARDBUTTONENUM_IDX],
        module, "eStandardButtonNo", (long) eStandardButtonNo))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_STANDARDBUTTONENUM_IDX],
        module, "eStandardButtonNoToAll", (long) eStandardButtonNoToAll))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_STANDARDBUTTONENUM_IDX],
        module, "eStandardButtonAbort", (long) eStandardButtonAbort))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_STANDARDBUTTONENUM_IDX],
        module, "eStandardButtonRetry", (long) eStandardButtonRetry))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_STANDARDBUTTONENUM_IDX],
        module, "eStandardButtonIgnore", (long) eStandardButtonIgnore))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_STANDARDBUTTONENUM_IDX],
        module, "eStandardButtonClose", (long) eStandardButtonClose))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_STANDARDBUTTONENUM_IDX],
        module, "eStandardButtonCancel", (long) eStandardButtonCancel))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_STANDARDBUTTONENUM_IDX],
        module, "eStandardButtonDiscard", (long) eStandardButtonDiscard))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_STANDARDBUTTONENUM_IDX],
        module, "eStandardButtonHelp", (long) eStandardButtonHelp))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_STANDARDBUTTONENUM_IDX],
        module, "eStandardButtonApply", (long) eStandardButtonApply))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_STANDARDBUTTONENUM_IDX],
        module, "eStandardButtonReset", (long) eStandardButtonReset))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_STANDARDBUTTONENUM_IDX],
        module, "eStandardButtonRestoreDefaults", (long) eStandardButtonRestoreDefaults))
        return SBK_MODULE_INIT_ERROR;
    // Register converter for enum 'StandardButtonEnum'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_STANDARDBUTTONENUM_IDX],
            StandardButtonEnum_CppToPython_StandardButtonEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            StandardButtonEnum_PythonToCpp_StandardButtonEnum,
            is_StandardButtonEnum_PythonToCpp_StandardButtonEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_STANDARDBUTTONENUM_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_STANDARDBUTTONENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "StandardButtonEnum");
    }
    // Register converter for flag 'QFlags<StandardButtonEnum>'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_QFLAGS_STANDARDBUTTONENUM__IDX],
            QFlags_StandardButtonEnum__CppToPython_QFlags_StandardButtonEnum_);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            StandardButtonEnum_PythonToCpp_QFlags_StandardButtonEnum_,
            is_StandardButtonEnum_PythonToCpp_QFlags_StandardButtonEnum__Convertible);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            QFlags_StandardButtonEnum__PythonToCpp_QFlags_StandardButtonEnum_,
            is_QFlags_StandardButtonEnum__PythonToCpp_QFlags_StandardButtonEnum__Convertible);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            number_PythonToCpp_QFlags_StandardButtonEnum_,
            is_number_PythonToCpp_QFlags_StandardButtonEnum__Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_QFLAGS_STANDARDBUTTONENUM__IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_QFLAGS_STANDARDBUTTONENUM__IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "QFlags<QFlags<StandardButtonEnum>");
    }
    // End of 'StandardButtonEnum' enum/flags.

    // Initialization of enum 'ImageComponentsEnum'.
    SbkNatronEngineTypes[SBK_IMAGECOMPONENTSENUM_IDX] = Shiboken::Enum::createGlobalEnum(module,
        "ImageComponentsEnum",
        "NatronEngine.ImageComponentsEnum",
        "ImageComponentsEnum");
    if (!SbkNatronEngineTypes[SBK_IMAGECOMPONENTSENUM_IDX])
        return SBK_MODULE_INIT_ERROR;

    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_IMAGECOMPONENTSENUM_IDX],
        module, "eImageComponentNone", (long) eImageComponentNone))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_IMAGECOMPONENTSENUM_IDX],
        module, "eImageComponentAlpha", (long) eImageComponentAlpha))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_IMAGECOMPONENTSENUM_IDX],
        module, "eImageComponentRGB", (long) eImageComponentRGB))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_IMAGECOMPONENTSENUM_IDX],
        module, "eImageComponentRGBA", (long) eImageComponentRGBA))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_IMAGECOMPONENTSENUM_IDX],
        module, "eImageComponentXY", (long) eImageComponentXY))
        return SBK_MODULE_INIT_ERROR;
    // Register converter for enum 'ImageComponentsEnum'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_IMAGECOMPONENTSENUM_IDX],
            ImageComponentsEnum_CppToPython_ImageComponentsEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            ImageComponentsEnum_PythonToCpp_ImageComponentsEnum,
            is_ImageComponentsEnum_PythonToCpp_ImageComponentsEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_IMAGECOMPONENTSENUM_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_IMAGECOMPONENTSENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "ImageComponentsEnum");
    }
    // End of 'ImageComponentsEnum' enum.

    // Initialization of enum 'ValueChangedReasonEnum'.
    SbkNatronEngineTypes[SBK_VALUECHANGEDREASONENUM_IDX] = Shiboken::Enum::createGlobalEnum(module,
        "ValueChangedReasonEnum",
        "NatronEngine.ValueChangedReasonEnum",
        "ValueChangedReasonEnum");
    if (!SbkNatronEngineTypes[SBK_VALUECHANGEDREASONENUM_IDX])
        return SBK_MODULE_INIT_ERROR;

    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_VALUECHANGEDREASONENUM_IDX],
        module, "eValueChangedReasonUserEdited", (long) eValueChangedReasonUserEdited))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_VALUECHANGEDREASONENUM_IDX],
        module, "eValueChangedReasonPluginEdited", (long) eValueChangedReasonPluginEdited))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_VALUECHANGEDREASONENUM_IDX],
        module, "eValueChangedReasonNatronGuiEdited", (long) eValueChangedReasonNatronGuiEdited))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_VALUECHANGEDREASONENUM_IDX],
        module, "eValueChangedReasonNatronInternalEdited", (long) eValueChangedReasonNatronInternalEdited))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_VALUECHANGEDREASONENUM_IDX],
        module, "eValueChangedReasonTimeChanged", (long) eValueChangedReasonTimeChanged))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_VALUECHANGEDREASONENUM_IDX],
        module, "eValueChangedReasonSlaveRefresh", (long) eValueChangedReasonSlaveRefresh))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_VALUECHANGEDREASONENUM_IDX],
        module, "eValueChangedReasonRestoreDefault", (long) eValueChangedReasonRestoreDefault))
        return SBK_MODULE_INIT_ERROR;
    // Register converter for enum 'ValueChangedReasonEnum'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_VALUECHANGEDREASONENUM_IDX],
            ValueChangedReasonEnum_CppToPython_ValueChangedReasonEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            ValueChangedReasonEnum_PythonToCpp_ValueChangedReasonEnum,
            is_ValueChangedReasonEnum_PythonToCpp_ValueChangedReasonEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_VALUECHANGEDREASONENUM_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_VALUECHANGEDREASONENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "ValueChangedReasonEnum");
    }
    // End of 'ValueChangedReasonEnum' enum.

    // Initialization of enum 'StatusEnum'.
    SbkNatronEngineTypes[SBK_STATUSENUM_IDX] = Shiboken::Enum::createGlobalEnum(module,
        "StatusEnum",
        "NatronEngine.StatusEnum",
        "StatusEnum");
    if (!SbkNatronEngineTypes[SBK_STATUSENUM_IDX])
        return SBK_MODULE_INIT_ERROR;

    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_STATUSENUM_IDX],
        module, "eStatusOK", (long) eStatusOK))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_STATUSENUM_IDX],
        module, "eStatusFailed", (long) eStatusFailed))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_STATUSENUM_IDX],
        module, "eStatusReplyDefault", (long) eStatusReplyDefault))
        return SBK_MODULE_INIT_ERROR;
    // Register converter for enum 'StatusEnum'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_STATUSENUM_IDX],
            StatusEnum_CppToPython_StatusEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            StatusEnum_PythonToCpp_StatusEnum,
            is_StatusEnum_PythonToCpp_StatusEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_STATUSENUM_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_STATUSENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "StatusEnum");
    }
    // End of 'StatusEnum' enum.

    // Initialization of enum 'ViewerColorSpaceEnum'.
    SbkNatronEngineTypes[SBK_VIEWERCOLORSPACEENUM_IDX] = Shiboken::Enum::createGlobalEnum(module,
        "ViewerColorSpaceEnum",
        "NatronEngine.ViewerColorSpaceEnum",
        "ViewerColorSpaceEnum");
    if (!SbkNatronEngineTypes[SBK_VIEWERCOLORSPACEENUM_IDX])
        return SBK_MODULE_INIT_ERROR;

    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_VIEWERCOLORSPACEENUM_IDX],
        module, "eViewerColorSpaceSRGB", (long) eViewerColorSpaceSRGB))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_VIEWERCOLORSPACEENUM_IDX],
        module, "eViewerColorSpaceLinear", (long) eViewerColorSpaceLinear))
        return SBK_MODULE_INIT_ERROR;
    if (!Shiboken::Enum::createGlobalEnumItem(SbkNatronEngineTypes[SBK_VIEWERCOLORSPACEENUM_IDX],
        module, "eViewerColorSpaceRec709", (long) eViewerColorSpaceRec709))
        return SBK_MODULE_INIT_ERROR;
    // Register converter for enum 'ViewerColorSpaceEnum'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_VIEWERCOLORSPACEENUM_IDX],
            ViewerColorSpaceEnum_CppToPython_ViewerColorSpaceEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            ViewerColorSpaceEnum_PythonToCpp_ViewerColorSpaceEnum,
            is_ViewerColorSpaceEnum_PythonToCpp_ViewerColorSpaceEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_VIEWERCOLORSPACEENUM_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_VIEWERCOLORSPACEENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "ViewerColorSpaceEnum");
    }
    // End of 'ViewerColorSpaceEnum' enum.

    // Register primitive types converters.

    Shiboken::Module::registerTypes(module, SbkNatronEngineTypes);
    Shiboken::Module::registerTypeConverters(module, SbkNatronEngineTypeConverters);

    if (PyErr_Occurred()) {
        PyErr_Print();
        Py_FatalError("can't initialize module NatronEngine");
    }
    qRegisterMetaType< ::AnimationLevelEnum >("AnimationLevelEnum");
    qRegisterMetaType< ::KeyframeTypeEnum >("KeyframeTypeEnum");
    qRegisterMetaType< ::PixmapEnum >("PixmapEnum");
    qRegisterMetaType< ::ImagePremultiplicationEnum >("ImagePremultiplicationEnum");
    qRegisterMetaType< ::ImageBitDepthEnum >("ImageBitDepthEnum");
    qRegisterMetaType< ::ViewerCompositingOperatorEnum >("ViewerCompositingOperatorEnum");
    qRegisterMetaType< ::PlaybackModeEnum >("PlaybackModeEnum");
    qRegisterMetaType< ::DisplayChannelsEnum >("DisplayChannelsEnum");
    qRegisterMetaType< ::OrientationEnum >("OrientationEnum");
    qRegisterMetaType< ::StandardButtonEnum >("StandardButtonEnum");
    qRegisterMetaType< ::ImageComponentsEnum >("ImageComponentsEnum");
    qRegisterMetaType< ::ValueChangedReasonEnum >("ValueChangedReasonEnum");
    qRegisterMetaType< ::StatusEnum >("StatusEnum");
    qRegisterMetaType< ::ViewerColorSpaceEnum >("ViewerColorSpaceEnum");
    PySide::registerCleanupFunction(cleanTypesAttributes);
SBK_MODULE_INIT_FUNCTION_END
