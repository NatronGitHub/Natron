

#ifndef SBK_NATRONENGINE_PYTHON_H
#define SBK_NATRONENGINE_PYTHON_H

#include <sbkpython.h>
#include <sbkconverter.h>
// Module Includes
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
CLANG_DIAG_OFF(keyword-macro)
#include <pyside2_qtcore_python.h> // produces warnings
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)
CLANG_DIAG_ON(keyword-macro)

// Bound library includes
#include <PyParameter.h>
#include <RectD.h>
#include <PyAppInstance.h>
#include <PyNodeGroup.h>
#include <PyTracker.h>
#include <PyExprUtils.h>
#include <PyRoto.h>
#include <PyGlobalFunctions.h>
#include <RectI.h>
#include <Enums.h>
#include <PyNode.h>
// Conversion Includes - Primitive Types
#include <qabstractitemmodel.h>
#include <QString>
#include <QStringList>
#include <signalmanager.h>

// Conversion Includes - Container Types
#include <pysideqflags.h>
#include <QLinkedList>
#include <QList>
#include <QMap>
#include <QMultiMap>
#include <QPair>
#include <QQueue>
#include <QSet>
#include <QStack>
#include <QVector>
#include <list>
#include <map>
#include <map>
#include <utility>
#include <set>
#include <vector>

// Type indices
enum : int {
    SBK_ANIMATEDPARAM_IDX                                    = 0,
    SBK_APP_IDX                                              = 1,
    SBK_APPSETTINGS_IDX                                      = 2,
    SBK_BEZIERCURVE_IDX                                      = 3,
    SBK_BOOLNODECREATIONPROPERTY_IDX                         = 4,
    SBK_BOOLEANPARAM_IDX                                     = 5,
    SBK_BUTTONPARAM_IDX                                      = 6,
    SBK_CHOICEPARAM_IDX                                      = 7,
    SBK_COLORPARAM_IDX                                       = 8,
    SBK_COLORTUPLE_IDX                                       = 9,
    SBK_DOUBLE2DPARAM_IDX                                    = 10,
    SBK_DOUBLE2DTUPLE_IDX                                    = 11,
    SBK_DOUBLE3DPARAM_IDX                                    = 12,
    SBK_DOUBLE3DTUPLE_IDX                                    = 13,
    SBK_DOUBLEPARAM_IDX                                      = 14,
    SBK_EFFECT_IDX                                           = 15,
    SBK_EXPRUTILS_IDX                                        = 16,
    SBK_FILEPARAM_IDX                                        = 17,
    SBK_FLOATNODECREATIONPROPERTY_IDX                        = 18,
    SBK_GROUP_IDX                                            = 19,
    SBK_GROUPPARAM_IDX                                       = 20,
    SBK_IMAGELAYER_IDX                                       = 21,
    SBK_INT2DPARAM_IDX                                       = 22,
    SBK_INT2DTUPLE_IDX                                       = 23,
    SBK_INT3DPARAM_IDX                                       = 24,
    SBK_INT3DTUPLE_IDX                                       = 25,
    SBK_INTNODECREATIONPROPERTY_IDX                          = 26,
    SBK_INTPARAM_IDX                                         = 27,
    SBK_ITEMBASE_IDX                                         = 28,
    SBK_LAYER_IDX                                            = 29,
    SBK_NATRON_ENUM_STATUSENUM_IDX                           = 41,
    SBK_NATRON_ENUM_STANDARDBUTTONENUM_IDX                   = 40,
    SBK_QFLAGS_NATRON_ENUM_STANDARDBUTTONENUM_IDX            = 52,
    SBK_NATRON_ENUM_KEYFRAMETYPEENUM_IDX                     = 35,
    SBK_NATRON_ENUM_PIXMAPENUM_IDX                           = 38,
    SBK_NATRON_ENUM_VALUECHANGEDREASONENUM_IDX               = 42,
    SBK_NATRON_ENUM_ANIMATIONLEVELENUM_IDX                   = 31,
    SBK_NATRON_ENUM_IMAGEPREMULTIPLICATIONENUM_IDX           = 34,
    SBK_NATRON_ENUM_VIEWERCOMPOSITINGOPERATORENUM_IDX        = 44,
    SBK_NATRON_ENUM_VIEWERCOLORSPACEENUM_IDX                 = 43,
    SBK_NATRON_ENUM_IMAGEBITDEPTHENUM_IDX                    = 33,
    SBK_NATRON_ENUM_ORIENTATIONENUM_IDX                      = 37,
    SBK_NATRON_ENUM_PLAYBACKMODEENUM_IDX                     = 39,
    SBK_NATRON_ENUM_DISPLAYCHANNELSENUM_IDX                  = 32,
    SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX                  = 36,
    SBK_NatronEngineNATRON_ENUM_IDX                          = 30,
    SBK_NODECREATIONPROPERTY_IDX                             = 45,
    SBK_OUTPUTFILEPARAM_IDX                                  = 46,
    SBK_PAGEPARAM_IDX                                        = 47,
    SBK_PARAM_IDX                                            = 48,
    SBK_PARAMETRICPARAM_IDX                                  = 49,
    SBK_PATHPARAM_IDX                                        = 50,
    SBK_PYCOREAPPLICATION_IDX                                = 51,
    SBK_RECTD_IDX                                            = 53,
    SBK_RECTI_IDX                                            = 54,
    SBK_ROTO_IDX                                             = 55,
    SBK_SEPARATORPARAM_IDX                                   = 56,
    SBK_STRINGNODECREATIONPROPERTY_IDX                       = 57,
    SBK_STRINGPARAM_TYPEENUM_IDX                             = 59,
    SBK_STRINGPARAM_IDX                                      = 58,
    SBK_STRINGPARAMBASE_IDX                                  = 60,
    SBK_TRACK_IDX                                            = 61,
    SBK_TRACKER_IDX                                          = 62,
    SBK_USERPARAMHOLDER_IDX                                  = 63,
    SBK_NatronEngine_IDX_COUNT                               = 64
};
// This variable stores all Python types exported by this module.
extern PyTypeObject **SbkNatronEngineTypes;

