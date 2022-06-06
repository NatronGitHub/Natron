#ifndef SBK_PYMODALDIALOGWRAPPER_H
#define SBK_PYMODALDIALOGWRAPPER_H

#include <PythonPanels.h>


// Extra includes
#include <qobject.h>
#include <PyParameter.h>
#include <qwidget.h>
#include <qsize.h>
#include <qevent.h>
#include <qpaintdevice.h>
#include <qpoint.h>
#include <qobjectdefs.h>
#include <qcoreevent.h>
#include <qbytearray.h>
namespace PySide { class DynamicQMetaObject; }

NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class PyModalDialogWrapper : public PyModalDialog
{
public:
    void accept() override;
    inline void actionEvent_protected(QActionEvent * event) { PyModalDialog::actionEvent(event); }
    void actionEvent(QActionEvent * event) override;
    inline void adjustPosition_protected(QWidget * arg__1) { PyModalDialog::adjustPosition(arg__1); }
    inline void changeEvent_protected(QEvent * event) { PyModalDialog::changeEvent(event); }
    void changeEvent(QEvent * event) override;
    inline void childEvent_protected(QChildEvent * event) { PyModalDialog::childEvent(event); }
    void childEvent(QChildEvent * event) override;
    inline void closeEvent_protected(QCloseEvent * arg__1) { PyModalDialog::closeEvent(arg__1); }
    void closeEvent(QCloseEvent * arg__1) override;
    inline void connectNotify_protected(const QMetaMethod & signal) { PyModalDialog::connectNotify(signal); }
    void connectNotify(const QMetaMethod & signal) override;
    inline void contextMenuEvent_protected(QContextMenuEvent * arg__1) { PyModalDialog::contextMenuEvent(arg__1); }
    void contextMenuEvent(QContextMenuEvent * arg__1) override;
    inline void customEvent_protected(QEvent * event) { PyModalDialog::customEvent(event); }
    void customEvent(QEvent * event) override;
    int devType() const override;
    inline void disconnectNotify_protected(const QMetaMethod & signal) { PyModalDialog::disconnectNotify(signal); }
    void disconnectNotify(const QMetaMethod & signal) override;
    void done(int arg__1) override;
    inline void dragEnterEvent_protected(QDragEnterEvent * event) { PyModalDialog::dragEnterEvent(event); }
    void dragEnterEvent(QDragEnterEvent * event) override;
    inline void dragLeaveEvent_protected(QDragLeaveEvent * event) { PyModalDialog::dragLeaveEvent(event); }
    void dragLeaveEvent(QDragLeaveEvent * event) override;
    inline void dragMoveEvent_protected(QDragMoveEvent * event) { PyModalDialog::dragMoveEvent(event); }
    void dragMoveEvent(QDragMoveEvent * event) override;
    inline void dropEvent_protected(QDropEvent * event) { PyModalDialog::dropEvent(event); }
    void dropEvent(QDropEvent * event) override;
    inline void enterEvent_protected(QEvent * event) { PyModalDialog::enterEvent(event); }
    void enterEvent(QEvent * event) override;
    inline bool event_protected(QEvent * event) { return PyModalDialog::event(event); }
    bool event(QEvent * event) override;
    inline bool eventFilter_protected(QObject * arg__1, QEvent * arg__2) { return PyModalDialog::eventFilter(arg__1, arg__2); }
    bool eventFilter(QObject * arg__1, QEvent * arg__2) override;
    int exec() override;
    inline void focusInEvent_protected(QFocusEvent * event) { PyModalDialog::focusInEvent(event); }
    void focusInEvent(QFocusEvent * event) override;
    inline bool focusNextPrevChild_protected(bool next) { return PyModalDialog::focusNextPrevChild(next); }
    bool focusNextPrevChild(bool next) override;
    inline void focusOutEvent_protected(QFocusEvent * event) { PyModalDialog::focusOutEvent(event); }
    void focusOutEvent(QFocusEvent * event) override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int arg__1) const override;
    inline void hideEvent_protected(QHideEvent * event) { PyModalDialog::hideEvent(event); }
    void hideEvent(QHideEvent * event) override;
    inline void initPainter_protected(QPainter * painter) const { PyModalDialog::initPainter(painter); }
    void initPainter(QPainter * painter) const override;
    inline void inputMethodEvent_protected(QInputMethodEvent * event) { PyModalDialog::inputMethodEvent(event); }
    void inputMethodEvent(QInputMethodEvent * event) override;
    QVariant inputMethodQuery(Qt::InputMethodQuery arg__1) const override;
    inline void keyPressEvent_protected(QKeyEvent * arg__1) { PyModalDialog::keyPressEvent(arg__1); }
    void keyPressEvent(QKeyEvent * arg__1) override;
    inline void keyReleaseEvent_protected(QKeyEvent * event) { PyModalDialog::keyReleaseEvent(event); }
    void keyReleaseEvent(QKeyEvent * event) override;
    inline void leaveEvent_protected(QEvent * event) { PyModalDialog::leaveEvent(event); }
    void leaveEvent(QEvent * event) override;
    const QMetaObject * metaObject() const override;
    inline int metric_protected(QPaintDevice::PaintDeviceMetric arg__1) const { return PyModalDialog::metric(QPaintDevice::PaintDeviceMetric(arg__1)); }
    int metric(QPaintDevice::PaintDeviceMetric arg__1) const override;
    inline void mouseDoubleClickEvent_protected(QMouseEvent * event) { PyModalDialog::mouseDoubleClickEvent(event); }
    void mouseDoubleClickEvent(QMouseEvent * event) override;
    inline void mouseMoveEvent_protected(QMouseEvent * event) { PyModalDialog::mouseMoveEvent(event); }
    void mouseMoveEvent(QMouseEvent * event) override;
    inline void mousePressEvent_protected(QMouseEvent * event) { PyModalDialog::mousePressEvent(event); }
    void mousePressEvent(QMouseEvent * event) override;
    inline void mouseReleaseEvent_protected(QMouseEvent * event) { PyModalDialog::mouseReleaseEvent(event); }
    void mouseReleaseEvent(QMouseEvent * event) override;
    inline void moveEvent_protected(QMoveEvent * event) { PyModalDialog::moveEvent(event); }
    void moveEvent(QMoveEvent * event) override;
    inline bool nativeEvent_protected(const QByteArray & eventType, void * message, long * result) { return PyModalDialog::nativeEvent(eventType, message, result); }
    bool nativeEvent(const QByteArray & eventType, void * message, long * result) override;
    void open() override;
    QPaintEngine * paintEngine() const override;
    inline void paintEvent_protected(QPaintEvent * event) { PyModalDialog::paintEvent(event); }
    void paintEvent(QPaintEvent * event) override;
    inline QPaintDevice * redirected_protected(QPoint * offset) const { return PyModalDialog::redirected(offset); }
    QPaintDevice * redirected(QPoint * offset) const override;
    void reject() override;
    inline void resizeEvent_protected(QResizeEvent * arg__1) { PyModalDialog::resizeEvent(arg__1); }
    void resizeEvent(QResizeEvent * arg__1) override;
    void setVisible(bool visible) override;
    inline QPainter * sharedPainter_protected() const { return PyModalDialog::sharedPainter(); }
    QPainter * sharedPainter() const override;
    inline void showEvent_protected(QShowEvent * arg__1) { PyModalDialog::showEvent(arg__1); }
    void showEvent(QShowEvent * arg__1) override;
    inline void tabletEvent_protected(QTabletEvent * event) { PyModalDialog::tabletEvent(event); }
    void tabletEvent(QTabletEvent * event) override;
    inline void timerEvent_protected(QTimerEvent * event) { PyModalDialog::timerEvent(event); }
    void timerEvent(QTimerEvent * event) override;
    inline void wheelEvent_protected(QWheelEvent * event) { PyModalDialog::wheelEvent(event); }
    void wheelEvent(QWheelEvent * event) override;
    ~PyModalDialogWrapper();
