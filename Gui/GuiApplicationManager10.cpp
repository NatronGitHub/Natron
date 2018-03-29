/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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
#include "Engine/KnobSerialization.h"
#include "Engine/Node.h"

#include "Global/QtCompat.h"

#include "Gui/CurveWidget.h"
#include "Gui/Gui.h"
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
#define registerKeybind2(group, id, description, modifiers1, symbol1, modifiers2, symbol2) ( _imp->addKeybind(group, id, description, modifiers1, symbol1, modifiers2, symbol2) )
#define registerKeybindWithMask(group, id, description, modifiers, symbol, modifiersMask) ( _imp->addKeybind(group, id, description, modifiers, symbol, modifiersMask) )

/**
 * @brief Performs the same that the registerKeybind macro, except that it takes a QKeySequence::StandardKey in parameter.
 * This way the keybind will be standard and will adapt bery well across all platforms supporting the standard.
 * If the standard parameter has no shortcut associated, the fallbackmodifiers and fallbacksymbol will be used instead.
 **/
#define registerStandardKeybind(group, id, description, key, fallbackmodifiers, fallbacksymbol) ( _imp->addStandardKeybind(group, id, description, key, fallbackmodifiers, fallbacksymbol) )

/**
 * @brief Performs the same that the registerKeybind macro, except that it works for shortcuts using mouse buttons instead of key symbols.
 * Generally you don't want this kind of interaction to be editable because it might break compatibility with non 3-button mouses.
 * Also the shortcut editor of Natron doesn't support editing mouse buttons.
 * In the end a mouse shortcut will be NON editable. The reason why you should call this is just to show the shortcut in the shortcut view
 * so that the user is aware it exists, but he/she will not be able to modify it.
 **/
#define registerMouseShortcut(group, id, description, modifiers, button) ( _imp->addMouseShortcut(group, id, description, modifiers, button) )

//Increment this when making change to default shortcuts or changes that would break expected default shortcuts
//in a way. This way the user will get prompted to restore default shortcuts on next launch
#define NATRON_SHORTCUTS_DEFAULT_VERSION 8

NATRON_NAMESPACE_ENTER


