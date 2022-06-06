
// default includes
#include "Global/Macros.h"
CLANG_DIAG_OFF(mismatched-tags)
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_OFF(missing-field-initializers)
GCC_DIAG_OFF(missing-declarations)
GCC_DIAG_OFF(uninitialized)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <pysidesignal.h>
#include <shiboken.h> // produces many warnings
#ifndef QT_NO_VERSION_TAGGING
#  define QT_NO_VERSION_TAGGING
#endif
#include <QDebug>
#include <pysidesignal.h>
#include <pysideproperty.h>
#include <pyside.h>
#include <pysideqenum.h>
#include <feature_select.h>
QT_WARNING_DISABLE_DEPRECATED

#include <typeinfo>
#include <iterator>

// module include
#include "natronengine_python.h"

// main header
#include "natron_enum_wrapper.h"

// inner classes
NATRON_NAMESPACE_USING NATRON_PYTHON_NAMESPACE_USING

#include <cctype>
#include <cstring>



template <class T>
static const char *typeNameOf(const T &t)
{
    const char *typeName =  typeid(t).name();
    auto size = std::strlen(typeName);
#if defined(Q_CC_MSVC) // MSVC: "class QPaintDevice * __ptr64"
    if (auto lastStar = strchr(typeName, '*')) {
        // MSVC: "class QPaintDevice * __ptr64"
        while (*--lastStar == ' ') {
        }
        size = lastStar - typeName + 1;
    }
#else // g++, Clang: "QPaintDevice *" -> "P12QPaintDevice"
    if (size > 2 && typeName[0] == 'P' && std::isdigit(typeName[1])) {
        ++typeName;
        --size;
    }
#endif
    char *result = new char[size + 1];
    result[size] = '\0';
    memcpy(result, typeName, size);
    return result;
}


// Target ---------------------------------------------------------

extern "C" {

static const char *Sbk_NATRON_ENUM_PropertyStrings[] = {
    nullptr // Sentinel
};

static PyMethodDef Sbk_NATRON_ENUM_methods[] = {

    {nullptr, nullptr} // Sentinel
};

} // extern "C"

static int Sbk_NATRON_ENUM_traverse(PyObject *self, visitproc visit, void *arg)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_traverse(self, visit, arg);
}
static int Sbk_NATRON_ENUM_clear(PyObject *self)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType *_Sbk_NATRON_ENUM_Type = nullptr;
static SbkObjectType *Sbk_NATRON_ENUM_TypeF(void)
{
    return _Sbk_NATRON_ENUM_Type;
}

static PyType_Slot Sbk_NATRON_ENUM_slots[] = {
    {Py_tp_base,        nullptr}, // inserted by introduceWrapperType
    {Py_tp_dealloc,     reinterpret_cast<void *>(Sbk_object_dealloc /* PYSIDE-832: Prevent replacement of "0" with subtype_dealloc. */)},
    {Py_tp_repr,        nullptr},
    {Py_tp_hash,        nullptr},
    {Py_tp_call,        nullptr},
    {Py_tp_str,         nullptr},
    {Py_tp_getattro,    nullptr},
    {Py_tp_setattro,    nullptr},
    {Py_tp_traverse,    reinterpret_cast<void *>(Sbk_NATRON_ENUM_traverse)},
    {Py_tp_clear,       reinterpret_cast<void *>(Sbk_NATRON_ENUM_clear)},
    {Py_tp_richcompare, nullptr},
    {Py_tp_iter,        nullptr},
    {Py_tp_iternext,    nullptr},
    {Py_tp_methods,     reinterpret_cast<void *>(Sbk_NATRON_ENUM_methods)},
    {Py_tp_getset,      nullptr},
    {Py_tp_init,        nullptr},
    {Py_tp_new,         reinterpret_cast<void *>(SbkDummyNew /* PYSIDE-595: Prevent replacement of "0" with base->tp_new. */)},
    {0, nullptr}
};
static PyType_Spec Sbk_NATRON_ENUM_spec = {
    "1:NatronEngine.NATRON_ENUM",
    sizeof(SbkObject),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    Sbk_NATRON_ENUM_slots
};

} //extern "C"

PyObject *SbkNatronEngine_NATRON_ENUM_StandardButtonEnum___and__(PyObject *self, PyObject *pyArg)
{
    ::NATRON_ENUM::StandardButtons cppResult, cppSelf, cppArg;
#ifdef IS_PY3K
    cppSelf = static_cast<::NATRON_ENUM::StandardButtons>(int(PyLong_AsLong(self)));
    cppArg = static_cast<NATRON_ENUM::StandardButtons>(int(PyLong_AsLong(pyArg)));
    if (PyErr_Occurred())
    return nullptr;
#else
    cppSelf = static_cast<::NATRON_ENUM::StandardButtons>(int(PyInt_AsLong(self)));
    cppArg = static_cast<NATRON_ENUM::StandardButtons>(int(PyInt_AsLong(pyArg)));
#endif

    if (PyErr_Occurred())
        return nullptr;
    cppResult = cppSelf & cppArg;
    return Shiboken::Conversions::copyToPython(*PepType_SGTP(SbkNatronEngineTypes[SBK_QFLAGS_NATRON_ENUM_STANDARDBUTTONENUM_IDX])->converter, &cppResult);
}

PyObject *SbkNatronEngine_NATRON_ENUM_StandardButtonEnum___or__(PyObject *self, PyObject *pyArg)
{
    ::NATRON_ENUM::StandardButtons cppResult, cppSelf, cppArg;
#ifdef IS_PY3K
    cppSelf = static_cast<::NATRON_ENUM::StandardButtons>(int(PyLong_AsLong(self)));
    cppArg = static_cast<NATRON_ENUM::StandardButtons>(int(PyLong_AsLong(pyArg)));
    if (PyErr_Occurred())
    return nullptr;
#else
    cppSelf = static_cast<::NATRON_ENUM::StandardButtons>(int(PyInt_AsLong(self)));
    cppArg = static_cast<NATRON_ENUM::StandardButtons>(int(PyInt_AsLong(pyArg)));
#endif

    if (PyErr_Occurred())
        return nullptr;
    cppResult = cppSelf | cppArg;
    return Shiboken::Conversions::copyToPython(*PepType_SGTP(SbkNatronEngineTypes[SBK_QFLAGS_NATRON_ENUM_STANDARDBUTTONENUM_IDX])->converter, &cppResult);
}

PyObject *SbkNatronEngine_NATRON_ENUM_StandardButtonEnum___xor__(PyObject *self, PyObject *pyArg)
{
    ::NATRON_ENUM::StandardButtons cppResult, cppSelf, cppArg;
#ifdef IS_PY3K
    cppSelf = static_cast<::NATRON_ENUM::StandardButtons>(int(PyLong_AsLong(self)));
    cppArg = static_cast<NATRON_ENUM::StandardButtons>(int(PyLong_AsLong(pyArg)));
    if (PyErr_Occurred())
    return nullptr;
#else
    cppSelf = static_cast<::NATRON_ENUM::StandardButtons>(int(PyInt_AsLong(self)));
    cppArg = static_cast<NATRON_ENUM::StandardButtons>(int(PyInt_AsLong(pyArg)));
#endif

    if (PyErr_Occurred())
        return nullptr;
    cppResult = cppSelf ^ cppArg;
    return Shiboken::Conversions::copyToPython(*PepType_SGTP(SbkNatronEngineTypes[SBK_QFLAGS_NATRON_ENUM_STANDARDBUTTONENUM_IDX])->converter, &cppResult);
}

PyObject *SbkNatronEngine_NATRON_ENUM_StandardButtonEnum___invert__(PyObject *self, PyObject *pyArg)
{
    ::NATRON_ENUM::StandardButtons cppSelf;
    Shiboken::Conversions::pythonToCppCopy(*PepType_SGTP(SbkNatronEngineTypes[SBK_QFLAGS_NATRON_ENUM_STANDARDBUTTONENUM_IDX])->converter, self, &cppSelf);
    ::NATRON_ENUM::StandardButtons cppResult = ~cppSelf;
    return Shiboken::Conversions::copyToPython(*PepType_SGTP(SbkNatronEngineTypes[SBK_QFLAGS_NATRON_ENUM_STANDARDBUTTONENUM_IDX])->converter, &cppResult);
}

static PyObject *SbkNatronEngine_NATRON_ENUM_StandardButtonEnum_long(PyObject *self)
{
    int val;
    Shiboken::Conversions::pythonToCppCopy(*PepType_SGTP(SbkNatronEngineTypes[SBK_QFLAGS_NATRON_ENUM_STANDARDBUTTONENUM_IDX])->converter, self, &val);
    return Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &val);
}
static int SbkNatronEngine_NATRON_ENUM_StandardButtonEnum__nonzero(PyObject *self)
{
    int val;
    Shiboken::Conversions::pythonToCppCopy(*PepType_SGTP(SbkNatronEngineTypes[SBK_QFLAGS_NATRON_ENUM_STANDARDBUTTONENUM_IDX])->converter, self, &val);
    return val != 0;
}

static PyType_Slot SbkNatronEngine_NATRON_ENUM_StandardButtonEnum_number_slots[] = {
#ifdef IS_PY3K
    {Py_nb_bool,    reinterpret_cast<void *>(SbkNatronEngine_NATRON_ENUM_StandardButtonEnum__nonzero)},
#else
    {Py_nb_nonzero, reinterpret_cast<void *>(SbkNatronEngine_NATRON_ENUM_StandardButtonEnum__nonzero)},
    {Py_nb_long,    reinterpret_cast<void *>(SbkNatronEngine_NATRON_ENUM_StandardButtonEnum_long)},
#endif
    {Py_nb_invert,  reinterpret_cast<void *>(SbkNatronEngine_NATRON_ENUM_StandardButtonEnum___invert__)},
    {Py_nb_and,     reinterpret_cast<void *>(SbkNatronEngine_NATRON_ENUM_StandardButtonEnum___and__)},
    {Py_nb_xor,     reinterpret_cast<void *>(SbkNatronEngine_NATRON_ENUM_StandardButtonEnum___xor__)},
    {Py_nb_or,      reinterpret_cast<void *>(SbkNatronEngine_NATRON_ENUM_StandardButtonEnum___or__)},
    {Py_nb_int,     reinterpret_cast<void *>(SbkNatronEngine_NATRON_ENUM_StandardButtonEnum_long)},
    {Py_nb_index,   reinterpret_cast<void *>(SbkNatronEngine_NATRON_ENUM_StandardButtonEnum_long)},
#ifndef IS_PY3K
    {Py_nb_long,    reinterpret_cast<void *>(SbkNatronEngine_NATRON_ENUM_StandardButtonEnum_long)},
#endif
    {0, nullptr} // sentinel
};



// Type conversion functions.

