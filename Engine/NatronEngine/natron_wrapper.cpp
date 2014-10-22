
// default includes
#include <shiboken.h>
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

PyObject* SbkNatronEngine_Natron_StandardButton___and__(PyObject* self, PyObject* pyArg)
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
    return Shiboken::Conversions::copyToPython(SBK_CONVERTER(SbkNatronEngineTypes[SBK_QFLAGS_NATRON_STANDARDBUTTON__IDX]), &cppResult);
}

PyObject* SbkNatronEngine_Natron_StandardButton___or__(PyObject* self, PyObject* pyArg)
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
    return Shiboken::Conversions::copyToPython(SBK_CONVERTER(SbkNatronEngineTypes[SBK_QFLAGS_NATRON_STANDARDBUTTON__IDX]), &cppResult);
}

PyObject* SbkNatronEngine_Natron_StandardButton___xor__(PyObject* self, PyObject* pyArg)
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
    return Shiboken::Conversions::copyToPython(SBK_CONVERTER(SbkNatronEngineTypes[SBK_QFLAGS_NATRON_STANDARDBUTTON__IDX]), &cppResult);
}

PyObject* SbkNatronEngine_Natron_StandardButton___invert__(PyObject* self, PyObject* pyArg)
{
    ::Natron::StandardButtons cppSelf;
    Shiboken::Conversions::pythonToCppCopy(SBK_CONVERTER(SbkNatronEngineTypes[SBK_QFLAGS_NATRON_STANDARDBUTTON__IDX]), self, &cppSelf);
    ::Natron::StandardButtons cppResult = ~cppSelf;
    return Shiboken::Conversions::copyToPython(SBK_CONVERTER(SbkNatronEngineTypes[SBK_QFLAGS_NATRON_STANDARDBUTTON__IDX]), &cppResult);
}

static PyObject* SbkNatronEngine_Natron_StandardButton_long(PyObject* self)
{
    int val;
    Shiboken::Conversions::pythonToCppCopy(SBK_CONVERTER(SbkNatronEngineTypes[SBK_QFLAGS_NATRON_STANDARDBUTTON__IDX]), self, &val);
    return Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &val);
}
static int SbkNatronEngine_Natron_StandardButton__nonzero(PyObject* self)
{
    int val;
    Shiboken::Conversions::pythonToCppCopy(SBK_CONVERTER(SbkNatronEngineTypes[SBK_QFLAGS_NATRON_STANDARDBUTTON__IDX]), self, &val);
    return val != 0;
}

static PyNumberMethods SbkNatronEngine_Natron_StandardButton_as_number = {
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
    /*nb_nonzero*/              SbkNatronEngine_Natron_StandardButton__nonzero,
    /*nb_invert*/               (unaryfunc)SbkNatronEngine_Natron_StandardButton___invert__,
    /*nb_lshift*/               0,
    /*nb_rshift*/               0,
    /*nb_and*/                  (binaryfunc)SbkNatronEngine_Natron_StandardButton___and__,
    /*nb_xor*/                  (binaryfunc)SbkNatronEngine_Natron_StandardButton___xor__,
    /*nb_or*/                   (binaryfunc)SbkNatronEngine_Natron_StandardButton___or__,
    #ifndef IS_PY3K
    /* nb_coerce */             0,
    #endif
    /*nb_int*/                  SbkNatronEngine_Natron_StandardButton_long,
    #ifdef IS_PY3K
    /*nb_reserved*/             0,
    /*nb_float*/                0,
    #else
    /*nb_long*/                 SbkNatronEngine_Natron_StandardButton_long,
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
static void Natron_Orientation_PythonToCpp_Natron_Orientation(PyObject* pyIn, void* cppOut) {
    *((::Natron::Orientation*)cppOut) = (::Natron::Orientation) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_Natron_Orientation_PythonToCpp_Natron_Orientation_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_ORIENTATION_IDX]))
        return Natron_Orientation_PythonToCpp_Natron_Orientation;
    return 0;
}
static PyObject* Natron_Orientation_CppToPython_Natron_Orientation(const void* cppIn) {
    int castCppIn = *((::Natron::Orientation*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_ORIENTATION_IDX], castCppIn);

}

static void Natron_PlaybackMode_PythonToCpp_Natron_PlaybackMode(PyObject* pyIn, void* cppOut) {
    *((::Natron::PlaybackMode*)cppOut) = (::Natron::PlaybackMode) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_Natron_PlaybackMode_PythonToCpp_Natron_PlaybackMode_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_PLAYBACKMODE_IDX]))
        return Natron_PlaybackMode_PythonToCpp_Natron_PlaybackMode;
    return 0;
}
static PyObject* Natron_PlaybackMode_CppToPython_Natron_PlaybackMode(const void* cppIn) {
    int castCppIn = *((::Natron::PlaybackMode*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_PLAYBACKMODE_IDX], castCppIn);

}

