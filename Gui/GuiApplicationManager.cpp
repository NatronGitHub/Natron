//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "GuiApplicationManager.h"

#include <stdexcept>

///gui
#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QPixmapCache>
#include <QBitmap>
#include <QImage>
#include <QApplication>
#include <QFontDatabase>
#include <QIcon>
#include <QFileOpenEvent>

CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

//core
#include <QSettings>
#include <QDebug>
#include <QMetaType>

//engine
#include "Global/Enums.h"
#include "Engine/KnobSerialization.h"
#include "Engine/Plugin.h"
#include "Engine/LibraryBinary.h"
#include "Engine/ViewerInstance.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include <SequenceParsing.h>

//gui
#include "Gui/Gui.h"
#include "Gui/SplashScreen.h"
#include "Gui/KnobGuiFactory.h"
#include "Gui/QtDecoder.h"
#include "Gui/QtEncoder.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/CurveWidget.h"
#include "Gui/ActionShortcuts.h"
#include "Gui/NodeClipBoard.h"

CLANG_DIAG_OFF(mismatched-tags)
GCC_DIAG_OFF(unused-parameter)
#include "NatronGui/natrongui_python.h"
CLANG_DIAG_ON(mismatched-tags)
GCC_DIAG_ON(unused-parameter)

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
#define registerKeybind(group,id,description, modifiers,symbol) ( _imp->addKeybind(group,id,description, modifiers,symbol) )

/**
 * @brief Performs the same that the registerKeybind macro, except that it takes a QKeySequence::StandardKey in parameter.
 * This way the keybind will be standard and will adapt bery well across all platforms supporting the standard.
 **/
#define registerStandardKeybind(group,id,description,key) ( _imp->addStandardKeybind(group,id,description,key) )

/**
 * @brief Performs the same that the registerKeybind macro, except that it works for shortcuts using mouse buttons instead of key symbols.
 * Generally you don't want this kind of interaction to be editable because it might break compatibility with non 3-button mouses.
 * Also the shortcut editor of Natron doesn't support editing mouse buttons.
 * In the end a mouse shortcut will be NON editable. The reason why you should call this is just to show the shortcut in the shortcut view
 * so that the user is aware it exists, but he/she will not be able to modify it.
 **/
#define registerMouseShortcut(group,id,description, modifiers,button) ( _imp->addMouseShortcut(group,id,description, modifiers,button) )

//Increment this when making change to default shortcuts or changes that would break expected default shortcuts
//in a way. This way the user will get prompted to restore default shortcuts on next launch
#define NATRON_SHORTCUTS_DEFAULT_VERSION 4

using namespace Natron;

struct KnobsClipBoard
{
    std::list<Variant> values; //< values
    std::list< boost::shared_ptr<Curve> > curves; //< animation
    std::list< boost::shared_ptr<Curve> > parametricCurves; //< for parametric knobs
    std::map<int,std::string> stringAnimation; //< for animating string knobs
    bool isEmpty; //< is the clipboard empty
    bool copyAnimation; //< should we copy all the animation or not
    
    std::string appID;
    std::string nodeFullyQualifiedName;
    std::string paramName;
};



struct GuiApplicationManagerPrivate
{
    GuiApplicationManager* _publicInterface;
    std::list<boost::shared_ptr<PluginGroupNode> > _topLevelToolButtons;
    boost::scoped_ptr<KnobsClipBoard> _knobsClipBoard;
    boost::scoped_ptr<KnobGuiFactory> _knobGuiFactory;
    QCursor* _colorPickerCursor;
    SplashScreen* _splashScreen;

    ///We store here the file open request that was made on startup but that
    ///we couldn't handle at that time
    QString _openFileRequest;
    AppShortcuts _actionShortcuts;

    bool _shortcutsChangedVersion;
    
    QString _fontFamily;
    int _fontSize;
    
    NodeClipBoard _nodeCB;
    
    std::list<PythonUserCommand> pythonCommands;
    
    GuiApplicationManagerPrivate(GuiApplicationManager* publicInterface)
        :   _publicInterface(publicInterface)
    , _topLevelToolButtons()
    , _knobsClipBoard(new KnobsClipBoard)
    , _knobGuiFactory( new KnobGuiFactory() )
    , _colorPickerCursor(NULL)
    , _splashScreen(NULL)
    , _openFileRequest()
    , _actionShortcuts()
    , _shortcutsChangedVersion(false)
    , _fontFamily()
    , _fontSize(0)
    , _nodeCB()
    , pythonCommands()
    {
    }

    void createColorPickerCursor();
    
    void removePluginToolButtonInternal(const boost::shared_ptr<PluginGroupNode>& n,const QStringList& grouping);
    void removePluginToolButton(const QStringList& grouping);

    void addStandardKeybind(const QString & grouping,const QString & id,
                            const QString & description,QKeySequence::StandardKey key);

    void addKeybind(const QString & grouping,const QString & id,
                    const QString & description,
                    const Qt::KeyboardModifiers & modifiers,Qt::Key symbol);
    
    void removeKeybind(const QString& grouping,const QString& id);

    void addMouseShortcut(const QString & grouping,const QString & id,
                          const QString & description,
                          const Qt::KeyboardModifiers & modifiers,Qt::MouseButton button);
    
    boost::shared_ptr<PluginGroupNode>  findPluginToolButtonInternal(const boost::shared_ptr<PluginGroupNode>& parent,
                                                                     const QStringList & grouping,
                                                                     const QString & name,
                                                                     const QString & iconPath,
                                                                     int major,
                                                                     int minor,
                                                                     bool isUserCreatable);
};

GuiApplicationManager::GuiApplicationManager()
    : AppManager()
      , _imp(new GuiApplicationManagerPrivate(this))
{
}

GuiApplicationManager::~GuiApplicationManager()
{

    delete _imp->_colorPickerCursor;
    for (AppShortcuts::iterator it = _imp->_actionShortcuts.begin(); it != _imp->_actionShortcuts.end(); ++it) {
        for (GroupShortcuts::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            delete it2->second;
        }
    }
}

