#ifndef SBK_GUIAPPWRAPPER_H
#define SBK_GUIAPPWRAPPER_H

#include <shiboken.h>

#include <PyGuiApp.h>

NATRON_NAMESPACE_ENTER NATRON_PYTHON_NAMESPACE_ENTER
class GuiAppWrapper : public GuiApp
{
public:
    virtual ~GuiAppWrapper();
    static void pysideInitQtMetaTypes();
};
NATRON_PYTHON_NAMESPACE_EXIT NATRON_NAMESPACE_EXIT

#endif // SBK_GUIAPPWRAPPER_H

