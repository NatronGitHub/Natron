
// default includes
#include "Global/Macros.h"
CLANG_DIAG_OFF(mismatched-tags)
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_OFF(missing-field-initializers)
#include <shiboken.h> // produces many warnings
#include <pysidesignal.h>
#include <pysideproperty.h>
#include <pyside.h>
#include <typeresolver.h>
#include <typeinfo>
#include "natronengine_python.h"

#include "natron_wrapper.h"

// Extra includes



// Target ---------------------------------------------------------

extern "C" {
static PyMethodDef Sbk_Natron_methods[] = {

    {0} // Sentinel
};

} // extern "C"

static int Sbk_Natron_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_Natron_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_Natron_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.Natron",
    /*tp_basicsize*/        sizeof(SbkObject),
    /*tp_itemsize*/         0,
    /*tp_dealloc*/          0,
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
    /*tp_traverse*/         Sbk_Natron_traverse,
    /*tp_clear*/            Sbk_Natron_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_Natron_methods,
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

PyObject* SbkNatronEngine_Natron_StandardButtonEnum___and__(PyObject* self, PyObject* pyArg)
{
    ::Natron::StandardButtons cppResult, cppSelf, cppArg;
#ifdef IS_PY3K
    cppSelf = (::Natron::StandardButtons)PyLong_AsLong(self);
    cppArg = (Natron::StandardButtons)PyLong_AsLong(pyArg);
#else
    cppSelf = (::Natron::StandardButtons)PyInt_AsLong(self);
    cppArg = (Natron::StandardButtons)PyInt_AsLong(pyArg);
#endif

    cppResult = cppSelf & cppArg;
    return Shiboken::Conversions::copyToPython(SBK_CONVERTER(SbkNatronEngineTypes[SBK_QFLAGS_NATRON_STANDARDBUTTONENUM__IDX]), &cppResult);
}

PyObject* SbkNatronEngine_Natron_StandardButtonEnum___or__(PyObject* self, PyObject* pyArg)
{
    ::Natron::StandardButtons cppResult, cppSelf, cppArg;
#ifdef IS_PY3K
    cppSelf = (::Natron::StandardButtons)PyLong_AsLong(self);
    cppArg = (Natron::StandardButtons)PyLong_AsLong(pyArg);
#else
    cppSelf = (::Natron::StandardButtons)PyInt_AsLong(self);
    cppArg = (Natron::StandardButtons)PyInt_AsLong(pyArg);
#endif

    cppResult = cppSelf | cppArg;
    return Shiboken::Conversions::copyToPython(SBK_CONVERTER(SbkNatronEngineTypes[SBK_QFLAGS_NATRON_STANDARDBUTTONENUM__IDX]), &cppResult);
}

PyObject* SbkNatronEngine_Natron_StandardButtonEnum___xor__(PyObject* self, PyObject* pyArg)
{
    ::Natron::StandardButtons cppResult, cppSelf, cppArg;
#ifdef IS_PY3K
    cppSelf = (::Natron::StandardButtons)PyLong_AsLong(self);
    cppArg = (Natron::StandardButtons)PyLong_AsLong(pyArg);
#else
    cppSelf = (::Natron::StandardButtons)PyInt_AsLong(self);
    cppArg = (Natron::StandardButtons)PyInt_AsLong(pyArg);
#endif

    cppResult = cppSelf ^ cppArg;
    return Shiboken::Conversions::copyToPython(SBK_CONVERTER(SbkNatronEngineTypes[SBK_QFLAGS_NATRON_STANDARDBUTTONENUM__IDX]), &cppResult);
}

PyObject* SbkNatronEngine_Natron_StandardButtonEnum___invert__(PyObject* self, PyObject* pyArg)
{
    ::Natron::StandardButtons cppSelf;
    Shiboken::Conversions::pythonToCppCopy(SBK_CONVERTER(SbkNatronEngineTypes[SBK_QFLAGS_NATRON_STANDARDBUTTONENUM__IDX]), self, &cppSelf);
    ::Natron::StandardButtons cppResult = ~cppSelf;
    return Shiboken::Conversions::copyToPython(SBK_CONVERTER(SbkNatronEngineTypes[SBK_QFLAGS_NATRON_STANDARDBUTTONENUM__IDX]), &cppResult);
}

static PyObject* SbkNatronEngine_Natron_StandardButtonEnum_long(PyObject* self)
{
    int val;
    Shiboken::Conversions::pythonToCppCopy(SBK_CONVERTER(SbkNatronEngineTypes[SBK_QFLAGS_NATRON_STANDARDBUTTONENUM__IDX]), self, &val);
    return Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &val);
}
static int SbkNatronEngine_Natron_StandardButtonEnum__nonzero(PyObject* self)
{
    int val;
    Shiboken::Conversions::pythonToCppCopy(SBK_CONVERTER(SbkNatronEngineTypes[SBK_QFLAGS_NATRON_STANDARDBUTTONENUM__IDX]), self, &val);
    return val != 0;
}

static PyNumberMethods SbkNatronEngine_Natron_StandardButtonEnum_as_number = {
    /*nb_add*/                  0,
    /*nb_subtract*/             0,
    /*nb_multiply*/             0,
    #ifndef IS_PY3K
    /* nb_divide */             0,
    #endif
    /*nb_remainder*/            0,
    /*nb_divmod*/               0,
    /*nb_power*/                0,
    /*nb_negative*/             0,
    /*nb_positive*/             0,
    /*nb_absolute*/             0,
    /*nb_nonzero*/              SbkNatronEngine_Natron_StandardButtonEnum__nonzero,
    /*nb_invert*/               (unaryfunc)SbkNatronEngine_Natron_StandardButtonEnum___invert__,
    /*nb_lshift*/               0,
    /*nb_rshift*/               0,
    /*nb_and*/                  (binaryfunc)SbkNatronEngine_Natron_StandardButtonEnum___and__,
    /*nb_xor*/                  (binaryfunc)SbkNatronEngine_Natron_StandardButtonEnum___xor__,
    /*nb_or*/                   (binaryfunc)SbkNatronEngine_Natron_StandardButtonEnum___or__,
    #ifndef IS_PY3K
    /* nb_coerce */             0,
    #endif
    /*nb_int*/                  SbkNatronEngine_Natron_StandardButtonEnum_long,
    #ifdef IS_PY3K
    /*nb_reserved*/             0,
    /*nb_float*/                0,
    #else
    /*nb_long*/                 SbkNatronEngine_Natron_StandardButtonEnum_long,
    /*nb_float*/                0,
    /*nb_oct*/                  0,
    /*nb_hex*/                  0,
    #endif
    /*nb_inplace_add*/          0,
    /*nb_inplace_subtract*/     0,
    /*nb_inplace_multiply*/     0,
    #ifndef IS_PY3K
    /*nb_inplace_divide*/       0,
    #endif
    /*nb_inplace_remainder*/    0,
    /*nb_inplace_power*/        0,
    /*nb_inplace_lshift*/       0,
    /*nb_inplace_rshift*/       0,
    /*nb_inplace_and*/          0,
    /*nb_inplace_xor*/          0,
    /*nb_inplace_or*/           0,
    /*nb_floor_divide*/         0,
    /*nb_true_divide*/          0,
    /*nb_inplace_floor_divide*/ 0,
    /*nb_inplace_true_divide*/  0,
    /*nb_index*/                0
};



// Type conversion functions.

// Python to C++ enum conversion.
static void Natron_StandardButtonEnum_PythonToCpp_Natron_StandardButtonEnum(PyObject* pyIn, void* cppOut) {
    *((::Natron::StandardButtonEnum*)cppOut) = (::Natron::StandardButtonEnum) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_Natron_StandardButtonEnum_PythonToCpp_Natron_StandardButtonEnum_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTONENUM_IDX]))
        return Natron_StandardButtonEnum_PythonToCpp_Natron_StandardButtonEnum;
    return 0;
}
static PyObject* Natron_StandardButtonEnum_CppToPython_Natron_StandardButtonEnum(const void* cppIn) {
    int castCppIn = *((::Natron::StandardButtonEnum*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTONENUM_IDX], castCppIn);

}

static void QFlags_Natron_StandardButtonEnum__PythonToCpp_QFlags_Natron_StandardButtonEnum_(PyObject* pyIn, void* cppOut) {
    *((::QFlags<Natron::StandardButtonEnum>*)cppOut) = ::QFlags<Natron::StandardButtonEnum>(QFlag(PySide::QFlags::getValue(reinterpret_cast<PySideQFlagsObject*>(pyIn))));

}
static PythonToCppFunc is_QFlags_Natron_StandardButtonEnum__PythonToCpp_QFlags_Natron_StandardButtonEnum__Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_QFLAGS_NATRON_STANDARDBUTTONENUM__IDX]))
        return QFlags_Natron_StandardButtonEnum__PythonToCpp_QFlags_Natron_StandardButtonEnum_;
    return 0;
}
static PyObject* QFlags_Natron_StandardButtonEnum__CppToPython_QFlags_Natron_StandardButtonEnum_(const void* cppIn) {
    int castCppIn = *((::QFlags<Natron::StandardButtonEnum>*)cppIn);
    return reinterpret_cast<PyObject*>(PySide::QFlags::newObject(castCppIn, SbkNatronEngineTypes[SBK_QFLAGS_NATRON_STANDARDBUTTONENUM__IDX]));

}