void
GuiApplicationManager::getIcon(Natron::PixmapEnum e,
                               QPixmap* pix) const
{
    int iconSet = appPTR->getCurrentSettings()->getIconsBlackAndWhite() ? 2 : 3;
    QString iconSetStr = QString::number(iconSet);

    if ( !QPixmapCache::find(QString::number(e),pix) ) {
        QString path;
        switch (e) {
            case NATRON_PIXMAP_PLAYER_PREVIOUS:
                path = NATRON_IMAGES_PATH "back1.png";
                break;
            case NATRON_PIXMAP_PLAYER_FIRST_FRAME:
                path = NATRON_IMAGES_PATH "firstFrame.png";
                break;
            case NATRON_PIXMAP_PLAYER_NEXT:
                path = NATRON_IMAGES_PATH "forward1.png";
                break;
            case NATRON_PIXMAP_PLAYER_LAST_FRAME:
                path = NATRON_IMAGES_PATH "lastFrame.png";
                break;
            case NATRON_PIXMAP_PLAYER_NEXT_INCR:
                path = NATRON_IMAGES_PATH "nextIncr.png";
                break;
            case NATRON_PIXMAP_PLAYER_NEXT_KEY:
                path = NATRON_IMAGES_PATH "nextKF.png";
                break;
            case NATRON_PIXMAP_PLAYER_PREVIOUS_INCR:
                path = NATRON_IMAGES_PATH "previousIncr.png";
                break;
            case NATRON_PIXMAP_PLAYER_PREVIOUS_KEY:
                path = NATRON_IMAGES_PATH "prevKF.png";
                break;
            case NATRON_PIXMAP_ADD_KEYFRAME:
                path = NATRON_IMAGES_PATH "addKF.png";
                break;
            case NATRON_PIXMAP_REMOVE_KEYFRAME:
                path = NATRON_IMAGES_PATH "removeKF.png";
                break;
            case NATRON_PIXMAP_PLAYER_REWIND_ENABLED:
                path = NATRON_IMAGES_PATH "rewind_enabled.png";
                break;
            case NATRON_PIXMAP_PLAYER_REWIND_DISABLED:
                path = NATRON_IMAGES_PATH "rewind.png";
                break;
            case NATRON_PIXMAP_PLAYER_PLAY_ENABLED:
                path = NATRON_IMAGES_PATH "play_enabled.png";
                break;
            case NATRON_PIXMAP_PLAYER_PLAY_DISABLED:
                path = NATRON_IMAGES_PATH "play.png";
                break;
            case NATRON_PIXMAP_PLAYER_STOP:
                path = NATRON_IMAGES_PATH "stop.png";
                break;
            case NATRON_PIXMAP_PLAYER_LOOP_MODE:
                path = NATRON_IMAGES_PATH "loopmode.png";
                break;
            case NATRON_PIXMAP_PLAYER_BOUNCE:
                path = NATRON_IMAGES_PATH "bounce.png";
                break;
            case NATRON_PIXMAP_PLAYER_PLAY_ONCE:
                path = NATRON_IMAGES_PATH "playOnce.png";
                break;
            case NATRON_PIXMAP_MAXIMIZE_WIDGET:
                path = NATRON_IMAGES_PATH "maximize.png";
                break;
            case NATRON_PIXMAP_MINIMIZE_WIDGET:
                path = NATRON_IMAGES_PATH "minimize.png";
                break;
            case NATRON_PIXMAP_CLOSE_WIDGET:
                path = NATRON_IMAGES_PATH "close.png";
                break;
            case NATRON_PIXMAP_CLOSE_PANEL:
                path = NATRON_IMAGES_PATH "closePanel.png";
                break;
            case NATRON_PIXMAP_HIDE_UNMODIFIED:
                path = NATRON_IMAGES_PATH "hide_unmodified.png";
                break;
            case NATRON_PIXMAP_UNHIDE_UNMODIFIED:
                path = NATRON_IMAGES_PATH "unhide_unmodified.png";
                break;
            case NATRON_PIXMAP_HELP_WIDGET:
                path = NATRON_IMAGES_PATH "help.png";
                break;
            case NATRON_PIXMAP_GROUPBOX_FOLDED:
                path = NATRON_IMAGES_PATH "groupbox_folded.png";
                break;
            case NATRON_PIXMAP_GROUPBOX_UNFOLDED:
                path = NATRON_IMAGES_PATH "groupbox_unfolded.png";
                break;
            case NATRON_PIXMAP_UNDO:
                path = NATRON_IMAGES_PATH "undo.png";
                break;
            case NATRON_PIXMAP_REDO:
                path = NATRON_IMAGES_PATH "redo.png";
                break;
            case NATRON_PIXMAP_UNDO_GRAYSCALE:
                path = NATRON_IMAGES_PATH "undo_grayscale.png";
                break;
            case NATRON_PIXMAP_REDO_GRAYSCALE:
                path = NATRON_IMAGES_PATH "redo_grayscale.png";
                break;
            case NATRON_PIXMAP_RESTORE_DEFAULTS_ENABLED:
                path = NATRON_IMAGES_PATH "restoreDefaultEnabled.png";
                break;
            case NATRON_PIXMAP_RESTORE_DEFAULTS_DISABLED:
                path = NATRON_IMAGES_PATH "restoreDefaultDisabled.png";
                break;
            case NATRON_PIXMAP_TAB_WIDGET_LAYOUT_BUTTON:
                path = NATRON_IMAGES_PATH "layout.png";
                break;
            case NATRON_PIXMAP_TAB_WIDGET_LAYOUT_BUTTON_ANCHOR:
                path = NATRON_IMAGES_PATH "layout_anchor.png";
                break;
            case NATRON_PIXMAP_TAB_WIDGET_SPLIT_HORIZONTALLY:
                path = NATRON_IMAGES_PATH "splitHorizontally.png";
                break;
            case NATRON_PIXMAP_TAB_WIDGET_SPLIT_VERTICALLY:
                path = NATRON_IMAGES_PATH "splitVertically.png";
                break;
            case NATRON_PIXMAP_VIEWER_CENTER:
                path = NATRON_IMAGES_PATH "centerViewer.png";
                break;
            case NATRON_PIXMAP_VIEWER_CLIP_TO_PROJECT_ENABLED:
                path = NATRON_IMAGES_PATH "cliptoprojectEnabled.png";
                break;
            case NATRON_PIXMAP_VIEWER_CLIP_TO_PROJECT_DISABLED:
                path = NATRON_IMAGES_PATH "cliptoprojectDisabled.png";
                break;
            case NATRON_PIXMAP_VIEWER_REFRESH:
                path = NATRON_IMAGES_PATH "refresh.png";
                break;
            case NATRON_PIXMAP_VIEWER_REFRESH_ACTIVE:
                path = NATRON_IMAGES_PATH "refreshActive.png";
                break;
            case NATRON_PIXMAP_VIEWER_ROI_ENABLED:
                path = NATRON_IMAGES_PATH "viewer_roiEnabled.png";
                break;
            case NATRON_PIXMAP_VIEWER_ROI_DISABLED:
                path = NATRON_IMAGES_PATH "viewer_roiDisabled.png";
                break;
            case NATRON_PIXMAP_VIEWER_RENDER_SCALE:
                path = NATRON_IMAGES_PATH "renderScale.png";
                break;
            case NATRON_PIXMAP_VIEWER_RENDER_SCALE_CHECKED:
                path = NATRON_IMAGES_PATH "renderScale_checked.png";
                break;
            case NATRON_PIXMAP_OPEN_FILE:
                path = NATRON_IMAGES_PATH "open-file.png";
                break;
            case NATRON_PIXMAP_COLORWHEEL:
                path = NATRON_IMAGES_PATH "colorwheel.png";
                break;
            case NATRON_PIXMAP_OVERLAY:
                path = NATRON_IMAGES_PATH "colorwheel_overlay.png";
                break;
            case NATRON_PIXMAP_ROTO_MERGE:
                path = NATRON_IMAGES_PATH "roto_merge.png";
                break;
            case NATRON_PIXMAP_COLOR_PICKER:
                path = NATRON_IMAGES_PATH "color_picker.png";
                break;


            case NATRON_PIXMAP_IO_GROUPING:
                path = NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/image_grouping_" + iconSetStr + ".png";
                break;
            case NATRON_PIXMAP_3D_GROUPING:
                path = NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/3D_grouping_" + iconSetStr + ".png";
                break;
            case NATRON_PIXMAP_CHANNEL_GROUPING:
                path = NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/channel_grouping_" + iconSetStr + ".png";
                break;
            case NATRON_PIXMAP_MERGE_GROUPING:
                path = NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/merge_grouping_" + iconSetStr + ".png";
                break;
            case NATRON_PIXMAP_COLOR_GROUPING:
                path = NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/color_grouping_" + iconSetStr + ".png";
                break;
            case NATRON_PIXMAP_TRANSFORM_GROUPING:
                path = NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/transform_grouping_" + iconSetStr + ".png";
                break;
            case NATRON_PIXMAP_DEEP_GROUPING:
                path = NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/deep_grouping_" + iconSetStr + ".png";
                break;
            case NATRON_PIXMAP_FILTER_GROUPING:
                path = NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/filter_grouping_" + iconSetStr + ".png";
                break;
            case NATRON_PIXMAP_MULTIVIEW_GROUPING:
                path = NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/multiview_grouping_" + iconSetStr + ".png";
                break;
            case NATRON_PIXMAP_MISC_GROUPING:
                path = NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/misc_grouping_" + iconSetStr + ".png";
                break;
            case NATRON_PIXMAP_OTHER_PLUGINS:
                path = NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/other_grouping_" + iconSetStr + ".png";
                break;
            case NATRON_PIXMAP_TOOLSETS_GROUPING:
                path = NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/toolsets_grouping_" + iconSetStr + ".png";
                break;
            case NATRON_PIXMAP_KEYER_GROUPING:
                path = NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/keyer_grouping_" + iconSetStr + ".png";
                break;
            case NATRON_PIXMAP_TIME_GROUPING:
                path = NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/time_grouping_" + iconSetStr + ".png";
                break;
            case NATRON_PIXMAP_PAINT_GROUPING:
                path = NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/paint_grouping_" + iconSetStr + ".png";
                break;


            case NATRON_PIXMAP_OPEN_EFFECTS_GROUPING:
                path = NATRON_IMAGES_PATH "openeffects.png";
                break;
            case NATRON_PIXMAP_COMBOBOX:
                path = NATRON_IMAGES_PATH "combobox.png";
                break;
            case NATRON_PIXMAP_COMBOBOX_PRESSED:
                path = NATRON_IMAGES_PATH "pressed_combobox.png";
                break;
            case NATRON_PIXMAP_READ_IMAGE:
                path = NATRON_IMAGES_PATH "readImage.png";
                break;
            case NATRON_PIXMAP_WRITE_IMAGE:
                path = NATRON_IMAGES_PATH "writeImage.png";
                break;

            case NATRON_PIXMAP_APP_ICON:
                path = NATRON_APPLICATION_ICON_PATH;
                break;
            case NATRON_PIXMAP_INVERTED:
                path = NATRON_IMAGES_PATH "inverted.png";
                break;
            case NATRON_PIXMAP_UNINVERTED:
                path = NATRON_IMAGES_PATH "uninverted.png";
                break;
            case NATRON_PIXMAP_VISIBLE:
                path = NATRON_IMAGES_PATH "visible.png";
                break;
            case NATRON_PIXMAP_UNVISIBLE:
                path = NATRON_IMAGES_PATH "unvisible.png";
                break;
            case NATRON_PIXMAP_LOCKED:
                path = NATRON_IMAGES_PATH "locked.png";
                break;
            case NATRON_PIXMAP_UNLOCKED:
                path = NATRON_IMAGES_PATH "unlocked.png";
                break;
            case NATRON_PIXMAP_LAYER:
                path = NATRON_IMAGES_PATH "layer.png";
                break;
            case NATRON_PIXMAP_BEZIER:
                path = NATRON_IMAGES_PATH "bezier.png";
                break;
            case NATRON_PIXMAP_CURVE:
                path = NATRON_IMAGES_PATH "curve.png";
                break;
            case NATRON_PIXMAP_BEZIER_32:
                path = NATRON_IMAGES_PATH "bezier32.png";
                break;
            case NATRON_PIXMAP_PENCIL:
                path = NATRON_IMAGES_PATH "rotoToolPencil.png";
                break;
            case NATRON_PIXMAP_ELLIPSE:
                path = NATRON_IMAGES_PATH "ellipse.png";
                break;
            case NATRON_PIXMAP_RECTANGLE:
                path = NATRON_IMAGES_PATH "rectangle.png";
                break;
            case NATRON_PIXMAP_ADD_POINTS:
                path = NATRON_IMAGES_PATH "addPoints.png";
                break;
            case NATRON_PIXMAP_REMOVE_POINTS:
                path = NATRON_IMAGES_PATH "removePoints.png";
                break;
            case NATRON_PIXMAP_REMOVE_FEATHER:
                path = NATRON_IMAGES_PATH "removeFeather.png";
                break;
            case NATRON_PIXMAP_CUSP_POINTS:
                path = NATRON_IMAGES_PATH "cuspPoints.png";
                break;
            case NATRON_PIXMAP_SMOOTH_POINTS:
                path = NATRON_IMAGES_PATH "smoothPoints.png";
                break;
            case NATRON_PIXMAP_OPEN_CLOSE_CURVE:
                path = NATRON_IMAGES_PATH "openCloseCurve.png";
                break;
            case NATRON_PIXMAP_SELECT_ALL:
                path = NATRON_IMAGES_PATH "cursor.png";
                break;
            case NATRON_PIXMAP_SELECT_POINTS:
                path = NATRON_IMAGES_PATH "selectPoints.png";
                break;
            case NATRON_PIXMAP_SELECT_FEATHER:
                path = NATRON_IMAGES_PATH "selectFeather.png";
                break;
            case NATRON_PIXMAP_SELECT_CURVES:
                path = NATRON_IMAGES_PATH "selectCurves.png";
                break;
            case NATRON_PIXMAP_AUTO_KEYING_ENABLED:
                path = NATRON_IMAGES_PATH "autoKeyingEnabled.png";
                break;
            case NATRON_PIXMAP_AUTO_KEYING_DISABLED:
                path = NATRON_IMAGES_PATH "autoKeyingDisabled.png";
                break;
            case NATRON_PIXMAP_STICKY_SELECTION_ENABLED:
                path = NATRON_IMAGES_PATH "stickySelectionEnabled.png";
                break;
            case NATRON_PIXMAP_STICKY_SELECTION_DISABLED:
                path = NATRON_IMAGES_PATH "stickySelectionDisabled.png";
                break;
            case NATRON_PIXMAP_FEATHER_LINK_ENABLED:
                path = NATRON_IMAGES_PATH "featherLinkEnabled.png";
                break;
            case NATRON_PIXMAP_FEATHER_LINK_DISABLED:
                path = NATRON_IMAGES_PATH "featherLinkDisabled.png";
                break;
            case NATRON_PIXMAP_FEATHER_VISIBLE:
                path = NATRON_IMAGES_PATH "featherEnabled.png";
                break;
            case NATRON_PIXMAP_FEATHER_UNVISIBLE:
                path = NATRON_IMAGES_PATH "featherDisabled.png";
                break;
            case NATRON_PIXMAP_RIPPLE_EDIT_ENABLED:
                path = NATRON_IMAGES_PATH "rippleEditEnabled.png";
                break;
            case NATRON_PIXMAP_RIPPLE_EDIT_DISABLED:
                path = NATRON_IMAGES_PATH "rippleEditDisabled.png";
                break;
            case NATRON_PIXMAP_BOLD_CHECKED:
                path = NATRON_IMAGES_PATH "bold_checked.png";
                break;
            case NATRON_PIXMAP_BOLD_UNCHECKED:
                path = NATRON_IMAGES_PATH "bold_unchecked.png";
                break;
            case NATRON_PIXMAP_ITALIC_UNCHECKED:
                path = NATRON_IMAGES_PATH "italic_unchecked.png";
                break;
            case NATRON_PIXMAP_ITALIC_CHECKED:
                path = NATRON_IMAGES_PATH "italic_checked.png";
                break;
            case NATRON_PIXMAP_CLEAR_ALL_ANIMATION:
                path = NATRON_IMAGES_PATH "clearAnimation.png";
                break;
            case NATRON_PIXMAP_CLEAR_BACKWARD_ANIMATION:
                path = NATRON_IMAGES_PATH "clearAnimationBw.png";
                break;
            case NATRON_PIXMAP_CLEAR_FORWARD_ANIMATION:
                path = NATRON_IMAGES_PATH "clearAnimationFw.png";
                break;
            case NATRON_PIXMAP_UPDATE_VIEWER_ENABLED:
                path = NATRON_IMAGES_PATH "updateViewerEnabled.png";
                break;
            case NATRON_PIXMAP_UPDATE_VIEWER_DISABLED:
                path = NATRON_IMAGES_PATH "updateViewerDisabled.png";
                break;
            case NATRON_PIXMAP_SETTINGS:
                path = NATRON_IMAGES_PATH "settings.png";
                break;
            case NATRON_PIXMAP_FREEZE_ENABLED:
                path = NATRON_IMAGES_PATH "turbo_on.png";
                break;
            case NATRON_PIXMAP_FREEZE_DISABLED:
                path = NATRON_IMAGES_PATH "turbo_off.png";
                break;
            case NATRON_PIXMAP_VIEWER_ICON:
                path = NATRON_IMAGES_PATH "viewer_icon.png";
                break;
            case NATRON_PIXMAP_VIEWER_CHECKERBOARD_ENABLED:
                path = NATRON_IMAGES_PATH "checkerboard_on.png";
                break;
            case NATRON_PIXMAP_VIEWER_CHECKERBOARD_DISABLED:
                path = NATRON_IMAGES_PATH "checkerboard_off.png";
                break;
            case NATRON_PIXMAP_VIEWER_ZEBRA_ENABLED:
                path = NATRON_IMAGES_PATH "zebra_on.png";
                break;
            case NATRON_PIXMAP_VIEWER_ZEBRA_DISABLED:
                path = NATRON_IMAGES_PATH "zebra_off.png";
                break;
            case NATRON_PIXMAP_VIEWER_GAMMA_ENABLED:
                path = NATRON_IMAGES_PATH "gammaON.png";
                break;
            case NATRON_PIXMAP_VIEWER_GAMMA_DISABLED:
                path = NATRON_IMAGES_PATH "gammaOFF.png";
                break;
            case NATRON_PIXMAP_VIEWER_GAIN_ENABLED:
                path = NATRON_IMAGES_PATH "expoON.png";
                break;
            case NATRON_PIXMAP_VIEWER_GAIN_DISABLED:
                path = NATRON_IMAGES_PATH "expoOFF.png";
                break;
            case NATRON_PIXMAP_ADD_TRACK:
                path = NATRON_IMAGES_PATH "addTrack.png";
                break;
            case NATRON_PIXMAP_SCRIPT_CLEAR_OUTPUT:
                path = NATRON_IMAGES_PATH "clearOutput.png";
                break;
            case NATRON_PIXMAP_SCRIPT_EXEC_SCRIPT:
                path = NATRON_IMAGES_PATH "execScript.png";
                break;
            case NATRON_PIXMAP_SCRIPT_LOAD_EXEC_SCRIPT:
                path = NATRON_IMAGES_PATH "loadAndExecScript.png";
                break;
            case NATRON_PIXMAP_SCRIPT_LOAD_SCRIPT:
                path = NATRON_IMAGES_PATH "loadScript.png";
                break;
            case NATRON_PIXMAP_SCRIPT_NEXT_SCRIPT:
                path = NATRON_IMAGES_PATH "nextScript.png";
                break;
            case NATRON_PIXMAP_SCRIPT_OUTPUT_PANE_ACTIVATED:
                path = NATRON_IMAGES_PATH "outputPanelActivated.png";
                break;
            case NATRON_PIXMAP_SCRIPT_OUTPUT_PANE_DEACTIVATED:
                path = NATRON_IMAGES_PATH "outputPanelDeactivated.png";
                break;
            case NATRON_PIXMAP_SCRIPT_PREVIOUS_SCRIPT:
                path = NATRON_IMAGES_PATH "previousScript.png";
                break;
            case NATRON_PIXMAP_SCRIPT_SAVE_SCRIPT:
                path = NATRON_IMAGES_PATH "saveScript.png";
                break;

            case NATRON_PIXMAP_MERGE_ATOP:
                path = NATRON_IMAGES_PATH "merge_atop.png";
                break;
            case NATRON_PIXMAP_MERGE_AVERAGE:
                path = NATRON_IMAGES_PATH "merge_average.png";
                break;
            case NATRON_PIXMAP_MERGE_COLOR_BURN:
                path = NATRON_IMAGES_PATH "merge_color_burn.png";
                break;
            case NATRON_PIXMAP_MERGE_COLOR_DODGE:
                path = NATRON_IMAGES_PATH "merge_color_dodge.png";
                break;
            case NATRON_PIXMAP_MERGE_CONJOINT_OVER:
                path = NATRON_IMAGES_PATH "merge_conjoint_over.png";
                break;
            case NATRON_PIXMAP_MERGE_COPY:
                path = NATRON_IMAGES_PATH "merge_copy.png";
                break;
            case NATRON_PIXMAP_MERGE_DIFFERENCE:
                path = NATRON_IMAGES_PATH "merge_difference.png";
                break;
            case NATRON_PIXMAP_MERGE_DISJOINT_OVER:
                path = NATRON_IMAGES_PATH "merge_disjoint_over.png";
                break;
            case NATRON_PIXMAP_MERGE_DIVIDE:
                path = NATRON_IMAGES_PATH "merge_divide.png";
                break;
            case NATRON_PIXMAP_MERGE_EXCLUSION:
                path = NATRON_IMAGES_PATH "merge_exclusion.png";
                break;
            case NATRON_PIXMAP_MERGE_FREEZE:
                path = NATRON_IMAGES_PATH "merge_freeze.png";
                break;
            case NATRON_PIXMAP_MERGE_FROM:
                path = NATRON_IMAGES_PATH "merge_from.png";
                break;
            case NATRON_PIXMAP_MERGE_GEOMETRIC:
                path = NATRON_IMAGES_PATH "merge_geometric.png";
                break;
            case NATRON_PIXMAP_MERGE_HARD_LIGHT:
                path = NATRON_IMAGES_PATH "merge_hard_light.png";
                break;
            case NATRON_PIXMAP_MERGE_HYPOT:
                path = NATRON_IMAGES_PATH "merge_hypot.png";
                break;
            case NATRON_PIXMAP_MERGE_IN:
                path = NATRON_IMAGES_PATH "merge_in.png";
                break;
            case NATRON_PIXMAP_MERGE_INTERPOLATED:
                path = NATRON_IMAGES_PATH "merge_interpolated.png";
                break;
            case NATRON_PIXMAP_MERGE_MASK:
                path = NATRON_IMAGES_PATH "merge_mask.png";
                break;
            case NATRON_PIXMAP_MERGE_MATTE:
                path = NATRON_IMAGES_PATH "merge_matte.png";
                break;
            case NATRON_PIXMAP_MERGE_MAX:
                path = NATRON_IMAGES_PATH "merge_max.png";
                break;
            case NATRON_PIXMAP_MERGE_MIN:
                path = NATRON_IMAGES_PATH "merge_min.png";
                break;
            case NATRON_PIXMAP_MERGE_MINUS:
                path = NATRON_IMAGES_PATH "merge_minus.png";
                break;
            case NATRON_PIXMAP_MERGE_MULTIPLY:
                path = NATRON_IMAGES_PATH "merge_multiply.png";
                break;
            case NATRON_PIXMAP_MERGE_OUT:
                path = NATRON_IMAGES_PATH "merge_out.png";
                break;
            case NATRON_PIXMAP_MERGE_OVER:
                path = NATRON_IMAGES_PATH "merge_over.png";
                break;
            case NATRON_PIXMAP_MERGE_OVERLAY:
                path = NATRON_IMAGES_PATH "merge_overlay.png";
                break;
            case NATRON_PIXMAP_MERGE_PINLIGHT:
                path = NATRON_IMAGES_PATH "merge_pinlight.png";
                break;
            case NATRON_PIXMAP_MERGE_PLUS:
                path = NATRON_IMAGES_PATH "merge_plus.png";
                break;
            case NATRON_PIXMAP_MERGE_REFLECT:
                path = NATRON_IMAGES_PATH "merge_reflect.png";
                break;
            case NATRON_PIXMAP_MERGE_SCREEN:
                path = NATRON_IMAGES_PATH "merge_screen.png";
                break;
            case NATRON_PIXMAP_MERGE_SOFT_LIGHT:
                path = NATRON_IMAGES_PATH "merge_soft_light.png";
                break;
            case NATRON_PIXMAP_MERGE_STENCIL:
                path = NATRON_IMAGES_PATH "merge_stencil.png";
                break;
            case NATRON_PIXMAP_MERGE_UNDER:
                path = NATRON_IMAGES_PATH "merge_under.png";
                break;
            case NATRON_PIXMAP_MERGE_XOR:
                path = NATRON_IMAGES_PATH "merge_xor.png";
                break;
            case NATRON_PIXMAP_LINK_CURSOR:
                path = NATRON_IMAGES_PATH "linkCursor.png";
                break;
            case NATRON_PIXMAP_ENTER_GROUP:
                path = NATRON_IMAGES_PATH "enter_group.png";
                break;
            case NATRON_PIXMAP_ROTOPAINT_BUILDUP_ENABLED:
                path = NATRON_IMAGES_PATH "rotopaint_buildup_on.png";
                break;
            case NATRON_PIXMAP_ROTOPAINT_BUILDUP_DISABLED:
                path = NATRON_IMAGES_PATH "rotopaint_buildup_off.png";
                break;
            case NATRON_PIXMAP_ROTOPAINT_BLUR:
                path = NATRON_IMAGES_PATH "rotopaint_blur.png";
                break;
            case NATRON_PIXMAP_ROTOPAINT_BURN:
                path = NATRON_IMAGES_PATH "rotopaint_burn.png";
                break;
            case NATRON_PIXMAP_ROTOPAINT_CLONE:
                path = NATRON_IMAGES_PATH "rotopaint_clone.png";
                break;
            case NATRON_PIXMAP_ROTOPAINT_DODGE:
                path = NATRON_IMAGES_PATH "rotopaint_dodge.png";
                break;
            case NATRON_PIXMAP_ROTOPAINT_ERASER:
                path = NATRON_IMAGES_PATH "rotopaint_eraser.png";
                break;
            case NATRON_PIXMAP_ROTOPAINT_PRESSURE_ENABLED:
                path = NATRON_IMAGES_PATH "rotopaint_pressure_on.png";
                break;
            case NATRON_PIXMAP_ROTOPAINT_PRESSURE_DISABLED:
                path = NATRON_IMAGES_PATH "rotopaint_pressure_off.png";
                break;
            case NATRON_PIXMAP_ROTOPAINT_REVEAL:
                path = NATRON_IMAGES_PATH "rotopaint_reveal.png";
                break;
            case NATRON_PIXMAP_ROTOPAINT_SHARPEN:
                path = NATRON_IMAGES_PATH "rotopaint_sharpen.png";
                break;
            case NATRON_PIXMAP_ROTOPAINT_SMEAR:
                path = NATRON_IMAGES_PATH "rotopaint_smear.png";
                break;
            case NATRON_PIXMAP_ROTOPAINT_SOLID:
                path = NATRON_IMAGES_PATH "rotopaint_solid.png";
                break;
            case NATRON_PIXMAP_ROTO_NODE_ICON:
                path = NATRON_IMAGES_PATH "rotoNodeIcon.png";
                break;
            case NATRON_PIXMAP_INTERP_LINEAR:
                path = NATRON_IMAGES_PATH "interp_linear.png";
                break;
            case NATRON_PIXMAP_INTERP_CURVE:
                path = NATRON_IMAGES_PATH "interp_curve.png";
                break;
            case NATRON_PIXMAP_INTERP_CONSTANT:
                path = NATRON_IMAGES_PATH "interp_constant.png";
                break;
            case NATRON_PIXMAP_INTERP_BREAK:
                path = NATRON_IMAGES_PATH "interp_break.png";
                break;
            case NATRON_PIXMAP_INTERP_CURVE_C:
                path = NATRON_IMAGES_PATH "interp_curve_c.png";
                break;
            case NATRON_PIXMAP_INTERP_CURVE_H:
                path = NATRON_IMAGES_PATH "interp_curve_h.png";
                break;
            case NATRON_PIXMAP_INTERP_CURVE_R:
                path = NATRON_IMAGES_PATH "interp_curve_r.png";
                break;
            case NATRON_PIXMAP_INTERP_CURVE_Z:
                path = NATRON_IMAGES_PATH "interp_curve_z.png";
                break;
            default:
                assert(!"Missing image.");
        } // switch

        // put a breakpoint in png_chunk_report to catch the error "libpng warning: iCCP: known incorrect sRGB profile"

        // old version:
        //QImage img;
        //img.load(path);
        //*pix = QPixmap::fromImage(img);

        // new version:
        pix->load(path);

        QPixmapCache::insert(QString::number(e), *pix);
    }
} // getIcon