public:
    int qt_metacall(QMetaObject::Call call, int id, void **args) override;
    void *qt_metacast(const char *_clname) override;
    static void pysideInitQtMetaTypes();
    void resetPyMethodCache();
private:
    mutable bool m_PyMethodCache[51];
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#  ifndef SBK_QDIALOGWRAPPER_H
#  define SBK_QDIALOGWRAPPER_H

// Inherited base class:
NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class QDialogWrapper : public QDialog
{
public:
    QDialogWrapper(QWidget * parent = nullptr, QFlags<Qt::WindowType> f = Qt::WindowFlags());
    void accept() override;
    inline void actionEvent_protected(QActionEvent * event) { QDialog::actionEvent(event); }
    void actionEvent(QActionEvent * event) override;
    inline void adjustPosition_protected(QWidget * arg__1) { QDialog::adjustPosition(arg__1); }
    inline void changeEvent_protected(QEvent * event) { QDialog::changeEvent(event); }
    void changeEvent(QEvent * event) override;
    inline void childEvent_protected(QChildEvent * event) { QDialog::childEvent(event); }
    void childEvent(QChildEvent * event) override;
    inline void closeEvent_protected(QCloseEvent * arg__1) { QDialog::closeEvent(arg__1); }
    void closeEvent(QCloseEvent * arg__1) override;
    inline void connectNotify_protected(const QMetaMethod & signal) { QDialog::connectNotify(signal); }
    void connectNotify(const QMetaMethod & signal) override;
    inline void contextMenuEvent_protected(QContextMenuEvent * arg__1) { QDialog::contextMenuEvent(arg__1); }
    void contextMenuEvent(QContextMenuEvent * arg__1) override;
    inline void create_protected(WId arg__1 = 0, bool initializeWindow = true, bool destroyOldWindow = true) { QDialog::create(arg__1, initializeWindow, destroyOldWindow); }
    inline void customEvent_protected(QEvent * event) { QDialog::customEvent(event); }
    void customEvent(QEvent * event) override;
    inline void destroy_protected(bool destroyWindow = true, bool destroySubWindows = true) { QDialog::destroy(destroyWindow, destroySubWindows); }
    int devType() const override;
    inline void disconnectNotify_protected(const QMetaMethod & signal) { QDialog::disconnectNotify(signal); }
    void disconnectNotify(const QMetaMethod & signal) override;
    void done(int arg__1) override;
    inline void dragEnterEvent_protected(QDragEnterEvent * event) { QDialog::dragEnterEvent(event); }
    void dragEnterEvent(QDragEnterEvent * event) override;
    inline void dragLeaveEvent_protected(QDragLeaveEvent * event) { QDialog::dragLeaveEvent(event); }
    void dragLeaveEvent(QDragLeaveEvent * event) override;
    inline void dragMoveEvent_protected(QDragMoveEvent * event) { QDialog::dragMoveEvent(event); }
    void dragMoveEvent(QDragMoveEvent * event) override;
    inline void dropEvent_protected(QDropEvent * event) { QDialog::dropEvent(event); }
    void dropEvent(QDropEvent * event) override;
    inline void enterEvent_protected(QEvent * event) { QDialog::enterEvent(event); }
    void enterEvent(QEvent * event) override;
    inline bool event_protected(QEvent * event) { return QDialog::event(event); }
    bool event(QEvent * event) override;
    inline bool eventFilter_protected(QObject * arg__1, QEvent * arg__2) { return QDialog::eventFilter(arg__1, arg__2); }
    bool eventFilter(QObject * arg__1, QEvent * arg__2) override;
    int exec() override;
    inline void focusInEvent_protected(QFocusEvent * event) { QDialog::focusInEvent(event); }
    void focusInEvent(QFocusEvent * event) override;
    inline bool focusNextChild_protected() { return QDialog::focusNextChild(); }
    inline bool focusNextPrevChild_protected(bool next) { return QDialog::focusNextPrevChild(next); }
    bool focusNextPrevChild(bool next) override;
    inline void focusOutEvent_protected(QFocusEvent * event) { QDialog::focusOutEvent(event); }
    void focusOutEvent(QFocusEvent * event) override;
    inline bool focusPreviousChild_protected() { return QDialog::focusPreviousChild(); }
    bool hasHeightForWidth() const override;
    int heightForWidth(int arg__1) const override;
    inline void hideEvent_protected(QHideEvent * event) { QDialog::hideEvent(event); }
    void hideEvent(QHideEvent * event) override;
    inline void initPainter_protected(QPainter * painter) const { QDialog::initPainter(painter); }
    void initPainter(QPainter * painter) const override;
    inline void inputMethodEvent_protected(QInputMethodEvent * event) { QDialog::inputMethodEvent(event); }
    void inputMethodEvent(QInputMethodEvent * event) override;
    QVariant inputMethodQuery(Qt::InputMethodQuery arg__1) const override;
    inline void keyPressEvent_protected(QKeyEvent * arg__1) { QDialog::keyPressEvent(arg__1); }
    void keyPressEvent(QKeyEvent * arg__1) override;
    inline void keyReleaseEvent_protected(QKeyEvent * event) { QDialog::keyReleaseEvent(event); }
    void keyReleaseEvent(QKeyEvent * event) override;
    inline void leaveEvent_protected(QEvent * event) { QDialog::leaveEvent(event); }
    void leaveEvent(QEvent * event) override;
    const QMetaObject * metaObject() const override;
    inline int metric_protected(QPaintDevice::PaintDeviceMetric arg__1) const { return QDialog::metric(QPaintDevice::PaintDeviceMetric(arg__1)); }
    int metric(QPaintDevice::PaintDeviceMetric arg__1) const override;
    QSize minimumSizeHint() const override;
    inline void mouseDoubleClickEvent_protected(QMouseEvent * event) { QDialog::mouseDoubleClickEvent(event); }
    void mouseDoubleClickEvent(QMouseEvent * event) override;
    inline void mouseMoveEvent_protected(QMouseEvent * event) { QDialog::mouseMoveEvent(event); }
    void mouseMoveEvent(QMouseEvent * event) override;
    inline void mousePressEvent_protected(QMouseEvent * event) { QDialog::mousePressEvent(event); }
    void mousePressEvent(QMouseEvent * event) override;
    inline void mouseReleaseEvent_protected(QMouseEvent * event) { QDialog::mouseReleaseEvent(event); }
    void mouseReleaseEvent(QMouseEvent * event) override;
    inline void moveEvent_protected(QMoveEvent * event) { QDialog::moveEvent(event); }
    void moveEvent(QMoveEvent * event) override;
    inline bool nativeEvent_protected(const QByteArray & eventType, void * message, long * result) { return QDialog::nativeEvent(eventType, message, result); }
    bool nativeEvent(const QByteArray & eventType, void * message, long * result) override;
    void open() override;
    QPaintEngine * paintEngine() const override;
    inline void paintEvent_protected(QPaintEvent * event) { QDialog::paintEvent(event); }
    void paintEvent(QPaintEvent * event) override;
    inline QPaintDevice * redirected_protected(QPoint * offset) const { return QDialog::redirected(offset); }
    QPaintDevice * redirected(QPoint * offset) const override;
    void reject() override;
    inline void resizeEvent_protected(QResizeEvent * arg__1) { QDialog::resizeEvent(arg__1); }
    void resizeEvent(QResizeEvent * arg__1) override;
    void setVisible(bool visible) override;
    inline QPainter * sharedPainter_protected() const { return QDialog::sharedPainter(); }
    QPainter * sharedPainter() const override;
    inline void showEvent_protected(QShowEvent * arg__1) { QDialog::showEvent(arg__1); }
    void showEvent(QShowEvent * arg__1) override;
    QSize sizeHint() const override;
    inline void tabletEvent_protected(QTabletEvent * event) { QDialog::tabletEvent(event); }
    void tabletEvent(QTabletEvent * event) override;
    inline void timerEvent_protected(QTimerEvent * event) { QDialog::timerEvent(event); }
    void timerEvent(QTimerEvent * event) override;
    inline void updateMicroFocus_protected() { QDialog::updateMicroFocus(); }
    inline void wheelEvent_protected(QWheelEvent * event) { QDialog::wheelEvent(event); }
    void wheelEvent(QWheelEvent * event) override;
    ~QDialogWrapper();
public:
    int qt_metacall(QMetaObject::Call call, int id, void **args) override;
    void *qt_metacast(const char *_clname) override;
    static void pysideInitQtMetaTypes();
    void resetPyMethodCache();
private:
    mutable bool m_PyMethodCache[53];
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#  endif // SBK_QDIALOGWRAPPER_H

#  ifndef SBK_QWIDGETWRAPPER_H
#  define SBK_QWIDGETWRAPPER_H

// Inherited base class:
NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class QWidgetWrapper : public QWidget
{
public:
    QWidgetWrapper(QWidget * parent = nullptr, QFlags<Qt::WindowType> f = Qt::WindowFlags());
    inline void actionEvent_protected(QActionEvent * event) { QWidget::actionEvent(event); }
    void actionEvent(QActionEvent * event) override;
    inline void changeEvent_protected(QEvent * event) { QWidget::changeEvent(event); }
    void changeEvent(QEvent * event) override;
    inline void childEvent_protected(QChildEvent * event) { QWidget::childEvent(event); }
    void childEvent(QChildEvent * event) override;
    inline void closeEvent_protected(QCloseEvent * event) { QWidget::closeEvent(event); }
    void closeEvent(QCloseEvent * event) override;
    inline void connectNotify_protected(const QMetaMethod & signal) { QWidget::connectNotify(signal); }
    void connectNotify(const QMetaMethod & signal) override;
    inline void contextMenuEvent_protected(QContextMenuEvent * event) { QWidget::contextMenuEvent(event); }
    void contextMenuEvent(QContextMenuEvent * event) override;
    inline void create_protected(WId arg__1 = 0, bool initializeWindow = true, bool destroyOldWindow = true) { QWidget::create(arg__1, initializeWindow, destroyOldWindow); }
    inline void customEvent_protected(QEvent * event) { QWidget::customEvent(event); }
    void customEvent(QEvent * event) override;
    inline void destroy_protected(bool destroyWindow = true, bool destroySubWindows = true) { QWidget::destroy(destroyWindow, destroySubWindows); }
    int devType() const override;
    inline void disconnectNotify_protected(const QMetaMethod & signal) { QWidget::disconnectNotify(signal); }
    void disconnectNotify(const QMetaMethod & signal) override;
    inline void dragEnterEvent_protected(QDragEnterEvent * event) { QWidget::dragEnterEvent(event); }
    void dragEnterEvent(QDragEnterEvent * event) override;
    inline void dragLeaveEvent_protected(QDragLeaveEvent * event) { QWidget::dragLeaveEvent(event); }
    void dragLeaveEvent(QDragLeaveEvent * event) override;
    inline void dragMoveEvent_protected(QDragMoveEvent * event) { QWidget::dragMoveEvent(event); }
    void dragMoveEvent(QDragMoveEvent * event) override;
    inline void dropEvent_protected(QDropEvent * event) { QWidget::dropEvent(event); }
    void dropEvent(QDropEvent * event) override;
    inline void enterEvent_protected(QEvent * event) { QWidget::enterEvent(event); }
    void enterEvent(QEvent * event) override;
    inline bool event_protected(QEvent * event) { return QWidget::event(event); }
    bool event(QEvent * event) override;
    bool eventFilter(QObject * watched, QEvent * event) override;
    inline void focusInEvent_protected(QFocusEvent * event) { QWidget::focusInEvent(event); }
    void focusInEvent(QFocusEvent * event) override;
    inline bool focusNextChild_protected() { return QWidget::focusNextChild(); }
    inline bool focusNextPrevChild_protected(bool next) { return QWidget::focusNextPrevChild(next); }
    bool focusNextPrevChild(bool next) override;
    inline void focusOutEvent_protected(QFocusEvent * event) { QWidget::focusOutEvent(event); }
    void focusOutEvent(QFocusEvent * event) override;
    inline bool focusPreviousChild_protected() { return QWidget::focusPreviousChild(); }
    bool hasHeightForWidth() const override;
    int heightForWidth(int arg__1) const override;
    inline void hideEvent_protected(QHideEvent * event) { QWidget::hideEvent(event); }
    void hideEvent(QHideEvent * event) override;
    inline void initPainter_protected(QPainter * painter) const { QWidget::initPainter(painter); }
    void initPainter(QPainter * painter) const override;
    inline void inputMethodEvent_protected(QInputMethodEvent * event) { QWidget::inputMethodEvent(event); }
    void inputMethodEvent(QInputMethodEvent * event) override;
    QVariant inputMethodQuery(Qt::InputMethodQuery arg__1) const override;
    inline bool isSignalConnected_protected(const QMetaMethod & signal) const { return QWidget::isSignalConnected(signal); }
    inline void keyPressEvent_protected(QKeyEvent * event) { QWidget::keyPressEvent(event); }
    void keyPressEvent(QKeyEvent * event) override;
    inline void keyReleaseEvent_protected(QKeyEvent * event) { QWidget::keyReleaseEvent(event); }
    void keyReleaseEvent(QKeyEvent * event) override;
    inline void leaveEvent_protected(QEvent * event) { QWidget::leaveEvent(event); }
    void leaveEvent(QEvent * event) override;
    const QMetaObject * metaObject() const override;
    inline int metric_protected(QPaintDevice::PaintDeviceMetric arg__1) const { return QWidget::metric(QPaintDevice::PaintDeviceMetric(arg__1)); }
    int metric(QPaintDevice::PaintDeviceMetric arg__1) const override;
    QSize minimumSizeHint() const override;
    inline void mouseDoubleClickEvent_protected(QMouseEvent * event) { QWidget::mouseDoubleClickEvent(event); }
    void mouseDoubleClickEvent(QMouseEvent * event) override;
    inline void mouseMoveEvent_protected(QMouseEvent * event) { QWidget::mouseMoveEvent(event); }
    void mouseMoveEvent(QMouseEvent * event) override;
    inline void mousePressEvent_protected(QMouseEvent * event) { QWidget::mousePressEvent(event); }
    void mousePressEvent(QMouseEvent * event) override;
    inline void mouseReleaseEvent_protected(QMouseEvent * event) { QWidget::mouseReleaseEvent(event); }
    void mouseReleaseEvent(QMouseEvent * event) override;
    inline void moveEvent_protected(QMoveEvent * event) { QWidget::moveEvent(event); }
    void moveEvent(QMoveEvent * event) override;
    inline bool nativeEvent_protected(const QByteArray & eventType, void * message, long * result) { return QWidget::nativeEvent(eventType, message, result); }
    bool nativeEvent(const QByteArray & eventType, void * message, long * result) override;
    QPaintEngine * paintEngine() const override;
    inline void paintEvent_protected(QPaintEvent * event) { QWidget::paintEvent(event); }
    void paintEvent(QPaintEvent * event) override;
    inline int receivers_protected(const char * signal) const { return QWidget::receivers(signal); }
    inline QPaintDevice * redirected_protected(QPoint * offset) const { return QWidget::redirected(offset); }
    QPaintDevice * redirected(QPoint * offset) const override;
    inline void resizeEvent_protected(QResizeEvent * event) { QWidget::resizeEvent(event); }
    void resizeEvent(QResizeEvent * event) override;
    inline QObject * sender_protected() const { return QWidget::sender(); }
    inline int senderSignalIndex_protected() const { return QWidget::senderSignalIndex(); }
    void setVisible(bool visible) override;
    inline QPainter * sharedPainter_protected() const { return QWidget::sharedPainter(); }
    QPainter * sharedPainter() const override;
    inline void showEvent_protected(QShowEvent * event) { QWidget::showEvent(event); }
    void showEvent(QShowEvent * event) override;
    QSize sizeHint() const override;
    inline void tabletEvent_protected(QTabletEvent * event) { QWidget::tabletEvent(event); }
    void tabletEvent(QTabletEvent * event) override;
    inline void timerEvent_protected(QTimerEvent * event) { QWidget::timerEvent(event); }
    void timerEvent(QTimerEvent * event) override;
    inline void updateMicroFocus_protected() { QWidget::updateMicroFocus(); }
    inline void wheelEvent_protected(QWheelEvent * event) { QWidget::wheelEvent(event); }
    void wheelEvent(QWheelEvent * event) override;
    ~QWidgetWrapper();
public:
    int qt_metacall(QMetaObject::Call call, int id, void **args) override;
    void *qt_metacast(const char *_clname) override;
    static void pysideInitQtMetaTypes();
    void resetPyMethodCache();
private:
    mutable bool m_PyMethodCache[48];
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#  endif // SBK_QWIDGETWRAPPER_H

#  ifndef SBK_QOBJECTWRAPPER_H
#  define SBK_QOBJECTWRAPPER_H

// Inherited base class:
NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class QObjectWrapper : public QObject
{
public:
    QObjectWrapper(QObject * parent = nullptr);
    inline void childEvent_protected(QChildEvent * event) { QObject::childEvent(event); }
    void childEvent(QChildEvent * event) override;
    inline void connectNotify_protected(const QMetaMethod & signal) { QObject::connectNotify(signal); }
    void connectNotify(const QMetaMethod & signal) override;
    inline void customEvent_protected(QEvent * event) { QObject::customEvent(event); }
    void customEvent(QEvent * event) override;
    inline void disconnectNotify_protected(const QMetaMethod & signal) { QObject::disconnectNotify(signal); }
    void disconnectNotify(const QMetaMethod & signal) override;
    bool event(QEvent * event) override;
    bool eventFilter(QObject * watched, QEvent * event) override;
    inline bool isSignalConnected_protected(const QMetaMethod & signal) const { return QObject::isSignalConnected(signal); }
    const QMetaObject * metaObject() const override;
    inline int receivers_protected(const char * signal) const { return QObject::receivers(signal); }
    inline QObject * sender_protected() const { return QObject::sender(); }
    inline int senderSignalIndex_protected() const { return QObject::senderSignalIndex(); }
    inline void timerEvent_protected(QTimerEvent * event) { QObject::timerEvent(event); }
    void timerEvent(QTimerEvent * event) override;
    ~QObjectWrapper();
public:
    int qt_metacall(QMetaObject::Call call, int id, void **args) override;
    void *qt_metacast(const char *_clname) override;
    static void pysideInitQtMetaTypes();
    void resetPyMethodCache();
private:
    mutable bool m_PyMethodCache[8];
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#  endif // SBK_QOBJECTWRAPPER_H

#endif // SBK_PYMODALDIALOGWRAPPER_H

