#ifndef SBK_APPWRAPPER_H
#define SBK_APPWRAPPER_H

#define protected public

#include <shiboken.h>

#include <AppInstanceWrapper.h>

class AppWrapper : public App
{
public:
    ~AppWrapper();
};

#endif // SBK_APPWRAPPER_H

