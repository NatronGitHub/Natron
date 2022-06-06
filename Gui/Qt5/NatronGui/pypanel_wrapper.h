#ifndef SBK_PYPANELWRAPPER_H
#define SBK_PYPANELWRAPPER_H

#include <PythonPanels.h>


// Extra includes
#include <qobject.h>
#include <PyGuiApp.h>
#include <PyParameter.h>
#include <list>
#include <qwidget.h>
#include <qevent.h>
#include <qcoreevent.h>
#include <qsize.h>
#include <qrect.h>
#include <qmargins.h>
#include <qpoint.h>
#include <qregion.h>
#include <QList>
#include <qfont.h>
#include <qkeysequence.h>
#include <qpalette.h>
#include <qsizepolicy.h>
#include <qcursor.h>
#include <qpaintdevice.h>
#include <qbytearray.h>
#include <qobjectdefs.h>
#include <qfontmetrics.h>
#include <qpixmap.h>
#include <qfontinfo.h>
namespace PySide { class DynamicQMetaObject; }

NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class PyPanelWrapper : public PyPanel
{
public:
    PyPanelWrapper(const QString & scriptName, const QString & label, bool useUserParameters, GuiApp * app);
    inline void actionEvent_protected(QActionEvent * event) { PyPanel::actionEvent(event); }
    void actionEvent(QActionEvent * event) override;
    inline void changeEvent_protected(QEvent * event) { PyPanel::changeEvent(event); }
    void changeEvent(QEvent * event) override;
    inline void childEvent_protected(QChildEvent * event) { PyPanel::childEvent(event); }
    void childEvent(QChildEvent * event) override;
    inline void closeEvent_protected(QCloseEvent * event) { PyPanel::closeEvent(event); }
    void closeEvent(QCloseEvent * event) override;
    inline void connectNotify_protected(const QMetaMethod & signal) { PyPanel::connectNotify(signal); }
    void connectNotify(const QMetaMethod & signal) override;
    inline void contextMenuEvent_protected(QContextMenuEvent * event) { PyPanel::contextMenuEvent(event); }
    void contextMenuEvent(QContextMenuEvent * event) override;
    inline void create_protected(WId arg__1 = 0, bool initializeWindow = true, bool destroyOldWindow = true) { PyPanel::create(arg__1, initializeWindow, destroyOldWindow); }
    inline void customEvent_protected(QEvent * event) { PyPanel::customEvent(event); }
    void customEvent(QEvent * event) override;
    inline void destroy_protected(bool destroyWindow = true, bool destroySubWindows = true) { PyPanel::destroy(destroyWindow, destroySubWindows); }
    int devType() const override;
    inline void disconnectNotify_protected(const QMetaMethod & signal) { PyPanel::disconnectNotify(signal); }
    void disconnectNotify(const QMetaMethod & signal) override;
    inline void dragEnterEvent_protected(QDragEnterEvent * event) { PyPanel::dragEnterEvent(event); }
    void dragEnterEvent(QDragEnterEvent * event) override;
    inline void dragLeaveEvent_protected(QDragLeaveEvent * event) { PyPanel::dragLeaveEvent(event); }
    void dragLeaveEvent(QDragLeaveEvent * event) override;
    inline void dragMoveEvent_protected(QDragMoveEvent * event) { PyPanel::dragMoveEvent(event); }
    void dragMoveEvent(QDragMoveEvent * event) override;
    inline void dropEvent_protected(QDropEvent * event) { PyPanel::dropEvent(event); }
    void dropEvent(QDropEvent * event) override;
    inline void enterEvent_protected(QEvent * e) { PyPanel::enterEvent(e); }
    void enterEvent(QEvent * e) override;
    inline bool event_protected(QEvent * event) { return PyPanel::event(event); }
    bool event(QEvent * event) override;
    bool eventFilter(QObject * watched, QEvent * event) override;
    inline void focusInEvent_protected(QFocusEvent * event) { PyPanel::focusInEvent(event); }
    void focusInEvent(QFocusEvent * event) override;
    inline bool focusNextChild_protected() { return PyPanel::focusNextChild(); }
    inline bool focusNextPrevChild_protected(bool next) { return PyPanel::focusNextPrevChild(next); }
    bool focusNextPrevChild(bool next) override;
    inline void focusOutEvent_protected(QFocusEvent * event) { PyPanel::focusOutEvent(event); }
    void focusOutEvent(QFocusEvent * event) override;
    inline bool focusPreviousChild_protected() { return PyPanel::focusPreviousChild(); }
    bool hasHeightForWidth() const override;
    int heightForWidth(int arg__1) const override;
    inline void hideEvent_protected(QHideEvent * event) { PyPanel::hideEvent(event); }
    void hideEvent(QHideEvent * event) override;
    inline void initPainter_protected(QPainter * painter) const { PyPanel::initPainter(painter); }
    void initPainter(QPainter * painter) const override;
    inline void inputMethodEvent_protected(QInputMethodEvent * event) { PyPanel::inputMethodEvent(event); }
    void inputMethodEvent(QInputMethodEvent * event) override;
    QVariant inputMethodQuery(Qt::InputMethodQuery arg__1) const override;
    inline void keyPressEvent_protected(QKeyEvent * e) { PyPanel::keyPressEvent(e); }
    void keyPressEvent(QKeyEvent * e) override;
    inline void keyReleaseEvent_protected(QKeyEvent * event) { PyPanel::keyReleaseEvent(event); }
    void keyReleaseEvent(QKeyEvent * event) override;
    inline void leaveEvent_protected(QEvent * e) { PyPanel::leaveEvent(e); }
    void leaveEvent(QEvent * e) override;
    const QMetaObject * metaObject() const override;
    inline int metric_protected(QPaintDevice::PaintDeviceMetric arg__1) const { return PyPanel::metric(QPaintDevice::PaintDeviceMetric(arg__1)); }
    int metric(QPaintDevice::PaintDeviceMetric arg__1) const override;
    QSize minimumSizeHint() const override;
    inline void mouseDoubleClickEvent_protected(QMouseEvent * event) { PyPanel::mouseDoubleClickEvent(event); }
    void mouseDoubleClickEvent(QMouseEvent * event) override;
    inline void mouseMoveEvent_protected(QMouseEvent * event) { PyPanel::mouseMoveEvent(event); }
    void mouseMoveEvent(QMouseEvent * event) override;
    inline void mousePressEvent_protected(QMouseEvent * e) { PyPanel::mousePressEvent(e); }
    void mousePressEvent(QMouseEvent * e) override;
    inline void mouseReleaseEvent_protected(QMouseEvent * event) { PyPanel::mouseReleaseEvent(event); }
    void mouseReleaseEvent(QMouseEvent * event) override;
    inline void moveEvent_protected(QMoveEvent * event) { PyPanel::moveEvent(event); }
    void moveEvent(QMoveEvent * event) override;
    inline bool nativeEvent_protected(const QByteArray & eventType, void * message, long * result) { return PyPanel::nativeEvent(eventType, message, result); }
    bool nativeEvent(const QByteArray & eventType, void * message, long * result) override;
    inline void onUserDataChanged_protected() { PyPanel::onUserDataChanged(); }
    QPaintEngine * paintEngine() const override;
    inline void paintEvent_protected(QPaintEvent * event) { PyPanel::paintEvent(event); }
    void paintEvent(QPaintEvent * event) override;
    inline QPaintDevice * redirected_protected(QPoint * offset) const { return PyPanel::redirected(offset); }
    QPaintDevice * redirected(QPoint * offset) const override;
    inline void resizeEvent_protected(QResizeEvent * event) { PyPanel::resizeEvent(event); }
    void resizeEvent(QResizeEvent * event) override;
    void restore(const QString & arg__1) override;
    inline QString save_protected() { return PyPanel::save(); }
    QString save() override;
    void setVisible(bool visible) override;
    inline QPainter * sharedPainter_protected() const { return PyPanel::sharedPainter(); }
    QPainter * sharedPainter() const override;
    inline void showEvent_protected(QShowEvent * event) { PyPanel::showEvent(event); }
    void showEvent(QShowEvent * event) override;
    QSize sizeHint() const override;
    inline void tabletEvent_protected(QTabletEvent * event) { PyPanel::tabletEvent(event); }
    void tabletEvent(QTabletEvent * event) override;
    inline void timerEvent_protected(QTimerEvent * event) { PyPanel::timerEvent(event); }
    void timerEvent(QTimerEvent * event) override;
    inline void updateMicroFocus_protected() { PyPanel::updateMicroFocus(); }
    inline void wheelEvent_protected(QWheelEvent * event) { PyPanel::wheelEvent(event); }
    void wheelEvent(QWheelEvent * event) override;
    ~PyPanelWrapper();
public:
    int qt_metacall(QMetaObject::Call call, int id, void **args) override;
    void *qt_metacast(const char *_clname) override;
    static void pysideInitQtMetaTypes();
    void resetPyMethodCache();
private:
    mutable bool m_PyMethodCache[50];
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

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

#endif // SBK_PYPANELWRAPPER_H

