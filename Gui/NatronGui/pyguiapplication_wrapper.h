#ifndef SBK_PYGUIAPPLICATIONWRAPPER_H
#define SBK_PYGUIAPPLICATIONWRAPPER_H

#include <shiboken.h>

#include <GlobalGuiWrapper.h>

class PyGuiApplicationWrapper : public PyGuiApplication
{
public:
    PyGuiApplicationWrapper();
    virtual ~PyGuiApplicationWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_PYGUIAPPLICATIONWRAPPER_H