// Python to C++ enum conversion.
static void NATRON_ENUM_StatusEnum_PythonToCpp_NATRON_ENUM_StatusEnum(PyObject *pyIn, void *cppOut) {
    *reinterpret_cast<::NATRON_ENUM::StatusEnum *>(cppOut) =
        static_cast<::NATRON_ENUM::StatusEnum>(Shiboken::Enum::getValue(pyIn));

}
static PythonToCppFunc is_NATRON_ENUM_StatusEnum_PythonToCpp_NATRON_ENUM_StatusEnum_Convertible(PyObject *pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_ENUM_STATUSENUM_IDX]))
        return NATRON_ENUM_StatusEnum_PythonToCpp_NATRON_ENUM_StatusEnum;
    return {};
}
static PyObject *NATRON_ENUM_StatusEnum_CppToPython_NATRON_ENUM_StatusEnum(const void *cppIn) {
    const int castCppIn = int(*reinterpret_cast<const ::NATRON_ENUM::StatusEnum *>(cppIn));
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_STATUSENUM_IDX], castCppIn);

}

static void NATRON_ENUM_StandardButtonEnum_PythonToCpp_NATRON_ENUM_StandardButtonEnum(PyObject *pyIn, void *cppOut) {
    *reinterpret_cast<::NATRON_ENUM::StandardButtonEnum *>(cppOut) =
        static_cast<::NATRON_ENUM::StandardButtonEnum>(Shiboken::Enum::getValue(pyIn));

}
static PythonToCppFunc is_NATRON_ENUM_StandardButtonEnum_PythonToCpp_NATRON_ENUM_StandardButtonEnum_Convertible(PyObject *pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_ENUM_STANDARDBUTTONENUM_IDX]))
        return NATRON_ENUM_StandardButtonEnum_PythonToCpp_NATRON_ENUM_StandardButtonEnum;
    return {};
}
static PyObject *NATRON_ENUM_StandardButtonEnum_CppToPython_NATRON_ENUM_StandardButtonEnum(const void *cppIn) {
    const int castCppIn = int(*reinterpret_cast<const ::NATRON_ENUM::StandardButtonEnum *>(cppIn));
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_STANDARDBUTTONENUM_IDX], castCppIn);

}

static void QFlags_NATRON_ENUM_StandardButtonEnum__PythonToCpp_QFlags_NATRON_ENUM_StandardButtonEnum_(PyObject *pyIn, void *cppOut) {
    *reinterpret_cast<::QFlags<NATRON_ENUM::StandardButtonEnum> *>(cppOut) =
        ::QFlags<NATRON_ENUM::StandardButtonEnum>(QFlag(int(PySide::QFlags::getValue(reinterpret_cast<PySideQFlagsObject *>(pyIn)))));

}
static PythonToCppFunc is_QFlags_NATRON_ENUM_StandardButtonEnum__PythonToCpp_QFlags_NATRON_ENUM_StandardButtonEnum__Convertible(PyObject *pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_QFLAGS_NATRON_ENUM_STANDARDBUTTONENUM_IDX]))
        return QFlags_NATRON_ENUM_StandardButtonEnum__PythonToCpp_QFlags_NATRON_ENUM_StandardButtonEnum_;
    return {};
}
static PyObject *QFlags_NATRON_ENUM_StandardButtonEnum__CppToPython_QFlags_NATRON_ENUM_StandardButtonEnum_(const void *cppIn) {
    const int castCppIn = int(*reinterpret_cast<const ::QFlags<NATRON_ENUM::StandardButtonEnum> *>(cppIn));
    return reinterpret_cast<PyObject *>(PySide::QFlags::newObject(castCppIn, SbkNatronEngineTypes[SBK_QFLAGS_NATRON_ENUM_STANDARDBUTTONENUM_IDX]));

}

static void NATRON_ENUM_StandardButtonEnum_PythonToCpp_QFlags_NATRON_ENUM_StandardButtonEnum_(PyObject *pyIn, void *cppOut) {
    *reinterpret_cast<::QFlags<NATRON_ENUM::StandardButtonEnum> *>(cppOut) =
        ::QFlags<NATRON_ENUM::StandardButtonEnum>(QFlag(int(Shiboken::Enum::getValue(pyIn))));

}
static PythonToCppFunc is_NATRON_ENUM_StandardButtonEnum_PythonToCpp_QFlags_NATRON_ENUM_StandardButtonEnum__Convertible(PyObject *pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_ENUM_STANDARDBUTTONENUM_IDX]))
        return NATRON_ENUM_StandardButtonEnum_PythonToCpp_QFlags_NATRON_ENUM_StandardButtonEnum_;
    return {};
}
static void number_PythonToCpp_QFlags_NATRON_ENUM_StandardButtonEnum_(PyObject *pyIn, void *cppOut) {
    Shiboken::AutoDecRef pyLong(PyNumber_Long(pyIn));
    *reinterpret_cast<::QFlags<NATRON_ENUM::StandardButtonEnum> *>(cppOut) =
        ::QFlags<NATRON_ENUM::StandardButtonEnum>(QFlag(int(PyLong_AsLong(pyLong.object()))));

}
static PythonToCppFunc is_number_PythonToCpp_QFlags_NATRON_ENUM_StandardButtonEnum__Convertible(PyObject *pyIn) {
    if (PyNumber_Check(pyIn) && PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_ENUM_STANDARDBUTTONENUM_IDX]))
        return number_PythonToCpp_QFlags_NATRON_ENUM_StandardButtonEnum_;
    return {};
}
static void NATRON_ENUM_KeyframeTypeEnum_PythonToCpp_NATRON_ENUM_KeyframeTypeEnum(PyObject *pyIn, void *cppOut) {
    *reinterpret_cast<::NATRON_ENUM::KeyframeTypeEnum *>(cppOut) =
        static_cast<::NATRON_ENUM::KeyframeTypeEnum>(Shiboken::Enum::getValue(pyIn));

}
static PythonToCppFunc is_NATRON_ENUM_KeyframeTypeEnum_PythonToCpp_NATRON_ENUM_KeyframeTypeEnum_Convertible(PyObject *pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_ENUM_KEYFRAMETYPEENUM_IDX]))
        return NATRON_ENUM_KeyframeTypeEnum_PythonToCpp_NATRON_ENUM_KeyframeTypeEnum;
    return {};
}
static PyObject *NATRON_ENUM_KeyframeTypeEnum_CppToPython_NATRON_ENUM_KeyframeTypeEnum(const void *cppIn) {
    const int castCppIn = int(*reinterpret_cast<const ::NATRON_ENUM::KeyframeTypeEnum *>(cppIn));
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_KEYFRAMETYPEENUM_IDX], castCppIn);

}

static void NATRON_ENUM_PixmapEnum_PythonToCpp_NATRON_ENUM_PixmapEnum(PyObject *pyIn, void *cppOut) {
    *reinterpret_cast<::NATRON_ENUM::PixmapEnum *>(cppOut) =
        static_cast<::NATRON_ENUM::PixmapEnum>(Shiboken::Enum::getValue(pyIn));

}
static PythonToCppFunc is_NATRON_ENUM_PixmapEnum_PythonToCpp_NATRON_ENUM_PixmapEnum_Convertible(PyObject *pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX]))
        return NATRON_ENUM_PixmapEnum_PythonToCpp_NATRON_ENUM_PixmapEnum;
    return {};
}
static PyObject *NATRON_ENUM_PixmapEnum_CppToPython_NATRON_ENUM_PixmapEnum(const void *cppIn) {
    const int castCppIn = int(*reinterpret_cast<const ::NATRON_ENUM::PixmapEnum *>(cppIn));
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX], castCppIn);

}

static void NATRON_ENUM_ValueChangedReasonEnum_PythonToCpp_NATRON_ENUM_ValueChangedReasonEnum(PyObject *pyIn, void *cppOut) {
    *reinterpret_cast<::NATRON_ENUM::ValueChangedReasonEnum *>(cppOut) =
        static_cast<::NATRON_ENUM::ValueChangedReasonEnum>(Shiboken::Enum::getValue(pyIn));

}
static PythonToCppFunc is_NATRON_ENUM_ValueChangedReasonEnum_PythonToCpp_NATRON_ENUM_ValueChangedReasonEnum_Convertible(PyObject *pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_ENUM_VALUECHANGEDREASONENUM_IDX]))
        return NATRON_ENUM_ValueChangedReasonEnum_PythonToCpp_NATRON_ENUM_ValueChangedReasonEnum;
    return {};
}
static PyObject *NATRON_ENUM_ValueChangedReasonEnum_CppToPython_NATRON_ENUM_ValueChangedReasonEnum(const void *cppIn) {
    const int castCppIn = int(*reinterpret_cast<const ::NATRON_ENUM::ValueChangedReasonEnum *>(cppIn));
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_VALUECHANGEDREASONENUM_IDX], castCppIn);

}

static void NATRON_ENUM_AnimationLevelEnum_PythonToCpp_NATRON_ENUM_AnimationLevelEnum(PyObject *pyIn, void *cppOut) {
    *reinterpret_cast<::NATRON_ENUM::AnimationLevelEnum *>(cppOut) =
        static_cast<::NATRON_ENUM::AnimationLevelEnum>(Shiboken::Enum::getValue(pyIn));

}
static PythonToCppFunc is_NATRON_ENUM_AnimationLevelEnum_PythonToCpp_NATRON_ENUM_AnimationLevelEnum_Convertible(PyObject *pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_ENUM_ANIMATIONLEVELENUM_IDX]))
        return NATRON_ENUM_AnimationLevelEnum_PythonToCpp_NATRON_ENUM_AnimationLevelEnum;
    return {};
}
static PyObject *NATRON_ENUM_AnimationLevelEnum_CppToPython_NATRON_ENUM_AnimationLevelEnum(const void *cppIn) {
    const int castCppIn = int(*reinterpret_cast<const ::NATRON_ENUM::AnimationLevelEnum *>(cppIn));
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_ANIMATIONLEVELENUM_IDX], castCppIn);

}

static void NATRON_ENUM_ImagePremultiplicationEnum_PythonToCpp_NATRON_ENUM_ImagePremultiplicationEnum(PyObject *pyIn, void *cppOut) {
    *reinterpret_cast<::NATRON_ENUM::ImagePremultiplicationEnum *>(cppOut) =
        static_cast<::NATRON_ENUM::ImagePremultiplicationEnum>(Shiboken::Enum::getValue(pyIn));

}
static PythonToCppFunc is_NATRON_ENUM_ImagePremultiplicationEnum_PythonToCpp_NATRON_ENUM_ImagePremultiplicationEnum_Convertible(PyObject *pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_ENUM_IMAGEPREMULTIPLICATIONENUM_IDX]))
        return NATRON_ENUM_ImagePremultiplicationEnum_PythonToCpp_NATRON_ENUM_ImagePremultiplicationEnum;
    return {};
}
static PyObject *NATRON_ENUM_ImagePremultiplicationEnum_CppToPython_NATRON_ENUM_ImagePremultiplicationEnum(const void *cppIn) {
    const int castCppIn = int(*reinterpret_cast<const ::NATRON_ENUM::ImagePremultiplicationEnum *>(cppIn));
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_IMAGEPREMULTIPLICATIONENUM_IDX], castCppIn);

}

