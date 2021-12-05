#ifndef SBK_PYPANELWRAPPER_H
#define SBK_PYPANELWRAPPER_H

#include <PythonPanels.h>


// Extra includes
#include <PyGuiApp.h>
#include <PyParameter.h>
#include <list>
NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class PyPanelWrapper : public PyPanel
{
public:
    PyPanelWrapper(const QString & scriptName, const QString & label, bool useUserParameters, GuiApp * app);
    inline void enterEvent_protected(QEvent * e) { PyPanel::enterEvent(e); }
    void enterEvent(QEvent * e) override;
    inline void keyPressEvent_protected(QKeyEvent * e) { PyPanel::keyPressEvent(e); }
    void keyPressEvent(QKeyEvent * e) override;
    inline void leaveEvent_protected(QEvent * e) { PyPanel::leaveEvent(e); }
    void leaveEvent(QEvent * e) override;
    inline void mousePressEvent_protected(QMouseEvent * e) { PyPanel::mousePressEvent(e); }
    void mousePressEvent(QMouseEvent * e) override;
    inline void onUserDataChanged_protected() { PyPanel::onUserDataChanged(); }
    void restore(const QString & arg__1) override;
    inline QString save_protected() { return PyPanel::save(); }
    QString save() override;
    ~PyPanelWrapper();
    static void pysideInitQtMetaTypes();
    void resetPyMethodCache();
private:
    mutable bool m_PyMethodCache[6];
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#endif // SBK_PYPANELWRAPPER_H

