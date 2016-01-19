#ifndef SBK_PYPANELWRAPPER_H
#define SBK_PYPANELWRAPPER_H

#include <shiboken.h>

#include <PythonPanels.h>

namespace PySide { class DynamicQMetaObject; }

NATRON_NAMESPACE_ENTER;
class PyPanelWrapper : public PyPanel
{
public:
    PyPanelWrapper(const std::string & scriptName, const std::string & label, bool useUserParameters, GuiApp * app);
    inline void actionEvent_protected(QActionEvent * event) { PyPanel::actionEvent(event); }
    virtual void actionEvent(QActionEvent * event);
    inline void changeEvent_protected(QEvent * event) { PyPanel::changeEvent(event); }
    virtual void changeEvent(QEvent * event);
    inline void childEvent_protected(QChildEvent * arg__1) { PyPanel::childEvent(arg__1); }
    virtual void childEvent(QChildEvent * arg__1);
    inline void closeEvent_protected(QCloseEvent * event) { PyPanel::closeEvent(event); }
    virtual void closeEvent(QCloseEvent * event);
    inline void connectNotify_protected(const char * signal) { PyPanel::connectNotify(signal); }
    virtual void connectNotify(const char * signal);
    inline void contextMenuEvent_protected(QContextMenuEvent * event) { PyPanel::contextMenuEvent(event); }
    virtual void contextMenuEvent(QContextMenuEvent * event);
    inline void customEvent_protected(QEvent * arg__1) { PyPanel::customEvent(arg__1); }
    virtual void customEvent(QEvent * arg__1);
    inline void destroy_protected(bool destroyWindow = true, bool destroySubWindows = true) { PyPanel::destroy(destroyWindow, destroySubWindows); }
    inline void disconnectNotify_protected(const char * signal) { PyPanel::disconnectNotify(signal); }
    virtual void disconnectNotify(const char * signal);
    inline void dragEnterEvent_protected(QDragEnterEvent * event) { PyPanel::dragEnterEvent(event); }
    virtual void dragEnterEvent(QDragEnterEvent * event);
    inline void dragLeaveEvent_protected(QDragLeaveEvent * event) { PyPanel::dragLeaveEvent(event); }
    virtual void dragLeaveEvent(QDragLeaveEvent * event);
    inline void dragMoveEvent_protected(QDragMoveEvent * event) { PyPanel::dragMoveEvent(event); }
    virtual void dragMoveEvent(QDragMoveEvent * event);
    inline void dropEvent_protected(QDropEvent * event) { PyPanel::dropEvent(event); }
    virtual void dropEvent(QDropEvent * event);
    inline void enterEvent_protected(QEvent * event) { PyPanel::enterEvent(event); }
    virtual void enterEvent(QEvent * event);
    inline bool event_protected(QEvent * arg__1) { return PyPanel::event(arg__1); }
    virtual bool event(QEvent * arg__1);
    virtual bool eventFilter(QObject * arg__1, QEvent * arg__2);
    inline void focusInEvent_protected(QFocusEvent * event) { PyPanel::focusInEvent(event); }
    virtual void focusInEvent(QFocusEvent * event);
    inline bool focusNextChild_protected() { return PyPanel::focusNextChild(); }
    inline bool focusNextPrevChild_protected(bool next) { return PyPanel::focusNextPrevChild(next); }
    virtual bool focusNextPrevChild(bool next);
    inline void focusOutEvent_protected(QFocusEvent * event) { PyPanel::focusOutEvent(event); }
    virtual void focusOutEvent(QFocusEvent * event);
    inline bool focusPreviousChild_protected() { return PyPanel::focusPreviousChild(); }
    virtual int heightForWidth(int arg__1) const;
    inline void hideEvent_protected(QHideEvent * event) { PyPanel::hideEvent(event); }
    virtual void hideEvent(QHideEvent * event);
    inline void inputMethodEvent_protected(QInputMethodEvent * event) { PyPanel::inputMethodEvent(event); }
    virtual void inputMethodEvent(QInputMethodEvent * event);
    virtual QVariant inputMethodQuery(Qt::InputMethodQuery arg__1) const;
    inline void keyPressEvent_protected(QKeyEvent * event) { PyPanel::keyPressEvent(event); }
    virtual void keyPressEvent(QKeyEvent * event);
    inline void keyReleaseEvent_protected(QKeyEvent * event) { PyPanel::keyReleaseEvent(event); }
    virtual void keyReleaseEvent(QKeyEvent * event);
    inline void languageChange_protected() { PyPanel::languageChange(); }
    virtual void languageChange();
    inline void leaveEvent_protected(QEvent * event) { PyPanel::leaveEvent(event); }
    virtual void leaveEvent(QEvent * event);
    virtual const QMetaObject * metaObject() const;
    inline int metric_protected(int arg__1) const { return PyPanel::metric(QPaintDevice::PaintDeviceMetric(arg__1)); }
    virtual QSize minimumSizeHint() const;
    inline void mouseDoubleClickEvent_protected(QMouseEvent * event) { PyPanel::mouseDoubleClickEvent(event); }
    virtual void mouseDoubleClickEvent(QMouseEvent * event);
    inline void mouseMoveEvent_protected(QMouseEvent * event) { PyPanel::mouseMoveEvent(event); }
    virtual void mouseMoveEvent(QMouseEvent * event);
    inline void mousePressEvent_protected(QMouseEvent * event) { PyPanel::mousePressEvent(event); }
    virtual void mousePressEvent(QMouseEvent * event);
    inline void mouseReleaseEvent_protected(QMouseEvent * event) { PyPanel::mouseReleaseEvent(event); }
    virtual void mouseReleaseEvent(QMouseEvent * event);
    inline void moveEvent_protected(QMoveEvent * event) { PyPanel::moveEvent(event); }
    virtual void moveEvent(QMoveEvent * event);
    inline void onUserDataChanged_protected() { PyPanel::onUserDataChanged(); }
    inline void paintEvent_protected(QPaintEvent * event) { PyPanel::paintEvent(event); }
    virtual void paintEvent(QPaintEvent * event);
    inline int receivers_protected(const char * signal) const { return PyPanel::receivers(signal); }
    inline void resetInputContext_protected() { PyPanel::resetInputContext(); }
    inline void resizeEvent_protected(QResizeEvent * event) { PyPanel::resizeEvent(event); }
    virtual void resizeEvent(QResizeEvent * event);
    virtual void restore(const std::string & arg__1);
    inline std::string save_protected() { return PyPanel::save(); }
    virtual std::string save();
    inline QObject * sender_protected() const { return PyPanel::sender(); }
    inline int senderSignalIndex_protected() const { return PyPanel::senderSignalIndex(); }
    virtual void setVisible(bool visible);
    inline void showEvent_protected(QShowEvent * event) { PyPanel::showEvent(event); }
    virtual void showEvent(QShowEvent * event);
    virtual QSize sizeHint() const;
    inline void tabletEvent_protected(QTabletEvent * event) { PyPanel::tabletEvent(event); }
    virtual void tabletEvent(QTabletEvent * event);
    inline void timerEvent_protected(QTimerEvent * arg__1) { PyPanel::timerEvent(arg__1); }
    virtual void timerEvent(QTimerEvent * arg__1);
    inline void updateMicroFocus_protected() { PyPanel::updateMicroFocus(); }
    inline void wheelEvent_protected(QWheelEvent * event) { PyPanel::wheelEvent(event); }
    virtual void wheelEvent(QWheelEvent * event);
    virtual ~PyPanelWrapper();
public:
    virtual int qt_metacall(QMetaObject::Call call, int id, void** args);
    virtual void* qt_metacast(const char* _clname);
    static void pysideInitQtMetaTypes();
};
NATRON_NAMESPACE_EXIT;

#endif // SBK_PYPANELWRAPPER_H