const std::list<boost::shared_ptr<PluginGroupNode> >&
GuiApplicationManager::getTopLevelPluginsToolButtons() const
{
    return _imp->_topLevelToolButtons;
}

const QCursor &
GuiApplicationManager::getColorPickerCursor() const
{
    return *(_imp->_colorPickerCursor);
}

void
GuiApplicationManager::initGui()
{
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);

    //load custom fonts
    QString fontResource = QString(":/Resources/Fonts/%1.ttf");
    QStringList fontFilenames;
    fontFilenames << fontResource.arg("DroidSans");
    fontFilenames << fontResource.arg("DroidSans-Bold");
    Q_FOREACH (QString fontFilename, fontFilenames) {
        int fontID = QFontDatabase::addApplicationFont(fontFilename);
        qDebug() << "fontID=" << fontID << "families=" << QFontDatabase::applicationFontFamilies(fontID);
    }
    QString fontFamily(NATRON_FONT);
    int fontSize = NATRON_FONT_SIZE_11;
    
    
    ///Do not load old font stored in the setting "systemFont" on Natron < 2 because it might contain a crappy font.
    if (settings.contains(kQSettingsSoftwareMajorVersionSettingName) && settings.value(kQSettingsSoftwareMajorVersionSettingName).toInt() >= 2) {
        if (settings.contains("systemFont")) {
            fontFamily = settings.value("systemFont").toString();
        }
    }
    
    
    if (settings.contains("fontSize")) {
        fontSize = settings.value("fontSize").toInt();
    }
    //fontFamily = "Courier"; fontSize = 24; // for debugging purposes
    qDebug() << "Setting application font to " << fontFamily << fontSize;
    {
        QFontDatabase database;
        Q_FOREACH (const QString &family, database.families()) {
            if (family == fontFamily) {
                qDebug() << "... found" << fontFamily << "available styles:";
                Q_FOREACH (const QString &style, database.styles(family)) {
                    qDebug() << family << style;
                }
            }
        }
        QFont font(fontFamily, fontSize);
        if (!font.exactMatch()) {
            QFontInfo fi(font);
            qDebug() << "Not an exact match, got: " << fi.family() << fi.pointSize();
        }
        QApplication::setFont(font);
#ifdef Q_OS_MAC
        // https://bugreports.qt.io/browse/QTBUG-32789
        QFont::insertSubstitution(".Lucida Grande UI", fontFamily/*"Lucida Grande"*/);
        // https://bugreports.qt.io/browse/QTBUG-40833
        QFont::insertSubstitution(".Helvetica Neue DeskInterface", fontFamily/*"Helvetica Neue"*/);
        // there are lots of remaining bugs on Yosemite in 4.8.6, to be fixed in 4.8.7&
#endif
    }
    _imp->_fontFamily = fontFamily;
    _imp->_fontSize = fontSize;

    /*Display a splashscreen while we wait for the engine to load*/
    QString filename(NATRON_IMAGES_PATH "splashscreen.png");

    _imp->_splashScreen = new SplashScreen(filename);
    QCoreApplication::processEvents();
    QPixmap appIcPixmap;
    appPTR->getIcon(Natron::NATRON_PIXMAP_APP_ICON, &appIcPixmap);
    QIcon appIc(appIcPixmap);
    qApp->setWindowIcon(appIc);
    
    QFontDatabase db;
    QStringList families = db.families(QFontDatabase::Latin); // We need a Latin font for the UI
    std::vector<std::string> systemFonts(families.size());
    for (int i = 0; i < families.size(); ++i) {
        systemFonts[i] = families[i].toStdString();
    }
    getCurrentSettings()->populateSystemFonts(settings,systemFonts);


    _imp->createColorPickerCursor();
    _imp->_knobsClipBoard->isEmpty = true;
}

