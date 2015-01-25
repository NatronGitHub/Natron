#include "GuiAppWrapper.h"

#include "Gui/PythonPanels.h"

GuiApp::GuiApp(AppInstance* app)
: App(app)
, _app(dynamic_cast<GuiAppInstance*>(app))
{
    assert(_app);
}


GuiApp::~GuiApp()
{
    
}

PyModalDialog*
GuiApp::createModalDialog()
{
    PyModalDialog* ret = new PyModalDialog(_app->getGui());
    return ret;
}