

#ifndef SBK_NATRONGUI_PYTHON_H
#define SBK_NATRONGUI_PYTHON_H

//workaround to access protected functions
#define protected public

#include <sbkpython.h>
#include <conversions.h>
#include <sbkenum.h>
#include <basewrapper.h>
#include <bindingmanager.h>
#include <memory>

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
#define SBK_PYGUIAPPLICATION_IDX                                     1
#define SBK_GUIAPP_IDX                                               0
#define SBK_PYMODALDIALOG_IDX                                        2
#define SBK_NatronGui_IDX_COUNT                                      3

// This variable stores all Python types exported by this module.
extern PyTypeObject** SbkNatronGuiTypes;

// This variable stores all type converters exported by this module.
extern SbkConverter** SbkNatronGuiTypeConverters;

// Converter indices
#define SBK_NATRONGUI_STD_LIST_STD_STRING_IDX                        0 // std::list<std::string >
#define SBK_NATRONGUI_STD_LIST_EFFECTPTR_IDX                         1 // std::list<Effect * >
#define SBK_NATRONGUI_STD_LIST_RENDERTASK_IDX                        2 // const std::list<RenderTask > &
#define SBK_NATRONGUI_QLIST_QACTIONPTR_IDX                           3 // QList<QAction * >
#define SBK_NATRONGUI_QLIST_QVARIANT_IDX                             4 // QList<QVariant >
#define SBK_NATRONGUI_QLIST_QSTRING_IDX                              5 // QList<QString >
#define SBK_NATRONGUI_QMAP_QSTRING_QVARIANT_IDX                      6 // QMap<QString, QVariant >
#define SBK_NatronGui_CONVERTERS_IDX_COUNT                           7

// Macros for type check

namespace Shiboken
{

// PyType functions, to get the PyObjectType for a type T
template<> inline PyTypeObject* SbkType< ::PyGuiApplication >() { return reinterpret_cast<PyTypeObject*>(SbkNatronGuiTypes[SBK_PYGUIAPPLICATION_IDX]); }
template<> inline PyTypeObject* SbkType< ::GuiApp >() { return reinterpret_cast<PyTypeObject*>(SbkNatronGuiTypes[SBK_GUIAPP_IDX]); }
template<> inline PyTypeObject* SbkType< ::PyModalDialog >() { return reinterpret_cast<PyTypeObject*>(SbkNatronGuiTypes[SBK_PYMODALDIALOG_IDX]); }

} // namespace Shiboken

#endif // SBK_NATRONGUI_PYTHON_H

