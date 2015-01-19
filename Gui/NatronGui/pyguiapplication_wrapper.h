#ifndef SBK_PYGUIAPPLICATIONWRAPPER_H
#define SBK_PYGUIAPPLICATIONWRAPPER_H

#define protected public

#include <shiboken.h>

#include <GlobalGuiWrapper.h>

class PyGuiApplicationWrapper : public PyGuiApplication
{
public:
    PyGuiApplicationWrapper();
    virtual ~PyGuiApplicationWrapper();
};

#endif // SBK_PYGUIAPPLICATIONWRAPPER_H

