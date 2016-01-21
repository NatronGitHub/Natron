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
    inline void actionEvent_protected(QActionEvent * event) { PyModalDialog::actionEvent(event); }
    virtual void actionEvent(QActionEvent * event);
    inline void adjustPosition_protected(QWidget * arg__1) { PyModalDialog::adjustPosition(arg__1); }
    inline void changeEvent_protected(QEvent * event) { PyModalDialog::changeEvent(event); }
    virtual void changeEvent(QEvent * event);
    inline void childEvent_protected(QChildEvent * arg__1) { PyModalDialog::childEvent(arg__1); }
    virtual void childEvent(QChildEvent * arg__1);
    inline void closeEvent_protected(QCloseEvent * arg__1) { PyModalDialog::closeEvent(arg__1); }
    virtual void closeEvent(QCloseEvent * arg__1);
    inline void connectNotify_protected(const char * signal) { PyModalDialog::connectNotify(signal); }
    virtual void connectNotify(const char * signal);
    inline void contextMenuEvent_protected(QContextMenuEvent * arg__1) { PyModalDialog::contextMenuEvent(arg__1); }
    virtual void contextMenuEvent(QContextMenuEvent * arg__1);
    inline void customEvent_protected(QEvent * arg__1) { PyModalDialog::customEvent(arg__1); }
    virtual void customEvent(QEvent * arg__1);
    inline void destroy_protected(bool destroyWindow = true, bool destroySubWindows = true) { PyModalDialog::destroy(destroyWindow, destroySubWindows); }
    virtual int devType() const;
    inline void disconnectNotify_protected(const char * signal) { PyModalDialog::disconnectNotify(signal); }
    virtual void disconnectNotify(const char * signal);
    virtual void done(int arg__1);
    inline void dragEnterEvent_protected(QDragEnterEvent * event) { PyModalDialog::dragEnterEvent(event); }
    virtual void dragEnterEvent(QDragEnterEvent * event);
    inline void dragLeaveEvent_protected(QDragLeaveEvent * event) { PyModalDialog::dragLeaveEvent(event); }
    virtual void dragLeaveEvent(QDragLeaveEvent * event);
    inline void dragMoveEvent_protected(QDragMoveEvent * event) { PyModalDialog::dragMoveEvent(event); }
    virtual void dragMoveEvent(QDragMoveEvent * event);
    inline void dropEvent_protected(QDropEvent * event) { PyModalDialog::dropEvent(event); }
    virtual void dropEvent(QDropEvent * event);
    inline void enterEvent_protected(QEvent * event) { PyModalDialog::enterEvent(event); }
    virtual void enterEvent(QEvent * event);
    inline bool event_protected(QEvent * arg__1) { return PyModalDialog::event(arg__1); }
    virtual bool event(QEvent * arg__1);
    inline bool eventFilter_protected(QObject * arg__1, QEvent * arg__2) { return PyModalDialog::eventFilter(arg__1, arg__2); }
    virtual bool eventFilter(QObject * arg__1, QEvent * arg__2);
    inline void focusInEvent_protected(QFocusEvent * event) { PyModalDialog::focusInEvent(event); }
    virtual void focusInEvent(QFocusEvent * event);
    inline bool focusNextChild_protected() { return PyModalDialog::focusNextChild(); }
    inline bool focusNextPrevChild_protected(bool next) { return PyModalDialog::focusNextPrevChild(next); }
    virtual bool focusNextPrevChild(bool next);
    inline void focusOutEvent_protected(QFocusEvent * event) { PyModalDialog::focusOutEvent(event); }
    virtual void focusOutEvent(QFocusEvent * event);
    inline bool focusPreviousChild_protected() { return PyModalDialog::focusPreviousChild(); }
    virtual int heightForWidth(int arg__1) const;
    inline void hideEvent_protected(QHideEvent * event) { PyModalDialog::hideEvent(event); }
    virtual void hideEvent(QHideEvent * event);
    inline void inputMethodEvent_protected(QInputMethodEvent * event) { PyModalDialog::inputMethodEvent(event); }
    virtual void inputMethodEvent(QInputMethodEvent * event);
    virtual QVariant inputMethodQuery(Qt::InputMethodQuery arg__1) const;
    inline void keyPressEvent_protected(QKeyEvent * arg__1) { PyModalDialog::keyPressEvent(arg__1); }
    virtual void keyPressEvent(QKeyEvent * arg__1);
    inline void keyReleaseEvent_protected(QKeyEvent * event) { PyModalDialog::keyReleaseEvent(event); }
    virtual void keyReleaseEvent(QKeyEvent * event);
    inline void languageChange_protected() { PyModalDialog::languageChange(); }
    virtual void languageChange();
    inline void leaveEvent_protected(QEvent * event) { PyModalDialog::leaveEvent(event); }
    virtual void leaveEvent(QEvent * event);
    virtual const QMetaObject * metaObject() const;
    inline int metric_protected(int arg__1) const { return PyModalDialog::metric(QPaintDevice::PaintDeviceMetric(arg__1)); }
    virtual int metric(QPaintDevice::PaintDeviceMetric arg__1) const;
    virtual QSize minimumSizeHint() const;
    inline void mouseDoubleClickEvent_protected(QMouseEvent * event) { PyModalDialog::mouseDoubleClickEvent(event); }
    virtual void mouseDoubleClickEvent(QMouseEvent * event);
    inline void mouseMoveEvent_protected(QMouseEvent * event) { PyModalDialog::mouseMoveEvent(event); }
    virtual void mouseMoveEvent(QMouseEvent * event);
    inline void mousePressEvent_protected(QMouseEvent * event) { PyModalDialog::mousePressEvent(event); }
    virtual void mousePressEvent(QMouseEvent * event);
    inline void mouseReleaseEvent_protected(QMouseEvent * event) { PyModalDialog::mouseReleaseEvent(event); }
    virtual void mouseReleaseEvent(QMouseEvent * event);
    inline void moveEvent_protected(QMoveEvent * event) { PyModalDialog::moveEvent(event); }
    virtual void moveEvent(QMoveEvent * event);
    virtual QPaintEngine * paintEngine() const;
    inline void paintEvent_protected(QPaintEvent * event) { PyModalDialog::paintEvent(event); }
    virtual void paintEvent(QPaintEvent * event);
    virtual void reject();
    inline void resetInputContext_protected() { PyModalDialog::resetInputContext(); }
    inline void resizeEvent_protected(QResizeEvent * arg__1) { PyModalDialog::resizeEvent(arg__1); }
    virtual void resizeEvent(QResizeEvent * arg__1);
    virtual void setVisible(bool visible);
    inline void showEvent_protected(QShowEvent * arg__1) { PyModalDialog::showEvent(arg__1); }
    virtual void showEvent(QShowEvent * arg__1);
    virtual QSize sizeHint() const;
    inline void tabletEvent_protected(QTabletEvent * event) { PyModalDialog::tabletEvent(event); }
    virtual void tabletEvent(QTabletEvent * event);
    inline void timerEvent_protected(QTimerEvent * arg__1) { PyModalDialog::timerEvent(arg__1); }
    virtual void timerEvent(QTimerEvent * arg__1);
    inline void updateMicroFocus_protected() { PyModalDialog::updateMicroFocus(); }
    inline void wheelEvent_protected(QWheelEvent * event) { PyModalDialog::wheelEvent(event); }
    virtual void wheelEvent(QWheelEvent * event);
    virtual ~PyModalDialogWrapper();
public:
    virtual int qt_metacall(QMetaObject::Call call, int id, void** args);
    virtual void* qt_metacast(const char* _clname);
    static void pysideInitQtMetaTypes();
};
NATRON_NAMESPACE_EXIT;

#endif // SBK_PYMODALDIALOGWRAPPER_H