void
GuiApplicationManager::updateAllRecentFileMenus()
{
    const AppInstanceVec& instances = getAppInstances();

    for (AppInstanceVec::const_iterator it = instances.begin(); it != instances.end(); ++it) {
        GuiAppInstance* appInstance = dynamic_cast<GuiAppInstance*>( it->get() );
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

void
GuiApplicationManager::loadBuiltinNodePlugins(IOPluginsMap* readersMap,
                                              IOPluginsMap* writersMap)
{
    // Nothing special, load base class implementation
    AppManager::loadBuiltinNodePlugins(readersMap, writersMap);
}

AppInstPtr
GuiApplicationManager::makeNewInstance(int appID) const
{
    return AppInstPtr( new GuiAppInstance(appID) );
}

KnobGui*
GuiApplicationManager::createGuiForKnob(KnobPtr knob,
                                        KnobGuiContainerI *container) const
{
    return _imp->_knobGuiFactory->createGuiForKnob(knob, container);
}

void
GuiApplicationManager::registerGuiMetaTypes() const
{
    qRegisterMetaType<CurveWidget*>();
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

    ///Register all the shortcuts.
    populateShortcuts();
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
GuiApplicationManager::onAllPluginsLoaded()
{
    ///Restore user shortcuts only when all plug-ins are populated.
    loadShortcuts();
    AppManager::onAllPluginsLoaded();
}

void
GuiApplicationManager::setUndoRedoStackLimit(int limit)
{
    const AppInstanceVec & apps = getAppInstances();

    for (AppInstanceVec::const_iterator it = apps.begin(); it != apps.end(); ++it) {
        GuiAppInstance* guiApp = dynamic_cast<GuiAppInstance*>( it->get() );
        if (guiApp) {
            guiApp->setUndoRedoStackLimit(limit);
        }
    }
}

void
GuiApplicationManager::debugImage(const Image* image,
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
    AppInstPtr mainInstance = appPTR->getTopLevelInstance();
    bool instanceCreated = false;

    if ( !mainInstance || !mainInstance->getProject()->isGraphWorthLess() ) {
        CLArgs cl;
        mainInstance = appPTR->newAppInstance(cl, false);
        instanceCreated = true;
    }
    if (!mainInstance) {
        return false;
    }

    CreateNodeArgs args(readerFileType, mainInstance->getProject() );
    args.addParamDefaultValue<std::string>(kOfxImageEffectFileParamName, filename);
    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
    NodePtr readerNode = mainInstance->createNode(args);
    if (!readerNode && instanceCreated) {
        mainInstance->quit();

        return false;
    }

    ///Find and connect the viewer
    NodesList allNodes = mainInstance->getProject()->getNodes();
    NodePtr viewerFound;
    for (NodesList::iterator it = allNodes.begin(); it != allNodes.end(); ++it) {
        if ( (*it)->isEffectViewer() ) {
            viewerFound = *it;
            break;
        }
    }

    ///If no viewer is found, create it
    if (!viewerFound) {
        CreateNodeArgs args(PLUGINID_NATRON_VIEWER, mainInstance->getProject() );
        args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
        args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
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
    AppInstPtr mainApp = getAppInstance(0);
    GuiAppInstance* guiApp = dynamic_cast<GuiAppInstance*>( mainApp.get() );

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
    std::list<GuiAppInstPtr> guiApps;

    for (AppInstanceVec::const_iterator it = instances.begin(); it != instances.end(); ++it) {
        GuiAppInstPtr app = boost::dynamic_pointer_cast<GuiAppInstance>(*it);
        if (app) {
            guiApps.push_back(app);
        }
    }

    std::set<GuiAppInstPtr> triedInstances;
    while ( !guiApps.empty() ) {
        GuiAppInstPtr app = guiApps.front();
        if (app) {
            triedInstances.insert(app);
            app->getGui()->abortProject(true, warnUserForSave, true);
        }

        //refreshg ui instances
        instances = getAppInstances();
        guiApps.clear();
        for (AppInstanceVec::const_iterator it = instances.begin(); it != instances.end(); ++it) {
            GuiAppInstPtr ga = boost::dynamic_pointer_cast<GuiAppInstance>(*it);
            if ( ga && ( triedInstances.find(ga) == triedInstances.end() ) ) {
                guiApps.push_back(ga);
            }
        }
    }
}

static bool
matchesModifers(const Qt::KeyboardModifiers & lhs,
                const Qt::KeyboardModifiers & rhs,
                const Qt::KeyboardModifiers& ignoreMask)
{
    bool hasMask = ignoreMask != Qt::NoModifier;

    return (!hasMask && lhs == rhs) ||
           (hasMask && (lhs & ~ignoreMask) == rhs);
}

static bool
matchesKey(Qt::Key lhs,
           Qt::Key rhs)
{
    if (lhs == rhs) {
        return true;
    }
    ///special case for the backspace and delete keys that mean the same thing generally
    else if ( ( (lhs == Qt::Key_Backspace) && (rhs == Qt::Key_Delete) ) ||
              ( ( lhs == Qt::Key_Delete) && ( rhs == Qt::Key_Backspace) ) ) {
        return true;
    }
    ///special case for the return and enter keys that mean the same thing generally
    else if ( ( (lhs == Qt::Key_Return) && (rhs == Qt::Key_Enter) ) ||
              ( ( lhs == Qt::Key_Enter) && ( rhs == Qt::Key_Return) ) ) {
        return true;
    }

    return false;
}

bool
GuiApplicationManager::matchesKeybind(const std::string & stdgroup,
                                      const std::string & stdactionID,
                                      const Qt::KeyboardModifiers & modifiers,
                                      int symbol) const
{
    QString actionID = QString::fromUtf8( stdactionID.c_str() );
    QString group = QString::fromUtf8( stdgroup.c_str() );
    AppShortcuts::const_iterator it = _imp->_actionShortcuts.find(group);

    if ( it == _imp->_actionShortcuts.end() ) {
        // we didn't find the group
        return false;
    }

    GroupShortcuts::const_iterator it2 = it->second.find(actionID);
    if ( it2 == it->second.end() ) {
        // we didn't find the action
        return false;
    }

    const KeyBoundAction* keybind = dynamic_cast<const KeyBoundAction*>(it2->second);
    if (!keybind) {
        return false;
    }

    // the following macro only tests the Control, Alt, and Shift modifiers, and discards the others
    Qt::KeyboardModifiers onlyCAS = modifiers & (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier);

    assert( keybind->modifiers.size() == keybind->currentShortcut.size() );
    std::list<Qt::Key>::const_iterator kit = keybind->currentShortcut.begin();


    for (std::list<Qt::KeyboardModifiers>::const_iterator it = keybind->modifiers.begin(); it != keybind->modifiers.end(); ++it, ++kit) {
        if ( matchesModifers(onlyCAS, *it, keybind->ignoreMask) ) {
            // modifiers are equal, now test symbol
            return matchesKey( (Qt::Key)symbol, *kit );
        }
    }

    return false;
}

bool
GuiApplicationManager::matchesMouseShortcut(const std::string & stdgroup,
                                            const std::string & stdactionID,
                                            const Qt::KeyboardModifiers & modifiers,
                                            int button) const
{
    QString actionID = QString::fromUtf8( stdactionID.c_str() );
    QString group = QString::fromUtf8( stdgroup.c_str() );
    AppShortcuts::const_iterator it = _imp->_actionShortcuts.find(group);

    if ( it == _imp->_actionShortcuts.end() ) {
        // we didn't find the group
        return false;
    }

    GroupShortcuts::const_iterator it2 = it->second.find(actionID);
    if ( it2 == it->second.end() ) {
        // we didn't find the action
        return false;
    }

    const MouseAction* mAction = dynamic_cast<const MouseAction*>(it2->second);
    if (!mAction) {
        return false;
    }

    // the following macro only tests the Control, Alt, and Shift (and cmd an mac) modifiers, and discards the others
    Qt::KeyboardModifiers onlyMCAS = modifiers & (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier | Qt::MetaModifier);

    /*Note that the default configuration of Apple's X11 (see X11 menu:Preferences:Input tab) is to "emulate three button mouse". This sets a single button mouse or laptop trackpad to behave as follows:
       X11 left mouse button click
       X11 middle mouse button Option-click
       X11 right mouse button Ctrl-click
       (Command=clover=apple key)*/

    if ( (onlyMCAS == Qt::AltModifier) && ( (Qt::MouseButton)button == Qt::LeftButton ) ) {
        onlyMCAS = Qt::NoModifier;
        button = Qt::MiddleButton;
    } else if ( (onlyMCAS == Qt::MetaModifier) && ( (Qt::MouseButton)button == Qt::LeftButton ) ) {
        onlyMCAS = Qt::NoModifier;
        button = Qt::RightButton;
    }

    for (std::list<Qt::KeyboardModifiers>::const_iterator it = mAction->modifiers.begin(); it != mAction->modifiers.end(); ++it) {
        if (onlyMCAS == *it) {
            // modifiers are equal, now test symbol
            if ( (Qt::MouseButton)button == mAction->button ) {
                return true;
            }
        }
    }


    return false;
}

void
GuiApplicationManager::saveShortcuts() const
{
    QSettings settings( QString::fromUtf8(NATRON_ORGANIZATION_NAME), QString::fromUtf8(NATRON_APPLICATION_NAME) );

    for (AppShortcuts::const_iterator it = _imp->_actionShortcuts.begin(); it != _imp->_actionShortcuts.end(); ++it) {
        settings.beginGroup(it->first);
        for (GroupShortcuts::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            const MouseAction* mAction = dynamic_cast<const MouseAction*>(it2->second);
            const KeyBoundAction* kAction = dynamic_cast<const KeyBoundAction*>(it2->second);

            settings.setValue( it2->first + QString::fromUtf8("_nbShortcuts"), QVariant( (int)it2->second->modifiers.size() ) );

            int c = 0;
            for (std::list<Qt::KeyboardModifiers>::const_iterator it3 = it2->second->modifiers.begin(); it3 != it2->second->modifiers.end(); ++it3, ++c) {
                settings.setValue(it2->first + QString::fromUtf8("_Modifiers") + QString::number(c), (int)*it3);
            }

            if (mAction) {
                settings.setValue(it2->first + QString::fromUtf8("_Button"), (int)mAction->button);
            } else if (kAction) {
                assert( it2->second->modifiers.size() == kAction->currentShortcut.size() );

                c = 0;
                for (std::list<Qt::Key>::const_iterator it3 = kAction->currentShortcut.begin(); it3 != kAction->currentShortcut.end(); ++it3, ++c) {
                    settings.setValue(it2->first + QString::fromUtf8("_Symbol") + QString::number(c), (int)*it3);
                }
            }
        }
        settings.endGroup();
    }
}

void
GuiApplicationManager::loadShortcuts()
{
    bool settingsExistd = getCurrentSettings()->didSettingsExistOnStartup();
    QSettings settings( QString::fromUtf8(NATRON_ORGANIZATION_NAME), QString::fromUtf8(NATRON_APPLICATION_NAME) );
    int settingsVersion = -1;

    if ( settings.contains( QString::fromUtf8("NATRON_SHORTCUTS_DEFAULT_VERSION") ) ) {
        settingsVersion = settings.value( QString::fromUtf8("NATRON_SHORTCUTS_DEFAULT_VERSION") ).toInt();
    }

    if ( settingsExistd && (settingsVersion < NATRON_SHORTCUTS_DEFAULT_VERSION) ) {
        _imp->_shortcutsChangedVersion = true;
    }

    settings.setValue(QString::fromUtf8("NATRON_SHORTCUTS_DEFAULT_VERSION"), NATRON_SHORTCUTS_DEFAULT_VERSION);


    for (AppShortcuts::iterator it = _imp->_actionShortcuts.begin(); it != _imp->_actionShortcuts.end(); ++it) {
        settings.beginGroup(it->first);
        for (GroupShortcuts::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            MouseAction* mAction = dynamic_cast<MouseAction*>(it2->second);
            KeyBoundAction* kAction = dynamic_cast<KeyBoundAction*>(it2->second);
            int nbShortcuts = 1;
            bool nbShortcutsSet = false;
            if ( settings.contains( it2->first + QString::fromUtf8("_nbShortcuts") ) ) {
                nbShortcuts = settings.value( it2->first + QString::fromUtf8("_nbShortcuts") ).toInt();
                nbShortcutsSet = true;
            }

            bool hasNonNullKeybind = false;

            if (!nbShortcutsSet) {
                if (kAction) {
                    if ( settings.contains( it2->first + QString::fromUtf8("_Symbol") ) ) {
                        Qt::Key key = (Qt::Key)settings.value( it2->first + QString::fromUtf8("_Symbol") ).toInt();
                        if (key != (Qt::Key)0) {
                            kAction->currentShortcut.clear();
                            kAction->currentShortcut.push_back(key);
                            hasNonNullKeybind = true;
                        }
                    }
                }
                if ( hasNonNullKeybind && settings.contains( it2->first + QString::fromUtf8("_Modifiers") ) ) {
                    it2->second->modifiers.clear();
                    it2->second->modifiers.push_back( Qt::KeyboardModifiers( settings.value( it2->first + QString::fromUtf8("_Modifiers") ).toInt() ) );
                }
            } else {
                for (int i = 0; i < nbShortcuts; ++i) {
                    QString idm = it2->first + QString::fromUtf8("_Modifiers") + QString::number(i);

                    if (kAction) {
                        QString ids = it2->first + QString::fromUtf8("_Symbol") + QString::number(i);
                        if ( settings.contains(ids) ) {
                            Qt::Key key = (Qt::Key)settings.value(ids).toInt();
                            if (key != (Qt::Key)0) {
                                if (i == 0) {
                                    kAction->currentShortcut.clear();
                                }
                                kAction->currentShortcut.push_back(key);
                                hasNonNullKeybind = true;
                            } else {
                                continue;
                            }
                        }
                    }
                    if ( settings.contains(idm) ) {
                        if (i == 0) {
                            it2->second->modifiers.clear();
                        }
                        it2->second->modifiers.push_back( Qt::KeyboardModifiers( settings.value(idm).toInt() ) );
                    }
                }
            }
            if ( mAction && settings.contains( it2->first + QString::fromUtf8("_Button") ) ) {
                mAction->button = (Qt::MouseButton)settings.value( it2->first + QString::fromUtf8("_Button") ).toInt();
            }
            //If this is a node shortcut, notify the Plugin object that it has a shortcut.
            if ( hasNonNullKeybind && it->first.startsWith( QString::fromUtf8(kShortcutGroupNodes) ) ) {
                const PluginsMap & allPlugins = getPluginsList();
                PluginsMap::const_iterator found = allPlugins.find( it2->first.toStdString() );
                if ( found != allPlugins.end() ) {
                    assert(found->second.size() > 0);
                    ( *found->second.rbegin() )->setHasShortcut(true);
                }
            }
        }
        settings.endGroup();
    }
} // GuiApplicationManager::loadShortcuts

bool
GuiApplicationManager::isShorcutVersionUpToDate() const
{
    return !_imp->_shortcutsChangedVersion;
}

void
GuiApplicationManager::restoreDefaultShortcuts()
{
    for (AppShortcuts::iterator it = _imp->_actionShortcuts.begin(); it != _imp->_actionShortcuts.end(); ++it) {
        for (GroupShortcuts::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            KeyBoundAction* kAction = dynamic_cast<KeyBoundAction*>(it2->second);
            it2->second->modifiers = it2->second->defaultModifiers;
            if (kAction) {
                kAction->currentShortcut = kAction->defaultShortcut;
                notifyShortcutChanged(kAction);
            }
            ///mouse actions cannot have their button changed
        }
    }
}

void
GuiApplicationManager::populateShortcuts()
{
    ///General
    registerStandardKeybind(kShortcutGroupGlobal, kShortcutIDActionNewProject, kShortcutDescActionNewProject, QKeySequence::New, Qt::ControlModifier, Qt::Key_N);
    registerStandardKeybind(kShortcutGroupGlobal, kShortcutIDActionOpenProject, kShortcutDescActionOpenProject, QKeySequence::Open, Qt::ControlModifier, Qt::Key_O);
    registerStandardKeybind(kShortcutGroupGlobal, kShortcutIDActionSaveProject, kShortcutDescActionSaveProject, QKeySequence::Save, Qt::ControlModifier, Qt::Key_S);
    registerStandardKeybind(kShortcutGroupGlobal, kShortcutIDActionSaveAsProject, kShortcutDescActionSaveAsProject, QKeySequence::SaveAs, Qt::ControlModifier | Qt::ShiftModifier, Qt::Key_S);
    registerStandardKeybind(kShortcutGroupGlobal, kShortcutIDActionCloseProject, kShortcutDescActionCloseProject, QKeySequence::Close, Qt::ControlModifier, Qt::Key_W);
    registerStandardKeybind(kShortcutGroupGlobal, kShortcutIDActionPreferences, kShortcutDescActionPreferences, QKeySequence::Preferences, Qt::ShiftModifier, Qt::Key_S);
    registerStandardKeybind(kShortcutGroupGlobal, kShortcutIDActionQuit, kShortcutDescActionQuit, QKeySequence::Quit, Qt::ControlModifier, Qt::Key_Q);

    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionSaveAndIncrVersion, kShortcutDescActionSaveAndIncrVersion, Qt::ControlModifier | Qt::ShiftModifier |
                    Qt::AltModifier, Qt::Key_S);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionReloadProject, kShortcutDescActionReloadProject, Qt::ControlModifier | Qt::ShiftModifier, Qt::Key_R);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionExportProject, kShortcutDescActionExportProject, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionShowAbout, kShortcutDescActionShowAbout, Qt::NoModifier, (Qt::Key)0);

    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionImportLayout, kShortcutDescActionImportLayout, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionExportLayout, kShortcutDescActionExportLayout, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionDefaultLayout, kShortcutDescActionDefaultLayout, Qt::NoModifier, (Qt::Key)0);

    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionProjectSettings, kShortcutDescActionProjectSettings, Qt::NoModifier, Qt::Key_S);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionShowErrorLog, kShortcutDescActionShowErrorLog, Qt::NoModifier, (Qt::Key)0);

    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionNewViewer, kShortcutDescActionNewViewer, Qt::ControlModifier, Qt::Key_I);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionFullscreen, kShortcutDescActionFullscreen, Qt::AltModifier, Qt::Key_S); // as in Nuke
    //registerKeybind(kShortcutGroupGlobal, kShortcutIDActionFullscreen, kShortcutDescActionFullscreen, Qt::ControlModifier | Qt::AltModifier, Qt::Key_F);

    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionClearDiskCache, kShortcutDescActionClearDiskCache, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionClearPlaybackCache, kShortcutDescActionClearPlaybackCache, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionClearNodeCache, kShortcutDescActionClearNodeCache, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionClearPluginsLoadCache, kShortcutDescActionClearPluginsLoadCache, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionClearAllCaches, kShortcutDescActionClearAllCaches, Qt::ControlModifier | Qt::ShiftModifier, Qt::Key_K);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionRenderSelected, kShortcutDescActionRenderSelected, Qt::NoModifier, Qt::Key_F7);

    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionRenderAll, kShortcutDescActionRenderAll, Qt::NoModifier, Qt::Key_F5);

    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionEnableRenderStats, kShortcutDescActionEnableRenderStats, Qt::NoModifier, Qt::Key_F2);

    // Note: keys 0-1 are handled by Gui::handleNativeKeys(), and should thus work even on international keyboards
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput1, kShortcutDescActionConnectViewerToInput1, Qt::NoModifier, Qt::Key_1);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput2, kShortcutDescActionConnectViewerToInput2, Qt::NoModifier, Qt::Key_2);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput3, kShortcutDescActionConnectViewerToInput3, Qt::NoModifier, Qt::Key_3);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput4, kShortcutDescActionConnectViewerToInput4, Qt::NoModifier, Qt::Key_4);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput5, kShortcutDescActionConnectViewerToInput5, Qt::NoModifier, Qt::Key_5);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput6, kShortcutDescActionConnectViewerToInput6, Qt::NoModifier, Qt::Key_6);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput7, kShortcutDescActionConnectViewerToInput7, Qt::NoModifier, Qt::Key_7);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput8, kShortcutDescActionConnectViewerToInput8, Qt::NoModifier, Qt::Key_8);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput9, kShortcutDescActionConnectViewerToInput9, Qt::NoModifier, Qt::Key_9);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput10, kShortcutDescActionConnectViewerToInput10, Qt::NoModifier, Qt::Key_0);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerBToInput1, kShortcutDescActionConnectViewerBToInput1, Qt::ShiftModifier, Qt::Key_1);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerBToInput2, kShortcutDescActionConnectViewerBToInput2, Qt::ShiftModifier, Qt::Key_2);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerBToInput3, kShortcutDescActionConnectViewerBToInput3, Qt::ShiftModifier, Qt::Key_3);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerBToInput4, kShortcutDescActionConnectViewerBToInput4, Qt::ShiftModifier, Qt::Key_4);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerBToInput5, kShortcutDescActionConnectViewerBToInput5, Qt::ShiftModifier, Qt::Key_5);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerBToInput6, kShortcutDescActionConnectViewerBToInput6, Qt::ShiftModifier, Qt::Key_6);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerBToInput7, kShortcutDescActionConnectViewerBToInput7, Qt::ShiftModifier, Qt::Key_7);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerBToInput8, kShortcutDescActionConnectViewerBToInput8, Qt::ShiftModifier, Qt::Key_8);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerBToInput9, kShortcutDescActionConnectViewerBToInput9, Qt::ShiftModifier, Qt::Key_9);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerBToInput10, kShortcutDescActionConnectViewerBToInput10, Qt::ShiftModifier, Qt::Key_0);

    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionShowPaneFullScreen, kShortcutDescActionShowPaneFullScreen, Qt::NoModifier, Qt::Key_Space);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionNextTab, kShortcutDescActionNextTab, Qt::ControlModifier, Qt::Key_T);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionPrevTab, kShortcutDescActionPrevTab, Qt::ShiftModifier | Qt::ControlModifier, Qt::Key_T);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionCloseTab, kShortcutDescActionCloseTab, Qt::ShiftModifier, Qt::Key_Escape);


    ///Viewer
    registerKeybindWithMask(kShortcutGroupViewer, kShortcutIDActionZoomIn, kShortcutDescActionZoomIn, Qt::NoModifier, Qt::Key_Plus,
                            Qt::ShiftModifier);
    registerKeybindWithMask(kShortcutGroupViewer, kShortcutIDActionZoomOut, kShortcutDescActionZoomOut, Qt::NoModifier, Qt::Key_Minus,
                            Qt::ShiftModifier);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionFitViewer, kShortcutDescActionFitViewer, Qt::NoModifier, Qt::Key_F);

    registerKeybind(kShortcutGroupViewer, kShortcutIDActionLuminance, kShortcutDescActionLuminance, Qt::NoModifier, Qt::Key_Y);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionRed, kShortcutDescActionRed, Qt::NoModifier, Qt::Key_R);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionGreen, kShortcutDescActionGreen, Qt::NoModifier, Qt::Key_G);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionBlue, kShortcutDescActionBlue, Qt::NoModifier, Qt::Key_B);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionAlpha, kShortcutDescActionAlpha, Qt::NoModifier, Qt::Key_A);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionMatteOverlay, kShortcutDescActionMatteOverlay, Qt::NoModifier, Qt::Key_M);

    registerKeybind(kShortcutGroupViewer, kShortcutIDActionLuminanceA, kShortcutDescActionLuminanceA, Qt::ShiftModifier, Qt::Key_Y);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionRedA, kShortcutDescActionRedA, Qt::ShiftModifier, Qt::Key_R);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionGreenA, kShortcutDescActionGreenA, Qt::ShiftModifier, Qt::Key_G);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionBlueA, kShortcutDescActionBlueA, Qt::ShiftModifier, Qt::Key_B);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionAlphaA, kShortcutDescActionAlphaA, Qt::ShiftModifier, Qt::Key_A);

    registerKeybind(kShortcutGroupViewer, kShortcutIDActionClipEnabled, kShortcutDescActionClipEnabled, Qt::ShiftModifier, Qt::Key_C);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionFullFrameProc, kShortcutDescActionFullFrameProc, Qt::NoModifier, (Qt::Key)0);

    registerKeybind(kShortcutGroupViewer, kShortcutIDActionRefresh, kShortcutDescActionRefresh, Qt::NoModifier, Qt::Key_U);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionRefreshWithStats, kShortcutDescActionRefreshWithStats, Qt::ShiftModifier | Qt::ControlModifier, Qt::Key_U);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionROIEnabled, kShortcutDescActionROIEnabled, Qt::ShiftModifier, Qt::Key_W);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionNewROI, kShortcutDescActionNewROI, Qt::AltModifier, Qt::Key_W);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionPauseViewer, kShortcutDescActionPauseViewer, Qt::ShiftModifier, Qt::Key_P);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionPauseViewerInputA, kShortcutDescActionPauseViewerInputA, Qt::NoModifier, Qt::Key_P);

    registerKeybind(kShortcutGroupViewer, kShortcutIDActionProxyEnabled, kShortcutDescActionProxyEnabled, Qt::ControlModifier, Qt::Key_P);
    registerKeybindWithMask(kShortcutGroupViewer, kShortcutIDActionProxyLevel2, kShortcutDescActionProxyLevel2, Qt::AltModifier, Qt::Key_1,
                            Qt::ShiftModifier);
    registerKeybindWithMask(kShortcutGroupViewer, kShortcutIDActionProxyLevel4, kShortcutDescActionProxyLevel4, Qt::AltModifier, Qt::Key_2,
                            Qt::ShiftModifier);
    registerKeybindWithMask(kShortcutGroupViewer, kShortcutIDActionProxyLevel8, kShortcutDescActionProxyLevel8, Qt::AltModifier, Qt::Key_3,
                            Qt::ShiftModifier);
    registerKeybindWithMask(kShortcutGroupViewer, kShortcutIDActionProxyLevel16, kShortcutDescActionProxyLevel16, Qt::AltModifier, Qt::Key_4,
                            Qt::ShiftModifier);
    registerKeybindWithMask(kShortcutGroupViewer, kShortcutIDActionProxyLevel32, kShortcutDescActionProxyLevel32, Qt::AltModifier, Qt::Key_5,
                            Qt::ShiftModifier);

    registerKeybind(kShortcutGroupViewer, kShortcutIDActionHideOverlays, kShortcutDescActionHideOverlays, Qt::NoModifier, Qt::Key_O);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionHidePlayer, kShortcutDescActionHidePlayer, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionHideTimeline, kShortcutDescActionHideTimeline, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionHideLeft, kShortcutDescActionHideLeft, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionHideRight, kShortcutDescActionHideRight, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionHideTop, kShortcutDescActionHideTop, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionHideInfobar, kShortcutDescActionHideInfobar, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionHideAll, kShortcutDescActionHideAll, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionShowAll, kShortcutDescActionShowAll, Qt::NoModifier, (Qt::Key)0);
    registerKeybindWithMask(kShortcutGroupViewer, kShortcutIDActionZoomLevel100, kShortcutDescActionZoomLevel100, Qt::ControlModifier, Qt::Key_1,
                            Qt::ShiftModifier);
    registerKeybind(kShortcutGroupViewer, kShortcutIDToggleWipe, kShortcutDescToggleWipe, Qt::NoModifier, Qt::Key_W);
    registerKeybind(kShortcutGroupViewer, kShortcutIDCenterWipe, kShortcutDescCenterWipe, Qt::ShiftModifier, Qt::Key_F);

    registerKeybind(kShortcutGroupViewer, kShortcutIDNextLayer, kShortcutDescNextLayer, Qt::NoModifier, Qt::Key_PageDown);
    registerKeybind(kShortcutGroupViewer, kShortcutIDPrevLayer, kShortcutDescPrevLayer, Qt::NoModifier, Qt::Key_PageUp);
    registerKeybind(kShortcutGroupViewer, kShortcutIDSwitchInputAAndB, kShortcutDescSwitchInputAAndB, Qt::NoModifier, Qt::Key_Return);

    registerKeybind(kShortcutGroupViewer, kShortcutIDPrevView, kShortcutDescPrevView, Qt::NoModifier, Qt::Key_Semicolon);
    registerKeybind(kShortcutGroupViewer, kShortcutIDNextView, kShortcutDescNextView, Qt::NoModifier, Qt::Key_Apostrophe);

    registerMouseShortcut(kShortcutGroupViewer, kShortcutIDMousePickColor, kShortcutDescMousePickColor, Qt::ControlModifier, Qt::LeftButton);
    registerMouseShortcut(kShortcutGroupViewer, kShortcutIDMouseRectanglePick, kShortcutDescMouseRectanglePick, Qt::ShiftModifier | Qt::ControlModifier, Qt::LeftButton);
    //registerMouseShortcut(kShortcutGroupViewer, kShortcutIDMousePickInputColor, kShortcutDescMousePickInputColor, Qt::ControlModifier | Qt::AltModifier, Qt::LeftButton);


    ///Player
    registerKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevious, kShortcutDescActionPlayerPrevious, Qt::NoModifier, Qt::Key_Left);
    registerKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNext, kShortcutDescActionPlayerNext, Qt::NoModifier, Qt::Key_Right);
    registerKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerBackward, kShortcutDescActionPlayerBackward, Qt::NoModifier, Qt::Key_J);
    registerKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerForward, kShortcutDescActionPlayerForward, Qt::NoModifier, Qt::Key_L);
    registerKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerStop, kShortcutDescActionPlayerStop, Qt::NoModifier, Qt::Key_K);
    registerKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevIncr, kShortcutDescActionPlayerPrevIncr, Qt::ShiftModifier, Qt::Key_Left);
    registerKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNextIncr, kShortcutDescActionPlayerNextIncr, Qt::ShiftModifier, Qt::Key_Right);
    registerKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevKF, kShortcutDescActionPlayerPrevKF, Qt::ShiftModifier | Qt::ControlModifier, Qt::Key_Left);
    registerKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNextKF, kShortcutDescActionPlayerNextKF, Qt::ShiftModifier | Qt::ControlModifier, Qt::Key_Right);
    registerKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerFirst, kShortcutDescActionPlayerFirst, Qt::ControlModifier, Qt::Key_Left);
    registerKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerLast, kShortcutDescActionPlayerLast, Qt::ControlModifier, Qt::Key_Right);

    registerKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPlaybackIn, kShortcutDescActionPlayerPlaybackIn, Qt::AltModifier, Qt::Key_I);
    registerKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPlaybackOut, kShortcutDescActionPlayerPlaybackOut, Qt::AltModifier, Qt::Key_O);


    ///Nodegraph
