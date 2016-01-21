

#ifndef SBK_NATRONENGINE_PYTHON_H
#define SBK_NATRONENGINE_PYTHON_H

#include <sbkpython.h>
#include <conversions.h>
#include <sbkenum.h>
#include <basewrapper.h>
#include <bindingmanager.h>
#include <memory>

#include <pysidesignal.h>
// Module Includes
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <pyside_qtcore_python.h> // produces warnings
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

// Binded library includes
#include <Enums.h>
#include <RectD.h>
#include <NodeWrapper.h>
#include <RectI.h>
#include <NodeGroupWrapper.h>
#include <GlobalFunctionsWrapper.h>
#include <RotoWrapper.h>
#include <AppInstanceWrapper.h>
#include <ParameterWrapper.h>
// Conversion Includes - Primitive Types
#include <QString>
#include <signalmanager.h>
#include <typeresolver.h>
#include <QtConcurrentFilter>
#include <QStringList>
#include <qabstractitemmodel.h>

// Conversion Includes - Container Types
#include <QList>
#include <QMap>
#include <QStack>
#include <QMultiMap>
#include <map>
#include <QVector>
#include <QPair>
#include <pysideconversions.h>
#include <QSet>
#include <vector>
#include <QQueue>
#include <map>
#include <utility>
#include <list>
#include <QLinkedList>
#include <set>

// Type indices
#define SBK_NATRON_NAMESPACE_IDX                                     27
#define SBK_NATRON_NAMESPACE_STANDARDBUTTONENUM_IDX                  37
#define SBK_QFLAGS_NATRON_NAMESPACE_STANDARDBUTTONENUM__IDX          48
#define SBK_NATRON_NAMESPACE_IMAGECOMPONENTSENUM_IDX                 31
#define SBK_NATRON_NAMESPACE_IMAGEBITDEPTHENUM_IDX                   30
#define SBK_NATRON_NAMESPACE_KEYFRAMETYPEENUM_IDX                    33
#define SBK_NATRON_NAMESPACE_VALUECHANGEDREASONENUM_IDX              39
#define SBK_NATRON_NAMESPACE_ANIMATIONLEVELENUM_IDX                  28
#define SBK_NATRON_NAMESPACE_ORIENTATIONENUM_IDX                     34
#define SBK_NATRON_NAMESPACE_DISPLAYCHANNELSENUM_IDX                 29
#define SBK_NATRON_NAMESPACE_IMAGEPREMULTIPLICATIONENUM_IDX          32
#define SBK_NATRON_NAMESPACE_STATUSENUM_IDX                          38
#define SBK_NATRON_NAMESPACE_VIEWERCOMPOSITINGOPERATORENUM_IDX       41
#define SBK_NATRON_NAMESPACE_PLAYBACKMODEENUM_IDX                    36
#define SBK_NATRON_NAMESPACE_PIXMAPENUM_IDX                          35
#define SBK_NATRON_NAMESPACE_VIEWERCOLORSPACEENUM_IDX                40
#define SBK_RECTD_IDX                                                49
#define SBK_RECTI_IDX                                                50
#define SBK_COLORTUPLE_IDX                                           9
#define SBK_DOUBLE3DTUPLE_IDX                                        13
#define SBK_DOUBLE2DTUPLE_IDX                                        11
#define SBK_INT3DTUPLE_IDX                                           23
#define SBK_INT2DTUPLE_IDX                                           21
#define SBK_PARAM_IDX                                                44
#define SBK_PARAMETRICPARAM_IDX                                      45
#define SBK_PAGEPARAM_IDX                                            43
#define SBK_GROUPPARAM_IDX                                           18
#define SBK_SEPARATORPARAM_IDX                                       52
#define SBK_BUTTONPARAM_IDX                                          6
#define SBK_ANIMATEDPARAM_IDX                                        0
#define SBK_CHOICEPARAM_IDX                                          7
#define SBK_COLORPARAM_IDX                                           8
#define SBK_DOUBLEPARAM_IDX                                          14
#define SBK_DOUBLE2DPARAM_IDX                                        10
#define SBK_DOUBLE3DPARAM_IDX                                        12
#define SBK_INTPARAM_IDX                                             24
#define SBK_INT2DPARAM_IDX                                           20
#define SBK_INT3DPARAM_IDX                                           22
#define SBK_STRINGPARAMBASE_IDX                                      55
#define SBK_PATHPARAM_IDX                                            46
#define SBK_OUTPUTFILEPARAM_IDX                                      42
#define SBK_FILEPARAM_IDX                                            16
#define SBK_STRINGPARAM_IDX                                          53
#define SBK_STRINGPARAM_TYPEENUM_IDX                                 54
#define SBK_BOOLEANPARAM_IDX                                         5
#define SBK_USERPARAMHOLDER_IDX                                      56
#define SBK_IMAGELAYER_IDX                                           19
#define SBK_ROTO_IDX                                                 51
#define SBK_ITEMBASE_IDX                                             25
#define SBK_BEZIERCURVE_IDX                                          3
#define SBK_BEZIERCURVE_CAIROOPERATORENUM_IDX                        4
#define SBK_LAYER_IDX                                                26
#define SBK_APPSETTINGS_IDX                                          2
#define SBK_GROUP_IDX                                                17
#define SBK_APP_IDX                                                  1
#define SBK_EFFECT_IDX                                               15
#define SBK_PYCOREAPPLICATION_IDX                                    47
#define SBK_NatronEngine_IDX_COUNT                                   57

