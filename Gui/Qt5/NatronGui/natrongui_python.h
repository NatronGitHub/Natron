

#ifndef SBK_NATRONGUI_PYTHON_H
#define SBK_NATRONGUI_PYTHON_H

#include <sbkpython.h>
#include <sbkconverter.h>
// Module Includes
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
CLANG_DIAG_OFF(keyword-macro)
#include <pyside2_qtgui_python.h> // produces warnings
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)
CLANG_DIAG_ON(keyword-macro)
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
CLANG_DIAG_OFF(keyword-macro)
#include <pyside2_qtcore_python.h> // produces warnings
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)
CLANG_DIAG_ON(keyword-macro)
#include <pyside2_qtwidgets_python.h>
#include <natronengine_python.h>

// Bound library includes
#include <PythonPanels.h>
#include <PyGlobalGui.h>
#include <PyGuiApp.h>
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
    SBK_GUIAPP_IDX                                           = 0,
    SBK_PYGUIAPPLICATION_IDX                                 = 1,
    SBK_PYMODALDIALOG_IDX                                    = 2,
    SBK_PYPANEL_IDX                                          = 3,
    SBK_PYTABWIDGET_IDX                                      = 4,
    SBK_PYVIEWER_IDX                                         = 5,
    SBK_NatronGui_IDX_COUNT                                  = 6
};
// This variable stores all Python types exported by this module.
extern PyTypeObject **SbkNatronGuiTypes;

// This variable stores the Python module object exported by this module.
extern PyObject *SbkNatronGuiModuleObject;

// This variable stores all type converters exported by this module.
extern SbkConverter **SbkNatronGuiTypeConverters;

// Converter indices
enum : int {
    SBK_NATRONGUI_STD_MAP_QSTRING_NODECREATIONPROPERTYPTR_IDX = 0, // const std::map<QString,NodeCreationProperty* > &
    SBK_NATRONGUI_STD_LIST_EFFECTPTR_IDX                     = 1, // std::list<Effect* >
    SBK_NATRONGUI_STD_LIST_QSTRING_IDX                       = 2, // std::list<QString >
    SBK_NATRONGUI_STD_LIST_INT_IDX                           = 3, // const std::list<int > &
    SBK_NATRONGUI_STD_LIST_PARAMPTR_IDX                      = 4, // std::list<Param* >
    SBK_NATRONGUI_QLIST_QVARIANT_IDX                         = 5, // QList<QVariant >
    SBK_NATRONGUI_QLIST_QSTRING_IDX                          = 6, // QList<QString >
    SBK_NATRONGUI_QMAP_QSTRING_QVARIANT_IDX                  = 7, // QMap<QString,QVariant >
    SBK_NatronGui_CONVERTERS_IDX_COUNT                       = 8
};
// Macros for type check

namespace Shiboken
{

// PyType functions, to get the PyObjectType for a type T
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::GuiApp >() { return reinterpret_cast<PyTypeObject *>(SbkNatronGuiTypes[SBK_GUIAPP_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::PyGuiApplication >() { return reinterpret_cast<PyTypeObject *>(SbkNatronGuiTypes[SBK_PYGUIAPPLICATION_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::PyModalDialog >() { return reinterpret_cast<PyTypeObject *>(SbkNatronGuiTypes[SBK_PYMODALDIALOG_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::PyPanel >() { return reinterpret_cast<PyTypeObject *>(SbkNatronGuiTypes[SBK_PYPANEL_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::PyTabWidget >() { return reinterpret_cast<PyTypeObject *>(SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX]); }
template<> inline PyTypeObject *SbkType<NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::PyViewer >() { return reinterpret_cast<PyTypeObject *>(SbkNatronGuiTypes[SBK_PYVIEWER_IDX]); }
QT_WARNING_POP

} // namespace Shiboken

#endif // SBK_NATRONGUI_PYTHON_H