#ifndef NATRON_ENABLE_IO_META_NODES
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphCreateReader, kShortcutDescActionGraphCreateReader, Qt::NoModifier, Qt::Key_R);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphCreateWriter, kShortcutDescActionGraphCreateWriter, Qt::NoModifier, Qt::Key_W);
#endif

    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphRearrangeNodes, kShortcutDescActionGraphRearrangeNodes, Qt::NoModifier, Qt::Key_L);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphDisableNodes, kShortcutDescActionGraphDisableNodes, Qt::NoModifier, Qt::Key_D);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphRemoveNodes, kShortcutDescActionGraphRemoveNodes, Qt::NoModifier, Qt::Key_Backspace);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphShowExpressions, kShortcutDescActionGraphShowExpressions, Qt::ShiftModifier, Qt::Key_E);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphNavigateDownstream, kShortcutDescActionGraphNavigateDownstram, Qt::NoModifier, Qt::Key_Down);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphNavigateUpstream, kShortcutDescActionGraphNavigateUpstream, Qt::NoModifier, Qt::Key_Up);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphSelectUp, kShortcutDescActionGraphSelectUp, Qt::ShiftModifier, Qt::Key_Up);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphSelectDown, kShortcutDescActionGraphSelectDown, Qt::ShiftModifier, Qt::Key_Down);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphSelectAll, kShortcutDescActionGraphSelectAll, Qt::ControlModifier, Qt::Key_A);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphSelectAllVisible, kShortcutDescActionGraphSelectAllVisible, Qt::ShiftModifier | Qt::ControlModifier, Qt::Key_A);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphAutoHideInputs, kShortcutDescActionGraphAutoHideInputs, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphHideInputs, kShortcutDescActionGraphHideInputs, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphSwitchInputs, kShortcutDescActionGraphSwitchInputs, Qt::ShiftModifier, Qt::Key_X);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphCopy, kShortcutDescActionGraphCopy, Qt::ControlModifier, Qt::Key_C);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphPaste, kShortcutDescActionGraphPaste, Qt::ControlModifier, Qt::Key_V);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphCut, kShortcutDescActionGraphCut, Qt::ControlModifier, Qt::Key_X);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphClone, kShortcutDescActionGraphClone, Qt::AltModifier, Qt::Key_K);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphDeclone, kShortcutDescActionGraphDeclone, Qt::AltModifier | Qt::ShiftModifier, Qt::Key_K);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphDuplicate, kShortcutDescActionGraphDuplicate, Qt::AltModifier, Qt::Key_C);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphForcePreview, kShortcutDescActionGraphForcePreview, Qt::ShiftModifier, Qt::Key_P);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphTogglePreview, kShortcutDescActionGraphToggleAutoPreview, Qt::AltModifier, Qt::Key_P);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphToggleAutoPreview, kShortcutDescActionGraphToggleAutoPreview, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphToggleAutoTurbo, kShortcutDescActionGraphToggleAutoTurbo, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphFrameNodes, kShortcutDescActionGraphFrameNodes, Qt::NoModifier, Qt::Key_F);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphShowCacheSize, kShortcutDescActionGraphShowCacheSize, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphFindNode, kShortcutDescActionGraphFindNode, Qt::ControlModifier, Qt::Key_F);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphRenameNode, kShortcutDescActionGraphRenameNode, Qt::NoModifier, Qt::Key_N);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphExtractNode, kShortcutDescActionGraphExtractNode, Qt::ControlModifier | Qt::ShiftModifier,
                    Qt::Key_X);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphMakeGroup, kShortcutDescActionGraphMakeGroup, Qt::ControlModifier,
                    Qt::Key_G);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphExpandGroup, kShortcutDescActionGraphExpandGroup, Qt::ControlModifier | Qt::AltModifier,
                    Qt::Key_G);

    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphOpenNodePanel, kShortcutDescActionGraphOpenNodePanel, Qt::NoModifier, Qt::Key_Return);


    ///CurveEditor
    registerKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorRemoveKeys, kShortcutDescActionCurveEditorRemoveKeys, Qt::NoModifier, Qt::Key_Backspace);
    registerKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorConstant, kShortcutDescActionCurveEditorConstant, Qt::NoModifier, Qt::Key_K);
    registerKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorSmooth, kShortcutDescActionCurveEditorSmooth, Qt::NoModifier, Qt::Key_Z);
    registerKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorLinear, kShortcutDescActionCurveEditorLinear, Qt::NoModifier, Qt::Key_L);
    registerKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorCatmullrom, kShortcutDescActionCurveEditorCatmullrom, Qt::NoModifier, Qt::Key_R);
    registerKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorCubic, kShortcutDescActionCurveEditorCubic, Qt::NoModifier, Qt::Key_C);
    registerKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorHorizontal, kShortcutDescActionCurveEditorHorizontal, Qt::NoModifier, Qt::Key_H);
    registerKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorBreak, kShortcutDescActionCurveEditorBreak, Qt::NoModifier, Qt::Key_X);
    registerKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorSelectAll, kShortcutDescActionCurveEditorSelectAll, Qt::ControlModifier, Qt::Key_A);
    registerKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorCenterAll, kShortcutDescActionCurveEditorCenterAll
                    , Qt::NoModifier, Qt::Key_A);
    registerKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorCenter, kShortcutDescActionCurveEditorCenter
                    , Qt::NoModifier, Qt::Key_F);
    registerKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorCopy, kShortcutDescActionCurveEditorCopy, Qt::ControlModifier, Qt::Key_C);
    registerKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorPaste, kShortcutDescActionCurveEditorPaste, Qt::ControlModifier, Qt::Key_V);

    // Dope Sheet Editor
    registerKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionDopeSheetEditorDeleteKeys, kShortcutDescActionDopeSheetEditorDeleteKeys, Qt::NoModifier, Qt::Key_Backspace);
    registerKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionDopeSheetEditorFrameSelection, kShortcutDescActionDopeSheetEditorFrameSelection, Qt::NoModifier, Qt::Key_F);
    registerKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionDopeSheetEditorSelectAllKeyframes, kShortcutDescActionDopeSheetEditorSelectAllKeyframes, Qt::ControlModifier, Qt::Key_A);

    registerKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionCurveEditorConstant, kShortcutDescActionCurveEditorConstant, Qt::NoModifier, Qt::Key_K);
    registerKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionCurveEditorSmooth, kShortcutDescActionCurveEditorSmooth, Qt::NoModifier, Qt::Key_Z);
    registerKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionCurveEditorLinear, kShortcutDescActionCurveEditorLinear, Qt::NoModifier, Qt::Key_L);
    registerKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionCurveEditorCatmullrom, kShortcutDescActionCurveEditorCatmullrom, Qt::NoModifier, Qt::Key_R);
    registerKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionCurveEditorCubic, kShortcutDescActionCurveEditorCubic, Qt::NoModifier, Qt::Key_C);
    registerKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionCurveEditorHorizontal, kShortcutDescActionCurveEditorHorizontal, Qt::NoModifier, Qt::Key_H);
    registerKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionCurveEditorBreak, kShortcutDescActionCurveEditorBreak, Qt::NoModifier, Qt::Key_X);

    registerKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionDopeSheetEditorCopySelectedKeyframes, kShortcutDescActionDopeSheetEditorCopySelectedKeyframes, Qt::ControlModifier, Qt::Key_C);
    registerKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionDopeSheetEditorPasteKeyframes, kShortcutDescActionDopeSheetEditorPasteKeyframes, Qt::ControlModifier, Qt::Key_V);
    registerKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionDopeSheetEditorPasteKeyframesAbsolute, kShortcutDescActionDopeSheetEditorPasteKeyframesAbsolute, Qt::ControlModifier | Qt::ShiftModifier, Qt::Key_V);

    //Script editor
    registerKeybindWithMask(kShortcutGroupScriptEditor, kShortcutIDActionScriptEditorPrevScript, kShortcutDescActionScriptEditorPrevScript, Qt::ControlModifier, Qt::Key_BracketLeft, Qt::ShiftModifier | Qt::AltModifier);
    registerKeybindWithMask(kShortcutGroupScriptEditor, kShortcutIDActionScriptEditorNextScript, kShortcutDescActionScriptEditorNextScript, Qt::ControlModifier, Qt::Key_BracketRight, Qt::ShiftModifier | Qt::AltModifier);
    registerKeybind(kShortcutGroupScriptEditor, kShortcutIDActionScriptEditorClearHistory, kShortcutDescActionScriptEditorClearHistory, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupScriptEditor, kShortcutIDActionScriptExecScript, kShortcutDescActionScriptExecScript, Qt::ControlModifier, Qt::Key_Return);
    registerKeybind(kShortcutGroupScriptEditor, kShortcutIDActionScriptClearOutput, kShortcutDescActionScriptClearOutput, Qt::ControlModifier, Qt::Key_Backspace);
    registerKeybind(kShortcutGroupScriptEditor, kShortcutIDActionScriptShowOutput, kShortcutDescActionScriptShowOutput, Qt::NoModifier, (Qt::Key)0);
} // populateShortcuts

