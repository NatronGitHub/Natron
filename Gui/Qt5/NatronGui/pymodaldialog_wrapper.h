#ifndef SBK_PYMODALDIALOGWRAPPER_H
#define SBK_PYMODALDIALOGWRAPPER_H

#include <PythonPanels.h>


// Extra includes
#include <PyParameter.h>
NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class PyModalDialogWrapper : public PyModalDialog
{
public:
    void accept() override;
    inline void adjustPosition_protected(QWidget * arg__1) { PyModalDialog::adjustPosition(arg__1); }
    inline void closeEvent_protected(QCloseEvent * arg__1) { PyModalDialog::closeEvent(arg__1); }
    void closeEvent(QCloseEvent * arg__1) override;
    inline void contextMenuEvent_protected(QContextMenuEvent * arg__1) { PyModalDialog::contextMenuEvent(arg__1); }
    void contextMenuEvent(QContextMenuEvent * arg__1) override;
    void done(int arg__1) override;
    inline bool eventFilter_protected(QObject * arg__1, QEvent * arg__2) { return PyModalDialog::eventFilter(arg__1, arg__2); }
    bool eventFilter(QObject * arg__1, QEvent * arg__2) override;
    int exec() override;
    inline void keyPressEvent_protected(QKeyEvent * arg__1) { PyModalDialog::keyPressEvent(arg__1); }
    void keyPressEvent(QKeyEvent * arg__1) override;
    void open() override;
    void reject() override;
    inline void resizeEvent_protected(QResizeEvent * arg__1) { PyModalDialog::resizeEvent(arg__1); }
    void resizeEvent(QResizeEvent * arg__1) override;
    void setVisible(bool visible) override;
    inline void showEvent_protected(QShowEvent * arg__1) { PyModalDialog::showEvent(arg__1); }
    void showEvent(QShowEvent * arg__1) override;
    ~PyModalDialogWrapper();
    static void pysideInitQtMetaTypes();
    void resetPyMethodCache();
private:
    mutable bool m_PyMethodCache[12];
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
    inline void adjustPosition_protected(QWidget * arg__1) { QDialog::adjustPosition(arg__1); }
    inline void closeEvent_protected(QCloseEvent * arg__1) { QDialog::closeEvent(arg__1); }
    void closeEvent(QCloseEvent * arg__1) override;
    inline void contextMenuEvent_protected(QContextMenuEvent * arg__1) { QDialog::contextMenuEvent(arg__1); }
    void contextMenuEvent(QContextMenuEvent * arg__1) override;
    void done(int arg__1) override;
    inline bool eventFilter_protected(QObject * arg__1, QEvent * arg__2) { return QDialog::eventFilter(arg__1, arg__2); }
    bool eventFilter(QObject * arg__1, QEvent * arg__2) override;
    int exec() override;
    inline void keyPressEvent_protected(QKeyEvent * arg__1) { QDialog::keyPressEvent(arg__1); }
    void keyPressEvent(QKeyEvent * arg__1) override;
    QSize minimumSizeHint() const override;
    void open() override;
    void reject() override;
    inline void resizeEvent_protected(QResizeEvent * arg__1) { QDialog::resizeEvent(arg__1); }
    void resizeEvent(QResizeEvent * arg__1) override;
    void setVisible(bool visible) override;
    inline void showEvent_protected(QShowEvent * arg__1) { QDialog::showEvent(arg__1); }
    void showEvent(QShowEvent * arg__1) override;
    QSize sizeHint() const override;
    ~QDialogWrapper();
    static void pysideInitQtMetaTypes();
    void resetPyMethodCache();
private:
    mutable bool m_PyMethodCache[14];
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#  endif // SBK_QDIALOGWRAPPER_H

#endif // SBK_PYMODALDIALOGWRAPPER_H