void
GuiApplicationManager::onPluginLoaded(Natron::Plugin* plugin)
{
    QString shortcutGrouping(kShortcutGroupNodes);
    const QStringList & groups = plugin->getGrouping();
    const QString & pluginID = plugin->getPluginID();
    const QString  pluginLabel = plugin->getLabelWithoutSuffix();
    const QString & pluginIconPath = plugin->getIconFilePath();
    const QString & groupIconPath = plugin->getGroupIconFilePath();

    QStringList groupingWithID = groups;
    groupingWithID.push_back(pluginID);
    boost::shared_ptr<PluginGroupNode> child = findPluginToolButtonOrCreate(groupingWithID,
                                                                            pluginLabel,
                                                                            groupIconPath,
                                                                            pluginIconPath,
                                                                            plugin->getMajorVersion(),
                                                                            plugin->getMinorVersion(),
                                                                            plugin->getIsUserCreatable());
    for (int i = 0; i < groups.size(); ++i) {

        shortcutGrouping.push_back('/');
        shortcutGrouping.push_back(groups[i]);
    }
    
    Qt::KeyboardModifiers modifiers = Qt::NoModifier;
    Qt::Key symbol = (Qt::Key)0;
    bool hasShortcut = true;

    /*These are the plug-ins which have a default shortcut. Other plug-ins can have a user-assigned shortcut.*/
    if (pluginID == PLUGINID_OFX_TRANSFORM) {
        symbol = Qt::Key_T;
    } else if (pluginID == PLUGINID_NATRON_ROTO) {
        symbol = Qt::Key_O;
    } else if (pluginID == PLUGINID_NATRON_ROTOPAINT) {
        symbol = Qt::Key_P;
    } else if (pluginID == PLUGINID_OFX_MERGE) {
        symbol = Qt::Key_M;
    } else if (pluginID == PLUGINID_OFX_GRADE) {
        symbol = Qt::Key_G;
    } else if (pluginID == PLUGINID_OFX_COLORCORRECT) {
        symbol = Qt::Key_C;
    } else if (pluginID == PLUGINID_OFX_BLURCIMG) {
        symbol = Qt::Key_B;
    } else if (pluginID == PLUGINID_NATRON_DOT) {
        symbol = Qt::Key_Period;
        modifiers |= Qt::ShiftModifier;
    } else {
        hasShortcut = false;
    }
    plugin->setHasShortcut(hasShortcut);
    
    if (plugin->getIsUserCreatable()) {
        _imp->addKeybind(shortcutGrouping, pluginID, pluginLabel, modifiers, symbol);
    }
    
}

void
GuiApplicationManager::ignorePlugin(Natron::Plugin* plugin)
{
    _imp->removePluginToolButton(plugin->getGrouping());
    _imp->removeKeybind(kShortcutGroupNodes, plugin->getPluginID());
}


void
GuiApplicationManagerPrivate::removePluginToolButtonInternal(const boost::shared_ptr<PluginGroupNode>& n,const QStringList& grouping)
{
    assert(grouping.size() > 0);
    
    const std::list<boost::shared_ptr<PluginGroupNode> >& children = n->getChildren();
    for (std::list<boost::shared_ptr<PluginGroupNode> >::const_iterator it = children.begin();
         it != children.end(); ++it) {
        if ((*it)->getID() == grouping[0]) {
            
            if (grouping.size() > 1) {
                QStringList newGrouping;
                for (int i = 1; i < grouping.size(); ++i) {
                    newGrouping.push_back(grouping[i]);
                }
                removePluginToolButtonInternal(*it, newGrouping);
                if ((*it)->getChildren().empty()) {
                    n->tryRemoveChild(it->get());
                }
            } else {
                n->tryRemoveChild(it->get());
            }
            break;
        }
    }
}
void
GuiApplicationManagerPrivate::removePluginToolButton(const QStringList& grouping)
{
    assert(grouping.size() > 0);
    
    for (std::list<boost::shared_ptr<PluginGroupNode> >::iterator it = _topLevelToolButtons.begin();
         it != _topLevelToolButtons.end(); ++it) {
        if ((*it)->getID() == grouping[0]) {
            
            if (grouping.size() > 1) {
                QStringList newGrouping;
                for (int i = 1; i < grouping.size(); ++i) {
                    newGrouping.push_back(grouping[i]);
                }
                removePluginToolButtonInternal(*it, newGrouping);
                if ((*it)->getChildren().empty()) {
                    _topLevelToolButtons.erase(it);
                }
            } else {
                _topLevelToolButtons.erase(it);
            }
            break;
        }
    }
}