std::list<QKeySequence>
GuiApplicationManager::getKeySequenceForAction(const QString & group,
                                               const QString & actionID) const
{
    std::list<QKeySequence> ret;
    AppShortcuts::const_iterator foundGroup = _imp->_actionShortcuts.find(group);

    if ( foundGroup != _imp->_actionShortcuts.end() ) {
        GroupShortcuts::const_iterator found = foundGroup->second.find(actionID);
        if ( found != foundGroup->second.end() ) {
            const KeyBoundAction* ka = dynamic_cast<const KeyBoundAction*>(found->second);
            if (ka) {
                assert( ka->modifiers.size() == ka->currentShortcut.size() );

                std::list<Qt::Key>::const_iterator sit = ka->currentShortcut.begin();
                for (std::list<Qt::KeyboardModifiers>::const_iterator it = ka->modifiers.begin(); it != ka->modifiers.end(); ++it, ++sit) {
                    ret.push_back( makeKeySequence(*it, *sit) );
                }
            }
        }
    }

    return ret;
}

const std::map<QString, std::map<QString, BoundAction*> > &
GuiApplicationManager::getAllShortcuts() const
{
    return _imp->_actionShortcuts;
}

void
GuiApplicationManager::addShortcutAction(const QString & group,
                                         const QString & actionID,
                                         ActionWithShortcut* action)
{
    AppShortcuts::iterator foundGroup = _imp->_actionShortcuts.find(group);

    if ( foundGroup != _imp->_actionShortcuts.end() ) {
        GroupShortcuts::iterator found = foundGroup->second.find(actionID);
        if ( found != foundGroup->second.end() ) {
            KeyBoundAction* ka = dynamic_cast<KeyBoundAction*>(found->second);
            if (ka) {
                ka->actions.push_back(action);

                return;
            }
        }
    }
}

