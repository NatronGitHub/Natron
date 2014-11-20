

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
#include <NodeWrapper.h>
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
#define SBK_NATRON_IDX                                               19
#define SBK_COLORTUPLE_IDX                                           6
#define SBK_DOUBLE3DTUPLE_IDX                                        10
#define SBK_DOUBLE2DTUPLE_IDX                                        8
#define SBK_INT3DTUPLE_IDX                                           17
#define SBK_INT2DTUPLE_IDX                                           15
#define SBK_PARAM_IDX                                                21
#define SBK_PARAMETRICPARAM_IDX                                      22
#define SBK_BUTTONPARAM_IDX                                          3
#define SBK_ANIMATEDPARAM_IDX                                        0
#define SBK_STRINGPARAMBASE_IDX                                      26
#define SBK_PATHPARAM_IDX                                            23
#define SBK_OUTPUTFILEPARAM_IDX                                      20
#define SBK_FILEPARAM_IDX                                            13
#define SBK_STRINGPARAM_IDX                                          25
#define SBK_BOOLEANPARAM_IDX                                         2
#define SBK_CHOICEPARAM_IDX                                          4
#define SBK_COLORPARAM_IDX                                           5
#define SBK_DOUBLEPARAM_IDX                                          11
#define SBK_DOUBLE2DPARAM_IDX                                        7
#define SBK_DOUBLE3DPARAM_IDX                                        9
#define SBK_INTPARAM_IDX                                             18
#define SBK_INT2DPARAM_IDX                                           14
#define SBK_INT3DPARAM_IDX                                           16
#define SBK_EFFECT_IDX                                               12
#define SBK_APP_IDX                                                  1
#define SBK_NatronEngine_IDX_COUNT                                   27

// This variable stores all Python types exported by this module.
extern PyTypeObject** SbkNatronEngineTypes;

// This variable stores all type converters exported by this module.
extern SbkConverter** SbkNatronEngineTypeConverters;

// Converter indices
#define SBK_STD_SIZE_T_IDX                                           0
#define SBK_NATRONENGINE_STD_LIST_STD_STRING_IDX                     1 // std::list<std::string >
#define SBK_NATRONENGINE_STD_LIST_PARAMPTR_IDX                       2 // std::list<Param * >
#define SBK_NATRONENGINE_STD_LIST_EFFECTPTR_IDX                      3 // std::list<Effect * >
#define SBK_NATRONENGINE_QLIST_QVARIANT_IDX                          4 // QList<QVariant >
#define SBK_NATRONENGINE_QLIST_QSTRING_IDX                           5 // QList<QString >
#define SBK_NATRONENGINE_QMAP_QSTRING_QVARIANT_IDX                   6 // QMap<QString, QVariant >
#define SBK_NatronEngine_CONVERTERS_IDX_COUNT                        7

// Macros for type check

namespace Shiboken
{

// PyType functions, to get the PyObjectType for a type T
template<> inline PyTypeObject* SbkType< ::ColorTuple >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_COLORTUPLE_IDX]); }
template<> inline PyTypeObject* SbkType< ::Double3DTuple >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_DOUBLE3DTUPLE_IDX]); }
template<> inline PyTypeObject* SbkType< ::Double2DTuple >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_DOUBLE2DTUPLE_IDX]); }
template<> inline PyTypeObject* SbkType< ::Int3DTuple >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_INT3DTUPLE_IDX]); }
template<> inline PyTypeObject* SbkType< ::Int2DTuple >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_INT2DTUPLE_IDX]); }
template<> inline PyTypeObject* SbkType< ::Param >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_PARAM_IDX]); }
template<> inline PyTypeObject* SbkType< ::ParametricParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_PARAMETRICPARAM_IDX]); }
template<> inline PyTypeObject* SbkType< ::ButtonParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_BUTTONPARAM_IDX]); }
template<> inline PyTypeObject* SbkType< ::AnimatedParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX]); }
template<> inline PyTypeObject* SbkType< ::StringParamBase >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_STRINGPARAMBASE_IDX]); }
template<> inline PyTypeObject* SbkType< ::PathParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_PATHPARAM_IDX]); }
template<> inline PyTypeObject* SbkType< ::OutputFileParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_OUTPUTFILEPARAM_IDX]); }
template<> inline PyTypeObject* SbkType< ::FileParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_FILEPARAM_IDX]); }
template<> inline PyTypeObject* SbkType< ::StringParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_STRINGPARAM_IDX]); }
template<> inline PyTypeObject* SbkType< ::BooleanParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_BOOLEANPARAM_IDX]); }
template<> inline PyTypeObject* SbkType< ::ChoiceParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_CHOICEPARAM_IDX]); }
template<> inline PyTypeObject* SbkType< ::ColorParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_COLORPARAM_IDX]); }
template<> inline PyTypeObject* SbkType< ::DoubleParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_DOUBLEPARAM_IDX]); }
template<> inline PyTypeObject* SbkType< ::Double2DParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_DOUBLE2DPARAM_IDX]); }
template<> inline PyTypeObject* SbkType< ::Double3DParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_DOUBLE3DPARAM_IDX]); }
template<> inline PyTypeObject* SbkType< ::IntParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_INTPARAM_IDX]); }
template<> inline PyTypeObject* SbkType< ::Int2DParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_INT2DPARAM_IDX]); }
template<> inline PyTypeObject* SbkType< ::Int3DParam >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_INT3DPARAM_IDX]); }
template<> inline PyTypeObject* SbkType< ::Effect >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_EFFECT_IDX]); }
template<> inline PyTypeObject* SbkType< ::App >() { return reinterpret_cast<PyTypeObject*>(SbkNatronEngineTypes[SBK_APP_IDX]); }

} // namespace Shiboken

#endif // SBK_NATRONENGINE_PYTHON_H

