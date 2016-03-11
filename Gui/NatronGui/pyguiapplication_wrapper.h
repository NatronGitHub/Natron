#ifndef SBK_PYGUIAPPLICATIONWRAPPER_H
#define SBK_PYGUIAPPLICATIONWRAPPER_H

#include <shiboken.h>

#include <PyGlobalGui.h>

NATRON_NAMESPACE_ENTER;
class PyGuiApplicationWrapper : public PyGuiApplication
{
public:
    PyGuiApplicationWrapper();
    virtual ~PyGuiApplicationWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_NAMESPACE_EXIT;

#endif // SBK_PYGUIAPPLICATIONWRAPPER_H