static void Natron_ValueChangedReason_PythonToCpp_Natron_ValueChangedReason(PyObject* pyIn, void* cppOut) {
    *((::Natron::ValueChangedReason*)cppOut) = (::Natron::ValueChangedReason) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_Natron_ValueChangedReason_PythonToCpp_Natron_ValueChangedReason_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_VALUECHANGEDREASON_IDX]))
        return Natron_ValueChangedReason_PythonToCpp_Natron_ValueChangedReason;
    return 0;
}
static PyObject* Natron_ValueChangedReason_CppToPython_Natron_ValueChangedReason(const void* cppIn) {
    int castCppIn = *((::Natron::ValueChangedReason*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_VALUECHANGEDREASON_IDX], castCppIn);

}

static void Natron_StandardButton_PythonToCpp_Natron_StandardButton(PyObject* pyIn, void* cppOut) {
    *((::Natron::StandardButton*)cppOut) = (::Natron::StandardButton) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_Natron_StandardButton_PythonToCpp_Natron_StandardButton_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTON_IDX]))
        return Natron_StandardButton_PythonToCpp_Natron_StandardButton;
    return 0;
}
static PyObject* Natron_StandardButton_CppToPython_Natron_StandardButton(const void* cppIn) {
    int castCppIn = *((::Natron::StandardButton*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTON_IDX], castCppIn);

}

static void QFlags_Natron_StandardButton__PythonToCpp_QFlags_Natron_StandardButton_(PyObject* pyIn, void* cppOut) {
    *((::QFlags<Natron::StandardButton>*)cppOut) = ::QFlags<Natron::StandardButton>(QFlag(PySide::QFlags::getValue(reinterpret_cast<PySideQFlagsObject*>(pyIn))));

}
static PythonToCppFunc is_QFlags_Natron_StandardButton__PythonToCpp_QFlags_Natron_StandardButton__Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_QFLAGS_NATRON_STANDARDBUTTON__IDX]))
        return QFlags_Natron_StandardButton__PythonToCpp_QFlags_Natron_StandardButton_;
    return 0;
}
static PyObject* QFlags_Natron_StandardButton__CppToPython_QFlags_Natron_StandardButton_(const void* cppIn) {
    int castCppIn = *((::QFlags<Natron::StandardButton>*)cppIn);
    return reinterpret_cast<PyObject*>(PySide::QFlags::newObject(castCppIn, SbkNatronEngineTypes[SBK_QFLAGS_NATRON_STANDARDBUTTON__IDX]));

}

static void Natron_StandardButton_PythonToCpp_QFlags_Natron_StandardButton_(PyObject* pyIn, void* cppOut) {
    *((::QFlags<Natron::StandardButton>*)cppOut) = ::QFlags<Natron::StandardButton>(QFlag(Shiboken::Enum::getValue(pyIn)));

}
static PythonToCppFunc is_Natron_StandardButton_PythonToCpp_QFlags_Natron_StandardButton__Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTON_IDX]))
        return Natron_StandardButton_PythonToCpp_QFlags_Natron_StandardButton_;
    return 0;
}
static void number_PythonToCpp_QFlags_Natron_StandardButton_(PyObject* pyIn, void* cppOut) {
    Shiboken::AutoDecRef pyLong(PyNumber_Long(pyIn));
    *((::QFlags<Natron::StandardButton>*)cppOut) = ::QFlags<Natron::StandardButton>(QFlag(PyLong_AsLong(pyLong.object())));

}
static PythonToCppFunc is_number_PythonToCpp_QFlags_Natron_StandardButton__Convertible(PyObject* pyIn) {
    if (PyNumber_Check(pyIn))
        return number_PythonToCpp_QFlags_Natron_StandardButton_;
    return 0;
}
static void Natron_ImagePremultiplication_PythonToCpp_Natron_ImagePremultiplication(PyObject* pyIn, void* cppOut) {
    *((::Natron::ImagePremultiplication*)cppOut) = (::Natron::ImagePremultiplication) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_Natron_ImagePremultiplication_PythonToCpp_Natron_ImagePremultiplication_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_IMAGEPREMULTIPLICATION_IDX]))
        return Natron_ImagePremultiplication_PythonToCpp_Natron_ImagePremultiplication;
    return 0;
}
static PyObject* Natron_ImagePremultiplication_CppToPython_Natron_ImagePremultiplication(const void* cppIn) {
    int castCppIn = *((::Natron::ImagePremultiplication*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_IMAGEPREMULTIPLICATION_IDX], castCppIn);

}