// This variable stores all Python types exported by this module.
extern PyTypeObject** SbkNatronEngineTypes;

// This variable stores all type converters exported by this module.
extern SbkConverter** SbkNatronEngineTypeConverters;

// Converter indices
#define SBK_STD_SIZE_T_IDX                                           0
#define SBK_NATRONENGINE_STD_VECTOR_RECTI_IDX                        1 // std::vector<RectI >
#define SBK_NATRONENGINE_STD_VECTOR_STD_STRING_IDX                   2 // std::vector<std::string >
#define SBK_NATRONENGINE_STD_PAIR_STD_STRING_STD_STRING_IDX          3 // std::pair<std::string, std::string >
#define SBK_NATRONENGINE_STD_LIST_STD_PAIR_STD_STRING_STD_STRING_IDX 4 // const std::list<std::pair<std::string, std::string > > &
#define SBK_NATRONENGINE_STD_LIST_ITEMBASEPTR_IDX                    5 // std::list<ItemBase * >
#define SBK_NATRONENGINE_STD_LIST_PARAMPTR_IDX                       6 // std::list<Param * >
#define SBK_NATRONENGINE_STD_LIST_EFFECTPTR_IDX                      7 // std::list<Effect * >
#define SBK_NATRONENGINE_STD_LIST_INT_IDX                            8 // const std::list<int > &
#define SBK_NATRONENGINE_STD_MAP_IMAGELAYER_EFFECTPTR_IDX            9 // std::map<ImageLayer, Effect * >
#define SBK_NATRONENGINE_STD_LIST_STD_STRING_IDX                     10 // const std::list<std::string > &
#define SBK_NATRONENGINE_QLIST_QVARIANT_IDX                          11 // QList<QVariant >
#define SBK_NATRONENGINE_QLIST_QSTRING_IDX                           12 // QList<QString >
#define SBK_NATRONENGINE_QMAP_QSTRING_QVARIANT_IDX                   13 // QMap<QString, QVariant >
#define SBK_NatronEngine_CONVERTERS_IDX_COUNT                        14

// Macros for type check