boost::shared_ptr<PluginGroupNode>
GuiApplicationManagerPrivate::findPluginToolButtonInternal(const boost::shared_ptr<PluginGroupNode>& parent,
                                                           const QStringList & grouping,
                                                           const QString & name,
                                                           const QString & iconPath,
                                                           int major,
                                                           int minor,
                                                           bool isUserCreatable)
{
    assert(grouping.size() > 0);
    
    const std::list<boost::shared_ptr<PluginGroupNode> >& children = parent->getChildren();
    
    for (std::list<boost::shared_ptr<PluginGroupNode> >::const_iterator it = children.begin(); it != children.end(); ++it) {
        if ((*it)->getID() == grouping[0]) {
            
            if (grouping.size() > 1) {
                QStringList newGrouping;
                for (int i = 1; i < grouping.size(); ++i) {
                    newGrouping.push_back(grouping[i]);
                }
                return findPluginToolButtonInternal(*it, newGrouping, name, iconPath, major,minor, isUserCreatable);
            }
            if (major == (*it)->getMajorVersion()) {
                return *it;
            } else {
                (*it)->setNotHighestMajorVersion(true);
            }
        }
    }
    boost::shared_ptr<PluginGroupNode> ret(new PluginGroupNode(grouping[0],grouping.size() == 1 ? name : grouping[0],iconPath,major,minor, isUserCreatable));
    parent->tryAddChild(ret);
    ret->setParent(parent);
    
    if (grouping.size() > 1) {
        QStringList newGrouping;
        for (int i = 1; i < grouping.size(); ++i) {
            newGrouping.push_back(grouping[i]);
        }
        return findPluginToolButtonInternal(ret, newGrouping, name, iconPath, major,minor, isUserCreatable);
    }
    return ret;
}

boost::shared_ptr<PluginGroupNode>
GuiApplicationManager::findPluginToolButtonOrCreate(const QStringList & grouping,
                                                    const QString & name,
                                                    const QString& groupIconPath,
                                                    const QString & iconPath,
                                                    int major,
                                                    int minor,
                                                    bool isUserCreatable)
{
    assert(grouping.size() > 0);
    
    for (std::list<boost::shared_ptr<PluginGroupNode> >::iterator it = _imp->_topLevelToolButtons.begin(); it != _imp->_topLevelToolButtons.end(); ++it) {
        if ((*it)->getID() == grouping[0]) {
            
            if (grouping.size() > 1) {
                QStringList newGrouping;
                for (int i = 1; i < grouping.size(); ++i) {
                    newGrouping.push_back(grouping[i]);
                }
                return _imp->findPluginToolButtonInternal(*it, newGrouping, name, iconPath, major , minor, isUserCreatable);
            }
            if (major == (*it)->getMajorVersion()) {
                return *it;
            } else {
                (*it)->setNotHighestMajorVersion(true);
            }
        }
    }
    
    boost::shared_ptr<PluginGroupNode> ret(new PluginGroupNode(grouping[0],grouping.size() == 1 ? name : grouping[0],iconPath,major,minor, isUserCreatable));
    _imp->_topLevelToolButtons.push_back(ret);
    if (grouping.size() > 1) {
        ret->setIconPath(groupIconPath);
        QStringList newGrouping;
        for (int i = 1; i < grouping.size(); ++i) {
            newGrouping.push_back(grouping[i]);
        }
        return _imp->findPluginToolButtonInternal(ret, newGrouping, name, iconPath, major,minor, isUserCreatable);
    }
    return ret;
}

void
GuiApplicationManagerPrivate::createColorPickerCursor()
{
    QImage originalImage;
    originalImage.load(NATRON_IMAGES_PATH "color_picker.png");
    originalImage = originalImage.scaled(16, 16);
    QImage dstImage(32,32,QImage::Format_ARGB32);
    dstImage.fill(QColor(0,0,0,0));
    
    int oW = originalImage.width();
    int oH = originalImage.height();
    for (int y = 0; y < oH; ++y) {
        for (int x = 0; x < oW; ++x) {
            dstImage.setPixel(x + oW, y, originalImage.pixel(x, y));
        }
    }
    QPixmap pix = QPixmap::fromImage(dstImage);
    _colorPickerCursor = new QCursor(pix);
}

bool
GuiApplicationManager::isSplashcreenVisible() const
{
    return _imp->_splashScreen ? true : false;
}

void
GuiApplicationManager::hideSplashScreen()
{
    if (_imp->_splashScreen) {
        _imp->_splashScreen->hide();
        delete _imp->_splashScreen;
        _imp->_splashScreen = 0;
    }
}

bool
GuiApplicationManager::isClipBoardEmpty() const
{
    return _imp->_knobsClipBoard->isEmpty;
}

void
GuiApplicationManager::setKnobClipBoard(bool copyAnimation,
                                        const std::list<Variant> & values,
                                        const std::list<boost::shared_ptr<Curve> > & animation,
                                        const std::map<int,std::string> & stringAnimation,
                                        const std::list<boost::shared_ptr<Curve> > & parametricCurves,
                                        const std::string& appID,
                                        const std::string& nodeFullyQualifiedName,
                                        const std::string& paramName)
{
    _imp->_knobsClipBoard->copyAnimation = copyAnimation;
    _imp->_knobsClipBoard->isEmpty = false;
    _imp->_knobsClipBoard->values = values;
    _imp->_knobsClipBoard->curves = animation;
    _imp->_knobsClipBoard->stringAnimation = stringAnimation;
    _imp->_knobsClipBoard->parametricCurves = parametricCurves;
    _imp->_knobsClipBoard->appID = appID;
    _imp->_knobsClipBoard->nodeFullyQualifiedName = nodeFullyQualifiedName;
    _imp->_knobsClipBoard->paramName = paramName;
}

void
GuiApplicationManager::getKnobClipBoard(bool* copyAnimation,
                                        std::list<Variant>* values,
                                        std::list<boost::shared_ptr<Curve> >* animation,
                                        std::map<int,std::string>* stringAnimation,
                                        std::list<boost::shared_ptr<Curve> >* parametricCurves,
                                        std::string* appID,
                                        std::string* nodeFullyQualifiedName,
                                        std::string* paramName) const
{
    *copyAnimation = _imp->_knobsClipBoard->copyAnimation;
    *values = _imp->_knobsClipBoard->values;
    *animation = _imp->_knobsClipBoard->curves;
    *stringAnimation = _imp->_knobsClipBoard->stringAnimation;
    *parametricCurves = _imp->_knobsClipBoard->parametricCurves;
    *appID = _imp->_knobsClipBoard->appID;
    *nodeFullyQualifiedName = _imp->_knobsClipBoard->nodeFullyQualifiedName;
    *paramName = _imp->_knobsClipBoard->paramName;
}