static void Natron_AnimationLevel_PythonToCpp_Natron_AnimationLevel(PyObject* pyIn, void* cppOut) {
    *((::Natron::AnimationLevel*)cppOut) = (::Natron::AnimationLevel) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_Natron_AnimationLevel_PythonToCpp_Natron_AnimationLevel_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_ANIMATIONLEVEL_IDX]))
        return Natron_AnimationLevel_PythonToCpp_Natron_AnimationLevel;
    return 0;
}
static PyObject* Natron_AnimationLevel_CppToPython_Natron_AnimationLevel(const void* cppIn) {
    int castCppIn = *((::Natron::AnimationLevel*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_ANIMATIONLEVEL_IDX], castCppIn);

}

static void Natron_KeyframeType_PythonToCpp_Natron_KeyframeType(PyObject* pyIn, void* cppOut) {
    *((::Natron::KeyframeType*)cppOut) = (::Natron::KeyframeType) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_Natron_KeyframeType_PythonToCpp_Natron_KeyframeType_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_KEYFRAMETYPE_IDX]))
        return Natron_KeyframeType_PythonToCpp_Natron_KeyframeType;
    return 0;
}
static PyObject* Natron_KeyframeType_CppToPython_Natron_KeyframeType(const void* cppIn) {
    int castCppIn = *((::Natron::KeyframeType*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_KEYFRAMETYPE_IDX], castCppIn);

}

static void Natron_ImageComponents_PythonToCpp_Natron_ImageComponents(PyObject* pyIn, void* cppOut) {
    *((::Natron::ImageComponents*)cppOut) = (::Natron::ImageComponents) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_Natron_ImageComponents_PythonToCpp_Natron_ImageComponents_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_IMAGECOMPONENTS_IDX]))
        return Natron_ImageComponents_PythonToCpp_Natron_ImageComponents;
    return 0;
}
static PyObject* Natron_ImageComponents_CppToPython_Natron_ImageComponents(const void* cppIn) {
    int castCppIn = *((::Natron::ImageComponents*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_IMAGECOMPONENTS_IDX], castCppIn);

}

static void Natron_ViewerColorSpace_PythonToCpp_Natron_ViewerColorSpace(PyObject* pyIn, void* cppOut) {
    *((::Natron::ViewerColorSpace*)cppOut) = (::Natron::ViewerColorSpace) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_Natron_ViewerColorSpace_PythonToCpp_Natron_ViewerColorSpace_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_VIEWERCOLORSPACE_IDX]))
        return Natron_ViewerColorSpace_PythonToCpp_Natron_ViewerColorSpace;
    return 0;
}
static PyObject* Natron_ViewerColorSpace_CppToPython_Natron_ViewerColorSpace(const void* cppIn) {
    int castCppIn = *((::Natron::ViewerColorSpace*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_VIEWERCOLORSPACE_IDX], castCppIn);

}

static void Natron_ImageBitDepth_PythonToCpp_Natron_ImageBitDepth(PyObject* pyIn, void* cppOut) {
    *((::Natron::ImageBitDepth*)cppOut) = (::Natron::ImageBitDepth) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_Natron_ImageBitDepth_PythonToCpp_Natron_ImageBitDepth_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_IMAGEBITDEPTH_IDX]))
        return Natron_ImageBitDepth_PythonToCpp_Natron_ImageBitDepth;
    return 0;
}
static PyObject* Natron_ImageBitDepth_CppToPython_Natron_ImageBitDepth(const void* cppIn) {
    int castCppIn = *((::Natron::ImageBitDepth*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_IMAGEBITDEPTH_IDX], castCppIn);

}

static void Natron_ViewerCompositingOperator_PythonToCpp_Natron_ViewerCompositingOperator(PyObject* pyIn, void* cppOut) {
    *((::Natron::ViewerCompositingOperator*)cppOut) = (::Natron::ViewerCompositingOperator) Shiboken::Enum::getValue(pyIn);

}
static PythonToCppFunc is_Natron_ViewerCompositingOperator_PythonToCpp_Natron_ViewerCompositingOperator_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_VIEWERCOMPOSITINGOPERATOR_IDX]))
        return Natron_ViewerCompositingOperator_PythonToCpp_Natron_ViewerCompositingOperator;
    return 0;
}
static PyObject* Natron_ViewerCompositingOperator_CppToPython_Natron_ViewerCompositingOperator(const void* cppIn) {
    int castCppIn = *((::Natron::ViewerCompositingOperator*)cppIn);
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_VIEWERCOMPOSITINGOPERATOR_IDX], castCppIn);

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

