#ifndef SBK_GUIAPPWRAPPER_H
#define SBK_GUIAPPWRAPPER_H

#include <shiboken.h>

#include <GuiAppWrapper.h>

class GuiAppWrapper : public GuiApp
{
public:
    virtual ~GuiAppWrapper();
    static void pysideInitQtMetaTypes();
};

#endif // SBK_GUIAPPWRAPPER_H