void
GuiApplicationManager::updateAllRecentFileMenus()
{
    const std::map<int,AppInstanceRef> & instances = getAppInstances();

    for (std::map<int,AppInstanceRef>::const_iterator it = instances.begin(); it != instances.end(); ++it) {
        GuiAppInstance* appInstance = dynamic_cast<GuiAppInstance*>(it->second.app);
        assert(appInstance);
        Gui* gui = appInstance->getGui();
        assert(gui);
        gui->updateRecentFileActions();
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
GuiApplicationManager::loadBuiltinNodePlugins(std::map<std::string,std::vector< std::pair<std::string,double> > >* readersMap,
                                              std::map<std::string,std::vector< std::pair<std::string,double> > >* writersMap)
{
    ////Use ReadQt and WriteQt only for debug versions of Natron.
    // these  are built-in nodes
    QStringList grouping;

    grouping.push_back(PLUGIN_GROUP_IMAGE); // Readers, Writers, and Generators are in the "Image" group in Nuke

# ifdef DEBUG
    {
        std::auto_ptr<QtReader> reader( dynamic_cast<QtReader*>( QtReader::BuildEffect( boost::shared_ptr<Natron::Node>() ) ) );
        assert(reader.get());
        std::map<std::string,void*> readerFunctions;
        readerFunctions.insert( std::make_pair("BuildEffect", (void*)&QtReader::BuildEffect) );
        LibraryBinary *readerPlugin = new LibraryBinary(readerFunctions);
        assert(readerPlugin);
        
        registerPlugin(grouping, reader->getPluginID().c_str(), reader->getPluginLabel().c_str(), "", "", false, false, readerPlugin, false, reader->getMajorVersion(), reader->getMinorVersion(), true);
 
        std::vector<std::string> extensions = reader->supportedFileFormats();
        for (U32 k = 0; k < extensions.size(); ++k) {
            std::map<std::string,std::vector< std::pair<std::string,double> > >::iterator it;
            it = readersMap->find(extensions[k]);

            if ( it != readersMap->end() ) {
                it->second.push_back( std::make_pair(reader->getPluginID(), -1) );
            } else {
                std::vector<std::pair<std::string,double> > newVec(1);
                newVec[0] = std::make_pair(reader->getPluginID(),-1);
                readersMap->insert( std::make_pair(extensions[k], newVec) );
            }
        }
    }

    {
        std::auto_ptr<QtWriter> writer( dynamic_cast<QtWriter*>( QtWriter::BuildEffect( boost::shared_ptr<Natron::Node>() ) ) );
        assert(writer.get());
        std::map<std::string,void*> writerFunctions;
        writerFunctions.insert( std::make_pair("BuildEffect", (void*)&QtWriter::BuildEffect) );
        LibraryBinary *writerPlugin = new LibraryBinary(writerFunctions);
        assert(writerPlugin);
        
        registerPlugin(grouping, writer->getPluginID().c_str(), writer->getPluginLabel().c_str(),"", "", false, false, writerPlugin, false, writer->getMajorVersion(), writer->getMinorVersion(), true);
        


        std::vector<std::string> extensions = writer->supportedFileFormats();
        for (U32 k = 0; k < extensions.size(); ++k) {
            std::map<std::string,std::vector< std::pair<std::string,double> > >::iterator it;
            it = writersMap->find(extensions[k]);

            if ( it != writersMap->end() ) {
                it->second.push_back( std::make_pair(writer->getPluginID(), -1) );
            } else {
                std::vector<std::pair<std::string,double> > newVec(1);
                newVec[0] = std::make_pair(writer->getPluginID(),-1);
                writersMap->insert( std::make_pair(extensions[k], newVec) );
            }
        }
    }
# else // !DEBUG
    Q_UNUSED(readersMap);
    Q_UNUSED(writersMap);
# endif // DEBUG

    {
        boost::shared_ptr<EffectInstance> viewer( ViewerInstance::BuildEffect( boost::shared_ptr<Natron::Node>() ) );
        assert(viewer);
        std::map<std::string,void*> viewerFunctions;
        viewerFunctions.insert( std::make_pair("BuildEffect", (void*)&ViewerInstance::BuildEffect) );
        LibraryBinary *viewerPlugin = new LibraryBinary(viewerFunctions);
        assert(viewerPlugin);
        
        registerPlugin(grouping, viewer->getPluginID().c_str(), viewer->getPluginLabel().c_str(),NATRON_IMAGES_PATH "viewer_icon.png", "", false, false, viewerPlugin, false, viewer->getMajorVersion(), viewer->getMinorVersion(), true);

    }

    ///Also load the plug-ins of the AppManager
    AppManager::loadBuiltinNodePlugins(readersMap, writersMap);
} // loadBuiltinNodePlugins

AppInstance*
GuiApplicationManager::makeNewInstance(int appID) const
{
    return new GuiAppInstance(appID);
}

KnobGui*
GuiApplicationManager::createGuiForKnob(boost::shared_ptr<KnobI> knob,
                                        DockablePanel *container) const
{
    return _imp->_knobGuiFactory->createGuiForKnob(knob,container);
}

void
GuiApplicationManager::registerGuiMetaTypes() const
{
    qRegisterMetaType<CurveWidget*>();
}

class Application
    : public QApplication
{
    GuiApplicationManager* _app;

public:

    Application(GuiApplicationManager* app,
                int &argc,
                char** argv)                                      ///< qapplication needs to be subclasses with reference otherwise lots of crashes will happen.
        : QApplication(argc,argv)
          , _app(app)
    {
    }

protected:

    bool event(QEvent* e);
};

bool
Application::event(QEvent* e)
{
    switch ( e->type() ) {
    case QEvent::FileOpen: {
        assert(_app);
        QFileOpenEvent* foe = dynamic_cast<QFileOpenEvent*>(e);
        assert(foe);
        if (foe) {
            QString file =  foe->file();
#ifdef Q_OS_UNIX
            if ( !file.isEmpty() ) {
                file = AppManager::qt_tildeExpansion(file);
            }
#endif
            _app->setFileToOpen(file);
        }
    }

        return true;
    default:

        return QApplication::event(e);
    }
}

void
GuiApplicationManager::initializeQApp(int &argc,
                                      char** argv)
{
    QApplication* app = new Application(this,argc, argv);

    app->setQuitOnLastWindowClosed(true);
    Q_INIT_RESOURCE(GuiResources);
    
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
    const std::map<int,AppInstanceRef> & apps = getAppInstances();

    for (std::map<int,AppInstanceRef>::const_iterator it = apps.begin(); it != apps.end(); ++it) {
        GuiAppInstance* guiApp = dynamic_cast<GuiAppInstance*>(it->second.app);
        if (guiApp) {
            guiApp->setUndoRedoStackLimit(limit);
        }
    }
}

void
GuiApplicationManager::debugImage(const Natron::Image* image,
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

void
GuiApplicationManager::handleOpenFileRequest()
{


    AppInstance* mainApp = getAppInstance(0);
    GuiAppInstance* guiApp = dynamic_cast<GuiAppInstance*>(mainApp);
    assert(guiApp);
    if (guiApp) {
        guiApp->getGui()->openProject(_imp->_openFileRequest.toStdString());
        _imp->_openFileRequest.clear();
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
GuiApplicationManager::exitApp()
{
    ///make a copy of the map because it will be modified when closing projects
    std::map<int,AppInstanceRef> instances = getAppInstances();

    for (std::map<int,AppInstanceRef>::const_iterator it = instances.begin(); it != instances.end(); ++it) {
        GuiAppInstance* app = dynamic_cast<GuiAppInstance*>(it->second.app);
        if ( app && !app->getGui()->closeInstance() ) {
            return;
        }
    }
}


static bool
matchesModifers(const Qt::KeyboardModifiers & lhs,
                const Qt::KeyboardModifiers & rhs,
                Qt::Key key)
{
    if (lhs == rhs) {
        return true;
    }

    bool isDigit =
        key == Qt::Key_0 ||
        key == Qt::Key_1 ||
        key == Qt::Key_2 ||
        key == Qt::Key_3 ||
        key == Qt::Key_4 ||
        key == Qt::Key_5 ||
        key == Qt::Key_6 ||
        key == Qt::Key_7 ||
        key == Qt::Key_8 ||
        key == Qt::Key_9;
    // On some keyboards (e.g. French AZERTY), the number keys are shifted
    if ( ( lhs == (Qt::ShiftModifier | Qt::AltModifier) ) && (rhs == Qt::AltModifier) && isDigit ) {
        return true;
    }

    return false;
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
GuiApplicationManager::matchesKeybind(const QString & group,
                                      const QString & actionID,
                                      const Qt::KeyboardModifiers & modifiers,
                                      int symbol) const
{
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

    if ( matchesModifers(onlyCAS,keybind->modifiers,(Qt::Key)symbol) ) {
        // modifiers are equal, now test symbol
        return matchesKey( (Qt::Key)symbol, keybind->currentShortcut );
    }

    return false;
}

bool
GuiApplicationManager::matchesMouseShortcut(const QString & group,
                                            const QString & actionID,
                                            const Qt::KeyboardModifiers & modifiers,
                                            int button) const
{
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

    if (onlyMCAS == mAction->modifiers) {
        // modifiers are equal, now test symbol
        if ( (Qt::MouseButton)button == mAction->button ) {
            return true;
        }
    }

    return false;
}

void
GuiApplicationManager::saveShortcuts() const
{
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);
    
    for (AppShortcuts::const_iterator it = _imp->_actionShortcuts.begin(); it != _imp->_actionShortcuts.end(); ++it) {
        settings.beginGroup(it->first);
        for (GroupShortcuts::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            const MouseAction* mAction = dynamic_cast<const MouseAction*>(it2->second);
            const KeyBoundAction* kAction = dynamic_cast<const KeyBoundAction*>(it2->second);
            settings.setValue(it2->first + "_Modifiers", (int)it2->second->modifiers);
            if (mAction) {
                settings.setValue(it2->first + "_Button", (int)mAction->button);
            } else if (kAction) {
                settings.setValue(it2->first + "_Symbol", (int)kAction->currentShortcut);
            }
        }
        settings.endGroup();
    }
}

void
GuiApplicationManager::loadShortcuts()
{
    
    bool settingsExistd = getCurrentSettings()->didSettingsExistOnStartup();
    
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);

    int settingsVersion = -1;
    if (settings.contains("NATRON_SHORTCUTS_DEFAULT_VERSION")) {
        settingsVersion = settings.value("NATRON_SHORTCUTS_DEFAULT_VERSION").toInt();
    }
    
    if (settingsExistd && settingsVersion < NATRON_SHORTCUTS_DEFAULT_VERSION) {
        _imp->_shortcutsChangedVersion = true;
    }
    
    settings.setValue("NATRON_SHORTCUTS_DEFAULT_VERSION", NATRON_SHORTCUTS_DEFAULT_VERSION);

    
    for (AppShortcuts::iterator it = _imp->_actionShortcuts.begin(); it != _imp->_actionShortcuts.end(); ++it) {
        settings.beginGroup(it->first);
        for (GroupShortcuts::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            MouseAction* mAction = dynamic_cast<MouseAction*>(it2->second);
            KeyBoundAction* kAction = dynamic_cast<KeyBoundAction*>(it2->second);

            if ( settings.contains(it2->first + "_Modifiers") ) {
                it2->second->modifiers = Qt::KeyboardModifiers( settings.value(it2->first + "_Modifiers").toInt() );
            }
            if ( mAction && settings.contains(it2->first + "_Button") ) {
                mAction->button = (Qt::MouseButton)settings.value(it2->first + "_Button").toInt();
            }
            if ( kAction && settings.contains(it2->first + "_Symbol") ) {
                
                Qt::Key newShortcut = (Qt::Key)settings.value(it2->first + "_Symbol").toInt();
                kAction->currentShortcut = newShortcut;
                

                //If this is a node shortcut, notify the Plugin object that it has a shortcut.
                if ( (kAction->currentShortcut != (Qt::Key)0) &&
                     it->first.startsWith(kShortcutGroupNodes) ) {
                    const PluginsMap & allPlugins = getPluginsList();
                    PluginsMap::const_iterator found = allPlugins.find(it2->first.toStdString());
                    if (found != allPlugins.end()) {
                        assert(found->second.size() > 0);
                        (*found->second.rbegin())->setHasShortcut(true);
                    }
                }
            }
        }
        settings.endGroup();
    }
}

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
    registerStandardKeybind(kShortcutGroupGlobal, kShortcutIDActionNewProject, kShortcutDescActionNewProject,QKeySequence::New);
    registerStandardKeybind(kShortcutGroupGlobal, kShortcutIDActionOpenProject, kShortcutDescActionOpenProject,QKeySequence::Open);
    registerStandardKeybind(kShortcutGroupGlobal, kShortcutIDActionSaveProject, kShortcutDescActionSaveProject,QKeySequence::Save);
    registerStandardKeybind(kShortcutGroupGlobal, kShortcutIDActionSaveAsProject, kShortcutDescActionSaveAsProject,QKeySequence::SaveAs);
    registerStandardKeybind(kShortcutGroupGlobal, kShortcutIDActionCloseProject, kShortcutDescActionCloseProject,QKeySequence::Close);
    registerStandardKeybind(kShortcutGroupGlobal, kShortcutIDActionPreferences, kShortcutDescActionPreferences,QKeySequence::Preferences);
    registerStandardKeybind(kShortcutGroupGlobal, kShortcutIDActionQuit, kShortcutDescActionQuit,QKeySequence::Quit);

    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionSaveAndIncrVersion, kShortcutDescActionSaveAndIncrVersion, Qt::ControlModifier | Qt::ShiftModifier |
                    Qt::AltModifier, Qt::Key_S);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionExportProject, kShortcutDescActionExportProject, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionShowAbout, kShortcutDescActionShowAbout, Qt::NoModifier, (Qt::Key)0);

    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionImportLayout, kShortcutDescActionImportLayout, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionExportLayout, kShortcutDescActionExportLayout, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionDefaultLayout, kShortcutDescActionDefaultLayout, Qt::NoModifier, (Qt::Key)0);

    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionProjectSettings, kShortcutDescActionProjectSettings, Qt::NoModifier, Qt::Key_S);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionShowOFXLog, kShortcutDescActionShowOFXLog, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionShowShortcutEditor, kShortcutDescActionShowShortcutEditor, Qt::NoModifier, (Qt::Key)0);

    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionNewViewer, kShortcutDescActionNewViewer, Qt::ControlModifier, Qt::Key_I);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionFullscreen, kShortcutDescActionFullscreen, Qt::AltModifier, Qt::Key_S); // as in Nuke
    //registerKeybind(kShortcutGroupGlobal, kShortcutIDActionFullscreen, kShortcutDescActionFullscreen, Qt::ControlModifier | Qt::AltModifier, Qt::Key_F);

    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionClearDiskCache, kShortcutDescActionClearDiskCache, Qt::NoModifier,(Qt::Key)0);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionClearPlaybackCache, kShortcutDescActionClearPlaybackCache, Qt::NoModifier,(Qt::Key)0);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionClearNodeCache, kShortcutDescActionClearNodeCache, Qt::NoModifier,(Qt::Key)0);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionClearPluginsLoadCache, kShortcutDescActionClearPluginsLoadCache, Qt::NoModifier,(Qt::Key)0);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionClearAllCaches, kShortcutDescActionClearAllCaches, Qt::ControlModifier | Qt::ShiftModifier, Qt::Key_K);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionRenderSelected, kShortcutDescActionRenderSelected, Qt::NoModifier, Qt::Key_F7);

    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionRenderAll, kShortcutDescActionRenderAll, Qt::NoModifier, Qt::Key_F5);


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

    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionShowPaneFullScreen, kShortcutDescActionShowPaneFullScreen, Qt::NoModifier, Qt::Key_Space);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionNextTab, kShortcutDescActionNextTab, Qt::ControlModifier, Qt::Key_T);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionPrevTab, kShortcutDescActionPrevTab, Qt::ShiftModifier | Qt::ControlModifier, Qt::Key_T);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionCloseTab, kShortcutDescActionCloseTab, Qt::ShiftModifier, Qt::Key_Escape);
    
    
    ///Viewer
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionZoomIn, kShortcutDescActionZoomIn, Qt::NoModifier, Qt::Key_Plus);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionZoomOut, kShortcutDescActionZoomOut, Qt::NoModifier, Qt::Key_Minus);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionFitViewer, kShortcutDescActionFitViewer, Qt::NoModifier, Qt::Key_F);

    registerKeybind(kShortcutGroupViewer, kShortcutIDActionLuminance, kShortcutDescActionLuminance, Qt::NoModifier, Qt::Key_Y);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionRed, kShortcutDescActionRed, Qt::NoModifier, Qt::Key_R);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionGreen, kShortcutDescActionGreen, Qt::NoModifier, Qt::Key_G);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionBlue, kShortcutDescActionBlue, Qt::NoModifier, Qt::Key_B);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionAlpha, kShortcutDescActionAlpha, Qt::NoModifier, Qt::Key_A);
    
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionLuminanceA, kShortcutDescActionLuminanceA, Qt::ShiftModifier, Qt::Key_Y);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionRedA, kShortcutDescActionRedA, Qt::ShiftModifier, Qt::Key_R);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionGreenA, kShortcutDescActionGreenA, Qt::ShiftModifier, Qt::Key_G);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionBlueA, kShortcutDescActionBlueA, Qt::ShiftModifier, Qt::Key_B);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionAlphaA, kShortcutDescActionAlphaA, Qt::ShiftModifier, Qt::Key_A);
    
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionClipEnabled, kShortcutDescActionClipEnabled, Qt::ShiftModifier, Qt::Key_C);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionRefresh, kShortcutDescActionRefresh, Qt::NoModifier, Qt::Key_U);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionROIEnabled, kShortcutDescActionROIEnabled, Qt::ShiftModifier, Qt::Key_W);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionProxyEnabled, kShortcutDescActionProxyEnabled, Qt::ControlModifier, Qt::Key_P);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionProxyLevel2, kShortcutDescActionProxyLevel2, Qt::AltModifier, Qt::Key_1);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionProxyLevel4, kShortcutDescActionProxyLevel4, Qt::AltModifier, Qt::Key_2);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionProxyLevel8, kShortcutDescActionProxyLevel8, Qt::AltModifier, Qt::Key_3);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionProxyLevel16, kShortcutDescActionProxyLevel16, Qt::AltModifier, Qt::Key_4);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionProxyLevel32, kShortcutDescActionProxyLevel32, Qt::AltModifier, Qt::Key_5);

    registerKeybind(kShortcutGroupViewer, kShortcutIDActionHideOverlays, kShortcutDescActionHideOverlays, Qt::NoModifier, Qt::Key_O);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionHidePlayer, kShortcutDescActionHidePlayer, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionHideTimeline, kShortcutDescActionHideTimeline, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionHideLeft, kShortcutDescActionHideLeft, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionHideRight, kShortcutDescActionHideRight, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionHideTop, kShortcutDescActionHideTop, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionHideInfobar, kShortcutDescActionHideInfobar, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionHideAll, kShortcutDescActionHideAll, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionShowAll, kShortcutDescActionShowAll, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionZoomLevel100, kShortcutDescActionZoomLevel100, Qt::ControlModifier, Qt::Key_1);
    registerKeybind(kShortcutGroupViewer, kShortcutIDToggleWipe, kShortcutDescToggleWipe, Qt::NoModifier, Qt::Key_W);
    
    registerMouseShortcut(kShortcutGroupViewer, kShortcutIDMousePickColor, kShortcutDescMousePickColor, Qt::ControlModifier, Qt::LeftButton);
    registerMouseShortcut(kShortcutGroupViewer, kShortcutIDMouseRectanglePick, kShortcutDescMouseRectanglePick, Qt::ShiftModifier | Qt::ControlModifier, Qt::LeftButton);
    
    

    ///Player
    registerKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevious, kShortcutDescActionPlayerPrevious, Qt::NoModifier, Qt::Key_Left);
    registerKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNext, kShortcutDescActionPlayerNext, Qt::NoModifier, Qt::Key_Right);
    registerKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerBackward, kShortcutDescActionPlayerBackward, Qt::NoModifier, Qt::Key_J);
    registerKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerForward, kShortcutDescActionPlayerForward, Qt::NoModifier, Qt::Key_L);
    registerKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerStop, kShortcutDescActionPlayerStop, Qt::NoModifier, Qt::Key_K);
    registerKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevIncr, kShortcutDescActionPlayerPrevIncr, Qt::ShiftModifier, Qt::Key_Left);
    registerKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNextIncr, kShortcutDescActionPlayerNextIncr, Qt::ShiftModifier, Qt::Key_Right);
    registerKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevKF, kShortcutDescActionPlayerPrevKF, Qt::ShiftModifier | Qt::ControlModifier, Qt::Key_Left);
    registerKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNextKF, kShortcutDescActionPlayerPrevKF, Qt::ShiftModifier | Qt::ControlModifier, Qt::Key_Right);
    registerKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerFirst, kShortcutDescActionPlayerFirst, Qt::ControlModifier, Qt::Key_Left);
    registerKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerLast, kShortcutDescActionPlayerLast, Qt::ControlModifier, Qt::Key_Right);

    ///Roto
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoDelete, kShortcutDescActionRotoDelete, Qt::NoModifier, Qt::Key_Backspace);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoCloseBezier, kShortcutDescActionRotoCloseBezier, Qt::NoModifier, Qt::Key_Enter);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoSelectAll, kShortcutDescActionRotoSelectAll, Qt::ControlModifier, Qt::Key_A);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoSelectionTool, kShortcutDescActionRotoSelectionTool, Qt::NoModifier, Qt::Key_Q);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoAddTool, kShortcutDescActionRotoAddTool, Qt::NoModifier, Qt::Key_D);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoEditTool, kShortcutDescActionRotoEditTool, Qt::NoModifier, Qt::Key_V);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoBrushTool, kShortcutDescActionRotoBrushTool, Qt::NoModifier, Qt::Key_N);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoCloneTool, kShortcutDescActionRotoCloneTool, Qt::NoModifier, Qt::Key_C);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoEffectTool, kShortcutDescActionRotoEffectTool, Qt::NoModifier, Qt::Key_X);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoColorTool, kShortcutDescActionRotoColorTool, Qt::NoModifier, Qt::Key_E);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoNudgeLeft, kShortcutDescActionRotoNudgeLeft, Qt::AltModifier, Qt::Key_Left);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoNudgeBottom, kShortcutDescActionRotoNudgeBottom, Qt::AltModifier, Qt::Key_Down);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoNudgeRight, kShortcutDescActionRotoNudgeRight, Qt::AltModifier, Qt::Key_Right);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoNudgeTop, kShortcutDescActionRotoNudgeTop, Qt::AltModifier, Qt::Key_Up);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoSmooth, kShortcutDescActionRotoSmooth, Qt::NoModifier, Qt::Key_Z);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoCuspBezier, kShortcutDescActionRotoCuspBezier, Qt::ShiftModifier, Qt::Key_Z);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoRemoveFeather, kShortcutDescActionRotoRemoveFeather, Qt::ShiftModifier, Qt::Key_E);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoLinkToTrack, kShortcutDescActionRotoLinkToTrack, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoUnlinkToTrack, kShortcutDescActionRotoUnlinkToTrack,
                    Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoLockCurve, kShortcutDescActionRotoLockCurve,
                    Qt::ShiftModifier, Qt::Key_L);


    ///Tracking
    registerKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingSelectAll, kShortcutDescActionTrackingSelectAll, Qt::ControlModifier, Qt::Key_A);
    registerKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingDelete, kShortcutDescActionTrackingDelete, Qt::NoModifier, Qt::Key_Backspace);
    registerKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingBackward, kShortcutDescActionTrackingBackward, Qt::NoModifier, Qt::Key_Z);
    registerKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingPrevious, kShortcutDescActionTrackingPrevious, Qt::NoModifier, Qt::Key_X);
    registerKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingNext, kShortcutDescActionTrackingNext, Qt::NoModifier, Qt::Key_C);
    registerKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingForward, kShortcutDescActionTrackingForward, Qt::NoModifier, Qt::Key_V);
    registerKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingStop, kShortcutDescActionTrackingStop, Qt::NoModifier, Qt::Key_Escape);

    ///Nodegraph
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphCreateReader, kShortcutDescActionGraphCreateReader, Qt::NoModifier, Qt::Key_R);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphCreateWriter, kShortcutDescActionGraphCreateWriter, Qt::NoModifier, Qt::Key_W);

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
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphEnableHints, kShortcutDescActionGraphEnableHints, Qt::NoModifier, Qt::Key_H);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphAutoHideInputs, kShortcutDescActionGraphAutoHideInputs, Qt::NoModifier, (Qt::Key)0);
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
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphMakeGroup, kShortcutDescActionGraphMakeGroup, Qt::ControlModifier | Qt::ShiftModifier,
                    Qt::Key_G);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphExpandGroup, kShortcutDescActionGraphExpandGroup, Qt::ControlModifier | Qt::ShiftModifier,
                    Qt::Key_E);
    
    ///CurveEditor
    registerKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorRemoveKeys, kShortcutDescActionCurveEditorRemoveKeys, Qt::NoModifier,Qt::Key_Backspace);
    registerKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorConstant, kShortcutDescActionCurveEditorConstant, Qt::NoModifier, Qt::Key_K);
    registerKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorSmooth, kShortcutDescActionCurveEditorSmooth, Qt::NoModifier, Qt::Key_Z);
    registerKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorLinear, kShortcutDescActionCurveEditorLinear, Qt::NoModifier, Qt::Key_L);
    registerKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorCatmullrom, kShortcutDescActionCurveEditorCatmullrom, Qt::NoModifier, Qt::Key_R);
    registerKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorCubic, kShortcutDescActionCurveEditorCubic, Qt::NoModifier, Qt::Key_C);
    registerKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorHorizontal, kShortcutDescActionCurveEditorHorizontal, Qt::NoModifier, Qt::Key_H);
    registerKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorBreak, kShortcutDescActionCurveEditorBreak, Qt::NoModifier, Qt::Key_X);
    registerKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorSelectAll, kShortcutDescActionCurveEditorSelectAll, Qt::ControlModifier, Qt::Key_A);
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
} // populateShortcuts