static void NATRON_ENUM_ViewerCompositingOperatorEnum_PythonToCpp_NATRON_ENUM_ViewerCompositingOperatorEnum(PyObject *pyIn, void *cppOut) {
    *reinterpret_cast<::NATRON_ENUM::ViewerCompositingOperatorEnum *>(cppOut) =
        static_cast<::NATRON_ENUM::ViewerCompositingOperatorEnum>(Shiboken::Enum::getValue(pyIn));

}
static PythonToCppFunc is_NATRON_ENUM_ViewerCompositingOperatorEnum_PythonToCpp_NATRON_ENUM_ViewerCompositingOperatorEnum_Convertible(PyObject *pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_ENUM_VIEWERCOMPOSITINGOPERATORENUM_IDX]))
        return NATRON_ENUM_ViewerCompositingOperatorEnum_PythonToCpp_NATRON_ENUM_ViewerCompositingOperatorEnum;
    return {};
}
static PyObject *NATRON_ENUM_ViewerCompositingOperatorEnum_CppToPython_NATRON_ENUM_ViewerCompositingOperatorEnum(const void *cppIn) {
    const int castCppIn = int(*reinterpret_cast<const ::NATRON_ENUM::ViewerCompositingOperatorEnum *>(cppIn));
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_VIEWERCOMPOSITINGOPERATORENUM_IDX], castCppIn);

}

static void NATRON_ENUM_ViewerColorSpaceEnum_PythonToCpp_NATRON_ENUM_ViewerColorSpaceEnum(PyObject *pyIn, void *cppOut) {
    *reinterpret_cast<::NATRON_ENUM::ViewerColorSpaceEnum *>(cppOut) =
        static_cast<::NATRON_ENUM::ViewerColorSpaceEnum>(Shiboken::Enum::getValue(pyIn));

}
static PythonToCppFunc is_NATRON_ENUM_ViewerColorSpaceEnum_PythonToCpp_NATRON_ENUM_ViewerColorSpaceEnum_Convertible(PyObject *pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_ENUM_VIEWERCOLORSPACEENUM_IDX]))
        return NATRON_ENUM_ViewerColorSpaceEnum_PythonToCpp_NATRON_ENUM_ViewerColorSpaceEnum;
    return {};
}
static PyObject *NATRON_ENUM_ViewerColorSpaceEnum_CppToPython_NATRON_ENUM_ViewerColorSpaceEnum(const void *cppIn) {
    const int castCppIn = int(*reinterpret_cast<const ::NATRON_ENUM::ViewerColorSpaceEnum *>(cppIn));
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_VIEWERCOLORSPACEENUM_IDX], castCppIn);

}

static void NATRON_ENUM_ImageBitDepthEnum_PythonToCpp_NATRON_ENUM_ImageBitDepthEnum(PyObject *pyIn, void *cppOut) {
    *reinterpret_cast<::NATRON_ENUM::ImageBitDepthEnum *>(cppOut) =
        static_cast<::NATRON_ENUM::ImageBitDepthEnum>(Shiboken::Enum::getValue(pyIn));

}
static PythonToCppFunc is_NATRON_ENUM_ImageBitDepthEnum_PythonToCpp_NATRON_ENUM_ImageBitDepthEnum_Convertible(PyObject *pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_ENUM_IMAGEBITDEPTHENUM_IDX]))
        return NATRON_ENUM_ImageBitDepthEnum_PythonToCpp_NATRON_ENUM_ImageBitDepthEnum;
    return {};
}
static PyObject *NATRON_ENUM_ImageBitDepthEnum_CppToPython_NATRON_ENUM_ImageBitDepthEnum(const void *cppIn) {
    const int castCppIn = int(*reinterpret_cast<const ::NATRON_ENUM::ImageBitDepthEnum *>(cppIn));
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_IMAGEBITDEPTHENUM_IDX], castCppIn);

}

static void NATRON_ENUM_OrientationEnum_PythonToCpp_NATRON_ENUM_OrientationEnum(PyObject *pyIn, void *cppOut) {
    *reinterpret_cast<::NATRON_ENUM::OrientationEnum *>(cppOut) =
        static_cast<::NATRON_ENUM::OrientationEnum>(Shiboken::Enum::getValue(pyIn));

}
static PythonToCppFunc is_NATRON_ENUM_OrientationEnum_PythonToCpp_NATRON_ENUM_OrientationEnum_Convertible(PyObject *pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_ENUM_ORIENTATIONENUM_IDX]))
        return NATRON_ENUM_OrientationEnum_PythonToCpp_NATRON_ENUM_OrientationEnum;
    return {};
}
static PyObject *NATRON_ENUM_OrientationEnum_CppToPython_NATRON_ENUM_OrientationEnum(const void *cppIn) {
    const int castCppIn = int(*reinterpret_cast<const ::NATRON_ENUM::OrientationEnum *>(cppIn));
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_ORIENTATIONENUM_IDX], castCppIn);

}

static void NATRON_ENUM_PlaybackModeEnum_PythonToCpp_NATRON_ENUM_PlaybackModeEnum(PyObject *pyIn, void *cppOut) {
    *reinterpret_cast<::NATRON_ENUM::PlaybackModeEnum *>(cppOut) =
        static_cast<::NATRON_ENUM::PlaybackModeEnum>(Shiboken::Enum::getValue(pyIn));

}
static PythonToCppFunc is_NATRON_ENUM_PlaybackModeEnum_PythonToCpp_NATRON_ENUM_PlaybackModeEnum_Convertible(PyObject *pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_ENUM_PLAYBACKMODEENUM_IDX]))
        return NATRON_ENUM_PlaybackModeEnum_PythonToCpp_NATRON_ENUM_PlaybackModeEnum;
    return {};
}
static PyObject *NATRON_ENUM_PlaybackModeEnum_CppToPython_NATRON_ENUM_PlaybackModeEnum(const void *cppIn) {
    const int castCppIn = int(*reinterpret_cast<const ::NATRON_ENUM::PlaybackModeEnum *>(cppIn));
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PLAYBACKMODEENUM_IDX], castCppIn);

}

static void NATRON_ENUM_DisplayChannelsEnum_PythonToCpp_NATRON_ENUM_DisplayChannelsEnum(PyObject *pyIn, void *cppOut) {
    *reinterpret_cast<::NATRON_ENUM::DisplayChannelsEnum *>(cppOut) =
        static_cast<::NATRON_ENUM::DisplayChannelsEnum>(Shiboken::Enum::getValue(pyIn));

}
static PythonToCppFunc is_NATRON_ENUM_DisplayChannelsEnum_PythonToCpp_NATRON_ENUM_DisplayChannelsEnum_Convertible(PyObject *pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_ENUM_DISPLAYCHANNELSENUM_IDX]))
        return NATRON_ENUM_DisplayChannelsEnum_PythonToCpp_NATRON_ENUM_DisplayChannelsEnum;
    return {};
}
static PyObject *NATRON_ENUM_DisplayChannelsEnum_CppToPython_NATRON_ENUM_DisplayChannelsEnum(const void *cppIn) {
    const int castCppIn = int(*reinterpret_cast<const ::NATRON_ENUM::DisplayChannelsEnum *>(cppIn));
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_DISPLAYCHANNELSENUM_IDX], castCppIn);

}

static void NATRON_ENUM_MergingFunctionEnum_PythonToCpp_NATRON_ENUM_MergingFunctionEnum(PyObject *pyIn, void *cppOut) {
    *reinterpret_cast<::NATRON_ENUM::MergingFunctionEnum *>(cppOut) =
        static_cast<::NATRON_ENUM::MergingFunctionEnum>(Shiboken::Enum::getValue(pyIn));

}
static PythonToCppFunc is_NATRON_ENUM_MergingFunctionEnum_PythonToCpp_NATRON_ENUM_MergingFunctionEnum_Convertible(PyObject *pyIn) {
    if (PyObject_TypeCheck(pyIn, SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX]))
        return NATRON_ENUM_MergingFunctionEnum_PythonToCpp_NATRON_ENUM_MergingFunctionEnum;
    return {};
}
static PyObject *NATRON_ENUM_MergingFunctionEnum_CppToPython_NATRON_ENUM_MergingFunctionEnum(const void *cppIn) {
    const int castCppIn = int(*reinterpret_cast<const ::NATRON_ENUM::MergingFunctionEnum *>(cppIn));
    return Shiboken::Enum::newItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX], castCppIn);

}

// The signatures string for the functions.
// Multiple signatures have their index "n:" in front.
static const char *NatronEngineNATRON_ENUM_SignatureStrings[] = {
    nullptr}; // Sentinel

