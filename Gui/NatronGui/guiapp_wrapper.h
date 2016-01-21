#ifndef SBK_GUIAPPWRAPPER_H
#define SBK_GUIAPPWRAPPER_H

#include <shiboken.h>

#include <GuiAppWrapper.h>

NATRON_NAMESPACE_ENTER;
class GuiAppWrapper : public GuiApp
{
public:
    virtual ~GuiAppWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_NAMESPACE_EXIT;

#endif // SBK_GUIAPPWRAPPER_H

