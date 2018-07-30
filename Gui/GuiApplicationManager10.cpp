/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "GuiApplicationManager.h"
#include "GuiApplicationManagerPrivate.h"

#include <stdexcept> // runtime_error

#include <boost/scoped_ptr.hpp>

#ifdef Q_OS_MAC
// for dockClickHandler and Application::Application()
#include <objc/objc.h>
#include <objc/message.h>
#endif

///gui
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QtGlobal> // for Q_OS_*
#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QtCore/QFileInfo>
#include <QApplication>
#include <QDesktopWidget>
#include <QFileOpenEvent>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/LibraryBinary.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/Settings.h"
#include "Engine/Project.h"
#include "Engine/ViewerInstance.h"
#include "Engine/Node.h"

#include "Global/QtCompat.h"

#include "Gui/Gui.h"
#include "Gui/ActionShortcuts.h"
#include "Gui/QtEnumConvert.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/KnobGuiFactory.h"
#include "Gui/SplashScreen.h"

// removed in qt5, just revert the commit (1b58d9acc493111390b31f0bffd6b2a76baca91b)
Q_INIT_RESOURCE_EXTERN(GuiResources);

/**
 * @macro Registers a keybind to the application.
 * @param group The name of the group under which the shortcut should be (e.g: Global, Viewer,NodeGraph...)
 * @param id The ID of the shortcut within the group so that we can find it later. The shorter the better, it isn't visible by the user.
 * @param description The label of the shortcut and the action visible in the GUI. This is what describes the action and what is visible in
 * the application's menus.
 * @param modifiers The modifiers of the keybind. This is of type Qt::KeyboardModifiers. Use Qt::NoModifier to indicate no modifier should
 * be held.
 * @param symbol The key symbol. This is of type Qt::Key. Use (Qt::Key)0 to indicate there's no keybind for this action.
 *
 * Note: Even actions that don't have a default shortcut should be registered this way so that the user can add its own shortcut
 * on them later. If you don't register an action with that macro, its shortcut won't be editable.
 * To register an action without a keybind use Qt::NoModifier and (Qt::Key)0
 **/
#define registerKeybind(group, id, description, modifiers, symbol) ( _imp->addKeybind(group, id, description, modifiers, symbol) )
#define registerKeybindWithMask(group, id, description, modifiers, symbol, modifiersMask) ( _imp->addKeybind(group, id, description, modifiers, symbol, modifiersMask) )


//Increment this when making change to default shortcuts or changes that would break expected default shortcuts
//in a way. This way the user will get prompted to restore default shortcuts on next launch
#define NATRON_SHORTCUTS_DEFAULT_VERSION 8

NATRON_NAMESPACE_ENTER


void
GuiApplicationManager::updateAllRecentFileMenus()
{
    const AppInstanceVec& instances = getAppInstances();

    for (AppInstanceVec::const_iterator it = instances.begin(); it != instances.end(); ++it) {
        GuiAppInstancePtr appInstance = toGuiAppInstance(*it);
        if (appInstance) {
            Gui* gui = appInstance->getGui();
            assert(gui);
            gui->updateRecentFileActions();
        }
    }
}

void
GuiApplicationManager::setLoadingStatus(const QString & str)
{
    if ( isLoaded() ) {
        return;
    }
    if (_imp->_splashScreen) {
        _imp->_splashScreen->updateText(str);
    }
}

AppInstancePtr
GuiApplicationManager::makeNewInstance(int appID) const
{
    return GuiAppInstance::create(appID);
}

KnobGuiWidgets*
GuiApplicationManager::createGuiForKnob(const KnobGuiPtr& knob, ViewIdx view) const
{
    return _imp->_knobGuiFactory->createGuiForKnob(knob, view);
}

void
GuiApplicationManager::registerGuiMetaTypes() const
{
}

#ifdef Q_OS_MAC
static bool
dockClickHandler(id self,SEL _cmd,...)
{
    Q_UNUSED(self)
    Q_UNUSED(_cmd)
    // Do something fun here!
    //qDebug() << "Dock icon clicked!";
    appPTR->onClickOnDock();

    // Return NO (false) to suppress the default OS X actions
    return false;
}

