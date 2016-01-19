

#ifndef SBK_NATRONGUI_PYTHON_H
#define SBK_NATRONGUI_PYTHON_H

#include <sbkpython.h>
#include <conversions.h>
#include <sbkenum.h>
#include <basewrapper.h>
#include <bindingmanager.h>
#include <memory>

#include <pysidesignal.h>
// Module Includes
#include <pyside_qtgui_python.h>
#include <pyside_qtcore_python.h>
#include <natronengine_python.h>

// Binded library includes
#include <GuiAppWrapper.h>
#include <PythonPanels.h>
#include <GlobalGuiWrapper.h>
// Conversion Includes - Primitive Types
#include <QStringList>
#include <qabstractitemmodel.h>
#include <QString>
#include <signalmanager.h>
#include <typeresolver.h>
#include <QtConcurrentFilter>

// Conversion Includes - Container Types
#include <QMap>
#include <QStack>
#include <QLinkedList>
#include <QVector>
#include <set>
#include <QSet>
#include <map>
#include <vector>
#include <list>
#include <QPair>
#include <pysideconversions.h>
#include <map>
#include <QQueue>
#include <QList>
#include <utility>
#include <QMultiMap>

// Type indices
#define SBK_PYTABWIDGET_IDX                                          4
#define SBK_PYVIEWER_IDX                                             5
#define SBK_GUIAPP_IDX                                               0
#define SBK_PYGUIAPPLICATION_IDX                                     1
#define SBK_PYPANEL_IDX                                              3
#define SBK_PYMODALDIALOG_IDX                                        2
#define SBK_NatronGui_IDX_COUNT                                      6

// This variable stores all Python types exported by this module.
extern PyTypeObject** SbkNatronGuiTypes;

// This variable stores all type converters exported by this module.
extern SbkConverter** SbkNatronGuiTypeConverters;

// Converter indices
#define SBK_NATRONGUI_STD_LIST_EFFECTPTR_IDX                         0 // std::list<Effect * >
#define SBK_NATRONGUI_STD_VECTOR_STD_STRING_IDX                      1 // const std::vector<std::string > &
#define SBK_NATRONGUI_STD_LIST_INT_IDX                               2 // const std::list<int > &
#define SBK_NATRONGUI_STD_LIST_STD_STRING_IDX                        3 // std::list<std::string >
#define SBK_NATRONGUI_QLIST_QACTIONPTR_IDX                           4 // QList<QAction * >
#define SBK_NATRONGUI_QLIST_QOBJECTPTR_IDX                           5 // const QList<QObject * > &
#define SBK_NATRONGUI_QLIST_QBYTEARRAY_IDX                           6 // QList<QByteArray >
#define SBK_NATRONGUI_STD_LIST_PARAMPTR_IDX                          7 // std::list<Param * >
#define SBK_NATRONGUI_QLIST_QVARIANT_IDX                             8 // QList<QVariant >
#define SBK_NATRONGUI_QLIST_QSTRING_IDX                              9 // QList<QString >
#define SBK_NATRONGUI_QMAP_QSTRING_QVARIANT_IDX                      10 // QMap<QString, QVariant >
#define SBK_NatronGui_CONVERTERS_IDX_COUNT                           11

// Macros for type check

namespace Shiboken
{

// PyType functions, to get the PyObjectType for a type T
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::PyTabWidget >() { return reinterpret_cast<PyTypeObject*>(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::PyViewer >() { return reinterpret_cast<PyTypeObject*>(SbkNatronGuiTypes[SBK_PYVIEWER_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::GuiApp >() { return reinterpret_cast<PyTypeObject*>(SbkNatronGuiTypes[SBK_GUIAPP_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::PyGuiApplication >() { return reinterpret_cast<PyTypeObject*>(SbkNatronGuiTypes[SBK_PYGUIAPPLICATION_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::PyPanel >() { return reinterpret_cast<PyTypeObject*>(SbkNatronGuiTypes[SBK_PYPANEL_IDX]); }
template<> inline PyTypeObject* SbkType<NATRON_NAMESPACE::PyModalDialog >() { return reinterpret_cast<PyTypeObject*>(SbkNatronGuiTypes[SBK_PYMODALDIALOG_IDX]); }

} // namespace Shiboken

#endif // SBK_NATRONGUI_PYTHON_H