static void Natron_StandardButtonEnum_PythonToCpp_QFlags_Natron_StandardButtonEnum_(PyObject* pyIn, void* cppOut) {
    *((::QFlags<Natron::StandardButtonEnum>*)cppOut) = ::QFlags<Natron::StandardButtonEnum>(QFlag(Shiboken::Enum::getValue(pyIn)));

}
static PythonToCppFunc is_Natron_StandardButtonEnum_PythonToCpp_QFlags_Natron_StandardButtonEnum__Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTONENUM_IDX]))
        return Natron_StandardButtonEnum_PythonToCpp_QFlags_Natron_StandardButtonEnum_;
    return 0;
}
static void number_PythonToCpp_QFlags_Natron_StandardButtonEnum_(PyObject* pyIn, void* cppOut) {
    Shiboken::AutoDecRef pyLong(PyNumber_Long(pyIn));
    *((::QFlags<Natron::StandardButtonEnum>*)cppOut) = ::QFlags<Natron::StandardButtonEnum>(QFlag(PyLong_AsLong(pyLong.object())));

}
static PythonToCppFunc is_number_PythonToCpp_QFlags_Natron_StandardButtonEnum__Convertible(PyObject* pyIn) {
    if (PyNumber_Check(pyIn))
        return number_PythonToCpp_QFlags_Natron_StandardButtonEnum_;
    return 0;
}
static void Natron_ImageComponentsEnum_PythonToCpp_Natron_ImageComponentsEnum(PyObject* pyIn, void* cppOut) {
    *((::Natron::ImageComponentsEnum*)cppOut) = (::Natron::ImageComponentsEnum) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_Natron_ImageComponentsEnum_PythonToCpp_Natron_ImageComponentsEnum_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_IMAGECOMPONENTSENUM_IDX]))
        return Natron_ImageComponentsEnum_PythonToCpp_Natron_ImageComponentsEnum;
    return 0;
}
static PyObject* Natron_ImageComponentsEnum_CppToPython_Natron_ImageComponentsEnum(const void* cppIn) {
    int castCppIn = *((::Natron::ImageComponentsEnum*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_IMAGECOMPONENTSENUM_IDX], castCppIn);

}

static void Natron_ImageBitDepthEnum_PythonToCpp_Natron_ImageBitDepthEnum(PyObject* pyIn, void* cppOut) {
    *((::Natron::ImageBitDepthEnum*)cppOut) = (::Natron::ImageBitDepthEnum) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_Natron_ImageBitDepthEnum_PythonToCpp_Natron_ImageBitDepthEnum_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_IMAGEBITDEPTHENUM_IDX]))
        return Natron_ImageBitDepthEnum_PythonToCpp_Natron_ImageBitDepthEnum;
    return 0;
}
static PyObject* Natron_ImageBitDepthEnum_CppToPython_Natron_ImageBitDepthEnum(const void* cppIn) {
    int castCppIn = *((::Natron::ImageBitDepthEnum*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_IMAGEBITDEPTHENUM_IDX], castCppIn);

}

static void Natron_KeyframeTypeEnum_PythonToCpp_Natron_KeyframeTypeEnum(PyObject* pyIn, void* cppOut) {
    *((::Natron::KeyframeTypeEnum*)cppOut) = (::Natron::KeyframeTypeEnum) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_Natron_KeyframeTypeEnum_PythonToCpp_Natron_KeyframeTypeEnum_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_KEYFRAMETYPEENUM_IDX]))
        return Natron_KeyframeTypeEnum_PythonToCpp_Natron_KeyframeTypeEnum;
    return 0;
}
static PyObject* Natron_KeyframeTypeEnum_CppToPython_Natron_KeyframeTypeEnum(const void* cppIn) {
    int castCppIn = *((::Natron::KeyframeTypeEnum*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_KEYFRAMETYPEENUM_IDX], castCppIn);

}

static void Natron_ValueChangedReasonEnum_PythonToCpp_Natron_ValueChangedReasonEnum(PyObject* pyIn, void* cppOut) {
    *((::Natron::ValueChangedReasonEnum*)cppOut) = (::Natron::ValueChangedReasonEnum) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_Natron_ValueChangedReasonEnum_PythonToCpp_Natron_ValueChangedReasonEnum_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_VALUECHANGEDREASONENUM_IDX]))
        return Natron_ValueChangedReasonEnum_PythonToCpp_Natron_ValueChangedReasonEnum;
    return 0;
}
static PyObject* Natron_ValueChangedReasonEnum_CppToPython_Natron_ValueChangedReasonEnum(const void* cppIn) {
    int castCppIn = *((::Natron::ValueChangedReasonEnum*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_VALUECHANGEDREASONENUM_IDX], castCppIn);

}

static void Natron_AnimationLevelEnum_PythonToCpp_Natron_AnimationLevelEnum(PyObject* pyIn, void* cppOut) {
    *((::Natron::AnimationLevelEnum*)cppOut) = (::Natron::AnimationLevelEnum) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_Natron_AnimationLevelEnum_PythonToCpp_Natron_AnimationLevelEnum_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_ANIMATIONLEVELENUM_IDX]))
        return Natron_AnimationLevelEnum_PythonToCpp_Natron_AnimationLevelEnum;
    return 0;
}
static PyObject* Natron_AnimationLevelEnum_CppToPython_Natron_AnimationLevelEnum(const void* cppIn) {
    int castCppIn = *((::Natron::AnimationLevelEnum*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_ANIMATIONLEVELENUM_IDX], castCppIn);

}

static void Natron_OrientationEnum_PythonToCpp_Natron_OrientationEnum(PyObject* pyIn, void* cppOut) {
    *((::Natron::OrientationEnum*)cppOut) = (::Natron::OrientationEnum) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_Natron_OrientationEnum_PythonToCpp_Natron_OrientationEnum_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_ORIENTATIONENUM_IDX]))
        return Natron_OrientationEnum_PythonToCpp_Natron_OrientationEnum;
    return 0;
}
static PyObject* Natron_OrientationEnum_CppToPython_Natron_OrientationEnum(const void* cppIn) {
    int castCppIn = *((::Natron::OrientationEnum*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_ORIENTATIONENUM_IDX], castCppIn);

}

static void Natron_DisplayChannelsEnum_PythonToCpp_Natron_DisplayChannelsEnum(PyObject* pyIn, void* cppOut) {
    *((::Natron::DisplayChannelsEnum*)cppOut) = (::Natron::DisplayChannelsEnum) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_Natron_DisplayChannelsEnum_PythonToCpp_Natron_DisplayChannelsEnum_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_DISPLAYCHANNELSENUM_IDX]))
        return Natron_DisplayChannelsEnum_PythonToCpp_Natron_DisplayChannelsEnum;
    return 0;
}
static PyObject* Natron_DisplayChannelsEnum_CppToPython_Natron_DisplayChannelsEnum(const void* cppIn) {
    int castCppIn = *((::Natron::DisplayChannelsEnum*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_DISPLAYCHANNELSENUM_IDX], castCppIn);

}

static void Natron_ImagePremultiplicationEnum_PythonToCpp_Natron_ImagePremultiplicationEnum(PyObject* pyIn, void* cppOut) {
    *((::Natron::ImagePremultiplicationEnum*)cppOut) = (::Natron::ImagePremultiplicationEnum) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_Natron_ImagePremultiplicationEnum_PythonToCpp_Natron_ImagePremultiplicationEnum_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_IMAGEPREMULTIPLICATIONENUM_IDX]))
        return Natron_ImagePremultiplicationEnum_PythonToCpp_Natron_ImagePremultiplicationEnum;
    return 0;
}
static PyObject* Natron_ImagePremultiplicationEnum_CppToPython_Natron_ImagePremultiplicationEnum(const void* cppIn) {
    int castCppIn = *((::Natron::ImagePremultiplicationEnum*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_IMAGEPREMULTIPLICATIONENUM_IDX], castCppIn);

}

static void Natron_StatusEnum_PythonToCpp_Natron_StatusEnum(PyObject* pyIn, void* cppOut) {
    *((::Natron::StatusEnum*)cppOut) = (::Natron::StatusEnum) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_Natron_StatusEnum_PythonToCpp_Natron_StatusEnum_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_STATUSENUM_IDX]))
        return Natron_StatusEnum_PythonToCpp_Natron_StatusEnum;
    return 0;
}
static PyObject* Natron_StatusEnum_CppToPython_Natron_StatusEnum(const void* cppIn) {
    int castCppIn = *((::Natron::StatusEnum*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_STATUSENUM_IDX], castCppIn);

}

static void Natron_ViewerCompositingOperatorEnum_PythonToCpp_Natron_ViewerCompositingOperatorEnum(PyObject* pyIn, void* cppOut) {
    *((::Natron::ViewerCompositingOperatorEnum*)cppOut) = (::Natron::ViewerCompositingOperatorEnum) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_Natron_ViewerCompositingOperatorEnum_PythonToCpp_Natron_ViewerCompositingOperatorEnum_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_VIEWERCOMPOSITINGOPERATORENUM_IDX]))
        return Natron_ViewerCompositingOperatorEnum_PythonToCpp_Natron_ViewerCompositingOperatorEnum;
    return 0;
}
static PyObject* Natron_ViewerCompositingOperatorEnum_CppToPython_Natron_ViewerCompositingOperatorEnum(const void* cppIn) {
    int castCppIn = *((::Natron::ViewerCompositingOperatorEnum*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_VIEWERCOMPOSITINGOPERATORENUM_IDX], castCppIn);

}

