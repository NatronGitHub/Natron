

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
CLANG_DIAG_OFF(keyword-macro)
#include <pyside_qtcore_python.h> // produces warnings
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)
CLANG_DIAG_ON(keyword-macro)

// Binded library includes
#include <Enums.h>
#include <RectD.h>
#include <RectI.h>
#include <PyTracker.h>
#include <PyExprUtils.h>
#include <PyOverlayInteract.h>
#include <PyParameter.h>
#include <PyRoto.h>
#include <PyItemsTable.h>
#include <PyGlobalFunctions.h>
#include <PyAppInstance.h>
#include <PyNodeGroup.h>
#include <PyNode.h>
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
#define SBK_NATRON_NAMESPACE_IDX                                     30
#define SBK_NATRON_NAMESPACE_ACTIONRETCODEENUM_IDX                   31
#define SBK_NATRON_NAMESPACE_STANDARDBUTTONENUM_IDX                  41
#define SBK_QFLAGS_NATRON_NAMESPACE_STANDARDBUTTONENUM__IDX          55
#define SBK_NATRON_NAMESPACE_KEYFRAMETYPEENUM_IDX                    35
#define SBK_NATRON_NAMESPACE_PIXMAPENUM_IDX                          39
#define SBK_NATRON_NAMESPACE_VIEWERCONTEXTLAYOUTTYPEENUM_IDX         43
#define SBK_NATRON_NAMESPACE_ANIMATIONLEVELENUM_IDX                  32
#define SBK_NATRON_NAMESPACE_IMAGEPREMULTIPLICATIONENUM_IDX          34
#define SBK_NATRON_NAMESPACE_IMAGEBITDEPTHENUM_IDX                   33
#define SBK_NATRON_NAMESPACE_ORIENTATIONENUM_IDX                     37
#define SBK_NATRON_NAMESPACE_ROTOSTROKETYPE_IDX                      40
#define SBK_NATRON_NAMESPACE_PENTYPE_IDX                             38
#define SBK_NATRON_NAMESPACE_MERGINGFUNCTIONENUM_IDX                 36
#define SBK_NATRON_NAMESPACE_TABLECHANGEREASONENUM_IDX               42
#define SBK_RECTD_IDX                                                56
#define SBK_RECTI_IDX                                                57
#define SBK_COLORTUPLE_IDX                                           9
#define SBK_DOUBLE3DTUPLE_IDX                                        13
#define SBK_DOUBLE2DTUPLE_IDX                                        11
#define SBK_INT3DTUPLE_IDX                                           25
#define SBK_INT2DTUPLE_IDX                                           23
#define SBK_PARAM_IDX                                                46
#define SBK_PARAMETRICPARAM_IDX                                      47
#define SBK_PAGEPARAM_IDX                                            45
#define SBK_GROUPPARAM_IDX                                           20
#define SBK_SEPARATORPARAM_IDX                                       59
#define SBK_BUTTONPARAM_IDX                                          6
#define SBK_ANIMATEDPARAM_IDX                                        0
#define SBK_STRINGPARAMBASE_IDX                                      63
#define SBK_PATHPARAM_IDX                                            48
#define SBK_FILEPARAM_IDX                                            17
#define SBK_STRINGPARAM_IDX                                          61
#define SBK_STRINGPARAM_TYPEENUM_IDX                                 62
#define SBK_BOOLEANPARAM_IDX                                         5
#define SBK_CHOICEPARAM_IDX                                          7
#define SBK_COLORPARAM_IDX                                           8
#define SBK_DOUBLEPARAM_IDX                                          14
#define SBK_DOUBLE2DPARAM_IDX                                        10
#define SBK_DOUBLE3DPARAM_IDX                                        12
#define SBK_INTPARAM_IDX                                             27
#define SBK_INT2DPARAM_IDX                                           22
#define SBK_INT3DPARAM_IDX                                           24
#define SBK_PYOVERLAYINTERACT_IDX                                    51
#define SBK_PYCORNERPINOVERLAYINTERACT_IDX                           50
#define SBK_PYTRANSFORMOVERLAYINTERACT_IDX                           54
#define SBK_PYPOINTOVERLAYINTERACT_IDX                               53
#define SBK_PYOVERLAYPARAMDESC_IDX                                   52
#define SBK_USERPARAMHOLDER_IDX                                      68
#define SBK_IMAGELAYER_IDX                                           21
#define SBK_STROKEPOINT_IDX                                          65
#define SBK_ITEMSTABLE_IDX                                           29
#define SBK_TRACKER_IDX                                              67
#define SBK_ROTO_IDX                                                 58
#define SBK_ITEMBASE_IDX                                             28
#define SBK_TRACK_IDX                                                66
#define SBK_STROKEITEM_IDX                                           64
#define SBK_BEZIERCURVE_IDX                                          3
#define SBK_NODECREATIONPROPERTY_IDX                                 44
#define SBK_STRINGNODECREATIONPROPERTY_IDX                           60
#define SBK_FLOATNODECREATIONPROPERTY_IDX                            18
#define SBK_BOOLNODECREATIONPROPERTY_IDX                             4
#define SBK_INTNODECREATIONPROPERTY_IDX                              26
#define SBK_APPSETTINGS_IDX                                          2
#define SBK_GROUP_IDX                                                19
#define SBK_EFFECT_IDX                                               15
#define SBK_APP_IDX                                                  1
#define SBK_PYCOREAPPLICATION_IDX                                    49
#define SBK_EXPRUTILS_IDX                                            16
#define SBK_NatronEngine_IDX_COUNT                                   69

