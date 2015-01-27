#include "GuiAppWrapper.h"

#include <QColorDialog>

#include "Gui/PythonPanels.h"
#include "Gui/Gui.h"
#include "Gui/TabWidget.h"
#include "Gui/SequenceFileDialog.h"

GuiApp::GuiApp(AppInstance* app)
: App(app)
, _app(dynamic_cast<GuiAppInstance*>(app))
{
    assert(_app);
}


GuiApp::~GuiApp()
{
    
}

Gui*
GuiApp::getGui() const
{
    return _app->getGui();
}

PyModalDialog*
GuiApp::createModalDialog()
{
    PyModalDialog* ret = new PyModalDialog(_app->getGui());
    return ret;
}


PyTabWidget*
GuiApp::getTabWidget(const std::string& name) const
{
    const std::list<TabWidget*>& tabs = _app->getGui()->getPanes();
    for ( std::list<TabWidget*>::const_iterator it = tabs.begin(); it != tabs.end(); ++it) {
        if ((*it)->objectName_mt_safe().toStdString() == name) {
            return new PyTabWidget(*it);
        }
    }
    return 0;
}

bool
GuiApp::moveTab(QWidget* tab,PyTabWidget* pane)
{
    return TabWidget::moveTab(tab, pane->getInternalTabWidget());
}

void
GuiApp::registerPythonPanel(PyPanel* panel,const std::string& pythonFunction)
{
    _app->getGui()->registerPyPanel(panel,pythonFunction);
}

void
GuiApp::unregisterPythonPanel(PyPanel* panel)
{
    _app->getGui()->unregisterPyPanel(panel);
}

std::string
GuiApp::getFilenameDialog(const std::vector<std::string>& filters,
                          const std::string& location) const
{
    Gui* gui = _app->getGui();
    
    SequenceFileDialog dialog(gui,
                              filters,
                              false,
                              SequenceFileDialog::eFileDialogModeOpen,
                              location,
                              gui,
                              false);
    if (dialog.exec()) {
        std::string ret = dialog.selectedFiles();
        return ret;
    }
    return std::string();
}

std::string
GuiApp::getSequenceDialog(const std::vector<std::string>& filters,
                          const std::string& location) const
{
    Gui* gui = _app->getGui();
    
    SequenceFileDialog dialog(gui,
                              filters,
                              true,
                              SequenceFileDialog::eFileDialogModeOpen,
                              location,
                              gui,
                              false);
    if (dialog.exec()) {
        std::string ret = dialog.selectedFiles();
        return ret;
    }
    return std::string();
}

std::string
GuiApp::getDirectoryDialog(const std::string& location) const
{
    Gui* gui = _app->getGui();
    std::vector<std::string> filters;
    SequenceFileDialog dialog(gui,
                              filters,
                              false,
                              SequenceFileDialog::eFileDialogModeDir,
                              location,
                              gui,
                              false);
    if (dialog.exec()) {
        std::string ret = dialog.selectedDirectory();
        return ret;
    }
    return std::string();

}

std::string
GuiApp::saveFilenameDialog(const std::vector<std::string>& filters,
                           const std::string& location) const
{
    Gui* gui = _app->getGui();
    
    SequenceFileDialog dialog(gui,
                              filters,
                              false,
                              SequenceFileDialog::eFileDialogModeSave,
                              location,
                              gui,
                              false);
    if (dialog.exec()) {
        std::string ret = dialog.filesToSave();
        return ret;
    }
    return std::string();

}

std::string
GuiApp::saveSequenceDialog(const std::vector<std::string>& filters,
                           const std::string& location) const
{
    Gui* gui = _app->getGui();
    
    SequenceFileDialog dialog(gui,
                              filters,
                              true,
                              SequenceFileDialog::eFileDialogModeSave,
                              location,
                              gui,
                              false);
    if (dialog.exec()) {
        std::string ret = dialog.filesToSave();
        return ret;
    }
    return std::string();

}

ColorTuple
GuiApp::getRGBColorDialog() const
{
    ColorTuple ret;
    
    QColorDialog dialog;
    if (dialog.exec()) {
        QColor color = dialog.currentColor();
        
        ret.r = color.redF();
        ret.g = color.greenF();
        ret.b = color.blueF();
        ret.a = 1.;
        
    } else {
        ret.r = ret.g = ret.b = ret.a = 0.;
    }
    return ret;
}