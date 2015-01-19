#ifndef SBK_GUIAPPWRAPPER_H
#define SBK_GUIAPPWRAPPER_H

#define protected public

#include <shiboken.h>

#include <GuiAppWrapper.h>

class GuiAppWrapper : public GuiApp
{
public:
    virtual ~GuiAppWrapper();
};

#endif // SBK_GUIAPPWRAPPER_H