void
GuiApplicationManager::onClickOnDock()
{
    //qDebug() << "Dock icon clicked!";
    // do something...
    Q_EMIT dockClicked();
}
#endif

class Application
    : public QApplication
{
    GuiApplicationManager* _app;

public:

    Application(GuiApplicationManager* app,
                int &argc,
                char** argv)                                      ///< qapplication needs to be subclasses with reference otherwise lots of crashes will happen.
        : QApplication(argc, argv)
        , _app(app)
    {
        //setAttribute(Qt::AA_UseHighDpiPixmaps); // Qt 5
        Q_UNUSED(_app);

#ifdef Q_OS_MAC
        // see http://stackoverflow.com/questions/15143369/qt-on-os-x-how-to-detect-clicking-the-app-dock-icon

        // Starting from Qt5.4.0 you can handle QEvent, that related to click on dock: QEvent::ApplicationActivate.
        // https://bugreports.qt.io/browse/QTBUG-10899
        // https://doc.qt.io/qt-5/qevent.html

        id cls = (id)objc_getClass("NSApplication");
        id appInst = objc_msgSend(cls, sel_registerName("sharedApplication"));

        if(appInst != NULL) {
            objc_object* delegate = objc_msgSend(appInst, sel_registerName("delegate"));
            Class delClass = (Class)objc_msgSend(delegate,  sel_registerName("class"));
            SEL shouldHandle = sel_registerName("applicationShouldHandleReopen:hasVisibleWindows:");
            if (class_getInstanceMethod(delClass, shouldHandle)) {
                if (class_replaceMethod(delClass, shouldHandle, (IMP)dockClickHandler, "B@:"))
                    qDebug() << "Registered dock click handler (replaced original method)";
                else
                    qWarning() << "Failed to replace method for dock click handler";
            }
            else {
                if (class_addMethod(delClass, shouldHandle, (IMP)dockClickHandler,"B@:"))
                    qDebug() << "Registered dock click handler";
                else
                    qWarning() << "Failed to register dock click handler";
            }
        }
#endif
    }

protected:

    bool event(QEvent* e);
};

bool
Application::event(QEvent* e)
{
    switch ( e->type() ) {
#if defined(Q_OS_MAC) || defined(Q_OS_SYMBIAN)
    case QEvent::FileOpen: {
        // This class is currently supported for Mac OS X and Symbian only
        // http://doc.qt.io/qt-4.8/qfileopenevent.html
        // Linux and MSWindows use command-line arguments instead.
        // File associations are done using a registry database entry on MSWindows:
        // https://wiki.qt.io/Assigning_a_file_type_to_an_Application_on_Windows
        // and a mime entry on most Linux destops (KDE, GNOME):
        // http://www.freedesktop.org/wiki/Specifications/AddingMIMETutor/
        // http://www.freedesktop.org/wiki/Specifications/shared-mime-info-spec/
        // The MIME types for Natron documents are:
        // *.ntp: application/vnd.natron.project
        // *.nps: application/vnd.natron.nodepresets
        // *.nl: application/vnd.natron.layout
        assert(_app);
        QFileOpenEvent* foe = dynamic_cast<QFileOpenEvent*>(e);
        assert(foe);
        if (foe) {
            QString file =  foe->file();
#ifdef Q_OS_UNIX
            if ( !file.isEmpty() ) {
                file = AppManager::qt_tildeExpansion(file);
                QFileInfo fi(file);
                if ( fi.exists() ) {
                    file = fi.canonicalFilePath();
                }
            }
#endif
            _app->setFileToOpen(file);
        }

        return true;
    }
#endif

    default:

        return QApplication::event(e);
    }
}