void
GuiApplicationManager::removeShortcutAction(const QString & group,
                                            const QString & actionID,
                                            QAction* action)
{
    AppShortcuts::iterator foundGroup = _imp->_actionShortcuts.find(group);

    if ( foundGroup != _imp->_actionShortcuts.end() ) {
        GroupShortcuts::iterator found = foundGroup->second.find(actionID);
        if ( found != foundGroup->second.end() ) {
            KeyBoundAction* ka = dynamic_cast<KeyBoundAction*>(found->second);
            if (ka) {
                std::list<ActionWithShortcut*>::iterator foundAction = std::find(ka->actions.begin(), ka->actions.end(), action);
                if ( foundAction != ka->actions.end() ) {
                    ka->actions.erase(foundAction);

                    return;
                }
            }
        }
    }
}

void
GuiApplicationManager::notifyShortcutChanged(KeyBoundAction* action)
{
    action->updateActionsShortcut();
    for (AppShortcuts::iterator it = _imp->_actionShortcuts.begin(); it != _imp->_actionShortcuts.end(); ++it) {
        if ( it->first.startsWith( QString::fromUtf8(kShortcutGroupNodes) ) ) {
            for (GroupShortcuts::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
                if (it2->second == action) {
                    const PluginsMap & allPlugins = getPluginsList();
                    PluginsMap::const_iterator found = allPlugins.find( it2->first.toStdString() );
                    if ( found != allPlugins.end() ) {
                        assert(found->second.size() > 0);
                        ( *found->second.rbegin() )->setHasShortcut( !action->currentShortcut.empty() );
                    }
                    break;
                }
            }
        }
    }
}

