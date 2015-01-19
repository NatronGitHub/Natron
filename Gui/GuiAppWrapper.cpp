#include "GuiAppWrapper.h"

GuiApp::GuiApp(AppInstance* app)
: App(app)
, _app(dynamic_cast<GuiAppInstance*>(app))
{
    assert(_app);
}


GuiApp::~GuiApp()
{
    
}