static void Natron_PlaybackModeEnum_PythonToCpp_Natron_PlaybackModeEnum(PyObject* pyIn, void* cppOut) {
    *((::Natron::PlaybackModeEnum*)cppOut) = (::Natron::PlaybackModeEnum) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_Natron_PlaybackModeEnum_PythonToCpp_Natron_PlaybackModeEnum_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_PLAYBACKMODEENUM_IDX]))
        return Natron_PlaybackModeEnum_PythonToCpp_Natron_PlaybackModeEnum;
    return 0;
}
static PyObject* Natron_PlaybackModeEnum_CppToPython_Natron_PlaybackModeEnum(const void* cppIn) {
    int castCppIn = *((::Natron::PlaybackModeEnum*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_PLAYBACKMODEENUM_IDX], castCppIn);

}

static void Natron_PixmapEnum_PythonToCpp_Natron_PixmapEnum(PyObject* pyIn, void* cppOut) {
    *((::Natron::PixmapEnum*)cppOut) = (::Natron::PixmapEnum) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_Natron_PixmapEnum_PythonToCpp_Natron_PixmapEnum_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX]))
        return Natron_PixmapEnum_PythonToCpp_Natron_PixmapEnum;
    return 0;
}
static PyObject* Natron_PixmapEnum_CppToPython_Natron_PixmapEnum(const void* cppIn) {
    int castCppIn = *((::Natron::PixmapEnum*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX], castCppIn);

}

static void Natron_ViewerColorSpaceEnum_PythonToCpp_Natron_ViewerColorSpaceEnum(PyObject* pyIn, void* cppOut) {
    *((::Natron::ViewerColorSpaceEnum*)cppOut) = (::Natron::ViewerColorSpaceEnum) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_Natron_ViewerColorSpaceEnum_PythonToCpp_Natron_ViewerColorSpaceEnum_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_VIEWERCOLORSPACEENUM_IDX]))
        return Natron_ViewerColorSpaceEnum_PythonToCpp_Natron_ViewerColorSpaceEnum;
    return 0;
}
static PyObject* Natron_ViewerColorSpaceEnum_CppToPython_Natron_ViewerColorSpaceEnum(const void* cppIn) {
    int castCppIn = *((::Natron::ViewerColorSpaceEnum*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_VIEWERCOLORSPACEENUM_IDX], castCppIn);

}