void init_Natron(PyObject* module)
{
    SbkNatronEngineTypes[SBK_NATRON_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_Natron_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "Natron", "Natron",
        &Sbk_Natron_Type)) {
        return;
    }


    // Initialization of enums.

    // Initialization of enum 'Orientation'.
    SbkNatronEngineTypes[SBK_NATRON_ORIENTATION_IDX] = Shiboken::Enum::createScopedEnum(&Sbk_Natron_Type,
        "Orientation",
        "NatronEngine.Natron.Orientation",
        "Natron::Orientation");
    if (!SbkNatronEngineTypes[SBK_NATRON_ORIENTATION_IDX])
        return ;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ORIENTATION_IDX],
        &Sbk_Natron_Type, "Horizontal", (long) Natron::Horizontal))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ORIENTATION_IDX],
        &Sbk_Natron_Type, "Vertical", (long) Natron::Vertical))
        return ;
    // Register converter for enum 'Natron::Orientation'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_ORIENTATION_IDX],
            Natron_Orientation_CppToPython_Natron_Orientation);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            Natron_Orientation_PythonToCpp_Natron_Orientation,
            is_Natron_Orientation_PythonToCpp_Natron_Orientation_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_ORIENTATION_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_ORIENTATION_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "Natron::Orientation");
        Shiboken::Conversions::registerConverterName(converter, "Orientation");
    }
    // End of 'Orientation' enum.

    // Initialization of enum 'PlaybackMode'.
    SbkNatronEngineTypes[SBK_NATRON_PLAYBACKMODE_IDX] = Shiboken::Enum::createScopedEnum(&Sbk_Natron_Type,
        "PlaybackMode",
        "NatronEngine.Natron.PlaybackMode",
        "Natron::PlaybackMode");
    if (!SbkNatronEngineTypes[SBK_NATRON_PLAYBACKMODE_IDX])
        return ;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PLAYBACKMODE_IDX],
        &Sbk_Natron_Type, "PLAYBACK_LOOP", (long) Natron::PLAYBACK_LOOP))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PLAYBACKMODE_IDX],
        &Sbk_Natron_Type, "PLAYBACK_BOUNCE", (long) Natron::PLAYBACK_BOUNCE))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_PLAYBACKMODE_IDX],
        &Sbk_Natron_Type, "PLAYBACK_ONCE", (long) Natron::PLAYBACK_ONCE))
        return ;
    // Register converter for enum 'Natron::PlaybackMode'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_PLAYBACKMODE_IDX],
            Natron_PlaybackMode_CppToPython_Natron_PlaybackMode);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            Natron_PlaybackMode_PythonToCpp_Natron_PlaybackMode,
            is_Natron_PlaybackMode_PythonToCpp_Natron_PlaybackMode_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_PLAYBACKMODE_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_PLAYBACKMODE_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "Natron::PlaybackMode");
        Shiboken::Conversions::registerConverterName(converter, "PlaybackMode");
    }
    // End of 'PlaybackMode' enum.

    // Initialization of enum 'ValueChangedReason'.
    SbkNatronEngineTypes[SBK_NATRON_VALUECHANGEDREASON_IDX] = Shiboken::Enum::createScopedEnum(&Sbk_Natron_Type,
        "ValueChangedReason",
        "NatronEngine.Natron.ValueChangedReason",
        "Natron::ValueChangedReason");
    if (!SbkNatronEngineTypes[SBK_NATRON_VALUECHANGEDREASON_IDX])
        return ;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_VALUECHANGEDREASON_IDX],
        &Sbk_Natron_Type, "USER_EDITED", (long) Natron::USER_EDITED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_VALUECHANGEDREASON_IDX],
        &Sbk_Natron_Type, "PLUGIN_EDITED", (long) Natron::PLUGIN_EDITED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_VALUECHANGEDREASON_IDX],
        &Sbk_Natron_Type, "NATRON_EDITED", (long) Natron::NATRON_EDITED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_VALUECHANGEDREASON_IDX],
        &Sbk_Natron_Type, "TIME_CHANGED", (long) Natron::TIME_CHANGED))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_VALUECHANGEDREASON_IDX],
        &Sbk_Natron_Type, "PROJECT_LOADING", (long) Natron::PROJECT_LOADING))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_VALUECHANGEDREASON_IDX],
        &Sbk_Natron_Type, "SLAVE_REFRESH", (long) Natron::SLAVE_REFRESH))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_VALUECHANGEDREASON_IDX],
        &Sbk_Natron_Type, "RESTORE_DEFAULT", (long) Natron::RESTORE_DEFAULT))
        return ;
    // Register converter for enum 'Natron::ValueChangedReason'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_VALUECHANGEDREASON_IDX],
            Natron_ValueChangedReason_CppToPython_Natron_ValueChangedReason);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            Natron_ValueChangedReason_PythonToCpp_Natron_ValueChangedReason,
            is_Natron_ValueChangedReason_PythonToCpp_Natron_ValueChangedReason_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_VALUECHANGEDREASON_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_VALUECHANGEDREASON_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "Natron::ValueChangedReason");
        Shiboken::Conversions::registerConverterName(converter, "ValueChangedReason");
    }
    // End of 'ValueChangedReason' enum.

    // Initialization of enum 'StandardButton'.
    SbkNatronEngineTypes[SBK_QFLAGS_NATRON_STANDARDBUTTON__IDX] = PySide::QFlags::create("StandardButtons", &SbkNatronEngine_Natron_StandardButton_as_number);
    SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTON_IDX] = Shiboken::Enum::createScopedEnum(&Sbk_Natron_Type,
        "StandardButton",
        "NatronEngine.Natron.StandardButton",
        "Natron::StandardButton",
        SbkNatronEngineTypes[SBK_QFLAGS_NATRON_STANDARDBUTTON__IDX]);
    if (!SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTON_IDX])
        return ;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTON_IDX],
        &Sbk_Natron_Type, "NoButton", (long) Natron::NoButton))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTON_IDX],
        &Sbk_Natron_Type, "Escape", (long) Natron::Escape))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTON_IDX],
        &Sbk_Natron_Type, "Ok", (long) Natron::Ok))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTON_IDX],
        &Sbk_Natron_Type, "Save", (long) Natron::Save))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTON_IDX],
        &Sbk_Natron_Type, "SaveAll", (long) Natron::SaveAll))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTON_IDX],
        &Sbk_Natron_Type, "Open", (long) Natron::Open))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTON_IDX],
        &Sbk_Natron_Type, "Yes", (long) Natron::Yes))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTON_IDX],
        &Sbk_Natron_Type, "YesToAll", (long) Natron::YesToAll))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTON_IDX],
        &Sbk_Natron_Type, "No", (long) Natron::No))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTON_IDX],
        &Sbk_Natron_Type, "NoToAll", (long) Natron::NoToAll))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTON_IDX],
        &Sbk_Natron_Type, "Abort", (long) Natron::Abort))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTON_IDX],
        &Sbk_Natron_Type, "Retry", (long) Natron::Retry))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTON_IDX],
        &Sbk_Natron_Type, "Ignore", (long) Natron::Ignore))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTON_IDX],
        &Sbk_Natron_Type, "Close", (long) Natron::Close))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTON_IDX],
        &Sbk_Natron_Type, "Cancel", (long) Natron::Cancel))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTON_IDX],
        &Sbk_Natron_Type, "Discard", (long) Natron::Discard))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTON_IDX],
        &Sbk_Natron_Type, "Help", (long) Natron::Help))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTON_IDX],
        &Sbk_Natron_Type, "Apply", (long) Natron::Apply))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTON_IDX],
        &Sbk_Natron_Type, "Reset", (long) Natron::Reset))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTON_IDX],
        &Sbk_Natron_Type, "RestoreDefaults", (long) Natron::RestoreDefaults))
        return ;
    // Register converter for enum 'Natron::StandardButton'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTON_IDX],
            Natron_StandardButton_CppToPython_Natron_StandardButton);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            Natron_StandardButton_PythonToCpp_Natron_StandardButton,
            is_Natron_StandardButton_PythonToCpp_Natron_StandardButton_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTON_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTON_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "Natron::StandardButton");
        Shiboken::Conversions::registerConverterName(converter, "StandardButton");
    }
    // Register converter for flag 'QFlags<Natron::StandardButton>'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_QFLAGS_NATRON_STANDARDBUTTON__IDX],
            QFlags_Natron_StandardButton__CppToPython_QFlags_Natron_StandardButton_);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            Natron_StandardButton_PythonToCpp_QFlags_Natron_StandardButton_,
            is_Natron_StandardButton_PythonToCpp_QFlags_Natron_StandardButton__Convertible);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            QFlags_Natron_StandardButton__PythonToCpp_QFlags_Natron_StandardButton_,
            is_QFlags_Natron_StandardButton__PythonToCpp_QFlags_Natron_StandardButton__Convertible);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            number_PythonToCpp_QFlags_Natron_StandardButton_,
            is_number_PythonToCpp_QFlags_Natron_StandardButton__Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_QFLAGS_NATRON_STANDARDBUTTON__IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_QFLAGS_NATRON_STANDARDBUTTON__IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "QFlags<QFlags<Natron::StandardButton>");
        Shiboken::Conversions::registerConverterName(converter, "QFlags<StandardButton>");
    }
    // End of 'StandardButton' enum/flags.

    // Initialization of enum 'ImagePremultiplication'.
    SbkNatronEngineTypes[SBK_NATRON_IMAGEPREMULTIPLICATION_IDX] = Shiboken::Enum::createScopedEnum(&Sbk_Natron_Type,
        "ImagePremultiplication",
        "NatronEngine.Natron.ImagePremultiplication",
        "Natron::ImagePremultiplication");
    if (!SbkNatronEngineTypes[SBK_NATRON_IMAGEPREMULTIPLICATION_IDX])
        return ;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_IMAGEPREMULTIPLICATION_IDX],
        &Sbk_Natron_Type, "ImageOpaque", (long) Natron::ImageOpaque))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_IMAGEPREMULTIPLICATION_IDX],
        &Sbk_Natron_Type, "ImagePremultiplied", (long) Natron::ImagePremultiplied))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_IMAGEPREMULTIPLICATION_IDX],
        &Sbk_Natron_Type, "ImageUnPremultiplied", (long) Natron::ImageUnPremultiplied))
        return ;
    // Register converter for enum 'Natron::ImagePremultiplication'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_IMAGEPREMULTIPLICATION_IDX],
            Natron_ImagePremultiplication_CppToPython_Natron_ImagePremultiplication);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            Natron_ImagePremultiplication_PythonToCpp_Natron_ImagePremultiplication,
            is_Natron_ImagePremultiplication_PythonToCpp_Natron_ImagePremultiplication_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_IMAGEPREMULTIPLICATION_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_IMAGEPREMULTIPLICATION_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "Natron::ImagePremultiplication");
        Shiboken::Conversions::registerConverterName(converter, "ImagePremultiplication");
    }
    // End of 'ImagePremultiplication' enum.

    // Initialization of enum 'AnimationLevel'.
    SbkNatronEngineTypes[SBK_NATRON_ANIMATIONLEVEL_IDX] = Shiboken::Enum::createScopedEnum(&Sbk_Natron_Type,
        "AnimationLevel",
        "NatronEngine.Natron.AnimationLevel",
        "Natron::AnimationLevel");
    if (!SbkNatronEngineTypes[SBK_NATRON_ANIMATIONLEVEL_IDX])
        return ;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ANIMATIONLEVEL_IDX],
        &Sbk_Natron_Type, "NO_ANIMATION", (long) Natron::NO_ANIMATION))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ANIMATIONLEVEL_IDX],
        &Sbk_Natron_Type, "INTERPOLATED_VALUE", (long) Natron::INTERPOLATED_VALUE))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ANIMATIONLEVEL_IDX],
        &Sbk_Natron_Type, "ON_KEYFRAME", (long) Natron::ON_KEYFRAME))
        return ;
    // Register converter for enum 'Natron::AnimationLevel'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_ANIMATIONLEVEL_IDX],
            Natron_AnimationLevel_CppToPython_Natron_AnimationLevel);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            Natron_AnimationLevel_PythonToCpp_Natron_AnimationLevel,
            is_Natron_AnimationLevel_PythonToCpp_Natron_AnimationLevel_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_ANIMATIONLEVEL_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_ANIMATIONLEVEL_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "Natron::AnimationLevel");
        Shiboken::Conversions::registerConverterName(converter, "AnimationLevel");
    }
    // End of 'AnimationLevel' enum.

    // Initialization of enum 'KeyframeType'.
    SbkNatronEngineTypes[SBK_NATRON_KEYFRAMETYPE_IDX] = Shiboken::Enum::createScopedEnum(&Sbk_Natron_Type,
        "KeyframeType",
        "NatronEngine.Natron.KeyframeType",
        "Natron::KeyframeType");
    if (!SbkNatronEngineTypes[SBK_NATRON_KEYFRAMETYPE_IDX])
        return ;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_KEYFRAMETYPE_IDX],
        &Sbk_Natron_Type, "KEYFRAME_CONSTANT", (long) Natron::KEYFRAME_CONSTANT))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_KEYFRAMETYPE_IDX],
        &Sbk_Natron_Type, "KEYFRAME_LINEAR", (long) Natron::KEYFRAME_LINEAR))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_KEYFRAMETYPE_IDX],
        &Sbk_Natron_Type, "KEYFRAME_SMOOTH", (long) Natron::KEYFRAME_SMOOTH))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_KEYFRAMETYPE_IDX],
        &Sbk_Natron_Type, "KEYFRAME_CATMULL_ROM", (long) Natron::KEYFRAME_CATMULL_ROM))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_KEYFRAMETYPE_IDX],
        &Sbk_Natron_Type, "KEYFRAME_CUBIC", (long) Natron::KEYFRAME_CUBIC))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_KEYFRAMETYPE_IDX],
        &Sbk_Natron_Type, "KEYFRAME_HORIZONTAL", (long) Natron::KEYFRAME_HORIZONTAL))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_KEYFRAMETYPE_IDX],
        &Sbk_Natron_Type, "KEYFRAME_FREE", (long) Natron::KEYFRAME_FREE))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_KEYFRAMETYPE_IDX],
        &Sbk_Natron_Type, "KEYFRAME_BROKEN", (long) Natron::KEYFRAME_BROKEN))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_KEYFRAMETYPE_IDX],
        &Sbk_Natron_Type, "KEYFRAME_NONE", (long) Natron::KEYFRAME_NONE))
        return ;
    // Register converter for enum 'Natron::KeyframeType'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_KEYFRAMETYPE_IDX],
            Natron_KeyframeType_CppToPython_Natron_KeyframeType);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            Natron_KeyframeType_PythonToCpp_Natron_KeyframeType,
            is_Natron_KeyframeType_PythonToCpp_Natron_KeyframeType_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_KEYFRAMETYPE_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_KEYFRAMETYPE_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "Natron::KeyframeType");
        Shiboken::Conversions::registerConverterName(converter, "KeyframeType");
    }
    // End of 'KeyframeType' enum.

    // Initialization of enum 'ImageComponents'.
    SbkNatronEngineTypes[SBK_NATRON_IMAGECOMPONENTS_IDX] = Shiboken::Enum::createScopedEnum(&Sbk_Natron_Type,
        "ImageComponents",
        "NatronEngine.Natron.ImageComponents",
        "Natron::ImageComponents");
    if (!SbkNatronEngineTypes[SBK_NATRON_IMAGECOMPONENTS_IDX])
        return ;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_IMAGECOMPONENTS_IDX],
        &Sbk_Natron_Type, "ImageComponentNone", (long) Natron::ImageComponentNone))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_IMAGECOMPONENTS_IDX],
        &Sbk_Natron_Type, "ImageComponentAlpha", (long) Natron::ImageComponentAlpha))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_IMAGECOMPONENTS_IDX],
        &Sbk_Natron_Type, "ImageComponentRGB", (long) Natron::ImageComponentRGB))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_IMAGECOMPONENTS_IDX],
        &Sbk_Natron_Type, "ImageComponentRGBA", (long) Natron::ImageComponentRGBA))
        return ;
    // Register converter for enum 'Natron::ImageComponents'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_IMAGECOMPONENTS_IDX],
            Natron_ImageComponents_CppToPython_Natron_ImageComponents);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            Natron_ImageComponents_PythonToCpp_Natron_ImageComponents,
            is_Natron_ImageComponents_PythonToCpp_Natron_ImageComponents_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_IMAGECOMPONENTS_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_IMAGECOMPONENTS_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "Natron::ImageComponents");
        Shiboken::Conversions::registerConverterName(converter, "ImageComponents");
    }
    // End of 'ImageComponents' enum.

    // Initialization of enum 'ViewerColorSpace'.
    SbkNatronEngineTypes[SBK_NATRON_VIEWERCOLORSPACE_IDX] = Shiboken::Enum::createScopedEnum(&Sbk_Natron_Type,
        "ViewerColorSpace",
        "NatronEngine.Natron.ViewerColorSpace",
        "Natron::ViewerColorSpace");
    if (!SbkNatronEngineTypes[SBK_NATRON_VIEWERCOLORSPACE_IDX])
        return ;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_VIEWERCOLORSPACE_IDX],
        &Sbk_Natron_Type, "sRGB", (long) Natron::sRGB))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_VIEWERCOLORSPACE_IDX],
        &Sbk_Natron_Type, "Linear", (long) Natron::Linear))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_VIEWERCOLORSPACE_IDX],
        &Sbk_Natron_Type, "Rec709", (long) Natron::Rec709))
        return ;
    // Register converter for enum 'Natron::ViewerColorSpace'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_VIEWERCOLORSPACE_IDX],
            Natron_ViewerColorSpace_CppToPython_Natron_ViewerColorSpace);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            Natron_ViewerColorSpace_PythonToCpp_Natron_ViewerColorSpace,
            is_Natron_ViewerColorSpace_PythonToCpp_Natron_ViewerColorSpace_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_VIEWERCOLORSPACE_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_VIEWERCOLORSPACE_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "Natron::ViewerColorSpace");
        Shiboken::Conversions::registerConverterName(converter, "ViewerColorSpace");
    }
    // End of 'ViewerColorSpace' enum.

    // Initialization of enum 'ImageBitDepth'.
    SbkNatronEngineTypes[SBK_NATRON_IMAGEBITDEPTH_IDX] = Shiboken::Enum::createScopedEnum(&Sbk_Natron_Type,
        "ImageBitDepth",
        "NatronEngine.Natron.ImageBitDepth",
        "Natron::ImageBitDepth");
    if (!SbkNatronEngineTypes[SBK_NATRON_IMAGEBITDEPTH_IDX])
        return ;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_IMAGEBITDEPTH_IDX],
        &Sbk_Natron_Type, "IMAGE_NONE", (long) Natron::IMAGE_NONE))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_IMAGEBITDEPTH_IDX],
        &Sbk_Natron_Type, "IMAGE_BYTE", (long) Natron::IMAGE_BYTE))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_IMAGEBITDEPTH_IDX],
        &Sbk_Natron_Type, "IMAGE_SHORT", (long) Natron::IMAGE_SHORT))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_IMAGEBITDEPTH_IDX],
        &Sbk_Natron_Type, "IMAGE_FLOAT", (long) Natron::IMAGE_FLOAT))
        return ;
    // Register converter for enum 'Natron::ImageBitDepth'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_IMAGEBITDEPTH_IDX],
            Natron_ImageBitDepth_CppToPython_Natron_ImageBitDepth);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            Natron_ImageBitDepth_PythonToCpp_Natron_ImageBitDepth,
            is_Natron_ImageBitDepth_PythonToCpp_Natron_ImageBitDepth_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_IMAGEBITDEPTH_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_IMAGEBITDEPTH_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "Natron::ImageBitDepth");
        Shiboken::Conversions::registerConverterName(converter, "ImageBitDepth");
    }
    // End of 'ImageBitDepth' enum.

    // Initialization of enum 'ViewerCompositingOperator'.
    SbkNatronEngineTypes[SBK_NATRON_VIEWERCOMPOSITINGOPERATOR_IDX] = Shiboken::Enum::createScopedEnum(&Sbk_Natron_Type,
        "ViewerCompositingOperator",
        "NatronEngine.Natron.ViewerCompositingOperator",
        "Natron::ViewerCompositingOperator");
    if (!SbkNatronEngineTypes[SBK_NATRON_VIEWERCOMPOSITINGOPERATOR_IDX])
        return ;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_VIEWERCOMPOSITINGOPERATOR_IDX],
        &Sbk_Natron_Type, "OPERATOR_NONE", (long) Natron::OPERATOR_NONE))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_VIEWERCOMPOSITINGOPERATOR_IDX],
        &Sbk_Natron_Type, "OPERATOR_OVER", (long) Natron::OPERATOR_OVER))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_VIEWERCOMPOSITINGOPERATOR_IDX],
        &Sbk_Natron_Type, "OPERATOR_MINUS", (long) Natron::OPERATOR_MINUS))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_VIEWERCOMPOSITINGOPERATOR_IDX],
        &Sbk_Natron_Type, "OPERATOR_UNDER", (long) Natron::OPERATOR_UNDER))
        return ;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_VIEWERCOMPOSITINGOPERATOR_IDX],
        &Sbk_Natron_Type, "OPERATOR_WIPE", (long) Natron::OPERATOR_WIPE))
        return ;
    // Register converter for enum 'Natron::ViewerCompositingOperator'.
    {
        SbkConverter* converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_VIEWERCOMPOSITINGOPERATOR_IDX],
            Natron_ViewerCompositingOperator_CppToPython_Natron_ViewerCompositingOperator);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            Natron_ViewerCompositingOperator_PythonToCpp_Natron_ViewerCompositingOperator,
            is_Natron_ViewerCompositingOperator_PythonToCpp_Natron_ViewerCompositingOperator_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_VIEWERCOMPOSITINGOPERATOR_IDX], converter);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_VIEWERCOMPOSITINGOPERATOR_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "Natron::ViewerCompositingOperator");
        Shiboken::Conversions::registerConverterName(converter, "ViewerCompositingOperator");
    }
    // End of 'ViewerCompositingOperator' enum.

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
        &Sbk_Natron_Type, "NATRON_PIXMAP_APP_ICON", (long) Natron::NATRON_PIXMAP_APP_ICON))
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


}
