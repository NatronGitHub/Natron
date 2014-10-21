

#ifndef SBK_NATRONENGINE_PYTHON_H
#define SBK_NATRONENGINE_PYTHON_H

//workaround to access protected functions
#define protected public

#include <sbkpython.h>
#include <conversions.h>
#include <sbkenum.h>
#include <basewrapper.h>
#include <bindingmanager.h>
#include <memory>

// Module Includes
#include <pyside_qtcore_python.h>

// Binded library includes
#include <Enums.h>
#include <GlobalDefines.h>
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
#define SBK_NATRON_IDX                                               0
#define SBK_NATRON_ORIENTATION_IDX                                   6
#define SBK_NATRON_PLAYBACKMODE_IDX                                  8
#define SBK_NATRON_VALUECHANGEDREASON_IDX                            10
#define SBK_NATRON_STANDARDBUTTON_IDX                                9
#define SBK_QFLAGS_NATRON_STANDARDBUTTON__IDX                        13
#define SBK_NATRON_IMAGEPREMULTIPLICATION_IDX                        4
#define SBK_NATRON_ANIMATIONLEVEL_IDX                                1
#define SBK_NATRON_KEYFRAMETYPE_IDX                                  5
#define SBK_NATRON_IMAGECOMPONENTS_IDX                               3
#define SBK_NATRON_VIEWERCOLORSPACE_IDX                              11
#define SBK_NATRON_IMAGEBITDEPTH_IDX                                 2
#define SBK_NATRON_VIEWERCOMPOSITINGOPERATOR_IDX                     12
#define SBK_NATRON_PIXMAPENUM_IDX                                    7
#define SBK_NatronEngine_IDX_COUNT                                   14

// This variable stores all Python types exported by this module.
extern PyTypeObject** SbkNatronEngineTypes;

// This variable stores all type converters exported by this module.
extern SbkConverter** SbkNatronEngineTypeConverters;

// Converter indices
#define SBK_STD_STRING_IDX                                           0
#define SBK_STD_SIZE_T_IDX                                           1
#define SBK_NATRONENGINE_STD_LIST_STD_STRING_IDX                     2 // std::list<std::string >
#define SBK_NATRONENGINE_QLIST_QVARIANT_IDX                          3 // QList<QVariant >
#define SBK_NATRONENGINE_QLIST_QSTRING_IDX                           4 // QList<QString >
#define SBK_NATRONENGINE_QMAP_QSTRING_QVARIANT_IDX                   5 // QMap<QString, QVariant >
#define SBK_NatronEngine_CONVERTERS_IDX_COUNT                        6

// Macros for type check

namespace Shiboken
{

// PyType functions, to get the PyObjectType for a type T
template<> inline PyTypeObject* SbkType< ::Natron::Orientation >() { return SbkNatronEngineTypes[SBK_NATRON_ORIENTATION_IDX]; }
template<> inline PyTypeObject* SbkType< ::Natron::PlaybackMode >() { return SbkNatronEngineTypes[SBK_NATRON_PLAYBACKMODE_IDX]; }
template<> inline PyTypeObject* SbkType< ::Natron::ValueChangedReason >() { return SbkNatronEngineTypes[SBK_NATRON_VALUECHANGEDREASON_IDX]; }
template<> inline PyTypeObject* SbkType< ::Natron::StandardButton >() { return SbkNatronEngineTypes[SBK_NATRON_STANDARDBUTTON_IDX]; }
template<> inline PyTypeObject* SbkType< ::QFlags<Natron::StandardButton> >() { return SbkNatronEngineTypes[SBK_QFLAGS_NATRON_STANDARDBUTTON__IDX]; }
template<> inline PyTypeObject* SbkType< ::Natron::ImagePremultiplication >() { return SbkNatronEngineTypes[SBK_NATRON_IMAGEPREMULTIPLICATION_IDX]; }
template<> inline PyTypeObject* SbkType< ::Natron::AnimationLevel >() { return SbkNatronEngineTypes[SBK_NATRON_ANIMATIONLEVEL_IDX]; }
template<> inline PyTypeObject* SbkType< ::Natron::KeyframeType >() { return SbkNatronEngineTypes[SBK_NATRON_KEYFRAMETYPE_IDX]; }
template<> inline PyTypeObject* SbkType< ::Natron::ImageComponents >() { return SbkNatronEngineTypes[SBK_NATRON_IMAGECOMPONENTS_IDX]; }
template<> inline PyTypeObject* SbkType< ::Natron::ViewerColorSpace >() { return SbkNatronEngineTypes[SBK_NATRON_VIEWERCOLORSPACE_IDX]; }
template<> inline PyTypeObject* SbkType< ::Natron::ImageBitDepth >() { return SbkNatronEngineTypes[SBK_NATRON_IMAGEBITDEPTH_IDX]; }
template<> inline PyTypeObject* SbkType< ::Natron::ViewerCompositingOperator >() { return SbkNatronEngineTypes[SBK_NATRON_VIEWERCOMPOSITINGOPERATOR_IDX]; }
template<> inline PyTypeObject* SbkType< ::Natron::PixmapEnum >() { return SbkNatronEngineTypes[SBK_NATRON_PIXMAPENUM_IDX]; }

} // namespace Shiboken

#endif // SBK_NATRONENGINE_PYTHON_H