void init_NatronEngineNATRON_ENUM(PyObject *module)
{
    _Sbk_NATRON_ENUM_Type = Shiboken::ObjectType::introduceWrapperType(
        module,
        "NATRON_ENUM",
        "NATRON_ENUM",
        &Sbk_NATRON_ENUM_spec,
        0,
        0,
        0,
        0    );
    
    auto pyType = reinterpret_cast<PyTypeObject *>(_Sbk_NATRON_ENUM_Type);
    InitSignatureStrings(pyType, NatronEngineNATRON_ENUM_SignatureStrings);
    SbkObjectType_SetPropertyStrings(reinterpret_cast<PyTypeObject *>(_Sbk_NATRON_ENUM_Type), Sbk_NATRON_ENUM_PropertyStrings);
    SbkNatronEngineTypes[SBK_NatronEngineNATRON_ENUM_IDX]
        = reinterpret_cast<PyTypeObject *>(Sbk_NATRON_ENUM_TypeF());


    // Initialization of enums.

    // Initialization of enum 'StatusEnum'.
    SbkNatronEngineTypes[SBK_NATRON_ENUM_STATUSENUM_IDX] = Shiboken::Enum::createScopedEnum(Sbk_NATRON_ENUM_TypeF(),
        "StatusEnum",
        "1:NatronEngine.NATRON_ENUM.StatusEnum",
        "NATRON_ENUM::StatusEnum");
    if (!SbkNatronEngineTypes[SBK_NATRON_ENUM_STATUSENUM_IDX])
        return;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_STATUSENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eStatusOK", (long) NATRON_ENUM::StatusEnum::eStatusOK))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_STATUSENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eStatusFailed", (long) NATRON_ENUM::StatusEnum::eStatusFailed))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_STATUSENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eStatusOutOfMemory", (long) NATRON_ENUM::StatusEnum::eStatusOutOfMemory))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_STATUSENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eStatusReplyDefault", (long) NATRON_ENUM::StatusEnum::eStatusReplyDefault))
        return;
    // Register converter for enum 'NATRON_ENUM::StatusEnum'.
    {
        SbkConverter *converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_ENUM_STATUSENUM_IDX],
            NATRON_ENUM_StatusEnum_CppToPython_NATRON_ENUM_StatusEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            NATRON_ENUM_StatusEnum_PythonToCpp_NATRON_ENUM_StatusEnum,
            is_NATRON_ENUM_StatusEnum_PythonToCpp_NATRON_ENUM_StatusEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_ENUM_STATUSENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "NATRON_ENUM::StatusEnum");
        Shiboken::Conversions::registerConverterName(converter, "StatusEnum");
    }
    // End of 'StatusEnum' enum.

    // Initialization of enum 'StandardButtonEnum'.
    SbkNatronEngineTypes[SBK_QFLAGS_NATRON_ENUM_STANDARDBUTTONENUM_IDX] = PySide::QFlags::create("1:NatronEngine.NATRON_ENUM.StandardButtons", SbkNatronEngine_NATRON_ENUM_StandardButtonEnum_number_slots);
    SbkNatronEngineTypes[SBK_NATRON_ENUM_STANDARDBUTTONENUM_IDX] = Shiboken::Enum::createScopedEnum(Sbk_NATRON_ENUM_TypeF(),
        "StandardButtonEnum",
        "1:NatronEngine.NATRON_ENUM.StandardButtonEnum",
        "NATRON_ENUM::StandardButtonEnum",
        SbkNatronEngineTypes[SBK_QFLAGS_NATRON_ENUM_STANDARDBUTTONENUM_IDX]);
    if (!SbkNatronEngineTypes[SBK_NATRON_ENUM_STANDARDBUTTONENUM_IDX])
        return;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_STANDARDBUTTONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eStandardButtonNoButton", (long) NATRON_ENUM::StandardButtonEnum::eStandardButtonNoButton))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_STANDARDBUTTONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eStandardButtonEscape", (long) NATRON_ENUM::StandardButtonEnum::eStandardButtonEscape))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_STANDARDBUTTONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eStandardButtonOk", (long) NATRON_ENUM::StandardButtonEnum::eStandardButtonOk))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_STANDARDBUTTONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eStandardButtonSave", (long) NATRON_ENUM::StandardButtonEnum::eStandardButtonSave))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_STANDARDBUTTONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eStandardButtonSaveAll", (long) NATRON_ENUM::StandardButtonEnum::eStandardButtonSaveAll))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_STANDARDBUTTONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eStandardButtonOpen", (long) NATRON_ENUM::StandardButtonEnum::eStandardButtonOpen))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_STANDARDBUTTONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eStandardButtonYes", (long) NATRON_ENUM::StandardButtonEnum::eStandardButtonYes))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_STANDARDBUTTONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eStandardButtonYesToAll", (long) NATRON_ENUM::StandardButtonEnum::eStandardButtonYesToAll))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_STANDARDBUTTONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eStandardButtonNo", (long) NATRON_ENUM::StandardButtonEnum::eStandardButtonNo))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_STANDARDBUTTONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eStandardButtonNoToAll", (long) NATRON_ENUM::StandardButtonEnum::eStandardButtonNoToAll))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_STANDARDBUTTONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eStandardButtonAbort", (long) NATRON_ENUM::StandardButtonEnum::eStandardButtonAbort))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_STANDARDBUTTONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eStandardButtonRetry", (long) NATRON_ENUM::StandardButtonEnum::eStandardButtonRetry))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_STANDARDBUTTONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eStandardButtonIgnore", (long) NATRON_ENUM::StandardButtonEnum::eStandardButtonIgnore))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_STANDARDBUTTONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eStandardButtonClose", (long) NATRON_ENUM::StandardButtonEnum::eStandardButtonClose))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_STANDARDBUTTONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eStandardButtonCancel", (long) NATRON_ENUM::StandardButtonEnum::eStandardButtonCancel))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_STANDARDBUTTONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eStandardButtonDiscard", (long) NATRON_ENUM::StandardButtonEnum::eStandardButtonDiscard))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_STANDARDBUTTONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eStandardButtonHelp", (long) NATRON_ENUM::StandardButtonEnum::eStandardButtonHelp))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_STANDARDBUTTONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eStandardButtonApply", (long) NATRON_ENUM::StandardButtonEnum::eStandardButtonApply))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_STANDARDBUTTONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eStandardButtonReset", (long) NATRON_ENUM::StandardButtonEnum::eStandardButtonReset))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_STANDARDBUTTONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eStandardButtonRestoreDefaults", (long) NATRON_ENUM::StandardButtonEnum::eStandardButtonRestoreDefaults))
        return;
    // Register converter for enum 'NATRON_ENUM::StandardButtonEnum'.
    {
        SbkConverter *converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_ENUM_STANDARDBUTTONENUM_IDX],
            NATRON_ENUM_StandardButtonEnum_CppToPython_NATRON_ENUM_StandardButtonEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            NATRON_ENUM_StandardButtonEnum_PythonToCpp_NATRON_ENUM_StandardButtonEnum,
            is_NATRON_ENUM_StandardButtonEnum_PythonToCpp_NATRON_ENUM_StandardButtonEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_ENUM_STANDARDBUTTONENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "NATRON_ENUM::StandardButtonEnum");
        Shiboken::Conversions::registerConverterName(converter, "StandardButtonEnum");
    }
    // Register converter for flag 'QFlags<NATRON_ENUM::StandardButtonEnum>'.
    {
        SbkConverter *converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_QFLAGS_NATRON_ENUM_STANDARDBUTTONENUM_IDX],
            QFlags_NATRON_ENUM_StandardButtonEnum__CppToPython_QFlags_NATRON_ENUM_StandardButtonEnum_);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            NATRON_ENUM_StandardButtonEnum_PythonToCpp_QFlags_NATRON_ENUM_StandardButtonEnum_,
            is_NATRON_ENUM_StandardButtonEnum_PythonToCpp_QFlags_NATRON_ENUM_StandardButtonEnum__Convertible);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            QFlags_NATRON_ENUM_StandardButtonEnum__PythonToCpp_QFlags_NATRON_ENUM_StandardButtonEnum_,
            is_QFlags_NATRON_ENUM_StandardButtonEnum__PythonToCpp_QFlags_NATRON_ENUM_StandardButtonEnum__Convertible);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            number_PythonToCpp_QFlags_NATRON_ENUM_StandardButtonEnum_,
            is_number_PythonToCpp_QFlags_NATRON_ENUM_StandardButtonEnum__Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_QFLAGS_NATRON_ENUM_STANDARDBUTTONENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "NATRON_ENUM::StandardButtons");
        Shiboken::Conversions::registerConverterName(converter, "StandardButtons");
    }
    // End of 'StandardButtonEnum' enum/flags.

    // Initialization of enum 'KeyframeTypeEnum'.
    SbkNatronEngineTypes[SBK_NATRON_ENUM_KEYFRAMETYPEENUM_IDX] = Shiboken::Enum::createScopedEnum(Sbk_NATRON_ENUM_TypeF(),
        "KeyframeTypeEnum",
        "1:NatronEngine.NATRON_ENUM.KeyframeTypeEnum",
        "NATRON_ENUM::KeyframeTypeEnum");
    if (!SbkNatronEngineTypes[SBK_NATRON_ENUM_KEYFRAMETYPEENUM_IDX])
        return;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_KEYFRAMETYPEENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eKeyframeTypeConstant", (long) NATRON_ENUM::KeyframeTypeEnum::eKeyframeTypeConstant))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_KEYFRAMETYPEENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eKeyframeTypeLinear", (long) NATRON_ENUM::KeyframeTypeEnum::eKeyframeTypeLinear))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_KEYFRAMETYPEENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eKeyframeTypeSmooth", (long) NATRON_ENUM::KeyframeTypeEnum::eKeyframeTypeSmooth))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_KEYFRAMETYPEENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eKeyframeTypeCatmullRom", (long) NATRON_ENUM::KeyframeTypeEnum::eKeyframeTypeCatmullRom))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_KEYFRAMETYPEENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eKeyframeTypeCubic", (long) NATRON_ENUM::KeyframeTypeEnum::eKeyframeTypeCubic))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_KEYFRAMETYPEENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eKeyframeTypeHorizontal", (long) NATRON_ENUM::KeyframeTypeEnum::eKeyframeTypeHorizontal))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_KEYFRAMETYPEENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eKeyframeTypeFree", (long) NATRON_ENUM::KeyframeTypeEnum::eKeyframeTypeFree))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_KEYFRAMETYPEENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eKeyframeTypeBroken", (long) NATRON_ENUM::KeyframeTypeEnum::eKeyframeTypeBroken))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_KEYFRAMETYPEENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eKeyframeTypeNone", (long) NATRON_ENUM::KeyframeTypeEnum::eKeyframeTypeNone))
        return;
    // Register converter for enum 'NATRON_ENUM::KeyframeTypeEnum'.
    {
        SbkConverter *converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_ENUM_KEYFRAMETYPEENUM_IDX],
            NATRON_ENUM_KeyframeTypeEnum_CppToPython_NATRON_ENUM_KeyframeTypeEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            NATRON_ENUM_KeyframeTypeEnum_PythonToCpp_NATRON_ENUM_KeyframeTypeEnum,
            is_NATRON_ENUM_KeyframeTypeEnum_PythonToCpp_NATRON_ENUM_KeyframeTypeEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_ENUM_KEYFRAMETYPEENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "NATRON_ENUM::KeyframeTypeEnum");
        Shiboken::Conversions::registerConverterName(converter, "KeyframeTypeEnum");
    }
    // End of 'KeyframeTypeEnum' enum.

    // Initialization of enum 'PixmapEnum'.
    SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX] = Shiboken::Enum::createScopedEnum(Sbk_NATRON_ENUM_TypeF(),
        "PixmapEnum",
        "1:NatronEngine.NATRON_ENUM.PixmapEnum",
        "NATRON_ENUM::PixmapEnum");
    if (!SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX])
        return;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_PLAYER_PREVIOUS", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_PLAYER_PREVIOUS))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_PLAYER_FIRST_FRAME", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_PLAYER_FIRST_FRAME))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_PLAYER_NEXT", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_PLAYER_NEXT))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_PLAYER_LAST_FRAME", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_PLAYER_LAST_FRAME))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_PLAYER_NEXT_INCR", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_PLAYER_NEXT_INCR))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_PLAYER_NEXT_KEY", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_PLAYER_NEXT_KEY))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_PLAYER_PREVIOUS_INCR", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_PLAYER_PREVIOUS_INCR))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_PLAYER_PREVIOUS_KEY", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_PLAYER_PREVIOUS_KEY))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_PLAYER_REWIND_ENABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_PLAYER_REWIND_ENABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_PLAYER_REWIND_DISABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_PLAYER_REWIND_DISABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_PLAYER_PLAY_ENABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_PLAYER_PLAY_ENABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_PLAYER_PLAY_DISABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_PLAYER_PLAY_DISABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_PLAYER_STOP_DISABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_PLAYER_STOP_DISABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_PLAYER_STOP_ENABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_PLAYER_STOP_ENABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_PLAYER_PAUSE_DISABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_PLAYER_PAUSE_DISABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_PLAYER_PAUSE_ENABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_PLAYER_PAUSE_ENABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_PLAYER_LOOP_MODE", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_PLAYER_LOOP_MODE))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_PLAYER_BOUNCE", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_PLAYER_BOUNCE))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_PLAYER_PLAY_ONCE", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_PLAYER_PLAY_ONCE))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_PLAYER_TIMELINE_IN", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_PLAYER_TIMELINE_IN))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_PLAYER_TIMELINE_OUT", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_PLAYER_TIMELINE_OUT))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MAXIMIZE_WIDGET", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MAXIMIZE_WIDGET))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MINIMIZE_WIDGET", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MINIMIZE_WIDGET))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_CLOSE_WIDGET", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_CLOSE_WIDGET))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_HELP_WIDGET", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_HELP_WIDGET))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_CLOSE_PANEL", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_CLOSE_PANEL))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_HIDE_UNMODIFIED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_HIDE_UNMODIFIED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_UNHIDE_UNMODIFIED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_UNHIDE_UNMODIFIED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_GROUPBOX_FOLDED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_GROUPBOX_FOLDED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_GROUPBOX_UNFOLDED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_GROUPBOX_UNFOLDED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_UNDO", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_UNDO))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_REDO", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_REDO))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_UNDO_GRAYSCALE", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_UNDO_GRAYSCALE))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_REDO_GRAYSCALE", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_REDO_GRAYSCALE))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_RESTORE_DEFAULTS_ENABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_RESTORE_DEFAULTS_ENABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_RESTORE_DEFAULTS_DISABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_RESTORE_DEFAULTS_DISABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_TAB_WIDGET_LAYOUT_BUTTON", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_TAB_WIDGET_LAYOUT_BUTTON))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_TAB_WIDGET_LAYOUT_BUTTON_ANCHOR", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_TAB_WIDGET_LAYOUT_BUTTON_ANCHOR))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_TAB_WIDGET_SPLIT_HORIZONTALLY", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_TAB_WIDGET_SPLIT_HORIZONTALLY))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_TAB_WIDGET_SPLIT_VERTICALLY", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_TAB_WIDGET_SPLIT_VERTICALLY))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_VIEWER_CENTER", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_VIEWER_CENTER))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_VIEWER_CLIP_TO_PROJECT_ENABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_VIEWER_CLIP_TO_PROJECT_ENABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_VIEWER_CLIP_TO_PROJECT_DISABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_VIEWER_CLIP_TO_PROJECT_DISABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_VIEWER_FULL_FRAME_ON", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_VIEWER_FULL_FRAME_ON))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_VIEWER_FULL_FRAME_OFF", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_VIEWER_FULL_FRAME_OFF))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_VIEWER_REFRESH", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_VIEWER_REFRESH))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_VIEWER_REFRESH_ACTIVE", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_VIEWER_REFRESH_ACTIVE))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_VIEWER_ROI_ENABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_VIEWER_ROI_ENABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_VIEWER_ROI_DISABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_VIEWER_ROI_DISABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_VIEWER_RENDER_SCALE", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_VIEWER_RENDER_SCALE))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_VIEWER_RENDER_SCALE_CHECKED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_VIEWER_RENDER_SCALE_CHECKED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_VIEWER_AUTOCONTRAST_ENABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_VIEWER_AUTOCONTRAST_ENABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_VIEWER_AUTOCONTRAST_DISABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_VIEWER_AUTOCONTRAST_DISABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_OPEN_FILE", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_OPEN_FILE))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_COLORWHEEL", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_COLORWHEEL))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_OVERLAY", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_OVERLAY))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_ROTO_MERGE", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_ROTO_MERGE))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_COLOR_PICKER", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_COLOR_PICKER))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_IO_GROUPING", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_IO_GROUPING))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_3D_GROUPING", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_3D_GROUPING))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_CHANNEL_GROUPING", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_CHANNEL_GROUPING))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_GROUPING", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_GROUPING))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_COLOR_GROUPING", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_COLOR_GROUPING))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_TRANSFORM_GROUPING", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_TRANSFORM_GROUPING))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_DEEP_GROUPING", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_DEEP_GROUPING))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_FILTER_GROUPING", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_FILTER_GROUPING))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MULTIVIEW_GROUPING", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MULTIVIEW_GROUPING))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_TOOLSETS_GROUPING", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_TOOLSETS_GROUPING))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MISC_GROUPING", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MISC_GROUPING))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_OPEN_EFFECTS_GROUPING", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_OPEN_EFFECTS_GROUPING))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_TIME_GROUPING", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_TIME_GROUPING))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_PAINT_GROUPING", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_PAINT_GROUPING))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_KEYER_GROUPING", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_KEYER_GROUPING))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_OTHER_PLUGINS", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_OTHER_PLUGINS))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_READ_IMAGE", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_READ_IMAGE))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_WRITE_IMAGE", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_WRITE_IMAGE))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_COMBOBOX", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_COMBOBOX))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_COMBOBOX_PRESSED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_COMBOBOX_PRESSED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_ADD_KEYFRAME", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_ADD_KEYFRAME))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_REMOVE_KEYFRAME", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_REMOVE_KEYFRAME))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_INVERTED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_INVERTED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_UNINVERTED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_UNINVERTED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_VISIBLE", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_VISIBLE))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_UNVISIBLE", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_UNVISIBLE))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_LOCKED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_LOCKED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_UNLOCKED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_UNLOCKED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_LAYER", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_LAYER))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_BEZIER", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_BEZIER))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_PENCIL", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_PENCIL))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_CURVE", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_CURVE))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_BEZIER_32", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_BEZIER_32))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_ELLIPSE", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_ELLIPSE))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_RECTANGLE", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_RECTANGLE))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_ADD_POINTS", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_ADD_POINTS))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_REMOVE_POINTS", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_REMOVE_POINTS))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_CUSP_POINTS", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_CUSP_POINTS))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_SMOOTH_POINTS", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_SMOOTH_POINTS))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_REMOVE_FEATHER", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_REMOVE_FEATHER))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_OPEN_CLOSE_CURVE", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_OPEN_CLOSE_CURVE))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_SELECT_ALL", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_SELECT_ALL))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_SELECT_POINTS", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_SELECT_POINTS))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_SELECT_FEATHER", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_SELECT_FEATHER))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_SELECT_CURVES", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_SELECT_CURVES))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_AUTO_KEYING_ENABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_AUTO_KEYING_ENABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_AUTO_KEYING_DISABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_AUTO_KEYING_DISABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_STICKY_SELECTION_ENABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_STICKY_SELECTION_ENABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_STICKY_SELECTION_DISABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_STICKY_SELECTION_DISABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_FEATHER_LINK_ENABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_FEATHER_LINK_ENABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_FEATHER_LINK_DISABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_FEATHER_LINK_DISABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_FEATHER_VISIBLE", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_FEATHER_VISIBLE))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_FEATHER_UNVISIBLE", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_FEATHER_UNVISIBLE))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_RIPPLE_EDIT_ENABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_RIPPLE_EDIT_ENABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_RIPPLE_EDIT_DISABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_RIPPLE_EDIT_DISABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_ROTOPAINT_BLUR", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_ROTOPAINT_BLUR))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_ROTOPAINT_BUILDUP_ENABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_ROTOPAINT_BUILDUP_ENABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_ROTOPAINT_BUILDUP_DISABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_ROTOPAINT_BUILDUP_DISABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_ROTOPAINT_BURN", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_ROTOPAINT_BURN))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_ROTOPAINT_CLONE", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_ROTOPAINT_CLONE))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_ROTOPAINT_DODGE", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_ROTOPAINT_DODGE))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_ROTOPAINT_ERASER", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_ROTOPAINT_ERASER))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_ROTOPAINT_PRESSURE_ENABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_ROTOPAINT_PRESSURE_ENABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_ROTOPAINT_PRESSURE_DISABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_ROTOPAINT_PRESSURE_DISABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_ROTOPAINT_REVEAL", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_ROTOPAINT_REVEAL))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_ROTOPAINT_SHARPEN", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_ROTOPAINT_SHARPEN))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_ROTOPAINT_SMEAR", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_ROTOPAINT_SMEAR))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_ROTOPAINT_SOLID", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_ROTOPAINT_SOLID))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_BOLD_CHECKED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_BOLD_CHECKED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_BOLD_UNCHECKED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_BOLD_UNCHECKED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_ITALIC_CHECKED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_ITALIC_CHECKED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_ITALIC_UNCHECKED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_ITALIC_UNCHECKED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_CLEAR_ALL_ANIMATION", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_CLEAR_ALL_ANIMATION))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_CLEAR_BACKWARD_ANIMATION", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_CLEAR_BACKWARD_ANIMATION))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_CLEAR_FORWARD_ANIMATION", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_CLEAR_FORWARD_ANIMATION))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_UPDATE_VIEWER_ENABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_UPDATE_VIEWER_ENABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_UPDATE_VIEWER_DISABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_UPDATE_VIEWER_DISABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_ADD_TRACK", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_ADD_TRACK))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_ADD_USER_KEY", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_ADD_USER_KEY))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_REMOVE_USER_KEY", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_REMOVE_USER_KEY))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_SHOW_TRACK_ERROR", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_SHOW_TRACK_ERROR))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_HIDE_TRACK_ERROR", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_HIDE_TRACK_ERROR))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_RESET_TRACK_OFFSET", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_RESET_TRACK_OFFSET))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_CREATE_USER_KEY_ON_MOVE_ON", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_CREATE_USER_KEY_ON_MOVE_ON))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_CREATE_USER_KEY_ON_MOVE_OFF", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_CREATE_USER_KEY_ON_MOVE_OFF))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_RESET_USER_KEYS", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_RESET_USER_KEYS))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_CENTER_VIEWER_ON_TRACK", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_CENTER_VIEWER_ON_TRACK))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_TRACK_BACKWARD_ON", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_TRACK_BACKWARD_ON))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_TRACK_BACKWARD_OFF", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_TRACK_BACKWARD_OFF))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_TRACK_FORWARD_ON", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_TRACK_FORWARD_ON))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_TRACK_FORWARD_OFF", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_TRACK_FORWARD_OFF))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_TRACK_PREVIOUS", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_TRACK_PREVIOUS))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_TRACK_NEXT", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_TRACK_NEXT))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_TRACK_RANGE", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_TRACK_RANGE))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_TRACK_ALL_KEYS", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_TRACK_ALL_KEYS))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_TRACK_CURRENT_KEY", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_TRACK_CURRENT_KEY))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_NEXT_USER_KEY", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_NEXT_USER_KEY))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_PREV_USER_KEY", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_PREV_USER_KEY))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_ENTER_GROUP", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_ENTER_GROUP))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_SETTINGS", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_SETTINGS))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_FREEZE_ENABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_FREEZE_ENABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_FREEZE_DISABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_FREEZE_DISABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_VIEWER_ICON", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_VIEWER_ICON))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_VIEWER_CHECKERBOARD_ENABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_VIEWER_CHECKERBOARD_ENABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_VIEWER_CHECKERBOARD_DISABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_VIEWER_CHECKERBOARD_DISABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_VIEWER_ZEBRA_ENABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_VIEWER_ZEBRA_ENABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_VIEWER_ZEBRA_DISABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_VIEWER_ZEBRA_DISABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_VIEWER_GAMMA_ENABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_VIEWER_GAMMA_ENABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_VIEWER_GAMMA_DISABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_VIEWER_GAMMA_DISABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_VIEWER_GAIN_ENABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_VIEWER_GAIN_ENABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_VIEWER_GAIN_DISABLED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_VIEWER_GAIN_DISABLED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_SCRIPT_CLEAR_OUTPUT", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_SCRIPT_CLEAR_OUTPUT))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_SCRIPT_EXEC_SCRIPT", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_SCRIPT_EXEC_SCRIPT))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_SCRIPT_LOAD_EXEC_SCRIPT", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_SCRIPT_LOAD_EXEC_SCRIPT))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_SCRIPT_LOAD_SCRIPT", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_SCRIPT_LOAD_SCRIPT))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_SCRIPT_NEXT_SCRIPT", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_SCRIPT_NEXT_SCRIPT))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_SCRIPT_OUTPUT_PANE_ACTIVATED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_SCRIPT_OUTPUT_PANE_ACTIVATED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_SCRIPT_OUTPUT_PANE_DEACTIVATED", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_SCRIPT_OUTPUT_PANE_DEACTIVATED))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_SCRIPT_PREVIOUS_SCRIPT", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_SCRIPT_PREVIOUS_SCRIPT))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_SCRIPT_SAVE_SCRIPT", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_SCRIPT_SAVE_SCRIPT))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_ATOP", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_ATOP))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_AVERAGE", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_AVERAGE))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_COLOR", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_COLOR))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_COLOR_BURN", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_COLOR_BURN))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_COLOR_DODGE", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_COLOR_DODGE))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_CONJOINT_OVER", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_CONJOINT_OVER))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_COPY", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_COPY))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_DIFFERENCE", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_DIFFERENCE))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_DISJOINT_OVER", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_DISJOINT_OVER))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_DIVIDE", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_DIVIDE))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_EXCLUSION", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_EXCLUSION))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_FREEZE", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_FREEZE))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_FROM", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_FROM))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_GEOMETRIC", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_GEOMETRIC))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_GRAIN_EXTRACT", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_GRAIN_EXTRACT))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_GRAIN_MERGE", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_GRAIN_MERGE))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_HARD_LIGHT", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_HARD_LIGHT))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_HUE", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_HUE))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_HYPOT", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_HYPOT))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_IN", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_IN))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_LUMINOSITY", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_LUMINOSITY))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_MASK", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_MASK))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_MATTE", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_MATTE))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_MAX", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_MAX))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_MIN", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_MIN))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_MINUS", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_MINUS))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_MULTIPLY", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_MULTIPLY))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_OUT", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_OUT))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_OVER", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_OVER))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_OVERLAY", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_OVERLAY))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_PINLIGHT", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_PINLIGHT))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_PLUS", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_PLUS))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_REFLECT", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_REFLECT))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_SATURATION", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_SATURATION))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_SCREEN", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_SCREEN))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_SOFT_LIGHT", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_SOFT_LIGHT))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_STENCIL", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_STENCIL))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_UNDER", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_UNDER))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_MERGE_XOR", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_MERGE_XOR))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_ROTO_NODE_ICON", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_ROTO_NODE_ICON))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_LINK_CURSOR", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_LINK_CURSOR))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_LINK_MULT_CURSOR", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_LINK_MULT_CURSOR))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_APP_ICON", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_APP_ICON))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_INTERP_LINEAR", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_INTERP_LINEAR))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_INTERP_CURVE", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_INTERP_CURVE))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_INTERP_CONSTANT", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_INTERP_CONSTANT))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_INTERP_BREAK", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_INTERP_BREAK))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_INTERP_CURVE_C", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_INTERP_CURVE_C))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_INTERP_CURVE_H", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_INTERP_CURVE_H))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_INTERP_CURVE_R", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_INTERP_CURVE_R))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "NATRON_PIXMAP_INTERP_CURVE_Z", (long) NATRON_ENUM::PixmapEnum::NATRON_PIXMAP_INTERP_CURVE_Z))
        return;
    // Register converter for enum 'NATRON_ENUM::PixmapEnum'.
    {
        SbkConverter *converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX],
            NATRON_ENUM_PixmapEnum_CppToPython_NATRON_ENUM_PixmapEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            NATRON_ENUM_PixmapEnum_PythonToCpp_NATRON_ENUM_PixmapEnum,
            is_NATRON_ENUM_PixmapEnum_PythonToCpp_NATRON_ENUM_PixmapEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "NATRON_ENUM::PixmapEnum");
        Shiboken::Conversions::registerConverterName(converter, "PixmapEnum");
    }
    // End of 'PixmapEnum' enum.

    // Initialization of enum 'ValueChangedReasonEnum'.
    SbkNatronEngineTypes[SBK_NATRON_ENUM_VALUECHANGEDREASONENUM_IDX] = Shiboken::Enum::createScopedEnum(Sbk_NATRON_ENUM_TypeF(),
        "ValueChangedReasonEnum",
        "1:NatronEngine.NATRON_ENUM.ValueChangedReasonEnum",
        "NATRON_ENUM::ValueChangedReasonEnum");
    if (!SbkNatronEngineTypes[SBK_NATRON_ENUM_VALUECHANGEDREASONENUM_IDX])
        return;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_VALUECHANGEDREASONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eValueChangedReasonUserEdited", (long) NATRON_ENUM::ValueChangedReasonEnum::eValueChangedReasonUserEdited))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_VALUECHANGEDREASONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eValueChangedReasonPluginEdited", (long) NATRON_ENUM::ValueChangedReasonEnum::eValueChangedReasonPluginEdited))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_VALUECHANGEDREASONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eValueChangedReasonNatronGuiEdited", (long) NATRON_ENUM::ValueChangedReasonEnum::eValueChangedReasonNatronGuiEdited))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_VALUECHANGEDREASONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eValueChangedReasonNatronInternalEdited", (long) NATRON_ENUM::ValueChangedReasonEnum::eValueChangedReasonNatronInternalEdited))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_VALUECHANGEDREASONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eValueChangedReasonTimeChanged", (long) NATRON_ENUM::ValueChangedReasonEnum::eValueChangedReasonTimeChanged))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_VALUECHANGEDREASONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eValueChangedReasonSlaveRefresh", (long) NATRON_ENUM::ValueChangedReasonEnum::eValueChangedReasonSlaveRefresh))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_VALUECHANGEDREASONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eValueChangedReasonRestoreDefault", (long) NATRON_ENUM::ValueChangedReasonEnum::eValueChangedReasonRestoreDefault))
        return;
    // Register converter for enum 'NATRON_ENUM::ValueChangedReasonEnum'.
    {
        SbkConverter *converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_ENUM_VALUECHANGEDREASONENUM_IDX],
            NATRON_ENUM_ValueChangedReasonEnum_CppToPython_NATRON_ENUM_ValueChangedReasonEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            NATRON_ENUM_ValueChangedReasonEnum_PythonToCpp_NATRON_ENUM_ValueChangedReasonEnum,
            is_NATRON_ENUM_ValueChangedReasonEnum_PythonToCpp_NATRON_ENUM_ValueChangedReasonEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_ENUM_VALUECHANGEDREASONENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "NATRON_ENUM::ValueChangedReasonEnum");
        Shiboken::Conversions::registerConverterName(converter, "ValueChangedReasonEnum");
    }
    // End of 'ValueChangedReasonEnum' enum.

    // Initialization of enum 'AnimationLevelEnum'.
    SbkNatronEngineTypes[SBK_NATRON_ENUM_ANIMATIONLEVELENUM_IDX] = Shiboken::Enum::createScopedEnum(Sbk_NATRON_ENUM_TypeF(),
        "AnimationLevelEnum",
        "1:NatronEngine.NATRON_ENUM.AnimationLevelEnum",
        "NATRON_ENUM::AnimationLevelEnum");
    if (!SbkNatronEngineTypes[SBK_NATRON_ENUM_ANIMATIONLEVELENUM_IDX])
        return;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_ANIMATIONLEVELENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eAnimationLevelNone", (long) NATRON_ENUM::AnimationLevelEnum::eAnimationLevelNone))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_ANIMATIONLEVELENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eAnimationLevelInterpolatedValue", (long) NATRON_ENUM::AnimationLevelEnum::eAnimationLevelInterpolatedValue))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_ANIMATIONLEVELENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eAnimationLevelOnKeyframe", (long) NATRON_ENUM::AnimationLevelEnum::eAnimationLevelOnKeyframe))
        return;
    // Register converter for enum 'NATRON_ENUM::AnimationLevelEnum'.
    {
        SbkConverter *converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_ENUM_ANIMATIONLEVELENUM_IDX],
            NATRON_ENUM_AnimationLevelEnum_CppToPython_NATRON_ENUM_AnimationLevelEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            NATRON_ENUM_AnimationLevelEnum_PythonToCpp_NATRON_ENUM_AnimationLevelEnum,
            is_NATRON_ENUM_AnimationLevelEnum_PythonToCpp_NATRON_ENUM_AnimationLevelEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_ENUM_ANIMATIONLEVELENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "NATRON_ENUM::AnimationLevelEnum");
        Shiboken::Conversions::registerConverterName(converter, "AnimationLevelEnum");
    }
    // End of 'AnimationLevelEnum' enum.

    // Initialization of enum 'ImagePremultiplicationEnum'.
    SbkNatronEngineTypes[SBK_NATRON_ENUM_IMAGEPREMULTIPLICATIONENUM_IDX] = Shiboken::Enum::createScopedEnum(Sbk_NATRON_ENUM_TypeF(),
        "ImagePremultiplicationEnum",
        "1:NatronEngine.NATRON_ENUM.ImagePremultiplicationEnum",
        "NATRON_ENUM::ImagePremultiplicationEnum");
    if (!SbkNatronEngineTypes[SBK_NATRON_ENUM_IMAGEPREMULTIPLICATIONENUM_IDX])
        return;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_IMAGEPREMULTIPLICATIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eImagePremultiplicationOpaque", (long) NATRON_ENUM::ImagePremultiplicationEnum::eImagePremultiplicationOpaque))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_IMAGEPREMULTIPLICATIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eImagePremultiplicationPremultiplied", (long) NATRON_ENUM::ImagePremultiplicationEnum::eImagePremultiplicationPremultiplied))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_IMAGEPREMULTIPLICATIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eImagePremultiplicationUnPremultiplied", (long) NATRON_ENUM::ImagePremultiplicationEnum::eImagePremultiplicationUnPremultiplied))
        return;
    // Register converter for enum 'NATRON_ENUM::ImagePremultiplicationEnum'.
    {
        SbkConverter *converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_ENUM_IMAGEPREMULTIPLICATIONENUM_IDX],
            NATRON_ENUM_ImagePremultiplicationEnum_CppToPython_NATRON_ENUM_ImagePremultiplicationEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            NATRON_ENUM_ImagePremultiplicationEnum_PythonToCpp_NATRON_ENUM_ImagePremultiplicationEnum,
            is_NATRON_ENUM_ImagePremultiplicationEnum_PythonToCpp_NATRON_ENUM_ImagePremultiplicationEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_ENUM_IMAGEPREMULTIPLICATIONENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "NATRON_ENUM::ImagePremultiplicationEnum");
        Shiboken::Conversions::registerConverterName(converter, "ImagePremultiplicationEnum");
    }
    // End of 'ImagePremultiplicationEnum' enum.

    // Initialization of enum 'ViewerCompositingOperatorEnum'.
    SbkNatronEngineTypes[SBK_NATRON_ENUM_VIEWERCOMPOSITINGOPERATORENUM_IDX] = Shiboken::Enum::createScopedEnum(Sbk_NATRON_ENUM_TypeF(),
        "ViewerCompositingOperatorEnum",
        "1:NatronEngine.NATRON_ENUM.ViewerCompositingOperatorEnum",
        "NATRON_ENUM::ViewerCompositingOperatorEnum");
    if (!SbkNatronEngineTypes[SBK_NATRON_ENUM_VIEWERCOMPOSITINGOPERATORENUM_IDX])
        return;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_VIEWERCOMPOSITINGOPERATORENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eViewerCompositingOperatorNone", (long) NATRON_ENUM::ViewerCompositingOperatorEnum::eViewerCompositingOperatorNone))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_VIEWERCOMPOSITINGOPERATORENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eViewerCompositingOperatorWipeUnder", (long) NATRON_ENUM::ViewerCompositingOperatorEnum::eViewerCompositingOperatorWipeUnder))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_VIEWERCOMPOSITINGOPERATORENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eViewerCompositingOperatorWipeOver", (long) NATRON_ENUM::ViewerCompositingOperatorEnum::eViewerCompositingOperatorWipeOver))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_VIEWERCOMPOSITINGOPERATORENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eViewerCompositingOperatorWipeMinus", (long) NATRON_ENUM::ViewerCompositingOperatorEnum::eViewerCompositingOperatorWipeMinus))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_VIEWERCOMPOSITINGOPERATORENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eViewerCompositingOperatorWipeOnionSkin", (long) NATRON_ENUM::ViewerCompositingOperatorEnum::eViewerCompositingOperatorWipeOnionSkin))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_VIEWERCOMPOSITINGOPERATORENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eViewerCompositingOperatorStackUnder", (long) NATRON_ENUM::ViewerCompositingOperatorEnum::eViewerCompositingOperatorStackUnder))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_VIEWERCOMPOSITINGOPERATORENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eViewerCompositingOperatorStackOver", (long) NATRON_ENUM::ViewerCompositingOperatorEnum::eViewerCompositingOperatorStackOver))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_VIEWERCOMPOSITINGOPERATORENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eViewerCompositingOperatorStackMinus", (long) NATRON_ENUM::ViewerCompositingOperatorEnum::eViewerCompositingOperatorStackMinus))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_VIEWERCOMPOSITINGOPERATORENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eViewerCompositingOperatorStackOnionSkin", (long) NATRON_ENUM::ViewerCompositingOperatorEnum::eViewerCompositingOperatorStackOnionSkin))
        return;
    // Register converter for enum 'NATRON_ENUM::ViewerCompositingOperatorEnum'.
    {
        SbkConverter *converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_ENUM_VIEWERCOMPOSITINGOPERATORENUM_IDX],
            NATRON_ENUM_ViewerCompositingOperatorEnum_CppToPython_NATRON_ENUM_ViewerCompositingOperatorEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            NATRON_ENUM_ViewerCompositingOperatorEnum_PythonToCpp_NATRON_ENUM_ViewerCompositingOperatorEnum,
            is_NATRON_ENUM_ViewerCompositingOperatorEnum_PythonToCpp_NATRON_ENUM_ViewerCompositingOperatorEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_ENUM_VIEWERCOMPOSITINGOPERATORENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "NATRON_ENUM::ViewerCompositingOperatorEnum");
        Shiboken::Conversions::registerConverterName(converter, "ViewerCompositingOperatorEnum");
    }
    // End of 'ViewerCompositingOperatorEnum' enum.

    // Initialization of enum 'ViewerColorSpaceEnum'.
    SbkNatronEngineTypes[SBK_NATRON_ENUM_VIEWERCOLORSPACEENUM_IDX] = Shiboken::Enum::createScopedEnum(Sbk_NATRON_ENUM_TypeF(),
        "ViewerColorSpaceEnum",
        "1:NatronEngine.NATRON_ENUM.ViewerColorSpaceEnum",
        "NATRON_ENUM::ViewerColorSpaceEnum");
    if (!SbkNatronEngineTypes[SBK_NATRON_ENUM_VIEWERCOLORSPACEENUM_IDX])
        return;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_VIEWERCOLORSPACEENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eViewerColorSpaceSRGB", (long) NATRON_ENUM::ViewerColorSpaceEnum::eViewerColorSpaceSRGB))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_VIEWERCOLORSPACEENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eViewerColorSpaceLinear", (long) NATRON_ENUM::ViewerColorSpaceEnum::eViewerColorSpaceLinear))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_VIEWERCOLORSPACEENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eViewerColorSpaceRec709", (long) NATRON_ENUM::ViewerColorSpaceEnum::eViewerColorSpaceRec709))
        return;
    // Register converter for enum 'NATRON_ENUM::ViewerColorSpaceEnum'.
    {
        SbkConverter *converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_ENUM_VIEWERCOLORSPACEENUM_IDX],
            NATRON_ENUM_ViewerColorSpaceEnum_CppToPython_NATRON_ENUM_ViewerColorSpaceEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            NATRON_ENUM_ViewerColorSpaceEnum_PythonToCpp_NATRON_ENUM_ViewerColorSpaceEnum,
            is_NATRON_ENUM_ViewerColorSpaceEnum_PythonToCpp_NATRON_ENUM_ViewerColorSpaceEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_ENUM_VIEWERCOLORSPACEENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "NATRON_ENUM::ViewerColorSpaceEnum");
        Shiboken::Conversions::registerConverterName(converter, "ViewerColorSpaceEnum");
    }
    // End of 'ViewerColorSpaceEnum' enum.

    // Initialization of enum 'ImageBitDepthEnum'.
    SbkNatronEngineTypes[SBK_NATRON_ENUM_IMAGEBITDEPTHENUM_IDX] = Shiboken::Enum::createScopedEnum(Sbk_NATRON_ENUM_TypeF(),
        "ImageBitDepthEnum",
        "1:NatronEngine.NATRON_ENUM.ImageBitDepthEnum",
        "NATRON_ENUM::ImageBitDepthEnum");
    if (!SbkNatronEngineTypes[SBK_NATRON_ENUM_IMAGEBITDEPTHENUM_IDX])
        return;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_IMAGEBITDEPTHENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eImageBitDepthNone", (long) NATRON_ENUM::ImageBitDepthEnum::eImageBitDepthNone))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_IMAGEBITDEPTHENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eImageBitDepthByte", (long) NATRON_ENUM::ImageBitDepthEnum::eImageBitDepthByte))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_IMAGEBITDEPTHENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eImageBitDepthShort", (long) NATRON_ENUM::ImageBitDepthEnum::eImageBitDepthShort))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_IMAGEBITDEPTHENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eImageBitDepthHalf", (long) NATRON_ENUM::ImageBitDepthEnum::eImageBitDepthHalf))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_IMAGEBITDEPTHENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eImageBitDepthFloat", (long) NATRON_ENUM::ImageBitDepthEnum::eImageBitDepthFloat))
        return;
    // Register converter for enum 'NATRON_ENUM::ImageBitDepthEnum'.
    {
        SbkConverter *converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_ENUM_IMAGEBITDEPTHENUM_IDX],
            NATRON_ENUM_ImageBitDepthEnum_CppToPython_NATRON_ENUM_ImageBitDepthEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            NATRON_ENUM_ImageBitDepthEnum_PythonToCpp_NATRON_ENUM_ImageBitDepthEnum,
            is_NATRON_ENUM_ImageBitDepthEnum_PythonToCpp_NATRON_ENUM_ImageBitDepthEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_ENUM_IMAGEBITDEPTHENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "NATRON_ENUM::ImageBitDepthEnum");
        Shiboken::Conversions::registerConverterName(converter, "ImageBitDepthEnum");
    }
    // End of 'ImageBitDepthEnum' enum.

    // Initialization of enum 'OrientationEnum'.
    SbkNatronEngineTypes[SBK_NATRON_ENUM_ORIENTATIONENUM_IDX] = Shiboken::Enum::createScopedEnum(Sbk_NATRON_ENUM_TypeF(),
        "OrientationEnum",
        "1:NatronEngine.NATRON_ENUM.OrientationEnum",
        "NATRON_ENUM::OrientationEnum");
    if (!SbkNatronEngineTypes[SBK_NATRON_ENUM_ORIENTATIONENUM_IDX])
        return;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_ORIENTATIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eOrientationHorizontal", (long) NATRON_ENUM::OrientationEnum::eOrientationHorizontal))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_ORIENTATIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eOrientationVertical", (long) NATRON_ENUM::OrientationEnum::eOrientationVertical))
        return;
    // Register converter for enum 'NATRON_ENUM::OrientationEnum'.
    {
        SbkConverter *converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_ENUM_ORIENTATIONENUM_IDX],
            NATRON_ENUM_OrientationEnum_CppToPython_NATRON_ENUM_OrientationEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            NATRON_ENUM_OrientationEnum_PythonToCpp_NATRON_ENUM_OrientationEnum,
            is_NATRON_ENUM_OrientationEnum_PythonToCpp_NATRON_ENUM_OrientationEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_ENUM_ORIENTATIONENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "NATRON_ENUM::OrientationEnum");
        Shiboken::Conversions::registerConverterName(converter, "OrientationEnum");
    }
    // End of 'OrientationEnum' enum.

    // Initialization of enum 'PlaybackModeEnum'.
    SbkNatronEngineTypes[SBK_NATRON_ENUM_PLAYBACKMODEENUM_IDX] = Shiboken::Enum::createScopedEnum(Sbk_NATRON_ENUM_TypeF(),
        "PlaybackModeEnum",
        "1:NatronEngine.NATRON_ENUM.PlaybackModeEnum",
        "NATRON_ENUM::PlaybackModeEnum");
    if (!SbkNatronEngineTypes[SBK_NATRON_ENUM_PLAYBACKMODEENUM_IDX])
        return;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PLAYBACKMODEENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "ePlaybackModeLoop", (long) NATRON_ENUM::PlaybackModeEnum::ePlaybackModeLoop))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PLAYBACKMODEENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "ePlaybackModeBounce", (long) NATRON_ENUM::PlaybackModeEnum::ePlaybackModeBounce))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_PLAYBACKMODEENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "ePlaybackModeOnce", (long) NATRON_ENUM::PlaybackModeEnum::ePlaybackModeOnce))
        return;
    // Register converter for enum 'NATRON_ENUM::PlaybackModeEnum'.
    {
        SbkConverter *converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_ENUM_PLAYBACKMODEENUM_IDX],
            NATRON_ENUM_PlaybackModeEnum_CppToPython_NATRON_ENUM_PlaybackModeEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            NATRON_ENUM_PlaybackModeEnum_PythonToCpp_NATRON_ENUM_PlaybackModeEnum,
            is_NATRON_ENUM_PlaybackModeEnum_PythonToCpp_NATRON_ENUM_PlaybackModeEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_ENUM_PLAYBACKMODEENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "NATRON_ENUM::PlaybackModeEnum");
        Shiboken::Conversions::registerConverterName(converter, "PlaybackModeEnum");
    }
    // End of 'PlaybackModeEnum' enum.

    // Initialization of enum 'DisplayChannelsEnum'.
    SbkNatronEngineTypes[SBK_NATRON_ENUM_DISPLAYCHANNELSENUM_IDX] = Shiboken::Enum::createScopedEnum(Sbk_NATRON_ENUM_TypeF(),
        "DisplayChannelsEnum",
        "1:NatronEngine.NATRON_ENUM.DisplayChannelsEnum",
        "NATRON_ENUM::DisplayChannelsEnum");
    if (!SbkNatronEngineTypes[SBK_NATRON_ENUM_DISPLAYCHANNELSENUM_IDX])
        return;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_DISPLAYCHANNELSENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eDisplayChannelsRGB", (long) NATRON_ENUM::DisplayChannelsEnum::eDisplayChannelsRGB))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_DISPLAYCHANNELSENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eDisplayChannelsR", (long) NATRON_ENUM::DisplayChannelsEnum::eDisplayChannelsR))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_DISPLAYCHANNELSENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eDisplayChannelsG", (long) NATRON_ENUM::DisplayChannelsEnum::eDisplayChannelsG))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_DISPLAYCHANNELSENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eDisplayChannelsB", (long) NATRON_ENUM::DisplayChannelsEnum::eDisplayChannelsB))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_DISPLAYCHANNELSENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eDisplayChannelsA", (long) NATRON_ENUM::DisplayChannelsEnum::eDisplayChannelsA))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_DISPLAYCHANNELSENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eDisplayChannelsY", (long) NATRON_ENUM::DisplayChannelsEnum::eDisplayChannelsY))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_DISPLAYCHANNELSENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eDisplayChannelsMatte", (long) NATRON_ENUM::DisplayChannelsEnum::eDisplayChannelsMatte))
        return;
    // Register converter for enum 'NATRON_ENUM::DisplayChannelsEnum'.
    {
        SbkConverter *converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_ENUM_DISPLAYCHANNELSENUM_IDX],
            NATRON_ENUM_DisplayChannelsEnum_CppToPython_NATRON_ENUM_DisplayChannelsEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            NATRON_ENUM_DisplayChannelsEnum_PythonToCpp_NATRON_ENUM_DisplayChannelsEnum,
            is_NATRON_ENUM_DisplayChannelsEnum_PythonToCpp_NATRON_ENUM_DisplayChannelsEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_ENUM_DISPLAYCHANNELSENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "NATRON_ENUM::DisplayChannelsEnum");
        Shiboken::Conversions::registerConverterName(converter, "DisplayChannelsEnum");
    }
    // End of 'DisplayChannelsEnum' enum.

    // Initialization of enum 'MergingFunctionEnum'.
    SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX] = Shiboken::Enum::createScopedEnum(Sbk_NATRON_ENUM_TypeF(),
        "MergingFunctionEnum",
        "1:NatronEngine.NATRON_ENUM.MergingFunctionEnum",
        "NATRON_ENUM::MergingFunctionEnum");
    if (!SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX])
        return;

    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeATop", (long) NATRON_ENUM::MergingFunctionEnum::eMergeATop))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeAverage", (long) NATRON_ENUM::MergingFunctionEnum::eMergeAverage))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeColor", (long) NATRON_ENUM::MergingFunctionEnum::eMergeColor))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeColorBurn", (long) NATRON_ENUM::MergingFunctionEnum::eMergeColorBurn))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeColorDodge", (long) NATRON_ENUM::MergingFunctionEnum::eMergeColorDodge))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeConjointOver", (long) NATRON_ENUM::MergingFunctionEnum::eMergeConjointOver))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeCopy", (long) NATRON_ENUM::MergingFunctionEnum::eMergeCopy))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeDifference", (long) NATRON_ENUM::MergingFunctionEnum::eMergeDifference))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeDisjointOver", (long) NATRON_ENUM::MergingFunctionEnum::eMergeDisjointOver))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeDivide", (long) NATRON_ENUM::MergingFunctionEnum::eMergeDivide))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeExclusion", (long) NATRON_ENUM::MergingFunctionEnum::eMergeExclusion))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeFreeze", (long) NATRON_ENUM::MergingFunctionEnum::eMergeFreeze))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeFrom", (long) NATRON_ENUM::MergingFunctionEnum::eMergeFrom))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeGeometric", (long) NATRON_ENUM::MergingFunctionEnum::eMergeGeometric))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeGrainExtract", (long) NATRON_ENUM::MergingFunctionEnum::eMergeGrainExtract))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeGrainMerge", (long) NATRON_ENUM::MergingFunctionEnum::eMergeGrainMerge))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeHardLight", (long) NATRON_ENUM::MergingFunctionEnum::eMergeHardLight))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeHue", (long) NATRON_ENUM::MergingFunctionEnum::eMergeHue))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeHypot", (long) NATRON_ENUM::MergingFunctionEnum::eMergeHypot))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeIn", (long) NATRON_ENUM::MergingFunctionEnum::eMergeIn))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeLuminosity", (long) NATRON_ENUM::MergingFunctionEnum::eMergeLuminosity))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeMask", (long) NATRON_ENUM::MergingFunctionEnum::eMergeMask))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeMatte", (long) NATRON_ENUM::MergingFunctionEnum::eMergeMatte))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeMax", (long) NATRON_ENUM::MergingFunctionEnum::eMergeMax))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeMin", (long) NATRON_ENUM::MergingFunctionEnum::eMergeMin))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeMinus", (long) NATRON_ENUM::MergingFunctionEnum::eMergeMinus))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeMultiply", (long) NATRON_ENUM::MergingFunctionEnum::eMergeMultiply))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeOut", (long) NATRON_ENUM::MergingFunctionEnum::eMergeOut))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeOver", (long) NATRON_ENUM::MergingFunctionEnum::eMergeOver))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeOverlay", (long) NATRON_ENUM::MergingFunctionEnum::eMergeOverlay))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergePinLight", (long) NATRON_ENUM::MergingFunctionEnum::eMergePinLight))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergePlus", (long) NATRON_ENUM::MergingFunctionEnum::eMergePlus))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeReflect", (long) NATRON_ENUM::MergingFunctionEnum::eMergeReflect))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeSaturation", (long) NATRON_ENUM::MergingFunctionEnum::eMergeSaturation))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeScreen", (long) NATRON_ENUM::MergingFunctionEnum::eMergeScreen))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeSoftLight", (long) NATRON_ENUM::MergingFunctionEnum::eMergeSoftLight))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeStencil", (long) NATRON_ENUM::MergingFunctionEnum::eMergeStencil))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeUnder", (long) NATRON_ENUM::MergingFunctionEnum::eMergeUnder))
        return;
    if (!Shiboken::Enum::createScopedEnumItem(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
        Sbk_NATRON_ENUM_TypeF(), "eMergeXOR", (long) NATRON_ENUM::MergingFunctionEnum::eMergeXOR))
        return;
    // Register converter for enum 'NATRON_ENUM::MergingFunctionEnum'.
    {
        SbkConverter *converter = Shiboken::Conversions::createConverter(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX],
            NATRON_ENUM_MergingFunctionEnum_CppToPython_NATRON_ENUM_MergingFunctionEnum);
        Shiboken::Conversions::addPythonToCppValueConversion(converter,
            NATRON_ENUM_MergingFunctionEnum_PythonToCpp_NATRON_ENUM_MergingFunctionEnum,
            is_NATRON_ENUM_MergingFunctionEnum_PythonToCpp_NATRON_ENUM_MergingFunctionEnum_Convertible);
        Shiboken::Enum::setTypeConverter(SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX], converter);
        Shiboken::Conversions::registerConverterName(converter, "NATRON_ENUM::MergingFunctionEnum");
        Shiboken::Conversions::registerConverterName(converter, "MergingFunctionEnum");
    }
    // End of 'MergingFunctionEnum' enum.

    qRegisterMetaType< ::NATRON_ENUM::StatusEnum >("NATRON_ENUM::StatusEnum");
    qRegisterMetaType< ::NATRON_ENUM::StandardButtonEnum >("NATRON_ENUM::StandardButtonEnum");
    qRegisterMetaType< ::NATRON_ENUM::StandardButtons >("NATRON_ENUM::StandardButtons");
    qRegisterMetaType< ::NATRON_ENUM::KeyframeTypeEnum >("NATRON_ENUM::KeyframeTypeEnum");
    qRegisterMetaType< ::NATRON_ENUM::PixmapEnum >("NATRON_ENUM::PixmapEnum");
    qRegisterMetaType< ::NATRON_ENUM::ValueChangedReasonEnum >("NATRON_ENUM::ValueChangedReasonEnum");
    qRegisterMetaType< ::NATRON_ENUM::AnimationLevelEnum >("NATRON_ENUM::AnimationLevelEnum");
    qRegisterMetaType< ::NATRON_ENUM::ImagePremultiplicationEnum >("NATRON_ENUM::ImagePremultiplicationEnum");
    qRegisterMetaType< ::NATRON_ENUM::ViewerCompositingOperatorEnum >("NATRON_ENUM::ViewerCompositingOperatorEnum");
    qRegisterMetaType< ::NATRON_ENUM::ViewerColorSpaceEnum >("NATRON_ENUM::ViewerColorSpaceEnum");
    qRegisterMetaType< ::NATRON_ENUM::ImageBitDepthEnum >("NATRON_ENUM::ImageBitDepthEnum");
    qRegisterMetaType< ::NATRON_ENUM::OrientationEnum >("NATRON_ENUM::OrientationEnum");
    qRegisterMetaType< ::NATRON_ENUM::PlaybackModeEnum >("NATRON_ENUM::PlaybackModeEnum");
    qRegisterMetaType< ::NATRON_ENUM::DisplayChannelsEnum >("NATRON_ENUM::DisplayChannelsEnum");
    qRegisterMetaType< ::NATRON_ENUM::MergingFunctionEnum >("NATRON_ENUM::MergingFunctionEnum");
}