void init_Natron(PyObject* module)
{
    SbkNatronEngineTypes[SBK_NATRON_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_Natron_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "Natron", "Natron",
        &Sbk_Natron_Type)) {
        return;
    }


    // Initialization of enums.

    // Initialization of enum 'StandardButtonEnum'.
    SbkNatronEngineTypes[SBK_QFLAGS_NATRON_STANDARDBUTTONENUM__IDX] = PySide::QFlags::create("StandardButtons", &SbkNatronEngine_Natron_StandardButtonEnum_as_number);
    SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTONENUM_IDX] = Shiboken::Enum::createScopedEnum(&Sbk_Natron_Type,
        "StandardButtonEnum",
        "NatronEngine.Natron.StandardButtonEnum",
        "Natron::StandardButtonEnum",
        SbkNatronEngineTypes[SBK_QFLAGS_NATRON_STANDARDBUTTONENUM__IDX]);
    if (!SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTONENUM_IDX])
        return ;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTONENUM_IDX],
        &Sbk_Natron_Type, "eStandardButtonNoButton", (long) Natron::eStandardButtonNoButton))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTONENUM_IDX],
        &Sbk_Natron_Type, "eStandardButtonEscape", (long) Natron::eStandardButtonEscape))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTONENUM_IDX],
        &Sbk_Natron_Type, "eStandardButtonOk", (long) Natron::eStandardButtonOk))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTONENUM_IDX],
        &Sbk_Natron_Type, "eStandardButtonSave", (long) Natron::eStandardButtonSave))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTONENUM_IDX],
        &Sbk_Natron_Type, "eStandardButtonSaveAll", (long) Natron::eStandardButtonSaveAll))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTONENUM_IDX],
        &Sbk_Natron_Type, "eStandardButtonOpen", (long) Natron::eStandardButtonOpen))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTONENUM_IDX],
        &Sbk_Natron_Type, "eStandardButtonYes", (long) Natron::eStandardButtonYes))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTONENUM_IDX],
        &Sbk_Natron_Type, "eStandardButtonYesToAll", (long) Natron::eStandardButtonYesToAll))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTONENUM_IDX],
        &Sbk_Natron_Type, "eStandardButtonNo", (long) Natron::eStandardButtonNo))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTONENUM_IDX],
        &Sbk_Natron_Type, "eStandardButtonNoToAll", (long) Natron::eStandardButtonNoToAll))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTONENUM_IDX],
        &Sbk_Natron_Type, "eStandardButtonAbort", (long) Natron::eStandardButtonAbort))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTONENUM_IDX],
        &Sbk_Natron_Type, "eStandardButtonRetry", (long) Natron::eStandardButtonRetry))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTONENUM_IDX],
        &Sbk_Natron_Type, "eStandardButtonIgnore", (long) Natron::eStandardButtonIgnore))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTONENUM_IDX],
        &Sbk_Natron_Type, "eStandardButtonClose", (long) Natron::eStandardButtonClose))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTONENUM_IDX],
        &Sbk_Natron_Type, "eStandardButtonCancel", (long) Natron::eStandardButtonCancel))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTONENUM_IDX],
        &Sbk_Natron_Type, "eStandardButtonDiscard", (long) Natron::eStandardButtonDiscard))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTONENUM_IDX],
        &Sbk_Natron_Type, "eStandardButtonHelp", (long) Natron::eStandardButtonHelp))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTONENUM_IDX],
        &Sbk_Natron_Type, "eStandardButtonApply", (long) Natron::eStandardButtonApply))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTONENUM_IDX],
        &Sbk_Natron_Type, "eStandardButtonReset", (long) Natron::eStandardButtonReset))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTONENUM_IDX],
        &Sbk_Natron_Type, "eStandardButtonRestoreDefaults", (long) Natron::eStandardButtonRestoreDefaults))
        return ;
    // Register converter for enum 'Natron::StandardButtonEnum'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTONENUM_IDX],
            Natron_StandardButtonEnum_CppToPython_Natron_StandardButtonEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            Natron_StandardButtonEnum_PythonToCpp_Natron_StandardButtonEnum,
            is_Natron_StandardButtonEnum_PythonToCpp_Natron_StandardButtonEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTONENUM_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTONENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "Natron::StandardButtonEnum");
        Shiboken::Conversions::registerConverterName(converter, "StandardButtonEnum");
    }
    // Register converter for flag 'QFlags<Natron::StandardButtonEnum>'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_QFLAGS_NATRON_STANDARDBUTTONENUM__IDX],
            QFlags_Natron_StandardButtonEnum__CppToPython_QFlags_Natron_StandardButtonEnum_);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            Natron_StandardButtonEnum_PythonToCpp_QFlags_Natron_StandardButtonEnum_,
            is_Natron_StandardButtonEnum_PythonToCpp_QFlags_Natron_StandardButtonEnum__Convertible);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            QFlags_Natron_StandardButtonEnum__PythonToCpp_QFlags_Natron_StandardButtonEnum_,
            is_QFlags_Natron_StandardButtonEnum__PythonToCpp_QFlags_Natron_StandardButtonEnum__Convertible);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            number_PythonToCpp_QFlags_Natron_StandardButtonEnum_,
            is_number_PythonToCpp_QFlags_Natron_StandardButtonEnum__Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_QFLAGS_NATRON_STANDARDBUTTONENUM__IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_QFLAGS_NATRON_STANDARDBUTTONENUM__IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "QFlags<QFlags<Natron::StandardButtonEnum>");
        Shiboken::Conversions::registerConverterName(converter, "QFlags<StandardButtonEnum>");
    }
    // End of 'StandardButtonEnum' enum/flags.

    // Initialization of enum 'ImageComponentsEnum'.
    SbkNatronEngineTypes[SBK_NATRON_IMAGECOMPONENTSENUM_IDX] = Shiboken::Enum::createScopedEnum(&Sbk_Natron_Type,
        "ImageComponentsEnum",
        "NatronEngine.Natron.ImageComponentsEnum",
        "Natron::ImageComponentsEnum");
    if (!SbkNatronEngineTypes[SBK_NATRON_IMAGECOMPONENTSENUM_IDX])
        return ;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_IMAGECOMPONENTSENUM_IDX],
        &Sbk_Natron_Type, "eImageComponentNone", (long) Natron::eImageComponentNone))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_IMAGECOMPONENTSENUM_IDX],
        &Sbk_Natron_Type, "eImageComponentAlpha", (long) Natron::eImageComponentAlpha))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_IMAGECOMPONENTSENUM_IDX],
        &Sbk_Natron_Type, "eImageComponentRGB", (long) Natron::eImageComponentRGB))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_IMAGECOMPONENTSENUM_IDX],
        &Sbk_Natron_Type, "eImageComponentRGBA", (long) Natron::eImageComponentRGBA))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_IMAGECOMPONENTSENUM_IDX],
        &Sbk_Natron_Type, "eImageComponentXY", (long) Natron::eImageComponentXY))
        return ;
    // Register converter for enum 'Natron::ImageComponentsEnum'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_IMAGECOMPONENTSENUM_IDX],
            Natron_ImageComponentsEnum_CppToPython_Natron_ImageComponentsEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            Natron_ImageComponentsEnum_PythonToCpp_Natron_ImageComponentsEnum,
            is_Natron_ImageComponentsEnum_PythonToCpp_Natron_ImageComponentsEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_IMAGECOMPONENTSENUM_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_IMAGECOMPONENTSENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "Natron::ImageComponentsEnum");
        Shiboken::Conversions::registerConverterName(converter, "ImageComponentsEnum");
    }
    // End of 'ImageComponentsEnum' enum.

    // Initialization of enum 'ImageBitDepthEnum'.
    SbkNatronEngineTypes[SBK_NATRON_IMAGEBITDEPTHENUM_IDX] = Shiboken::Enum::createScopedEnum(&Sbk_Natron_Type,
        "ImageBitDepthEnum",
        "NatronEngine.Natron.ImageBitDepthEnum",
        "Natron::ImageBitDepthEnum");
    if (!SbkNatronEngineTypes[SBK_NATRON_IMAGEBITDEPTHENUM_IDX])
        return ;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_IMAGEBITDEPTHENUM_IDX],
        &Sbk_Natron_Type, "eImageBitDepthNone", (long) Natron::eImageBitDepthNone))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_IMAGEBITDEPTHENUM_IDX],
        &Sbk_Natron_Type, "eImageBitDepthByte", (long) Natron::eImageBitDepthByte))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_IMAGEBITDEPTHENUM_IDX],
        &Sbk_Natron_Type, "eImageBitDepthShort", (long) Natron::eImageBitDepthShort))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_IMAGEBITDEPTHENUM_IDX],
        &Sbk_Natron_Type, "eImageBitDepthHalf", (long) Natron::eImageBitDepthHalf))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_IMAGEBITDEPTHENUM_IDX],
        &Sbk_Natron_Type, "eImageBitDepthFloat", (long) Natron::eImageBitDepthFloat))
        return ;
    // Register converter for enum 'Natron::ImageBitDepthEnum'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_IMAGEBITDEPTHENUM_IDX],
            Natron_ImageBitDepthEnum_CppToPython_Natron_ImageBitDepthEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            Natron_ImageBitDepthEnum_PythonToCpp_Natron_ImageBitDepthEnum,
            is_Natron_ImageBitDepthEnum_PythonToCpp_Natron_ImageBitDepthEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_IMAGEBITDEPTHENUM_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_IMAGEBITDEPTHENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "Natron::ImageBitDepthEnum");
        Shiboken::Conversions::registerConverterName(converter, "ImageBitDepthEnum");
    }
    // End of 'ImageBitDepthEnum' enum.

    // Initialization of enum 'KeyframeTypeEnum'.
    SbkNatronEngineTypes[SBK_NATRON_KEYFRAMETYPEENUM_IDX] = Shiboken::Enum::createScopedEnum(&Sbk_Natron_Type,
        "KeyframeTypeEnum",
        "NatronEngine.Natron.KeyframeTypeEnum",
        "Natron::KeyframeTypeEnum");
    if (!SbkNatronEngineTypes[SBK_NATRON_KEYFRAMETYPEENUM_IDX])
        return ;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_KEYFRAMETYPEENUM_IDX],
        &Sbk_Natron_Type, "eKeyframeTypeConstant", (long) Natron::eKeyframeTypeConstant))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_KEYFRAMETYPEENUM_IDX],
        &Sbk_Natron_Type, "eKeyframeTypeLinear", (long) Natron::eKeyframeTypeLinear))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_KEYFRAMETYPEENUM_IDX],
        &Sbk_Natron_Type, "eKeyframeTypeSmooth", (long) Natron::eKeyframeTypeSmooth))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_KEYFRAMETYPEENUM_IDX],
        &Sbk_Natron_Type, "eKeyframeTypeCatmullRom", (long) Natron::eKeyframeTypeCatmullRom))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_KEYFRAMETYPEENUM_IDX],
        &Sbk_Natron_Type, "eKeyframeTypeCubic", (long) Natron::eKeyframeTypeCubic))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_KEYFRAMETYPEENUM_IDX],
        &Sbk_Natron_Type, "eKeyframeTypeHorizontal", (long) Natron::eKeyframeTypeHorizontal))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_KEYFRAMETYPEENUM_IDX],
        &Sbk_Natron_Type, "eKeyframeTypeFree", (long) Natron::eKeyframeTypeFree))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_KEYFRAMETYPEENUM_IDX],
        &Sbk_Natron_Type, "eKeyframeTypeBroken", (long) Natron::eKeyframeTypeBroken))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_KEYFRAMETYPEENUM_IDX],
        &Sbk_Natron_Type, "eKeyframeTypeNone", (long) Natron::eKeyframeTypeNone))
        return ;
    // Register converter for enum 'Natron::KeyframeTypeEnum'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_KEYFRAMETYPEENUM_IDX],
            Natron_KeyframeTypeEnum_CppToPython_Natron_KeyframeTypeEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            Natron_KeyframeTypeEnum_PythonToCpp_Natron_KeyframeTypeEnum,
            is_Natron_KeyframeTypeEnum_PythonToCpp_Natron_KeyframeTypeEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_KEYFRAMETYPEENUM_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_KEYFRAMETYPEENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "Natron::KeyframeTypeEnum");
        Shiboken::Conversions::registerConverterName(converter, "KeyframeTypeEnum");
    }
    // End of 'KeyframeTypeEnum' enum.

    // Initialization of enum 'ValueChangedReasonEnum'.
    SbkNatronEngineTypes[SBK_NATRON_VALUECHANGEDREASONENUM_IDX] = Shiboken::Enum::createScopedEnum(&Sbk_Natron_Type,
        "ValueChangedReasonEnum",
        "NatronEngine.Natron.ValueChangedReasonEnum",
        "Natron::ValueChangedReasonEnum");
    if (!SbkNatronEngineTypes[SBK_NATRON_VALUECHANGEDREASONENUM_IDX])
        return ;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_VALUECHANGEDREASONENUM_IDX],
        &Sbk_Natron_Type, "eValueChangedReasonUserEdited", (long) Natron::eValueChangedReasonUserEdited))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_VALUECHANGEDREASONENUM_IDX],
        &Sbk_Natron_Type, "eValueChangedReasonPluginEdited", (long) Natron::eValueChangedReasonPluginEdited))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_VALUECHANGEDREASONENUM_IDX],
        &Sbk_Natron_Type, "eValueChangedReasonNatronGuiEdited", (long) Natron::eValueChangedReasonNatronGuiEdited))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_VALUECHANGEDREASONENUM_IDX],
        &Sbk_Natron_Type, "eValueChangedReasonNatronInternalEdited", (long) Natron::eValueChangedReasonNatronInternalEdited))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_VALUECHANGEDREASONENUM_IDX],
        &Sbk_Natron_Type, "eValueChangedReasonTimeChanged", (long) Natron::eValueChangedReasonTimeChanged))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_VALUECHANGEDREASONENUM_IDX],
        &Sbk_Natron_Type, "eValueChangedReasonSlaveRefresh", (long) Natron::eValueChangedReasonSlaveRefresh))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_VALUECHANGEDREASONENUM_IDX],
        &Sbk_Natron_Type, "eValueChangedReasonRestoreDefault", (long) Natron::eValueChangedReasonRestoreDefault))
        return ;
    // Register converter for enum 'Natron::ValueChangedReasonEnum'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_VALUECHANGEDREASONENUM_IDX],
            Natron_ValueChangedReasonEnum_CppToPython_Natron_ValueChangedReasonEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            Natron_ValueChangedReasonEnum_PythonToCpp_Natron_ValueChangedReasonEnum,
            is_Natron_ValueChangedReasonEnum_PythonToCpp_Natron_ValueChangedReasonEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_VALUECHANGEDREASONENUM_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_VALUECHANGEDREASONENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "Natron::ValueChangedReasonEnum");
        Shiboken::Conversions::registerConverterName(converter, "ValueChangedReasonEnum");
    }
    // End of 'ValueChangedReasonEnum' enum.

    // Initialization of enum 'AnimationLevelEnum'.
    SbkNatronEngineTypes[SBK_NATRON_ANIMATIONLEVELENUM_IDX] = Shiboken::Enum::createScopedEnum(&Sbk_Natron_Type,
        "AnimationLevelEnum",
        "NatronEngine.Natron.AnimationLevelEnum",
        "Natron::AnimationLevelEnum");
    if (!SbkNatronEngineTypes[SBK_NATRON_ANIMATIONLEVELENUM_IDX])
        return ;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ANIMATIONLEVELENUM_IDX],
        &Sbk_Natron_Type, "eAnimationLevelNone", (long) Natron::eAnimationLevelNone))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ANIMATIONLEVELENUM_IDX],
        &Sbk_Natron_Type, "eAnimationLevelInterpolatedValue", (long) Natron::eAnimationLevelInterpolatedValue))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ANIMATIONLEVELENUM_IDX],
        &Sbk_Natron_Type, "eAnimationLevelOnKeyframe", (long) Natron::eAnimationLevelOnKeyframe))
        return ;
    // Register converter for enum 'Natron::AnimationLevelEnum'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_ANIMATIONLEVELENUM_IDX],
            Natron_AnimationLevelEnum_CppToPython_Natron_AnimationLevelEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            Natron_AnimationLevelEnum_PythonToCpp_Natron_AnimationLevelEnum,
            is_Natron_AnimationLevelEnum_PythonToCpp_Natron_AnimationLevelEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_ANIMATIONLEVELENUM_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_ANIMATIONLEVELENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "Natron::AnimationLevelEnum");
        Shiboken::Conversions::registerConverterName(converter, "AnimationLevelEnum");
    }
    // End of 'AnimationLevelEnum' enum.

    // Initialization of enum 'OrientationEnum'.
    SbkNatronEngineTypes[SBK_NATRON_ORIENTATIONENUM_IDX] = Shiboken::Enum::createScopedEnum(&Sbk_Natron_Type,
        "OrientationEnum",
        "NatronEngine.Natron.OrientationEnum",
        "Natron::OrientationEnum");
    if (!SbkNatronEngineTypes[SBK_NATRON_ORIENTATIONENUM_IDX])
        return ;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ORIENTATIONENUM_IDX],
        &Sbk_Natron_Type, "eOrientationHorizontal", (long) Natron::eOrientationHorizontal))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ORIENTATIONENUM_IDX],
        &Sbk_Natron_Type, "eOrientationVertical", (long) Natron::eOrientationVertical))
        return ;
    // Register converter for enum 'Natron::OrientationEnum'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_ORIENTATIONENUM_IDX],
            Natron_OrientationEnum_CppToPython_Natron_OrientationEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            Natron_OrientationEnum_PythonToCpp_Natron_OrientationEnum,
            is_Natron_OrientationEnum_PythonToCpp_Natron_OrientationEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_ORIENTATIONENUM_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_ORIENTATIONENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "Natron::OrientationEnum");
        Shiboken::Conversions::registerConverterName(converter, "OrientationEnum");
    }
    // End of 'OrientationEnum' enum.

    // Initialization of enum 'DisplayChannelsEnum'.
    SbkNatronEngineTypes[SBK_NATRON_DISPLAYCHANNELSENUM_IDX] = Shiboken::Enum::createScopedEnum(&Sbk_Natron_Type,
        "DisplayChannelsEnum",
        "NatronEngine.Natron.DisplayChannelsEnum",
        "Natron::DisplayChannelsEnum");
    if (!SbkNatronEngineTypes[SBK_NATRON_DISPLAYCHANNELSENUM_IDX])
        return ;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_DISPLAYCHANNELSENUM_IDX],
        &Sbk_Natron_Type, "eDisplayChannelsRGB", (long) Natron::eDisplayChannelsRGB))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_DISPLAYCHANNELSENUM_IDX],
        &Sbk_Natron_Type, "eDisplayChannelsR", (long) Natron::eDisplayChannelsR))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_DISPLAYCHANNELSENUM_IDX],
        &Sbk_Natron_Type, "eDisplayChannelsG", (long) Natron::eDisplayChannelsG))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_DISPLAYCHANNELSENUM_IDX],
        &Sbk_Natron_Type, "eDisplayChannelsB", (long) Natron::eDisplayChannelsB))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_DISPLAYCHANNELSENUM_IDX],
        &Sbk_Natron_Type, "eDisplayChannelsA", (long) Natron::eDisplayChannelsA))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_DISPLAYCHANNELSENUM_IDX],
        &Sbk_Natron_Type, "eDisplayChannelsY", (long) Natron::eDisplayChannelsY))
        return ;
    // Register converter for enum 'Natron::DisplayChannelsEnum'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_DISPLAYCHANNELSENUM_IDX],
            Natron_DisplayChannelsEnum_CppToPython_Natron_DisplayChannelsEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            Natron_DisplayChannelsEnum_PythonToCpp_Natron_DisplayChannelsEnum,
            is_Natron_DisplayChannelsEnum_PythonToCpp_Natron_DisplayChannelsEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_DISPLAYCHANNELSENUM_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_DISPLAYCHANNELSENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "Natron::DisplayChannelsEnum");
        Shiboken::Conversions::registerConverterName(converter, "DisplayChannelsEnum");
    }
    // End of 'DisplayChannelsEnum' enum.

    // Initialization of enum 'ImagePremultiplicationEnum'.
    SbkNatronEngineTypes[SBK_NATRON_IMAGEPREMULTIPLICATIONENUM_IDX] = Shiboken::Enum::createScopedEnum(&Sbk_Natron_Type,
        "ImagePremultiplicationEnum",
        "NatronEngine.Natron.ImagePremultiplicationEnum",
        "Natron::ImagePremultiplicationEnum");
    if (!SbkNatronEngineTypes[SBK_NATRON_IMAGEPREMULTIPLICATIONENUM_IDX])
        return ;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_IMAGEPREMULTIPLICATIONENUM_IDX],
        &Sbk_Natron_Type, "eImagePremultiplicationOpaque", (long) Natron::eImagePremultiplicationOpaque))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_IMAGEPREMULTIPLICATIONENUM_IDX],
        &Sbk_Natron_Type, "eImagePremultiplicationPremultiplied", (long) Natron::eImagePremultiplicationPremultiplied))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_IMAGEPREMULTIPLICATIONENUM_IDX],
        &Sbk_Natron_Type, "eImagePremultiplicationUnPremultiplied", (long) Natron::eImagePremultiplicationUnPremultiplied))
        return ;
    // Register converter for enum 'Natron::ImagePremultiplicationEnum'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_IMAGEPREMULTIPLICATIONENUM_IDX],
            Natron_ImagePremultiplicationEnum_CppToPython_Natron_ImagePremultiplicationEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            Natron_ImagePremultiplicationEnum_PythonToCpp_Natron_ImagePremultiplicationEnum,
            is_Natron_ImagePremultiplicationEnum_PythonToCpp_Natron_ImagePremultiplicationEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_IMAGEPREMULTIPLICATIONENUM_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_IMAGEPREMULTIPLICATIONENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "Natron::ImagePremultiplicationEnum");
        Shiboken::Conversions::registerConverterName(converter, "ImagePremultiplicationEnum");
    }
    // End of 'ImagePremultiplicationEnum' enum.

    // Initialization of enum 'StatusEnum'.
    SbkNatronEngineTypes[SBK_NATRON_STATUSENUM_IDX] = Shiboken::Enum::createScopedEnum(&Sbk_Natron_Type,
        "StatusEnum",
        "NatronEngine.Natron.StatusEnum",
        "Natron::StatusEnum");
    if (!SbkNatronEngineTypes[SBK_NATRON_STATUSENUM_IDX])
        return ;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STATUSENUM_IDX],
        &Sbk_Natron_Type, "eStatusOK", (long) Natron::eStatusOK))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STATUSENUM_IDX],
        &Sbk_Natron_Type, "eStatusFailed", (long) Natron::eStatusFailed))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STATUSENUM_IDX],
        &Sbk_Natron_Type, "eStatusReplyDefault", (long) Natron::eStatusReplyDefault))
        return ;
    // Register converter for enum 'Natron::StatusEnum'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_STATUSENUM_IDX],
            Natron_StatusEnum_CppToPython_Natron_StatusEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            Natron_StatusEnum_PythonToCpp_Natron_StatusEnum,
            is_Natron_StatusEnum_PythonToCpp_Natron_StatusEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_STATUSENUM_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_STATUSENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "Natron::StatusEnum");
        Shiboken::Conversions::registerConverterName(converter, "StatusEnum");
    }
    // End of 'StatusEnum' enum.

    // Initialization of enum 'ViewerCompositingOperatorEnum'.
    SbkNatronEngineTypes[SBK_NATRON_VIEWERCOMPOSITINGOPERATORENUM_IDX] = Shiboken::Enum::createScopedEnum(&Sbk_Natron_Type,
        "ViewerCompositingOperatorEnum",
        "NatronEngine.Natron.ViewerCompositingOperatorEnum",
        "Natron::ViewerCompositingOperatorEnum");
    if (!SbkNatronEngineTypes[SBK_NATRON_VIEWERCOMPOSITINGOPERATORENUM_IDX])
        return ;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_VIEWERCOMPOSITINGOPERATORENUM_IDX],
        &Sbk_Natron_Type, "eViewerCompositingOperatorNone", (long) Natron::eViewerCompositingOperatorNone))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_VIEWERCOMPOSITINGOPERATORENUM_IDX],
        &Sbk_Natron_Type, "eViewerCompositingOperatorOver", (long) Natron::eViewerCompositingOperatorOver))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_VIEWERCOMPOSITINGOPERATORENUM_IDX],
        &Sbk_Natron_Type, "eViewerCompositingOperatorMinus", (long) Natron::eViewerCompositingOperatorMinus))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_VIEWERCOMPOSITINGOPERATORENUM_IDX],
        &Sbk_Natron_Type, "eViewerCompositingOperatorUnder", (long) Natron::eViewerCompositingOperatorUnder))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_VIEWERCOMPOSITINGOPERATORENUM_IDX],
        &Sbk_Natron_Type, "eViewerCompositingOperatorWipe", (long) Natron::eViewerCompositingOperatorWipe))
        return ;
    // Register converter for enum 'Natron::ViewerCompositingOperatorEnum'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_VIEWERCOMPOSITINGOPERATORENUM_IDX],
            Natron_ViewerCompositingOperatorEnum_CppToPython_Natron_ViewerCompositingOperatorEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            Natron_ViewerCompositingOperatorEnum_PythonToCpp_Natron_ViewerCompositingOperatorEnum,
            is_Natron_ViewerCompositingOperatorEnum_PythonToCpp_Natron_ViewerCompositingOperatorEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_VIEWERCOMPOSITINGOPERATORENUM_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_VIEWERCOMPOSITINGOPERATORENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "Natron::ViewerCompositingOperatorEnum");
        Shiboken::Conversions::registerConverterName(converter, "ViewerCompositingOperatorEnum");
    }
    // End of 'ViewerCompositingOperatorEnum' enum.

    // Initialization of enum 'PlaybackModeEnum'.
    SbkNatronEngineTypes[SBK_NATRON_PLAYBACKMODEENUM_IDX] = Shiboken::Enum::createScopedEnum(&Sbk_Natron_Type,
        "PlaybackModeEnum",
        "NatronEngine.Natron.PlaybackModeEnum",
        "Natron::PlaybackModeEnum");
    if (!SbkNatronEngineTypes[SBK_NATRON_PLAYBACKMODEENUM_IDX])
        return ;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PLAYBACKMODEENUM_IDX],
        &Sbk_Natron_Type, "ePlaybackModeLoop", (long) Natron::ePlaybackModeLoop))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PLAYBACKMODEENUM_IDX],
        &Sbk_Natron_Type, "ePlaybackModeBounce", (long) Natron::ePlaybackModeBounce))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PLAYBACKMODEENUM_IDX],
        &Sbk_Natron_Type, "ePlaybackModeOnce", (long) Natron::ePlaybackModeOnce))
        return ;
    // Register converter for enum 'Natron::PlaybackModeEnum'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_PLAYBACKMODEENUM_IDX],
            Natron_PlaybackModeEnum_CppToPython_Natron_PlaybackModeEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            Natron_PlaybackModeEnum_PythonToCpp_Natron_PlaybackModeEnum,
            is_Natron_PlaybackModeEnum_PythonToCpp_Natron_PlaybackModeEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_PLAYBACKMODEENUM_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_PLAYBACKMODEENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "Natron::PlaybackModeEnum");
        Shiboken::Conversions::registerConverterName(converter, "PlaybackModeEnum");
    }
    // End of 'PlaybackModeEnum' enum.

    // Initialization of enum 'PixmapEnum'.
    SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX] = Shiboken::Enum::createScopedEnum(&Sbk_Natron_Type,
        "PixmapEnum",
        "NatronEngine.Natron.PixmapEnum",
        "Natron::PixmapEnum");
    if (!SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX])
        return ;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_PLAYER_PREVIOUS", (long) Natron::NATRON_PIXMAP_PLAYER_PREVIOUS))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_PLAYER_FIRST_FRAME", (long) Natron::NATRON_PIXMAP_PLAYER_FIRST_FRAME))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_PLAYER_NEXT", (long) Natron::NATRON_PIXMAP_PLAYER_NEXT))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_PLAYER_LAST_FRAME", (long) Natron::NATRON_PIXMAP_PLAYER_LAST_FRAME))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_PLAYER_NEXT_INCR", (long) Natron::NATRON_PIXMAP_PLAYER_NEXT_INCR))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_PLAYER_NEXT_KEY", (long) Natron::NATRON_PIXMAP_PLAYER_NEXT_KEY))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_PLAYER_PREVIOUS_INCR", (long) Natron::NATRON_PIXMAP_PLAYER_PREVIOUS_INCR))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_PLAYER_PREVIOUS_KEY", (long) Natron::NATRON_PIXMAP_PLAYER_PREVIOUS_KEY))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_PLAYER_REWIND_ENABLED", (long) Natron::NATRON_PIXMAP_PLAYER_REWIND_ENABLED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_PLAYER_REWIND_DISABLED", (long) Natron::NATRON_PIXMAP_PLAYER_REWIND_DISABLED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_PLAYER_PLAY_ENABLED", (long) Natron::NATRON_PIXMAP_PLAYER_PLAY_ENABLED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_PLAYER_PLAY_DISABLED", (long) Natron::NATRON_PIXMAP_PLAYER_PLAY_DISABLED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_PLAYER_STOP", (long) Natron::NATRON_PIXMAP_PLAYER_STOP))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_PLAYER_LOOP_MODE", (long) Natron::NATRON_PIXMAP_PLAYER_LOOP_MODE))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_PLAYER_BOUNCE", (long) Natron::NATRON_PIXMAP_PLAYER_BOUNCE))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_PLAYER_PLAY_ONCE", (long) Natron::NATRON_PIXMAP_PLAYER_PLAY_ONCE))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MAXIMIZE_WIDGET", (long) Natron::NATRON_PIXMAP_MAXIMIZE_WIDGET))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MINIMIZE_WIDGET", (long) Natron::NATRON_PIXMAP_MINIMIZE_WIDGET))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_CLOSE_WIDGET", (long) Natron::NATRON_PIXMAP_CLOSE_WIDGET))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_HELP_WIDGET", (long) Natron::NATRON_PIXMAP_HELP_WIDGET))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_CLOSE_PANEL", (long) Natron::NATRON_PIXMAP_CLOSE_PANEL))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_HIDE_UNMODIFIED", (long) Natron::NATRON_PIXMAP_HIDE_UNMODIFIED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_UNHIDE_UNMODIFIED", (long) Natron::NATRON_PIXMAP_UNHIDE_UNMODIFIED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_GROUPBOX_FOLDED", (long) Natron::NATRON_PIXMAP_GROUPBOX_FOLDED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_GROUPBOX_UNFOLDED", (long) Natron::NATRON_PIXMAP_GROUPBOX_UNFOLDED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_UNDO", (long) Natron::NATRON_PIXMAP_UNDO))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_REDO", (long) Natron::NATRON_PIXMAP_REDO))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_UNDO_GRAYSCALE", (long) Natron::NATRON_PIXMAP_UNDO_GRAYSCALE))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_REDO_GRAYSCALE", (long) Natron::NATRON_PIXMAP_REDO_GRAYSCALE))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_RESTORE_DEFAULTS_ENABLED", (long) Natron::NATRON_PIXMAP_RESTORE_DEFAULTS_ENABLED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_RESTORE_DEFAULTS_DISABLED", (long) Natron::NATRON_PIXMAP_RESTORE_DEFAULTS_DISABLED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_TAB_WIDGET_LAYOUT_BUTTON", (long) Natron::NATRON_PIXMAP_TAB_WIDGET_LAYOUT_BUTTON))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_TAB_WIDGET_LAYOUT_BUTTON_ANCHOR", (long) Natron::NATRON_PIXMAP_TAB_WIDGET_LAYOUT_BUTTON_ANCHOR))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_TAB_WIDGET_SPLIT_HORIZONTALLY", (long) Natron::NATRON_PIXMAP_TAB_WIDGET_SPLIT_HORIZONTALLY))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_TAB_WIDGET_SPLIT_VERTICALLY", (long) Natron::NATRON_PIXMAP_TAB_WIDGET_SPLIT_VERTICALLY))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_VIEWER_CENTER", (long) Natron::NATRON_PIXMAP_VIEWER_CENTER))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_VIEWER_CLIP_TO_PROJECT_ENABLED", (long) Natron::NATRON_PIXMAP_VIEWER_CLIP_TO_PROJECT_ENABLED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_VIEWER_CLIP_TO_PROJECT_DISABLED", (long) Natron::NATRON_PIXMAP_VIEWER_CLIP_TO_PROJECT_DISABLED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_VIEWER_REFRESH", (long) Natron::NATRON_PIXMAP_VIEWER_REFRESH))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_VIEWER_REFRESH_ACTIVE", (long) Natron::NATRON_PIXMAP_VIEWER_REFRESH_ACTIVE))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_VIEWER_ROI_ENABLED", (long) Natron::NATRON_PIXMAP_VIEWER_ROI_ENABLED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_VIEWER_ROI_DISABLED", (long) Natron::NATRON_PIXMAP_VIEWER_ROI_DISABLED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_VIEWER_RENDER_SCALE", (long) Natron::NATRON_PIXMAP_VIEWER_RENDER_SCALE))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_VIEWER_RENDER_SCALE_CHECKED", (long) Natron::NATRON_PIXMAP_VIEWER_RENDER_SCALE_CHECKED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_OPEN_FILE", (long) Natron::NATRON_PIXMAP_OPEN_FILE))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_COLORWHEEL", (long) Natron::NATRON_PIXMAP_COLORWHEEL))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_OVERLAY", (long) Natron::NATRON_PIXMAP_OVERLAY))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_ROTO_MERGE", (long) Natron::NATRON_PIXMAP_ROTO_MERGE))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_COLOR_PICKER", (long) Natron::NATRON_PIXMAP_COLOR_PICKER))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_IO_GROUPING", (long) Natron::NATRON_PIXMAP_IO_GROUPING))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_3D_GROUPING", (long) Natron::NATRON_PIXMAP_3D_GROUPING))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_CHANNEL_GROUPING", (long) Natron::NATRON_PIXMAP_CHANNEL_GROUPING))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MERGE_GROUPING", (long) Natron::NATRON_PIXMAP_MERGE_GROUPING))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_COLOR_GROUPING", (long) Natron::NATRON_PIXMAP_COLOR_GROUPING))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_TRANSFORM_GROUPING", (long) Natron::NATRON_PIXMAP_TRANSFORM_GROUPING))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_DEEP_GROUPING", (long) Natron::NATRON_PIXMAP_DEEP_GROUPING))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_FILTER_GROUPING", (long) Natron::NATRON_PIXMAP_FILTER_GROUPING))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MULTIVIEW_GROUPING", (long) Natron::NATRON_PIXMAP_MULTIVIEW_GROUPING))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_TOOLSETS_GROUPING", (long) Natron::NATRON_PIXMAP_TOOLSETS_GROUPING))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MISC_GROUPING", (long) Natron::NATRON_PIXMAP_MISC_GROUPING))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_OPEN_EFFECTS_GROUPING", (long) Natron::NATRON_PIXMAP_OPEN_EFFECTS_GROUPING))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_TIME_GROUPING", (long) Natron::NATRON_PIXMAP_TIME_GROUPING))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_PAINT_GROUPING", (long) Natron::NATRON_PIXMAP_PAINT_GROUPING))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_KEYER_GROUPING", (long) Natron::NATRON_PIXMAP_KEYER_GROUPING))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_OTHER_PLUGINS", (long) Natron::NATRON_PIXMAP_OTHER_PLUGINS))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_READ_IMAGE", (long) Natron::NATRON_PIXMAP_READ_IMAGE))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_WRITE_IMAGE", (long) Natron::NATRON_PIXMAP_WRITE_IMAGE))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_COMBOBOX", (long) Natron::NATRON_PIXMAP_COMBOBOX))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_COMBOBOX_PRESSED", (long) Natron::NATRON_PIXMAP_COMBOBOX_PRESSED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_ADD_KEYFRAME", (long) Natron::NATRON_PIXMAP_ADD_KEYFRAME))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_REMOVE_KEYFRAME", (long) Natron::NATRON_PIXMAP_REMOVE_KEYFRAME))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_INVERTED", (long) Natron::NATRON_PIXMAP_INVERTED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_UNINVERTED", (long) Natron::NATRON_PIXMAP_UNINVERTED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_VISIBLE", (long) Natron::NATRON_PIXMAP_VISIBLE))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_UNVISIBLE", (long) Natron::NATRON_PIXMAP_UNVISIBLE))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_LOCKED", (long) Natron::NATRON_PIXMAP_LOCKED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_UNLOCKED", (long) Natron::NATRON_PIXMAP_UNLOCKED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_LAYER", (long) Natron::NATRON_PIXMAP_LAYER))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_BEZIER", (long) Natron::NATRON_PIXMAP_BEZIER))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_PENCIL", (long) Natron::NATRON_PIXMAP_PENCIL))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_CURVE", (long) Natron::NATRON_PIXMAP_CURVE))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_BEZIER_32", (long) Natron::NATRON_PIXMAP_BEZIER_32))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_ELLIPSE", (long) Natron::NATRON_PIXMAP_ELLIPSE))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_RECTANGLE", (long) Natron::NATRON_PIXMAP_RECTANGLE))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_ADD_POINTS", (long) Natron::NATRON_PIXMAP_ADD_POINTS))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_REMOVE_POINTS", (long) Natron::NATRON_PIXMAP_REMOVE_POINTS))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_CUSP_POINTS", (long) Natron::NATRON_PIXMAP_CUSP_POINTS))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_SMOOTH_POINTS", (long) Natron::NATRON_PIXMAP_SMOOTH_POINTS))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_REMOVE_FEATHER", (long) Natron::NATRON_PIXMAP_REMOVE_FEATHER))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_OPEN_CLOSE_CURVE", (long) Natron::NATRON_PIXMAP_OPEN_CLOSE_CURVE))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_SELECT_ALL", (long) Natron::NATRON_PIXMAP_SELECT_ALL))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_SELECT_POINTS", (long) Natron::NATRON_PIXMAP_SELECT_POINTS))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_SELECT_FEATHER", (long) Natron::NATRON_PIXMAP_SELECT_FEATHER))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_SELECT_CURVES", (long) Natron::NATRON_PIXMAP_SELECT_CURVES))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_AUTO_KEYING_ENABLED", (long) Natron::NATRON_PIXMAP_AUTO_KEYING_ENABLED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_AUTO_KEYING_DISABLED", (long) Natron::NATRON_PIXMAP_AUTO_KEYING_DISABLED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_STICKY_SELECTION_ENABLED", (long) Natron::NATRON_PIXMAP_STICKY_SELECTION_ENABLED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_STICKY_SELECTION_DISABLED", (long) Natron::NATRON_PIXMAP_STICKY_SELECTION_DISABLED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_FEATHER_LINK_ENABLED", (long) Natron::NATRON_PIXMAP_FEATHER_LINK_ENABLED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_FEATHER_LINK_DISABLED", (long) Natron::NATRON_PIXMAP_FEATHER_LINK_DISABLED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_FEATHER_VISIBLE", (long) Natron::NATRON_PIXMAP_FEATHER_VISIBLE))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_FEATHER_UNVISIBLE", (long) Natron::NATRON_PIXMAP_FEATHER_UNVISIBLE))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_RIPPLE_EDIT_ENABLED", (long) Natron::NATRON_PIXMAP_RIPPLE_EDIT_ENABLED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_RIPPLE_EDIT_DISABLED", (long) Natron::NATRON_PIXMAP_RIPPLE_EDIT_DISABLED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_ROTOPAINT_BLUR", (long) Natron::NATRON_PIXMAP_ROTOPAINT_BLUR))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_ROTOPAINT_BUILDUP_ENABLED", (long) Natron::NATRON_PIXMAP_ROTOPAINT_BUILDUP_ENABLED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_ROTOPAINT_BUILDUP_DISABLED", (long) Natron::NATRON_PIXMAP_ROTOPAINT_BUILDUP_DISABLED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_ROTOPAINT_BURN", (long) Natron::NATRON_PIXMAP_ROTOPAINT_BURN))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_ROTOPAINT_CLONE", (long) Natron::NATRON_PIXMAP_ROTOPAINT_CLONE))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_ROTOPAINT_DODGE", (long) Natron::NATRON_PIXMAP_ROTOPAINT_DODGE))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_ROTOPAINT_ERASER", (long) Natron::NATRON_PIXMAP_ROTOPAINT_ERASER))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_ROTOPAINT_PRESSURE_ENABLED", (long) Natron::NATRON_PIXMAP_ROTOPAINT_PRESSURE_ENABLED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_ROTOPAINT_PRESSURE_DISABLED", (long) Natron::NATRON_PIXMAP_ROTOPAINT_PRESSURE_DISABLED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_ROTOPAINT_REVEAL", (long) Natron::NATRON_PIXMAP_ROTOPAINT_REVEAL))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_ROTOPAINT_SHARPEN", (long) Natron::NATRON_PIXMAP_ROTOPAINT_SHARPEN))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_ROTOPAINT_SMEAR", (long) Natron::NATRON_PIXMAP_ROTOPAINT_SMEAR))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_ROTOPAINT_SOLID", (long) Natron::NATRON_PIXMAP_ROTOPAINT_SOLID))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_BOLD_CHECKED", (long) Natron::NATRON_PIXMAP_BOLD_CHECKED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_BOLD_UNCHECKED", (long) Natron::NATRON_PIXMAP_BOLD_UNCHECKED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_ITALIC_CHECKED", (long) Natron::NATRON_PIXMAP_ITALIC_CHECKED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_ITALIC_UNCHECKED", (long) Natron::NATRON_PIXMAP_ITALIC_UNCHECKED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_CLEAR_ALL_ANIMATION", (long) Natron::NATRON_PIXMAP_CLEAR_ALL_ANIMATION))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_CLEAR_BACKWARD_ANIMATION", (long) Natron::NATRON_PIXMAP_CLEAR_BACKWARD_ANIMATION))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_CLEAR_FORWARD_ANIMATION", (long) Natron::NATRON_PIXMAP_CLEAR_FORWARD_ANIMATION))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_UPDATE_VIEWER_ENABLED", (long) Natron::NATRON_PIXMAP_UPDATE_VIEWER_ENABLED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_UPDATE_VIEWER_DISABLED", (long) Natron::NATRON_PIXMAP_UPDATE_VIEWER_DISABLED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_ADD_TRACK", (long) Natron::NATRON_PIXMAP_ADD_TRACK))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_ENTER_GROUP", (long) Natron::NATRON_PIXMAP_ENTER_GROUP))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_SETTINGS", (long) Natron::NATRON_PIXMAP_SETTINGS))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_FREEZE_ENABLED", (long) Natron::NATRON_PIXMAP_FREEZE_ENABLED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_FREEZE_DISABLED", (long) Natron::NATRON_PIXMAP_FREEZE_DISABLED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_VIEWER_ICON", (long) Natron::NATRON_PIXMAP_VIEWER_ICON))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_VIEWER_CHECKERBOARD_ENABLED", (long) Natron::NATRON_PIXMAP_VIEWER_CHECKERBOARD_ENABLED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_VIEWER_CHECKERBOARD_DISABLED", (long) Natron::NATRON_PIXMAP_VIEWER_CHECKERBOARD_DISABLED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_VIEWER_ZEBRA_ENABLED", (long) Natron::NATRON_PIXMAP_VIEWER_ZEBRA_ENABLED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_VIEWER_ZEBRA_DISABLED", (long) Natron::NATRON_PIXMAP_VIEWER_ZEBRA_DISABLED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_VIEWER_GAMMA_ENABLED", (long) Natron::NATRON_PIXMAP_VIEWER_GAMMA_ENABLED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_VIEWER_GAMMA_DISABLED", (long) Natron::NATRON_PIXMAP_VIEWER_GAMMA_DISABLED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_VIEWER_GAIN_ENABLED", (long) Natron::NATRON_PIXMAP_VIEWER_GAIN_ENABLED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_VIEWER_GAIN_DISABLED", (long) Natron::NATRON_PIXMAP_VIEWER_GAIN_DISABLED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_SCRIPT_CLEAR_OUTPUT", (long) Natron::NATRON_PIXMAP_SCRIPT_CLEAR_OUTPUT))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_SCRIPT_EXEC_SCRIPT", (long) Natron::NATRON_PIXMAP_SCRIPT_EXEC_SCRIPT))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_SCRIPT_LOAD_EXEC_SCRIPT", (long) Natron::NATRON_PIXMAP_SCRIPT_LOAD_EXEC_SCRIPT))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_SCRIPT_LOAD_SCRIPT", (long) Natron::NATRON_PIXMAP_SCRIPT_LOAD_SCRIPT))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_SCRIPT_NEXT_SCRIPT", (long) Natron::NATRON_PIXMAP_SCRIPT_NEXT_SCRIPT))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_SCRIPT_OUTPUT_PANE_ACTIVATED", (long) Natron::NATRON_PIXMAP_SCRIPT_OUTPUT_PANE_ACTIVATED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_SCRIPT_OUTPUT_PANE_DEACTIVATED", (long) Natron::NATRON_PIXMAP_SCRIPT_OUTPUT_PANE_DEACTIVATED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_SCRIPT_PREVIOUS_SCRIPT", (long) Natron::NATRON_PIXMAP_SCRIPT_PREVIOUS_SCRIPT))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_SCRIPT_SAVE_SCRIPT", (long) Natron::NATRON_PIXMAP_SCRIPT_SAVE_SCRIPT))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MERGE_ATOP", (long) Natron::NATRON_PIXMAP_MERGE_ATOP))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MERGE_AVERAGE", (long) Natron::NATRON_PIXMAP_MERGE_AVERAGE))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MERGE_COLOR_BURN", (long) Natron::NATRON_PIXMAP_MERGE_COLOR_BURN))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MERGE_COLOR_DODGE", (long) Natron::NATRON_PIXMAP_MERGE_COLOR_DODGE))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MERGE_CONJOINT_OVER", (long) Natron::NATRON_PIXMAP_MERGE_CONJOINT_OVER))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MERGE_COPY", (long) Natron::NATRON_PIXMAP_MERGE_COPY))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MERGE_DIFFERENCE", (long) Natron::NATRON_PIXMAP_MERGE_DIFFERENCE))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MERGE_DISJOINT_OVER", (long) Natron::NATRON_PIXMAP_MERGE_DISJOINT_OVER))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MERGE_DIVIDE", (long) Natron::NATRON_PIXMAP_MERGE_DIVIDE))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MERGE_EXCLUSION", (long) Natron::NATRON_PIXMAP_MERGE_EXCLUSION))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MERGE_FREEZE", (long) Natron::NATRON_PIXMAP_MERGE_FREEZE))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MERGE_FROM", (long) Natron::NATRON_PIXMAP_MERGE_FROM))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MERGE_GEOMETRIC", (long) Natron::NATRON_PIXMAP_MERGE_GEOMETRIC))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MERGE_HARD_LIGHT", (long) Natron::NATRON_PIXMAP_MERGE_HARD_LIGHT))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MERGE_HYPOT", (long) Natron::NATRON_PIXMAP_MERGE_HYPOT))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MERGE_IN", (long) Natron::NATRON_PIXMAP_MERGE_IN))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MERGE_INTERPOLATED", (long) Natron::NATRON_PIXMAP_MERGE_INTERPOLATED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MERGE_MASK", (long) Natron::NATRON_PIXMAP_MERGE_MASK))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MERGE_MATTE", (long) Natron::NATRON_PIXMAP_MERGE_MATTE))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MERGE_MAX", (long) Natron::NATRON_PIXMAP_MERGE_MAX))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MERGE_MIN", (long) Natron::NATRON_PIXMAP_MERGE_MIN))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MERGE_MINUS", (long) Natron::NATRON_PIXMAP_MERGE_MINUS))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MERGE_MULTIPLY", (long) Natron::NATRON_PIXMAP_MERGE_MULTIPLY))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MERGE_OUT", (long) Natron::NATRON_PIXMAP_MERGE_OUT))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MERGE_OVER", (long) Natron::NATRON_PIXMAP_MERGE_OVER))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MERGE_OVERLAY", (long) Natron::NATRON_PIXMAP_MERGE_OVERLAY))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MERGE_PINLIGHT", (long) Natron::NATRON_PIXMAP_MERGE_PINLIGHT))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MERGE_PLUS", (long) Natron::NATRON_PIXMAP_MERGE_PLUS))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MERGE_REFLECT", (long) Natron::NATRON_PIXMAP_MERGE_REFLECT))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MERGE_SCREEN", (long) Natron::NATRON_PIXMAP_MERGE_SCREEN))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MERGE_SOFT_LIGHT", (long) Natron::NATRON_PIXMAP_MERGE_SOFT_LIGHT))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MERGE_STENCIL", (long) Natron::NATRON_PIXMAP_MERGE_STENCIL))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MERGE_UNDER", (long) Natron::NATRON_PIXMAP_MERGE_UNDER))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_MERGE_XOR", (long) Natron::NATRON_PIXMAP_MERGE_XOR))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_ROTO_NODE_ICON", (long) Natron::NATRON_PIXMAP_ROTO_NODE_ICON))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_LINK_CURSOR", (long) Natron::NATRON_PIXMAP_LINK_CURSOR))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_APP_ICON", (long) Natron::NATRON_PIXMAP_APP_ICON))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_INTERP_LINEAR", (long) Natron::NATRON_PIXMAP_INTERP_LINEAR))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_INTERP_CURVE", (long) Natron::NATRON_PIXMAP_INTERP_CURVE))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_INTERP_CONSTANT", (long) Natron::NATRON_PIXMAP_INTERP_CONSTANT))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_INTERP_BREAK", (long) Natron::NATRON_PIXMAP_INTERP_BREAK))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_INTERP_CURVE_C", (long) Natron::NATRON_PIXMAP_INTERP_CURVE_C))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_INTERP_CURVE_H", (long) Natron::NATRON_PIXMAP_INTERP_CURVE_H))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_INTERP_CURVE_R", (long) Natron::NATRON_PIXMAP_INTERP_CURVE_R))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
        &Sbk_Natron_Type, "NATRON_PIXMAP_INTERP_CURVE_Z", (long) Natron::NATRON_PIXMAP_INTERP_CURVE_Z))
        return ;
    // Register converter for enum 'Natron::PixmapEnum'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX],
            Natron_PixmapEnum_CppToPython_Natron_PixmapEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            Natron_PixmapEnum_PythonToCpp_Natron_PixmapEnum,
            is_Natron_PixmapEnum_PythonToCpp_Natron_PixmapEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "Natron::PixmapEnum");
        Shiboken::Conversions::registerConverterName(converter, "PixmapEnum");
    }
    // End of 'PixmapEnum' enum.

    // Initialization of enum 'ViewerColorSpaceEnum'.
    SbkNatronEngineTypes[SBK_NATRON_VIEWERCOLORSPACEENUM_IDX] = Shiboken::Enum::createScopedEnum(&Sbk_Natron_Type,
        "ViewerColorSpaceEnum",
        "NatronEngine.Natron.ViewerColorSpaceEnum",
        "Natron::ViewerColorSpaceEnum");
    if (!SbkNatronEngineTypes[SBK_NATRON_VIEWERCOLORSPACEENUM_IDX])
        return ;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_VIEWERCOLORSPACEENUM_IDX],
        &Sbk_Natron_Type, "eViewerColorSpaceSRGB", (long) Natron::eViewerColorSpaceSRGB))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_VIEWERCOLORSPACEENUM_IDX],
        &Sbk_Natron_Type, "eViewerColorSpaceLinear", (long) Natron::eViewerColorSpaceLinear))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_VIEWERCOLORSPACEENUM_IDX],
        &Sbk_Natron_Type, "eViewerColorSpaceRec709", (long) Natron::eViewerColorSpaceRec709))
        return ;
    // Register converter for enum 'Natron::ViewerColorSpaceEnum'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_VIEWERCOLORSPACEENUM_IDX],
            Natron_ViewerColorSpaceEnum_CppToPython_Natron_ViewerColorSpaceEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            Natron_ViewerColorSpaceEnum_PythonToCpp_Natron_ViewerColorSpaceEnum,
            is_Natron_ViewerColorSpaceEnum_PythonToCpp_Natron_ViewerColorSpaceEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_VIEWERCOLORSPACEENUM_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_VIEWERCOLORSPACEENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "Natron::ViewerColorSpaceEnum");
        Shiboken::Conversions::registerConverterName(converter, "ViewerColorSpaceEnum");
    }
    // End of 'ViewerColorSpaceEnum' enum.


    qRegisterMetaType< ::Natron::StandardButtonEnum >("Natron::StandardButtonEnum");
    qRegisterMetaType< ::Natron::StandardButtons >("Natron::StandardButtons");
    qRegisterMetaType< ::Natron::ImageComponentsEnum >("Natron::ImageComponentsEnum");
    qRegisterMetaType< ::Natron::ImageBitDepthEnum >("Natron::ImageBitDepthEnum");
    qRegisterMetaType< ::Natron::KeyframeTypeEnum >("Natron::KeyframeTypeEnum");
    qRegisterMetaType< ::Natron::ValueChangedReasonEnum >("Natron::ValueChangedReasonEnum");
    qRegisterMetaType< ::Natron::AnimationLevelEnum >("Natron::AnimationLevelEnum");
    qRegisterMetaType< ::Natron::OrientationEnum >("Natron::OrientationEnum");
    qRegisterMetaType< ::Natron::DisplayChannelsEnum >("Natron::DisplayChannelsEnum");
    qRegisterMetaType< ::Natron::ImagePremultiplicationEnum >("Natron::ImagePremultiplicationEnum");
    qRegisterMetaType< ::Natron::StatusEnum >("Natron::StatusEnum");
    qRegisterMetaType< ::Natron::ViewerCompositingOperatorEnum >("Natron::ViewerCompositingOperatorEnum");
    qRegisterMetaType< ::Natron::PlaybackModeEnum >("Natron::PlaybackModeEnum");
    qRegisterMetaType< ::Natron::PixmapEnum >("Natron::PixmapEnum");
    qRegisterMetaType< ::Natron::ViewerColorSpaceEnum >("Natron::ViewerColorSpaceEnum");
}
