#ifndef SBK_PYGUIAPPLICATIONWRAPPER_H
#define SBK_PYGUIAPPLICATIONWRAPPER_H

#include <PyGlobalGui.h>


// Extra includes
#include <qpixmap.h>
#include <PyGuiApp.h>
#include <PyAppInstance.h>
NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class PyGuiApplicationWrapper : public PyGuiApplication
{
public:
    PyGuiApplicationWrapper();
    ~PyGuiApplicationWrapper();
    static void pysideInitQtMetaTypes();
    void resetPyMethodCache();
private:
    mutable bool m_PyMethodCache[1];
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#  ifndef SBK_PYCOREAPPLICATIONWRAPPER_H
#  define SBK_PYCOREAPPLICATIONWRAPPER_H

// Inherited base class:
NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class PyCoreApplicationWrapper : public PyCoreApplication
{
public:
    PyCoreApplicationWrapper();
    ~PyCoreApplicationWrapper();
    static void pysideInitQtMetaTypes();
    void resetPyMethodCache();
private:
    mutable bool m_PyMethodCache[1];
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#  endif // SBK_PYCOREAPPLICATIONWRAPPER_H

#endif // SBK_PYGUIAPPLICATIONWRAPPER_H