void
GuiApplicationManagerPrivate::addKeybind(const QString & grouping,
                                         const QString & id,
                                         const QString & description,
                                         const Qt::KeyboardModifiers & modifiers,
                                         Qt::Key symbol)
{
    
    AppShortcuts::iterator foundGroup = _actionShortcuts.find(grouping);
    if ( foundGroup != _actionShortcuts.end() ) {
        GroupShortcuts::iterator foundAction = foundGroup->second.find(id);
        if ( foundAction != foundGroup->second.end() ) {
            return;
        }
    }
    KeyBoundAction* kA = new KeyBoundAction;

    kA->grouping = grouping;
    kA->description = description;
    kA->defaultModifiers = modifiers;
    kA->modifiers = modifiers;
    kA->defaultShortcut = symbol;
    kA->currentShortcut = symbol;
    if ( foundGroup != _actionShortcuts.end() ) {
        foundGroup->second.insert( std::make_pair(id, kA) );
    } else {
        GroupShortcuts group;
        group.insert( std::make_pair(id, kA) );
        _actionShortcuts.insert( std::make_pair(grouping, group) );
    }
    
    GuiAppInstance* app = dynamic_cast<GuiAppInstance*>(_publicInterface->getTopLevelInstance());
    if ( app && app->getGui()->hasShortcutEditorAlreadyBeenBuilt() ) {
        app->getGui()->addShortcut(kA);
    }
}