// This variable stores the Python module object exported by this module.
extern PyObject *SbkNatronEngineModuleObject;

// This variable stores all type converters exported by this module.
extern SbkConverter **SbkNatronEngineTypeConverters;

// Converter indices
enum : int {
    SBK_STD_SIZE_T_IDX                                       = 0,
    SBK_NATRONENGINE_STD_MAP_QSTRING_NODECREATIONPROPERTYPTR_IDX = 1, // const std::map<QString,NodeCreationProperty* > &
    SBK_NATRONENGINE_STD_LIST_EFFECTPTR_IDX                  = 2, // std::list<Effect* >
    SBK_NATRONENGINE_STD_LIST_QSTRING_IDX                    = 3, // std::list<QString >
    SBK_NATRONENGINE_STD_LIST_INT_IDX                        = 4, // const std::list<int > &
    SBK_NATRONENGINE_STD_LIST_PARAMPTR_IDX                   = 5, // std::list<Param* >
    SBK_NATRONENGINE_STD_LIST_DOUBLE_IDX                     = 6, // std::list<double > *
    SBK_NATRONENGINE_STD_VECTOR_BOOL_IDX                     = 7, // const std::vector<bool > &
    SBK_NATRONENGINE_STD_PAIR_QSTRING_QSTRING_IDX            = 8, // std::pair<QString,QString >
    SBK_NATRONENGINE_STD_LIST_STD_PAIR_QSTRING_QSTRING_IDX   = 9, // const std::list<std::pair< QString,QString > > &
    SBK_NATRONENGINE_STD_LIST_IMAGELAYER_IDX                 = 10, // std::list<ImageLayer >
    SBK_NATRONENGINE_STD_VECTOR_DOUBLE_IDX                   = 11, // const std::vector<double > &
    SBK_NATRONENGINE_STD_VECTOR_INT_IDX                      = 12, // const std::vector<int > &
    SBK_NATRONENGINE_STD_LIST_ITEMBASEPTR_IDX                = 13, // std::list<ItemBase* >
    SBK_NATRONENGINE_STD_VECTOR_STD_STRING_IDX               = 14, // std::vector<std::string >
    SBK_NATRONENGINE_STD_LIST_STD_VECTOR_STD_STRING_IDX      = 15, // std::list<std::vector< std::string > > *
    SBK_NATRONENGINE_STD_VECTOR_RECTI_IDX                    = 16, // std::vector<RectI >
    SBK_NATRONENGINE_STD_LIST_TRACKPTR_IDX                   = 17, // std::list<Track* > *
    SBK_NATRONENGINE_QLIST_QVARIANT_IDX                      = 18, // QList<QVariant >
    SBK_NATRONENGINE_QLIST_QSTRING_IDX                       = 19, // QList<QString >
    SBK_NATRONENGINE_QMAP_QSTRING_QVARIANT_IDX               = 20, // QMap<QString,QVariant >
    SBK_NatronEngine_CONVERTERS_IDX_COUNT                    = 21
};
// Macros for type check