// This variable stores all Python types exported by this module.
extern PyTypeObject** SbkNatronEngineTypes;

// This variable stores all type converters exported by this module.
extern SbkConverter** SbkNatronEngineTypeConverters;

// Converter indices
#define SBK_STD_SIZE_T_IDX                                           0
#define SBK_NATRONENGINE_STD_LIST_RECTI_IDX                          1 // std::list<RectI >
#define SBK_NATRONENGINE_STD_VECTOR_STD_STRING_IDX                   2 // std::vector<std::string >
#define SBK_NATRONENGINE_STD_LIST_STD_VECTOR_STD_STRING_IDX          3 // std::list<std::vector<std::string > > *
#define SBK_NATRONENGINE_STD_LIST_QSTRING_IDX                        4 // std::list<QString > *
#define SBK_NATRONENGINE_STD_PAIR_QSTRING_QSTRING_IDX                5 // std::pair<QString, QString >
#define SBK_NATRONENGINE_STD_LIST_STD_PAIR_QSTRING_QSTRING_IDX       6 // const std::list<std::pair<QString, QString > > &
#define SBK_NATRONENGINE_STD_MAP_QSTRING_PYOVERLAYPARAMDESC_IDX      7 // std::map<QString, PyOverlayParamDesc >
#define SBK_NATRONENGINE_STD_MAP_QSTRING_QSTRING_IDX                 8 // const std::map<QString, QString > &
#define SBK_NATRONENGINE_STD_LIST_ITEMBASEPTR_IDX                    9 // std::list<ItemBase * >
#define SBK_NATRONENGINE_STD_LIST_TRACKPTR_IDX                       10 // const std::list<Track * > &
#define SBK_NATRONENGINE_STD_LIST_PARAMPTR_IDX                       11 // std::list<Param * >
#define SBK_NATRONENGINE_STD_LIST_STROKEPOINT_IDX                    12 // std::list<StrokePoint >
#define SBK_NATRONENGINE_STD_LIST_STD_LIST_STROKEPOINT_IDX           13 // std::list<std::list<StrokePoint > >
#define SBK_NATRONENGINE_STD_VECTOR_DOUBLE_IDX                       14 // const std::vector<double > &
#define SBK_NATRONENGINE_STD_VECTOR_BOOL_IDX                         15 // const std::vector<bool > &
#define SBK_NATRONENGINE_STD_VECTOR_INT_IDX                          16 // const std::vector<int > &
#define SBK_NATRONENGINE_STD_LIST_EFFECTPTR_IDX                      17 // std::list<Effect * >
#define SBK_NATRONENGINE_STD_LIST_ITEMSTABLEPTR_IDX                  18 // std::list<ItemsTable * >
#define SBK_NATRONENGINE_STD_LIST_IMAGELAYER_IDX                     19 // std::list<ImageLayer >
#define SBK_NATRONENGINE_STD_MAP_QSTRING_NODECREATIONPROPERTYPTR_IDX 20 // const std::map<QString, NodeCreationProperty * > &
#define SBK_NATRONENGINE_STD_LIST_INT_IDX                            21 // const std::list<int > &
#define SBK_NATRONENGINE_QLIST_QVARIANT_IDX                          22 // QList<QVariant >
#define SBK_NATRONENGINE_QLIST_QSTRING_IDX                           23 // QList<QString >
#define SBK_NATRONENGINE_QMAP_QSTRING_QVARIANT_IDX                   24 // QMap<QString, QVariant >
#define SBK_NatronEngine_CONVERTERS_IDX_COUNT                        25

// Macros for type check

