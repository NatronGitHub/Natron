#ifndef SBK_PYMODALDIALOGWRAPPER_H
#define SBK_PYMODALDIALOGWRAPPER_H

#include <shiboken.h>

#include <PythonPanels.h>

namespace PySide { class DynamicQMetaObject; }

NATRON_NAMESPACE_ENTER;
class PyModalDialogWrapper : public PyModalDialog
{
public:
    virtual void accept();
    inline void adjustPosition_protected(QWidget * arg__1) { PyModalDialog::adjustPosition(arg__1); }
    inline void closeEvent_protected(QCloseEvent * arg__1) { PyModalDialog::closeEvent(arg__1); }
    inline void contextMenuEvent_protected(QContextMenuEvent * arg__1) { PyModalDialog::contextMenuEvent(arg__1); }
    virtual void done(int arg__1);
    inline bool eventFilter_protected(QObject * arg__1, QEvent * arg__2) { return PyModalDialog::eventFilter(arg__1, arg__2); }
    inline void keyPressEvent_protected(QKeyEvent * arg__1) { PyModalDialog::keyPressEvent(arg__1); }
    virtual void reject();
    inline void resizeEvent_protected(QResizeEvent * arg__1) { PyModalDialog::resizeEvent(arg__1); }
    inline void showEvent_protected(QShowEvent * arg__1) { PyModalDialog::showEvent(arg__1); }
    virtual ~PyModalDialogWrapper();
public:
    virtual int qt_metacall(QMetaObject::Call call, int id, void** args);
    virtual void* qt_metacast(const char* _clname);
    static void pysideInitQtMetaTypes();
};
NATRON_NAMESPACE_EXIT;

#endif // SBK_PYMODALDIALOGWRAPPER_H