void
GuiApplicationManager::initializeQApp(int &argc,
                                      char** argv)
{
    QApplication* app = new Application(this, argc, argv);
    QDesktopWidget* desktop = app->desktop();
    int dpiX = desktop->logicalDpiX();
    int dpiY = desktop->logicalDpiY();

    setCurrentLogicalDPI(dpiX, dpiY);

    app->setQuitOnLastWindowClosed(true);

    //Q_INIT_RESOURCE(GuiResources);
    // Q_INIT_RESOURCES expanded, and fixed for use from inside a namespace:
    // (requires using Q_INIT_RESOURCES_EXTERN(GuiResources) before entering the namespace)
    ::qInitResources_GuiResources();

#ifdef DEBUG
    QLocale loc;
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    loc = QApplication::keyboardInputLocale();
#else
    loc = QGuiApplication::inputMethod()->locale();
#endif
    qDebug() << "keyboard input locale:" << loc.bcp47Name();
#endif

}

QString
GuiApplicationManager::getAppFont() const
{
    return _imp->_fontFamily;
}

int
GuiApplicationManager::getAppFontSize() const
{
    return _imp->_fontSize;
}


void
GuiApplicationManager::setUndoRedoStackLimit(int limit)
{
    const AppInstanceVec & apps = getAppInstances();

    for (AppInstanceVec::const_iterator it = apps.begin(); it != apps.end(); ++it) {
        GuiAppInstancePtr guiApp = toGuiAppInstance(*it);
        if (guiApp) {
            guiApp->setUndoRedoStackLimit(limit);
        }
    }
}

void
GuiApplicationManager::debugImage(const ImagePtr& image,
                                  const RectI& roi,
                                  const QString & filename) const
{
    Gui::debugImage(image, roi, filename);
}

void
GuiApplicationManager::setFileToOpen(const QString & str)
{
    _imp->_openFileRequest = str;
    if ( isLoaded() && !_imp->_openFileRequest.isEmpty() ) {
        handleOpenFileRequest();
    }
}

bool
GuiApplicationManager::handleImageFileOpenRequest(const std::string& filename)
{
    QString fileCopy( QString::fromUtf8( filename.c_str() ) );
    QString ext = QtCompat::removeFileExtension(fileCopy);
    std::string readerFileType = appPTR->getReaderPluginIDForFileType( ext.toStdString() );
    AppInstancePtr mainInstance = appPTR->getTopLevelInstance();
    bool instanceCreated = false;

    if ( !mainInstance || !mainInstance->getProject()->isGraphWorthLess() ) {
        CLArgs cl;
        mainInstance = appPTR->newAppInstance(cl, false);
        instanceCreated = true;
    }
    if (!mainInstance) {
        return false;
    }

    CreateNodeArgsPtr args(CreateNodeArgs::create(readerFileType, mainInstance->getProject() ));
    args->addParamDefaultValue<std::string>(kOfxImageEffectFileParamName, filename);
    args->setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    args->setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
    NodePtr readerNode = mainInstance->createNode(args);
    if (!readerNode && instanceCreated) {
        mainInstance->quit();

        return false;
    }

    ///Find and connect the viewer
    NodesList allNodes = mainInstance->getProject()->getNodes();
    NodePtr viewerFound;
    for (NodesList::iterator it = allNodes.begin(); it != allNodes.end(); ++it) {
        if ( (*it)->isEffectViewerInstance() ) {
            viewerFound = *it;
            break;
        }
    }

    ///If no viewer is found, create it
    if (!viewerFound) {
        CreateNodeArgsPtr args(CreateNodeArgs::create(PLUGINID_NATRON_VIEWER_GROUP, mainInstance->getProject() ));
        args->setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
        args->setProperty<bool>(kCreateNodeArgsPropSubGraphOpened, false);
        args->setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
        viewerFound = mainInstance->createNode(args);
    }
    if (viewerFound) {
        viewerFound->connectInput(readerNode, 0);
    } else {
        return false;
    }

    return true;
}

void
GuiApplicationManager::handleOpenFileRequest()
{
    AppInstancePtr mainApp = getAppInstance(0);
    GuiAppInstancePtr guiApp = toGuiAppInstance(mainApp);

    assert(guiApp);
    if (guiApp) {
        ///Called when double-clicking a file from desktop
        std::string filename = _imp->_openFileRequest.toStdString();
        _imp->_openFileRequest.clear();
        guiApp->handleFileOpenEvent(filename);
    }
}