namespace Shiboken
{

// PyType functions, to get the PyObjectType for a type T
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::StandardButtonEnum >() { return SbkNatronEngineTypes[SBK_NATRON_NAMESPACE_STANDARDBUTTONENUM_IDX]; }
template<> inline PyTypeObject* SbkType< ::QFlags<NATRON_NAMESPACE::StandardButtonEnum> >() { return SbkNatronEngineTypes[SBK_QFLAGS_NATRON_NAMESPACE_STANDARDBUTTONENUM__IDX]; }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::ImageComponentsEnum >() { return SbkNatronEngineTypes[SBK_NATRON_NAMESPACE_IMAGECOMPONENTSENUM_IDX]; }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::ImageBitDepthEnum >() { return SbkNatronEngineTypes[SBK_NATRON_NAMESPACE_IMAGEBITDEPTHENUM_IDX]; }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::KeyframeTypeEnum >() { return SbkNatronEngineTypes[SBK_NATRON_NAMESPACE_KEYFRAMETYPEENUM_IDX]; }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::ValueChangedReasonEnum >() { return SbkNatronEngineTypes[SBK_NATRON_NAMESPACE_VALUECHANGEDREASONENUM_IDX]; }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::AnimationLevelEnum >() { return SbkNatronEngineTypes[SBK_NATRON_NAMESPACE_ANIMATIONLEVELENUM_IDX]; }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::OrientationEnum >() { return SbkNatronEngineTypes[SBK_NATRON_NAMESPACE_ORIENTATIONENUM_IDX]; }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::DisplayChannelsEnum >() { return SbkNatronEngineTypes[SBK_NATRON_NAMESPACE_DISPLAYCHANNELSENUM_IDX]; }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::ImagePremultiplicationEnum >() { return SbkNatronEngineTypes[SBK_NATRON_NAMESPACE_IMAGEPREMULTIPLICATIONENUM_IDX]; }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::StatusEnum >() { return SbkNatronEngineTypes[SBK_NATRON_NAMESPACE_STATUSENUM_IDX]; }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::ViewerCompositingOperatorEnum >() { return SbkNatronEngineTypes[SBK_NATRON_NAMESPACE_VIEWERCOMPOSITINGOPERATORENUM_IDX]; }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::PlaybackModeEnum >() { return SbkNatronEngineTypes[SBK_NATRON_NAMESPACE_PLAYBACKMODEENUM_IDX]; }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::PixmapEnum >() { return SbkNatronEngineTypes[SBK_NATRON_NAMESPACE_PIXMAPENUM_IDX]; }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::ViewerColorSpaceEnum >() { return SbkNatronEngineTypes[SBK_NATRON_NAMESPACE_VIEWERCOLORSPACEENUM_IDX]; }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::RectD >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_RECTD_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::RectI >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_RECTI_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::ColorTuple >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_COLORTUPLE_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::Double3DTuple >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_DOUBLE3DTUPLE_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::Double2DTuple >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_DOUBLE2DTUPLE_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::Int3DTuple >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_INT3DTUPLE_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::Int2DTuple >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_INT2DTUPLE_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::Param >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_PARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::ParametricParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_PARAMETRICPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::PageParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_PAGEPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::GroupParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_GROUPPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::SeparatorParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_SEPARATORPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::ButtonParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_BUTTONPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::AnimatedParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::ChoiceParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_CHOICEPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::ColorParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_COLORPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::DoubleParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_DOUBLEPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::Double2DParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_DOUBLE2DPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::Double3DParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_DOUBLE3DPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::IntParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_INTPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::Int2DParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_INT2DPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::Int3DParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_INT3DPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::StringParamBase >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_STRINGPARAMBASE_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::PathParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_PATHPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::OutputFileParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_OUTPUTFILEPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::FileParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_FILEPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::StringParam::TypeEnum >() { return SbkNatronEngineTypes[SBK_STRINGPARAM_TYPEENUM_IDX]; }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::StringParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_STRINGPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::BooleanParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_BOOLEANPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::UserParamHolder >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_USERPARAMHOLDER_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::ImageLayer >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::Roto >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_ROTO_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::ItemBase >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_ITEMBASE_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::BezierCurve::CairoOperatorEnum >() { return SbkNatronEngineTypes[SBK_BEZIERCURVE_CAIROOPERATORENUM_IDX]; }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::BezierCurve >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::Layer >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_LAYER_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::AppSettings >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_APPSETTINGS_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::Group >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_GROUP_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::App >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_APP_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::Effect >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_EFFECT_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::PyCoreApplication >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_PYCOREAPPLICATION_IDX]); }

} // namespace Shiboken

#endif // SBK_NATRONENGINE_PYTHON_H

