#ifndef SBK_APPWRAPPER_H
#define SBK_APPWRAPPER_H

#include <shiboken.h>

#include <AppInstanceWrapper.h>

class AppWrapper : public App
{
public:
    virtual ~AppWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_APPWRAPPER_H