void
GuiApplicationManagerPrivate::removeKeybind(const QString& grouping,const QString& id)
{
    AppShortcuts::iterator foundGroup = _actionShortcuts.find(grouping);
    if ( foundGroup != _actionShortcuts.end() ) {
        GroupShortcuts::iterator foundAction = foundGroup->second.find(id);
        if ( foundAction != foundGroup->second.end() ) {
            foundGroup->second.erase(foundAction);
        }
        if (foundGroup->second.empty()) {
            _actionShortcuts.erase(foundGroup);
        }
    }
}

void
GuiApplicationManagerPrivate::addMouseShortcut(const QString & grouping,
                                               const QString & id,
                                               const QString & description,
                                               const Qt::KeyboardModifiers & modifiers,
                                               Qt::MouseButton button)
{
    
    AppShortcuts::iterator foundGroup = _actionShortcuts.find(grouping);
    if ( foundGroup != _actionShortcuts.end() ) {
        GroupShortcuts::iterator foundAction = foundGroup->second.find(id);
        if ( foundAction != foundGroup->second.end() ) {
            return;
        }
    }
    MouseAction* mA = new MouseAction;

    mA->grouping = grouping;
    mA->description = description;
    mA->defaultModifiers = modifiers;
    if ( modifiers & (Qt::AltModifier | Qt::MetaModifier) ) {
        qDebug() << "Warning: mouse shortcut " << grouping << '/' << description << '(' << id << ')' << " uses the Alt or Meta modifier, which is reserved for three-button mouse emulation. Fix this ASAP.";
    }
    mA->modifiers = modifiers;
    mA->button = button;

    ///Mouse shortcuts are not editable.
    mA->editable = false;

    if ( foundGroup != _actionShortcuts.end() ) {
        foundGroup->second.insert( std::make_pair(id, mA) );
    } else {
        GroupShortcuts group;
        group.insert( std::make_pair(id, mA) );
        _actionShortcuts.insert( std::make_pair(grouping, group) );
    }
    
    GuiAppInstance* app = dynamic_cast<GuiAppInstance*>(_publicInterface->getTopLevelInstance());
    if ( app && app->getGui()->hasShortcutEditorAlreadyBeenBuilt() ) {
        app->getGui()->addShortcut(mA);
    }
}

void
GuiApplicationManagerPrivate::addStandardKeybind(const QString & grouping,
                                                 const QString & id,
                                                 const QString & description,
                                                 QKeySequence::StandardKey key)
{
    AppShortcuts::iterator foundGroup = _actionShortcuts.find(grouping);
    if ( foundGroup != _actionShortcuts.end() ) {
        GroupShortcuts::iterator foundAction = foundGroup->second.find(id);
        if ( foundAction != foundGroup->second.end() ) {
            return;
        }
    }
    
    Qt::KeyboardModifiers modifiers;
    Qt::Key symbol;

    extractKeySequence(QKeySequence(key), modifiers, symbol);
    KeyBoundAction* kA = new KeyBoundAction;
    kA->grouping = grouping;
    kA->description = description;
    kA->defaultModifiers = modifiers;
    kA->modifiers = modifiers;
    kA->defaultShortcut = symbol;
    kA->currentShortcut = symbol;
    if ( foundGroup != _actionShortcuts.end() ) {
        foundGroup->second.insert( std::make_pair(id, kA) );
    } else {
        GroupShortcuts group;
        group.insert( std::make_pair(id, kA) );
        _actionShortcuts.insert( std::make_pair(grouping, group) );
    }
    
    GuiAppInstance* app = dynamic_cast<GuiAppInstance*>(_publicInterface->getTopLevelInstance());
    if ( app && app->getGui()->hasShortcutEditorAlreadyBeenBuilt() ) {
        app->getGui()->addShortcut(kA);
    }
}

QKeySequence
GuiApplicationManager::getKeySequenceForAction(const QString & group,
                                               const QString & actionID) const
{
    AppShortcuts::const_iterator foundGroup = _imp->_actionShortcuts.find(group);

    if ( foundGroup != _imp->_actionShortcuts.end() ) {
        GroupShortcuts::const_iterator found = foundGroup->second.find(actionID);
        if ( found != foundGroup->second.end() ) {
            const KeyBoundAction* ka = dynamic_cast<const KeyBoundAction*>(found->second);
            if (ka) {
                return makeKeySequence(found->second->modifiers, ka->currentShortcut);
            }
        }
    }

    return QKeySequence();
}

bool
GuiApplicationManager::getModifiersAndKeyForAction(const QString & group,
                                                   const QString & actionID,
                                                   Qt::KeyboardModifiers & modifiers,
                                                   int & symbol) const
{
    AppShortcuts::const_iterator foundGroup = _imp->_actionShortcuts.find(group);

    if ( foundGroup != _imp->_actionShortcuts.end() ) {
        GroupShortcuts::const_iterator found = foundGroup->second.find(actionID);
        if ( found != foundGroup->second.end() ) {
            const KeyBoundAction* ka = dynamic_cast<const KeyBoundAction*>(found->second);
            if (ka) {
                modifiers = found->second->modifiers;
                symbol = ka->currentShortcut;

                return true;
            }
        }
    }

    return false;
}

const std::map<QString,std::map<QString,BoundAction*> > &
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
                std::list<ActionWithShortcut*>::iterator foundAction = std::find(ka->actions.begin(),ka->actions.end(),action);
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
        if ( it->first.startsWith(kShortcutGroupNodes) ) {
            for (GroupShortcuts::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
                if (it2->second == action) {
                    const PluginsMap & allPlugins = getPluginsList();
                    PluginsMap::const_iterator found = allPlugins.find(it2->first.toStdString());
                    if (found != allPlugins.end()) {
                        assert(found->second.size() > 0);
                        (*found->second.rbegin())->setHasShortcut(action->currentShortcut != (Qt::Key)0);
                    }
                    break;
                }
            }
        }
    }
}

void
GuiApplicationManager::showOfxLog()
{
    GuiAppInstance* app = dynamic_cast<GuiAppInstance*>(getTopLevelInstance());
    if (app) {
        app->getGui()->showOfxLog();
    }
    
}

void
GuiApplicationManager::clearLastRenderedTextures()
{
    const std::map<int,AppInstanceRef>& instances = getAppInstances();
    for (std::map<int,AppInstanceRef>::const_iterator it = instances.begin(); it != instances.end(); ++it) {
        GuiAppInstance* guiApp = dynamic_cast<GuiAppInstance*>(it->second.app);
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
#ifndef IS_PYTHON_2
    PyObject* PyInit_NatronGui();
#else
    void initNatronGui();
#endif
}


void
GuiApplicationManager::initBuiltinPythonModules()
{
    AppManager::initBuiltinPythonModules();
    
#ifndef IS_PYTHON_2
    int ret = PyImport_AppendInittab(NATRON_GUI_PYTHON_MODULE_NAME,&PyInit_NatronGui);
#else
    int ret = PyImport_AppendInittab(NATRON_GUI_PYTHON_MODULE_NAME,&initNatronGui);
#endif
    if (ret == -1) {
        throw std::runtime_error("Failed to initialize built-in Python module.");
    }
    
}

void
GuiApplicationManager::addCommand(const QString& grouping,const std::string& pythonFunction, Qt::Key key,const Qt::KeyboardModifiers& modifiers)
{
    
    QStringList split = grouping.split('/');
    if (grouping.isEmpty() || split.isEmpty()) {
        return;
    }
    PythonUserCommand c;
    c.grouping = grouping;
    c.pythonFunction = pythonFunction;
    c.key = key;
    c.modifiers = modifiers;
    _imp->pythonCommands.push_back(c);
    
    
    registerKeybind(kShortcutGroupGlobal, split[split.size() -1], split[split.size() - 1], modifiers, key);
}


const std::list<PythonUserCommand>&
GuiApplicationManager::getUserPythonCommands() const
{
    return _imp->pythonCommands;
}
void
GuiApplicationManager::reloadStylesheets()
{
    const std::map<int,AppInstanceRef>& instances = getAppInstances();
    for (std::map<int,AppInstanceRef>::const_iterator it = instances.begin(); it != instances.end(); ++it) {
        GuiAppInstance* guiApp = dynamic_cast<GuiAppInstance*>(it->second.app);
        if (guiApp) {
            guiApp->reloadStylesheet();
        }
    }
}