void
GuiApplicationManager::showErrorLog()
{
    hideSplashScreen();
    GuiAppInstance* app = dynamic_cast<GuiAppInstance*>( getTopLevelInstance().get() );
    if (app) {
        app->getGui()->showErrorLog();
    }
}

void
GuiApplicationManager::clearLastRenderedTextures()
{
    const AppInstanceVec& instances = getAppInstances();

    for (AppInstanceVec::const_iterator it = instances.begin(); it != instances.end(); ++it) {
        GuiAppInstance* guiApp = dynamic_cast<GuiAppInstance*>( it->get() );
        if (guiApp) {
            guiApp->clearAllLastRenderedImages();
        }
    }
}

bool
GuiApplicationManager::isNodeClipBoardEmpty() const
{
    return _imp->_nodeCB.isEmpty();
}

NodeClipBoard&
GuiApplicationManager::getNodeClipBoard()
{
    return _imp->_nodeCB;
}

void
GuiApplicationManager::clearNodeClipBoard()
{
    _imp->_nodeCB.nodes.clear();
    _imp->_nodeCB.nodesUI.clear();
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
GuiApplicationManager::addCommand(const QString& grouping,
                                  const std::string& pythonFunction,
                                  Qt::Key key,
                                  const Qt::KeyboardModifiers& modifiers)
{
    QStringList split = grouping.split( QLatin1Char('/') );

    if ( grouping.isEmpty() || split.isEmpty() ) {
        return;
    }
    PythonUserCommand c;
    c.grouping = grouping;
    c.pythonFunction = pythonFunction;
    c.key = key;
    c.modifiers = modifiers;
    _imp->pythonCommands.push_back(c);


    registerKeybind(kShortcutGroupGlobal, split[split.size() - 1].toStdString(), split[split.size() - 1].toStdString(), modifiers, key);
}

const std::list<PythonUserCommand>&
GuiApplicationManager::getUserPythonCommands() const
{
    return _imp->pythonCommands;
}

void
GuiApplicationManager::reloadStylesheets()
{
    const AppInstanceVec& instances = getAppInstances();

    for (AppInstanceVec::const_iterator it = instances.begin(); it != instances.end(); ++it) {
        GuiAppInstance* guiApp = dynamic_cast<GuiAppInstance*>( it->get() );
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
        GuiAppInstance* guiApp = dynamic_cast<GuiAppInstance*>( it->get() );
        if (guiApp) {
            guiApp->reloadScriptEditorFonts();
        }
    }
}

NATRON_NAMESPACE_EXIT
