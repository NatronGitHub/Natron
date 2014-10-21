

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
#include <QQueue>
#include <set>
#include <map>
#include <utility>
#include <list>
#include <QLinkedList>

// Type indices
#define                                   SBK_NatronEngine_IDX_COUNT 2

// This variable stores all Python types exported by this module.
extern PyTypeObject** SbkNatronEngineTypes;

// This variable stores all type converters exported by this module.
extern SbkConverter** SbkNatronEngineTypeConverters;

// Converter indices
#define                          SBK_NATRONENGINE_QLIST_QVARIANT_IDX 0 // QList<QVariant >
#define                           SBK_NATRONENGINE_QLIST_QSTRING_IDX 1 // QList<QString >
#define                   SBK_NATRONENGINE_QMAP_QSTRING_QVARIANT_IDX 2 // QMap<QString, QVariant >
#define                        SBK_NatronEngine_CONVERTERS_IDX_COUNT 3

// Macros for type check

namespace Shiboken
{

// PyType functions, to get the PyObjectType for a type T

} // namespace Shiboken

#endif // SBK_NATRONENGINE_PYTHON_H