namespace Shiboken
{

// PyType functions, to get the PyObjectType for a type T
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::ActionRetCodeEnum >() { return SbkNatronEngineTypes[SBK_NATRON_NAMESPACE_ACTIONRETCODEENUM_IDX]; }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::StandardButtonEnum >() { return SbkNatronEngineTypes[SBK_NATRON_NAMESPACE_STANDARDBUTTONENUM_IDX]; }
template<> inline PyTypeObject* SbkType< ::QFlags<NATRON_NAMESPACE::StandardButtonEnum> >() { return SbkNatronEngineTypes[SBK_QFLAGS_NATRON_NAMESPACE_STANDARDBUTTONENUM__IDX]; }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::KeyframeTypeEnum >() { return SbkNatronEngineTypes[SBK_NATRON_NAMESPACE_KEYFRAMETYPEENUM_IDX]; }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::PixmapEnum >() { return SbkNatronEngineTypes[SBK_NATRON_NAMESPACE_PIXMAPENUM_IDX]; }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::ViewerContextLayoutTypeEnum >() { return SbkNatronEngineTypes[SBK_NATRON_NAMESPACE_VIEWERCONTEXTLAYOUTTYPEENUM_IDX]; }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::AnimationLevelEnum >() { return SbkNatronEngineTypes[SBK_NATRON_NAMESPACE_ANIMATIONLEVELENUM_IDX]; }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::ImagePremultiplicationEnum >() { return SbkNatronEngineTypes[SBK_NATRON_NAMESPACE_IMAGEPREMULTIPLICATIONENUM_IDX]; }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::ImageBitDepthEnum >() { return SbkNatronEngineTypes[SBK_NATRON_NAMESPACE_IMAGEBITDEPTHENUM_IDX]; }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::OrientationEnum >() { return SbkNatronEngineTypes[SBK_NATRON_NAMESPACE_ORIENTATIONENUM_IDX]; }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::RotoStrokeType >() { return SbkNatronEngineTypes[SBK_NATRON_NAMESPACE_ROTOSTROKETYPE_IDX]; }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::PenType >() { return SbkNatronEngineTypes[SBK_NATRON_NAMESPACE_PENTYPE_IDX]; }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::MergingFunctionEnum >() { return SbkNatronEngineTypes[SBK_NATRON_NAMESPACE_MERGINGFUNCTIONENUM_IDX]; }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::TableChangeReasonEnum >() { return SbkNatronEngineTypes[SBK_NATRON_NAMESPACE_TABLECHANGEREASONENUM_IDX]; }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::RectD >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_RECTD_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::RectI >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_RECTI_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::ColorTuple >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_COLORTUPLE_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Double3DTuple >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_DOUBLE3DTUPLE_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Double2DTuple >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_DOUBLE2DTUPLE_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Int3DTuple >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_INT3DTUPLE_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Int2DTuple >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_INT2DTUPLE_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Param >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_PARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::ParametricParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_PARAMETRICPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::PageParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_PAGEPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::GroupParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_GROUPPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::SeparatorParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_SEPARATORPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::ButtonParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_BUTTONPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::AnimatedParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::StringParamBase >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_STRINGPARAMBASE_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::PathParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_PATHPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::FileParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_FILEPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::StringParam::TypeEnum >() { return SbkNatronEngineTypes[SBK_STRINGPARAM_TYPEENUM_IDX]; }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::StringParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_STRINGPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::BooleanParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_BOOLEANPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::ChoiceParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_CHOICEPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::ColorParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_COLORPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::DoubleParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_DOUBLEPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Double2DParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_DOUBLE2DPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Double3DParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_DOUBLE3DPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::IntParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_INTPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Int2DParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_INT2DPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Int3DParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_INT3DPARAM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::PyOverlayInteract >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_PYOVERLAYINTERACT_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::PyCornerPinOverlayInteract >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_PYCORNERPINOVERLAYINTERACT_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::PyTransformOverlayInteract >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_PYTRANSFORMOVERLAYINTERACT_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::PyPointOverlayInteract >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_PYPOINTOVERLAYINTERACT_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::PyOverlayParamDesc >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_PYOVERLAYPARAMDESC_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::UserParamHolder >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_USERPARAMHOLDER_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::ImageLayer >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::StrokePoint >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_STROKEPOINT_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::ItemsTable >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_ITEMSTABLE_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Tracker >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_TRACKER_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Roto >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_ROTO_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::ItemBase >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_ITEMBASE_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Track >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_TRACK_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::StrokeItem >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_STROKEITEM_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::BezierCurve >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::NodeCreationProperty >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_NODECREATIONPROPERTY_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::StringNodeCreationProperty >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_STRINGNODECREATIONPROPERTY_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::FloatNodeCreationProperty >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_FLOATNODECREATIONPROPERTY_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::BoolNodeCreationProperty >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_BOOLNODECREATIONPROPERTY_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::IntNodeCreationProperty >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_INTNODECREATIONPROPERTY_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::AppSettings >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_APPSETTINGS_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Group >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_GROUP_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Effect >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_EFFECT_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::App >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_APP_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::PyCoreApplication >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_PYCOREAPPLICATION_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::ExprUtils >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_EXPRUTILS_IDX]); }

} // namespace Shiboken

#endif // SBK_NATRONENGINE_PYTHON_H