void
GuiApplicationManager::onLoadCompleted()
{
    if ( !_imp->_openFileRequest.isEmpty() ) {
        handleOpenFileRequest();
    }
}

void
GuiApplicationManager::exitApp(bool warnUserForSave)
{
    ///make a copy of the map because it will be modified when closing projects
    AppInstanceVec instances = getAppInstances();
    std::list<GuiAppInstancePtr> guiApps;

    for (AppInstanceVec::const_iterator it = instances.begin(); it != instances.end(); ++it) {
        GuiAppInstancePtr app = toGuiAppInstance(*it);
        if (app) {
            guiApps.push_back(app);
        }
    }

    std::set<GuiAppInstancePtr> triedInstances;
    while ( !guiApps.empty() ) {
        GuiAppInstancePtr app = guiApps.front();
        if (app) {
            triedInstances.insert(app);
            app->getGui()->abortProject(true, warnUserForSave, true);
        }

        //refreshg ui instances
        instances = getAppInstances();
        guiApps.clear();
        for (AppInstanceVec::const_iterator it = instances.begin(); it != instances.end(); ++it) {
            GuiAppInstancePtr ga = toGuiAppInstance(*it);
            if ( ga && ( triedInstances.find(ga) == triedInstances.end() ) ) {
                guiApps.push_back(ga);
            }
        }
    }
}

bool
GuiApplicationManager::matchesKeybind(const std::string & group,
                                      const std::string & actionID,
                                      const Qt::KeyboardModifiers & modifiers,
                                      int symbol) const
{
    return getCurrentSettings()->matchesKeybind(group, actionID, QtEnumConvert::fromQtModifiers(modifiers), QtEnumConvert::fromQtKey((Qt::Key)symbol));
}

QKeySequence
GuiApplicationManager::getKeySequenceForAction(const QString & group, const QString & actionID) const
{
    QKeySequence ret;
    KeyboardModifiers modif;
    Key key;
    if (!getCurrentSettings()->getShortcutKeybind(group.toStdString(), actionID.toStdString(), &modif, &key)) {
        return ret;
    }

    ret = makeKeySequence(QtEnumConvert::toQtModifiers(modif), QtEnumConvert::toQtKey(key));
    return ret;
}


void
GuiApplicationManager::showErrorLog()
{
    hideSplashScreen();
    GuiAppInstancePtr app = toGuiAppInstance( getTopLevelInstance() );
    if (app) {
        app->getGui()->showErrorLog();
    }
}




///The symbol has been generated by Shiboken in  Engine/NatronEngine/natronengine_module_wrapper.cpp
extern "C"
{
#if PY_MAJOR_VERSION >= 3
// Python 3
PyObject* PyInit_NatronGui();
#else
void initNatronGui();
#endif
}


void
GuiApplicationManager::initBuiltinPythonModules()
{
    AppManager::initBuiltinPythonModules();

#if PY_MAJOR_VERSION >= 3
    // Python 3
    int ret = PyImport_AppendInittab(NATRON_GUI_PYTHON_MODULE_NAME, &PyInit_NatronGui);
#else
    int ret = PyImport_AppendInittab(NATRON_GUI_PYTHON_MODULE_NAME, &initNatronGui);
#endif
    if (ret == -1) {
        throw std::runtime_error("Failed to initialize built-in Python module.");
    }
}


void
GuiApplicationManager::reloadStylesheets()
{
    const AppInstanceVec& instances = getAppInstances();

    for (AppInstanceVec::const_iterator it = instances.begin(); it != instances.end(); ++it) {
        GuiAppInstancePtr guiApp = toGuiAppInstance(*it);
        if (guiApp) {
            guiApp->reloadStylesheet();
        }
    }
}

void
GuiApplicationManager::reloadScriptEditorFonts()
{
    const AppInstanceVec& instances = getAppInstances();

    for (AppInstanceVec::const_iterator it = instances.begin(); it != instances.end(); ++it) {
        GuiAppInstancePtr guiApp = toGuiAppInstance(*it);
        if (guiApp) {
            guiApp->reloadScriptEditorFonts();
        }
    }
}

NATRON_NAMESPACE_EXIT