namespace Shiboken
{

// PyType functions, to get the PyObjectType for a type T
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::AnimatedParam >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::App >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_APP_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::AppSettings >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_APPSETTINGS_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::BezierCurve >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::BoolNodeCreationProperty >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_BOOLNODECREATIONPROPERTY_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::BooleanParam >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_BOOLEANPARAM_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::ButtonParam >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_BUTTONPARAM_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::ChoiceParam >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_CHOICEPARAM_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::ColorParam >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_COLORPARAM_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::ColorTuple >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_COLORTUPLE_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Double2DParam >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_DOUBLE2DPARAM_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Double2DTuple >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_DOUBLE2DTUPLE_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Double3DParam >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_DOUBLE3DPARAM_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Double3DTuple >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_DOUBLE3DTUPLE_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::DoubleParam >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_DOUBLEPARAM_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Effect >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_EFFECT_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::ExprUtils >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_EXPRUTILS_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::FileParam >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_FILEPARAM_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::FloatNodeCreationProperty >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_FLOATNODECREATIONPROPERTY_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Group >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_GROUP_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::GroupParam >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_GROUPPARAM_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::ImageLayer >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Int2DParam >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_INT2DPARAM_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Int2DTuple >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_INT2DTUPLE_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Int3DParam >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_INT3DPARAM_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Int3DTuple >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_INT3DTUPLE_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::IntNodeCreationProperty >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_INTNODECREATIONPROPERTY_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::IntParam >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_INTPARAM_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::ItemBase >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_ITEMBASE_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Layer >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_LAYER_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_ENUM::StatusEnum >() { return SbkNatronEngineTypes[SBK_NATRON_ENUM_STATUSENUM_IDX]; }
template<> inline PyTypeObject *SbkType<NATRON_ENUM::StandardButtonEnum >() { return SbkNatronEngineTypes[SBK_NATRON_ENUM_STANDARDBUTTONENUM_IDX]; }
template<> inline PyTypeObject *SbkType< ::QFlags<::NATRON_ENUM::StandardButtonEnum> >() { return SbkNatronEngineTypes[SBK_QFLAGS_NATRON_ENUM_STANDARDBUTTONENUM_IDX]; }
template<> inline PyTypeObject *SbkType<NATRON_ENUM::KeyframeTypeEnum >() { return SbkNatronEngineTypes[SBK_NATRON_ENUM_KEYFRAMETYPEENUM_IDX]; }
template<> inline PyTypeObject *SbkType<NATRON_ENUM::PixmapEnum >() { return SbkNatronEngineTypes[SBK_NATRON_ENUM_PIXMAPENUM_IDX]; }
template<> inline PyTypeObject *SbkType<NATRON_ENUM::ValueChangedReasonEnum >() { return SbkNatronEngineTypes[SBK_NATRON_ENUM_VALUECHANGEDREASONENUM_IDX]; }
template<> inline PyTypeObject *SbkType<NATRON_ENUM::AnimationLevelEnum >() { return SbkNatronEngineTypes[SBK_NATRON_ENUM_ANIMATIONLEVELENUM_IDX]; }
template<> inline PyTypeObject *SbkType<NATRON_ENUM::ImagePremultiplicationEnum >() { return SbkNatronEngineTypes[SBK_NATRON_ENUM_IMAGEPREMULTIPLICATIONENUM_IDX]; }
template<> inline PyTypeObject *SbkType<NATRON_ENUM::ViewerCompositingOperatorEnum >() { return SbkNatronEngineTypes[SBK_NATRON_ENUM_VIEWERCOMPOSITINGOPERATORENUM_IDX]; }
template<> inline PyTypeObject *SbkType<NATRON_ENUM::ViewerColorSpaceEnum >() { return SbkNatronEngineTypes[SBK_NATRON_ENUM_VIEWERCOLORSPACEENUM_IDX]; }
template<> inline PyTypeObject *SbkType<NATRON_ENUM::ImageBitDepthEnum >() { return SbkNatronEngineTypes[SBK_NATRON_ENUM_IMAGEBITDEPTHENUM_IDX]; }
template<> inline PyTypeObject *SbkType<NATRON_ENUM::OrientationEnum >() { return SbkNatronEngineTypes[SBK_NATRON_ENUM_ORIENTATIONENUM_IDX]; }
template<> inline PyTypeObject *SbkType<NATRON_ENUM::PlaybackModeEnum >() { return SbkNatronEngineTypes[SBK_NATRON_ENUM_PLAYBACKMODEENUM_IDX]; }
template<> inline PyTypeObject *SbkType<NATRON_ENUM::DisplayChannelsEnum >() { return SbkNatronEngineTypes[SBK_NATRON_ENUM_DISPLAYCHANNELSENUM_IDX]; }
template<> inline PyTypeObject *SbkType<NATRON_ENUM::MergingFunctionEnum >() { return SbkNatronEngineTypes[SBK_NATRON_ENUM_MERGINGFUNCTIONENUM_IDX]; }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::NodeCreationProperty >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_NODECREATIONPROPERTY_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::OutputFileParam >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_OUTPUTFILEPARAM_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::PageParam >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_PAGEPARAM_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Param >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_PARAM_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::ParametricParam >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_PARAMETRICPARAM_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::PathParam >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_PATHPARAM_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::PyCoreApplication >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_PYCOREAPPLICATION_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::RectD >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_RECTD_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::RectI >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_RECTI_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Roto >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_ROTO_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::SeparatorParam >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_SEPARATORPARAM_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::StringNodeCreationProperty >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_STRINGNODECREATIONPROPERTY_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::StringParam::TypeEnum >() { return SbkNatronEngineTypes[SBK_STRINGPARAM_TYPEENUM_IDX]; }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::StringParam >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_STRINGPARAM_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::StringParamBase >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_STRINGPARAMBASE_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Track >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_TRACK_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Tracker >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_TRACKER_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::UserParamHolder >() { return reinterpret_cast<PyTypeObject *>(SbkNatronEngineTypes[SBK_USERPARAMHOLDER_IDX]); }
QT_WARNING_POP

} // namespace Shiboken

#endif // SBK_NATRONENGINE_PYTHON_H

