//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
*contact: immarespond at gmail dot com
*
*/

#include "GuiApplicationManager.h"

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

#define registerKeybind(group,id,description,modifiers,symbol) (_imp->addKeybind(group,id,description,modifiers,symbol))
#define registerStandardKeybind(group,id,description,key) (_imp->addStandardKeybind(group,id,description,key))

#define registerMouseShortcut(group,id,description,modifiers,button) (_imp->addMouseShortcut(group,id,description,modifiers,button))


using namespace Natron;

struct KnobsClipBoard {
    std::list<Variant> values; //< values
    std::list< boost::shared_ptr<Curve> > curves; //< animation
    std::list< boost::shared_ptr<Curve> > parametricCurves; //< for parametric knobs
    std::map<int,std::string> stringAnimation; //< for animating string knobs
    bool isEmpty; //< is the clipboard empty
    bool copyAnimation; //< should we copy all the animation or not
    int dimension;//< -1 if all dims, otherwise the dim index
};

class BoundAction {
    
public:
    
    bool editable;
    QString grouping; //< the grouping of the action, such as CurveEditor/
    QString description; //< the description that will be in the shortcut editor
    Qt::KeyboardModifiers modifiers; //< the keyboard modifiers that must be held down during the action
    Qt::KeyboardModifiers defaultModifiers; //< the default keyboard modifiers
    
    BoundAction() : editable(true) {}
    
    virtual ~BoundAction() {}
};

class KeyBoundAction : public BoundAction
{
public:
    
    Qt::Key currentShortcut; //< the actual shortcut for the keybind
    Qt::Key defaultShortcut; //< the default shortcut proposed by the dev team
    
    KeyBoundAction() : BoundAction() {}
    
    virtual ~KeyBoundAction() {}
};

class MouseAction : public BoundAction {
    
public:
    
    Qt::MouseButton button; //< the button that must be held down for the action. This cannot be edited!
    
    MouseAction() : BoundAction() {}
    
    virtual ~MouseAction() {}
};


///All the shortcuts of a group matched against their
///internal id to find and match the action in the event handlers
typedef std::map<QString,BoundAction*> GroupShortcuts;

///All groups shortcuts mapped against the name of the group
typedef std::map<QString,GroupShortcuts> AppShortcuts;

struct GuiApplicationManagerPrivate {
    
    std::vector<PluginGroupNode*> _toolButtons;
    
    boost::scoped_ptr<KnobsClipBoard> _knobsClipBoard;
    
    boost::scoped_ptr<KnobGuiFactory> _knobGuiFactory;
    
    QCursor* _colorPickerCursor;
    
    SplashScreen* _splashScreen;
    
    ///We store here the file open request that was made on startup but that
    ///we couldn't handle at that time
    QString _openFileRequest;
   
    AppShortcuts _actionShortcuts;
    
    GuiApplicationManagerPrivate()
    : _toolButtons()
    , _knobsClipBoard(new KnobsClipBoard)
    , _knobGuiFactory(new KnobGuiFactory())
    , _colorPickerCursor(NULL)
    , _splashScreen(NULL)
    , _openFileRequest()
    , _actionShortcuts()
    {
    }
    
    void createColorPickerCursor();

    void addStandardKeybind(const QString& grouping,const QString& id,
                            const QString& description,QKeySequence::StandardKey key);
    
    void addKeybind(const QString& grouping,const QString& id,
                    const QString& description,
                    const Qt::KeyboardModifiers& modifiers,Qt::Key symbol);
    
    void addMouseShortcut(const QString& grouping,const QString& id,
                          const QString& description,
                          const Qt::KeyboardModifiers& modifiers,Qt::MouseButton button);
};

GuiApplicationManager::GuiApplicationManager()
: AppManager()
, _imp(new GuiApplicationManagerPrivate)
{
}

GuiApplicationManager::~GuiApplicationManager() {
    for (U32 i = 0; i < _imp->_toolButtons.size();++i) {
        delete _imp->_toolButtons[i];
    }
    delete _imp->_colorPickerCursor;
    for (AppShortcuts::iterator it = _imp->_actionShortcuts.begin(); it!=_imp->_actionShortcuts.end(); ++it) {
        for (GroupShortcuts::iterator it2 = it->second.begin(); it2!=it->second.end(); ++it2) {
            delete it2->second;
        }
    }
    
}


void GuiApplicationManager::getIcon(Natron::PixmapEnum e,QPixmap* pix) const {
    int iconSet = appPTR->getCurrentSettings()->getIconsBlackAndWhite() ? 2 : 3;
    QString iconSetStr = QString::number(iconSet);
    if(!QPixmapCache::find(QString::number(e),pix)){
        QImage img;
        switch(e){
            case NATRON_PIXMAP_PLAYER_PREVIOUS:
                img.load(NATRON_IMAGES_PATH"back1.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_PLAYER_FIRST_FRAME:
                img.load(NATRON_IMAGES_PATH"firstFrame.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_PLAYER_NEXT:
                img.load(NATRON_IMAGES_PATH"forward1.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_PLAYER_LAST_FRAME:
                img.load(NATRON_IMAGES_PATH"lastFrame.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_PLAYER_NEXT_INCR:
                img.load(NATRON_IMAGES_PATH"nextIncr.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_PLAYER_NEXT_KEY:
                img.load(NATRON_IMAGES_PATH"nextKF.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_PLAYER_PREVIOUS_INCR:
                img.load(NATRON_IMAGES_PATH"previousIncr.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_PLAYER_PREVIOUS_KEY:
                img.load(NATRON_IMAGES_PATH"prevKF.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_ADD_KEYFRAME:
                img.load(NATRON_IMAGES_PATH"addKF.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_REMOVE_KEYFRAME:
                img.load(NATRON_IMAGES_PATH"removeKF.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_PLAYER_REWIND:
                img.load(NATRON_IMAGES_PATH"rewind.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_PLAYER_PLAY:
                img.load(NATRON_IMAGES_PATH"play.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_PLAYER_STOP:
                img.load(NATRON_IMAGES_PATH"stop.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_PLAYER_LOOP_MODE:
                img.load(NATRON_IMAGES_PATH"loopmode.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MAXIMIZE_WIDGET:
                img.load(NATRON_IMAGES_PATH"maximize.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MINIMIZE_WIDGET:
                img.load(NATRON_IMAGES_PATH"minimize.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_CLOSE_WIDGET:
                img.load(NATRON_IMAGES_PATH"close.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_CLOSE_PANEL:
                img.load(NATRON_IMAGES_PATH"closePanel.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_HELP_WIDGET:
                img.load(NATRON_IMAGES_PATH"help.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_GROUPBOX_FOLDED:
                img.load(NATRON_IMAGES_PATH"groupbox_folded.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_GROUPBOX_UNFOLDED:
                img.load(NATRON_IMAGES_PATH"groupbox_unfolded.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_UNDO:
                img.load(NATRON_IMAGES_PATH"undo.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_REDO:
                img.load(NATRON_IMAGES_PATH"redo.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_UNDO_GRAYSCALE:
                img.load(NATRON_IMAGES_PATH"undo_grayscale.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_REDO_GRAYSCALE:
                img.load(NATRON_IMAGES_PATH"redo_grayscale.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_RESTORE_DEFAULTS_ENABLED:
                img.load(NATRON_IMAGES_PATH"restoreDefaultEnabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_RESTORE_DEFAULTS_DISABLED:
                img.load(NATRON_IMAGES_PATH"restoreDefaultDisabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_TAB_WIDGET_LAYOUT_BUTTON:
                img.load(NATRON_IMAGES_PATH"layout.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_TAB_WIDGET_SPLIT_HORIZONTALLY:
                img.load(NATRON_IMAGES_PATH"splitHorizontally.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_TAB_WIDGET_SPLIT_VERTICALLY:
                img.load(NATRON_IMAGES_PATH"splitVertically.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_VIEWER_CENTER:
                img.load(NATRON_IMAGES_PATH"centerViewer.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_VIEWER_CLIP_TO_PROJECT_ENABLED:
                img.load(NATRON_IMAGES_PATH"cliptoprojectEnabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_VIEWER_CLIP_TO_PROJECT_DISABLED:
                img.load(NATRON_IMAGES_PATH"cliptoprojectDisabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_VIEWER_REFRESH:
                img.load(NATRON_IMAGES_PATH"refresh.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_VIEWER_ROI_ENABLED:
                img.load(NATRON_IMAGES_PATH"viewer_roiEnabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_VIEWER_ROI_DISABLED:
                img.load(NATRON_IMAGES_PATH"viewer_roiDisabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_VIEWER_RENDER_SCALE:
                img.load(NATRON_IMAGES_PATH"renderScale.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_VIEWER_RENDER_SCALE_CHECKED:
                img.load(NATRON_IMAGES_PATH"renderScale_checked.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_OPEN_FILE:
                img.load(NATRON_IMAGES_PATH"open-file.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_COLORWHEEL:
                img.load(NATRON_IMAGES_PATH"colorwheel.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_COLOR_PICKER:
                img.load(NATRON_IMAGES_PATH"color_picker.png");
                *pix = QPixmap::fromImage(img);
                break;
                
                
            case NATRON_PIXMAP_IO_GROUPING:
                img.load(NATRON_IMAGES_PATH"GroupingIcons/Set" + iconSetStr + "/image_grouping_" + iconSetStr + ".png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_3D_GROUPING:
                img.load(NATRON_IMAGES_PATH"GroupingIcons/Set" + iconSetStr + "/3D_grouping_" + iconSetStr + ".png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_CHANNEL_GROUPING:
                img.load(NATRON_IMAGES_PATH"GroupingIcons/Set" + iconSetStr + "/channel_grouping_" + iconSetStr + ".png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MERGE_GROUPING:
                img.load(NATRON_IMAGES_PATH"GroupingIcons/Set" + iconSetStr + "/merge_grouping_" + iconSetStr + ".png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_COLOR_GROUPING:
                img.load(NATRON_IMAGES_PATH"GroupingIcons/Set" + iconSetStr + "/color_grouping_" + iconSetStr + ".png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_TRANSFORM_GROUPING:
                img.load(NATRON_IMAGES_PATH"GroupingIcons/Set" + iconSetStr + "/transform_grouping_" + iconSetStr + ".png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_DEEP_GROUPING:
                img.load(NATRON_IMAGES_PATH"GroupingIcons/Set" + iconSetStr + "/deep_grouping_" + iconSetStr + ".png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_FILTER_GROUPING:
                img.load(NATRON_IMAGES_PATH"GroupingIcons/Set" + iconSetStr + "/filter_grouping_" + iconSetStr + ".png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MULTIVIEW_GROUPING:
                img.load(NATRON_IMAGES_PATH"GroupingIcons/Set" + iconSetStr + "/multiview_grouping_" + iconSetStr + ".png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MISC_GROUPING:
                img.load(NATRON_IMAGES_PATH"GroupingIcons/Set" + iconSetStr + "/misc_grouping_" + iconSetStr + ".png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_OTHER_PLUGINS:
                img.load(NATRON_IMAGES_PATH"GroupingIcons/Set" + iconSetStr + "/other_grouping_" + iconSetStr + ".png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_TOOLSETS_GROUPING:
                img.load(NATRON_IMAGES_PATH"GroupingIcons/Set" + iconSetStr + "/toolsets_grouping_" + iconSetStr + ".png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_KEYER_GROUPING:
                img.load(NATRON_IMAGES_PATH"GroupingIcons/Set" + iconSetStr + "/keyer_grouping_" + iconSetStr + ".png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_TIME_GROUPING:
                img.load(NATRON_IMAGES_PATH"GroupingIcons/Set" + iconSetStr + "/time_grouping_" + iconSetStr + ".png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_PAINT_GROUPING:
                img.load(NATRON_IMAGES_PATH"GroupingIcons/Set" + iconSetStr + "/paint_grouping_" + iconSetStr + ".png");
                *pix = QPixmap::fromImage(img);
                break;
                
               
            case NATRON_PIXMAP_OPEN_EFFECTS_GROUPING:
                img.load(NATRON_IMAGES_PATH"openeffects.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_COMBOBOX:
                img.load(NATRON_IMAGES_PATH"combobox.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_COMBOBOX_PRESSED:
                img.load(NATRON_IMAGES_PATH"pressed_combobox.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_READ_IMAGE:
                img.load(NATRON_IMAGES_PATH"readImage.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_WRITE_IMAGE:
                img.load(NATRON_IMAGES_PATH"writeImage.png");
                *pix = QPixmap::fromImage(img);
                break;

            case NATRON_PIXMAP_APP_ICON:
                img.load(NATRON_APPLICATION_ICON_PATH);
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_INVERTED:
                img.load(NATRON_IMAGES_PATH"inverted.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_UNINVERTED:
                img.load(NATRON_IMAGES_PATH"uninverted.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_VISIBLE:
                img.load(NATRON_IMAGES_PATH"visible.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_UNVISIBLE:
                img.load(NATRON_IMAGES_PATH"unvisible.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_LOCKED:
                img.load(NATRON_IMAGES_PATH"locked.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_UNLOCKED:
                img.load(NATRON_IMAGES_PATH"unlocked.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_LAYER:
                img.load(NATRON_IMAGES_PATH"layer.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_BEZIER:
                img.load(NATRON_IMAGES_PATH"bezier.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_CURVE:
                img.load(NATRON_IMAGES_PATH"curve.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_BEZIER_32:
                img.load(NATRON_IMAGES_PATH"bezier32.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_ELLIPSE:
                img.load(NATRON_IMAGES_PATH"ellipse.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_RECTANGLE:
                img.load(NATRON_IMAGES_PATH"rectangle.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_ADD_POINTS:
                img.load(NATRON_IMAGES_PATH"addPoints.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_REMOVE_POINTS:
                img.load(NATRON_IMAGES_PATH"removePoints.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_REMOVE_FEATHER:
                img.load(NATRON_IMAGES_PATH"removeFeather.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_CUSP_POINTS:
                img.load(NATRON_IMAGES_PATH"cuspPoints.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_SMOOTH_POINTS:
                img.load(NATRON_IMAGES_PATH"smoothPoints.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_OPEN_CLOSE_CURVE:
                img.load(NATRON_IMAGES_PATH"openCloseCurve.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_SELECT_ALL:
                img.load(NATRON_IMAGES_PATH"cursor.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_SELECT_POINTS:
                img.load(NATRON_IMAGES_PATH"selectPoints.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_SELECT_FEATHER:
                img.load(NATRON_IMAGES_PATH"selectFeather.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_SELECT_CURVES:
                img.load(NATRON_IMAGES_PATH"selectCurves.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_AUTO_KEYING_ENABLED:
                img.load(NATRON_IMAGES_PATH"autoKeyingEnabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_AUTO_KEYING_DISABLED:
                img.load(NATRON_IMAGES_PATH"autoKeyingDisabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_STICKY_SELECTION_ENABLED:
                img.load(NATRON_IMAGES_PATH"stickySelectionEnabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_STICKY_SELECTION_DISABLED:
                img.load(NATRON_IMAGES_PATH"stickySelectionDisabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_FEATHER_LINK_ENABLED:
                img.load(NATRON_IMAGES_PATH"featherLinkEnabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_FEATHER_LINK_DISABLED:
                img.load(NATRON_IMAGES_PATH"featherLinkDisabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_FEATHER_VISIBLE:
                img.load(NATRON_IMAGES_PATH"featherEnabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_FEATHER_UNVISIBLE:
                img.load(NATRON_IMAGES_PATH"featherDisabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_RIPPLE_EDIT_ENABLED:
                img.load(NATRON_IMAGES_PATH"rippleEditEnabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_RIPPLE_EDIT_DISABLED:
                img.load(NATRON_IMAGES_PATH"rippleEditDisabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_BOLD_CHECKED:
                img.load(NATRON_IMAGES_PATH"bold_checked.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_BOLD_UNCHECKED:
                img.load(NATRON_IMAGES_PATH"bold_unchecked.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_ITALIC_UNCHECKED:
                img.load(NATRON_IMAGES_PATH"italic_unchecked.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_ITALIC_CHECKED:
                img.load(NATRON_IMAGES_PATH"italic_checked.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_CLEAR_ALL_ANIMATION:
                img.load(NATRON_IMAGES_PATH"clearAnimation.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_CLEAR_BACKWARD_ANIMATION:
                img.load(NATRON_IMAGES_PATH"clearAnimationBw.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_CLEAR_FORWARD_ANIMATION:
                img.load(NATRON_IMAGES_PATH"clearAnimationFw.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_UPDATE_VIEWER_ENABLED:
                img.load(NATRON_IMAGES_PATH"updateViewerEnabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_UPDATE_VIEWER_DISABLED:
                img.load(NATRON_IMAGES_PATH"updateViewerDisabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            default:
                assert(!"Missing image.");
        }
        QPixmapCache::insert(QString::number(e),*pix);
    }
}

const std::vector<PluginGroupNode*>& GuiApplicationManager::getPluginsToolButtons() const  { return _imp->_toolButtons; }

const QCursor& GuiApplicationManager::getColorPickerCursor() const  { return *(_imp->_colorPickerCursor); }


void GuiApplicationManager::initGui() {
    
    /*Display a splashscreen while we wait for the engine to load*/
    QString filename(NATRON_IMAGES_PATH"splashscreen.png");
    _imp->_splashScreen = new SplashScreen(filename);
    QCoreApplication::processEvents();
    
    QPixmap appIcPixmap;
    appPTR->getIcon(Natron::NATRON_PIXMAP_APP_ICON, &appIcPixmap);
    QIcon appIc(appIcPixmap);
    qApp->setWindowIcon(appIc);
    //load custom fonts
    QString fontResource = QString(":/Resources/Fonts/%1.ttf");
    
    
    QStringList fontFilenames;
    fontFilenames << fontResource.arg("DroidSans");
    fontFilenames << fontResource.arg("DroidSans-Bold");
    
    foreach(QString fontFilename, fontFilenames)
    {
        _imp->_splashScreen->updateText("Loading font " + fontFilename);
        //qDebug() << "attempting to load" << fontFilename;
        int fontID = QFontDatabase::addApplicationFont(fontFilename);
        qDebug() << "fontID=" << fontID << "families=" << QFontDatabase::applicationFontFamilies(fontID);
    }
    
    
    _imp->createColorPickerCursor();
    _imp->_knobsClipBoard->isEmpty = true;
    
}

void GuiApplicationManager::onPluginLoaded(const QStringList& groups,
                          const QString& pluginID,
                          const QString& pluginLabel,
                          const QString& pluginIconPath,
                          const QString& groupIconPath) {
    PluginGroupNode* parent = NULL;
    for(int i = 0; i < groups.size();++i){
        PluginGroupNode* child = findPluginToolButtonOrCreate(groups.at(i),groups.at(i),groupIconPath);
        if (parent && parent != child) {
            parent->tryAddChild(child);
            child->setParent(parent);
        }
        parent = child;
        
    }
    PluginGroupNode* lastChild = findPluginToolButtonOrCreate(pluginID,pluginLabel,pluginIconPath);
    if (parent && parent != lastChild) {
        parent->tryAddChild(lastChild);
        lastChild->setParent(parent);
    }
    
    //_toolButtons.push_back(new PluginToolButton(groups,pluginName,pluginIconPath,groupIconPath));
}

PluginGroupNode* GuiApplicationManager::findPluginToolButtonOrCreate(const QString& pluginID,const QString& name,const QString& iconPath) {
    for(U32 i = 0 ; i < _imp->_toolButtons.size();++i){
        if(_imp->_toolButtons[i]->getID() == pluginID)
            return _imp->_toolButtons[i];
    }
    PluginGroupNode* ret = new PluginGroupNode(pluginID,name,iconPath);
    _imp->_toolButtons.push_back(ret);
    return ret;
}

void GuiApplicationManagerPrivate::createColorPickerCursor(){
    QPixmap pickerPix;
    appPTR->getIcon(Natron::NATRON_PIXMAP_COLOR_PICKER, &pickerPix);
    pickerPix = pickerPix.scaled(16, 16);
    pickerPix.setMask(pickerPix.createHeuristicMask());
    _colorPickerCursor = new QCursor(pickerPix,0,pickerPix.height());
}


void GuiApplicationManager::hideSplashScreen() {
    if(_imp->_splashScreen) {
        _imp->_splashScreen->hide();
        delete _imp->_splashScreen;
        _imp->_splashScreen = 0;
    }
}


bool GuiApplicationManager::isClipBoardEmpty() const{
    return  _imp->_knobsClipBoard->isEmpty;
}


void GuiApplicationManager::setKnobClipBoard(bool copyAnimation,int dimension,
                      const std::list<Variant>& values,
                      const std::list<boost::shared_ptr<Curve> >& animation,
                      const std::map<int,std::string>& stringAnimation,
                      const std::list<boost::shared_ptr<Curve> >& parametricCurves)
{
    _imp->_knobsClipBoard->copyAnimation = copyAnimation;
    _imp->_knobsClipBoard->dimension = dimension;
    _imp->_knobsClipBoard->isEmpty = false;
    _imp->_knobsClipBoard->values = values;
    _imp->_knobsClipBoard->curves = animation;
    _imp->_knobsClipBoard->stringAnimation = stringAnimation;
    _imp->_knobsClipBoard->parametricCurves = parametricCurves;
    
}

void GuiApplicationManager::getKnobClipBoard(bool* copyAnimation,int* dimension,
                      std::list<Variant>* values,
                      std::list<boost::shared_ptr<Curve> >* animation,
                      std::map<int,std::string>* stringAnimation,
                      std::list<boost::shared_ptr<Curve> >* parametricCurves) const
{
    *copyAnimation = _imp->_knobsClipBoard->copyAnimation;
    *dimension = _imp->_knobsClipBoard->dimension;
    *values = _imp->_knobsClipBoard->values;
    *animation = _imp->_knobsClipBoard->curves;
    *stringAnimation = _imp->_knobsClipBoard->stringAnimation;
    *parametricCurves = _imp->_knobsClipBoard->parametricCurves;
    
}

void GuiApplicationManager::updateAllRecentFileMenus() {
    
    const std::map<int,AppInstanceRef>& instances = getAppInstances();
    for (std::map<int,AppInstanceRef>::const_iterator it = instances.begin(); it!= instances.end(); ++it) {
        dynamic_cast<GuiAppInstance*>(it->second.app)->getGui()->updateRecentFileActions();
    }
}

void GuiApplicationManager::setLoadingStatus(const QString& str) {
    if (isLoaded()) {
        return;
    }
    if (_imp->_splashScreen) {
        _imp->_splashScreen->updateText(str);
    }

}

void GuiApplicationManager::loadBuiltinNodePlugins(std::vector<Natron::Plugin*>* plugins,
                            std::map<std::string,std::vector<std::string> >* readersMap,
                            std::map<std::string,std::vector<std::string> >* writersMap)
{
    ////Use ReadQt and WriteQt only for debug versions of Natron.
    // these  are built-in nodes
    QStringList grouping;
    grouping.push_back(PLUGIN_GROUP_IMAGE); // Readers, Writers, and Generators are in the "Image" group in Nuke
    
# ifdef DEBUG
    {
        boost::shared_ptr<QtReader> reader(dynamic_cast<QtReader*>(QtReader::BuildEffect(boost::shared_ptr<Natron::Node>())));
        assert(reader);
        std::map<std::string,void*> readerFunctions;
        readerFunctions.insert(std::make_pair("BuildEffect", (void*)&QtReader::BuildEffect));
        LibraryBinary *readerPlugin = new LibraryBinary(readerFunctions);
        assert(readerPlugin);
        Natron::Plugin* plugin = new Natron::Plugin(readerPlugin,reader->getPluginID().c_str(),reader->getPluginLabel().c_str(),
                                                    "",(QMutex*)NULL,reader->getMajorVersion(),reader->getMinorVersion());
        plugins->push_back(plugin);
        onPluginLoaded(grouping,reader->getPluginID().c_str(),reader->getPluginLabel().c_str(), "","");
        
        std::vector<std::string> extensions = reader->supportedFileFormats();
        for(U32 k = 0; k < extensions.size();++k){
            std::map<std::string,std::vector<std::string> >::iterator it;
            it = readersMap->find(extensions[k]);
            
            if(it != readersMap->end()){
                it->second.push_back(reader->getPluginID());
            }else{
                std::vector<std::string> newVec(1);
                newVec[0] = reader->getPluginID();
                readersMap->insert(std::make_pair(extensions[k], newVec));
            }
        }
    }
    
    {
        boost::shared_ptr<QtWriter> writer(dynamic_cast<QtWriter*>(QtWriter::BuildEffect(boost::shared_ptr<Natron::Node>())));
        assert(writer);
        std::map<std::string,void*> writerFunctions;
        writerFunctions.insert(std::make_pair("BuildEffect", (void*)&QtWriter::BuildEffect));
        LibraryBinary *writerPlugin = new LibraryBinary(writerFunctions);
        assert(writerPlugin);
        Natron::Plugin* plugin = new Natron::Plugin(writerPlugin,writer->getPluginID().c_str(),writer->getPluginLabel().c_str(),
                                                    "",(QMutex*)NULL,writer->getMajorVersion(),writer->getMinorVersion());
        plugins->push_back(plugin);
        onPluginLoaded(grouping,writer->getPluginID().c_str(),writer->getPluginLabel().c_str(), "","");
        
        std::vector<std::string> extensions = writer->supportedFileFormats();
        for(U32 k = 0; k < extensions.size();++k){
            std::map<std::string,std::vector<std::string> >::iterator it;
            it = writersMap->find(extensions[k]);
            
            if(it != writersMap->end()){
                it->second.push_back(writer->getPluginID());
            }else{
                std::vector<std::string> newVec(1);
                newVec[0] = writer->getPluginID();
                writersMap->insert(std::make_pair(extensions[k], newVec));
            }
        }
        
    }
# else // !DEBUG
    (void)readersMap;
    (void)writersMap;
# endif // DEBUG
    
    {
        boost::shared_ptr<EffectInstance> viewer(ViewerInstance::BuildEffect(boost::shared_ptr<Natron::Node>()));
        assert(viewer);
        std::map<std::string,void*> viewerFunctions;
        viewerFunctions.insert(std::make_pair("BuildEffect", (void*)&ViewerInstance::BuildEffect));
        LibraryBinary *viewerPlugin = new LibraryBinary(viewerFunctions);
        assert(viewerPlugin);
        Natron::Plugin* plugin = new Natron::Plugin(viewerPlugin,viewer->getPluginID().c_str(),viewer->getPluginLabel().c_str(),
                                                    "",(QMutex*)NULL,viewer->getMajorVersion(),viewer->getMinorVersion());
        plugins->push_back(plugin);
        onPluginLoaded(grouping,viewer->getPluginID().c_str(),viewer->getPluginLabel().c_str(), "","");
    }
    
    {
        QString label(NATRON_BACKDROP_NODE_NAME);
        Natron::Plugin* plugin = new Natron::Plugin(NULL,label,label,"",NULL,1,0);
        plugins->push_back(plugin);
        QStringList backdropGrouping(PLUGIN_GROUP_OTHER);
        onPluginLoaded(backdropGrouping, label, label, "", "");
    }
    
    ///Also load the plug-ins of the AppManager
    AppManager::loadBuiltinNodePlugins(plugins, readersMap, writersMap);
}

AppInstance* GuiApplicationManager::makeNewInstance(int appID) const {
    return new GuiAppInstance(appID);
}

KnobGui* GuiApplicationManager::createGuiForKnob(boost::shared_ptr<KnobI> knob, DockablePanel *container) const {
    return _imp->_knobGuiFactory->createGuiForKnob(knob,container);
}


void GuiApplicationManager::registerGuiMetaTypes() const {
    qRegisterMetaType<CurveWidget*>();
}

class Application : public QApplication
{
    GuiApplicationManager* _app;
    
public:
    
    Application(GuiApplicationManager* app,int &argc,char** argv) ///< qapplication needs to be subclasses with reference otherwise lots of crashes will happen.
    : QApplication(argc,argv)
    , _app(app)
    {
    }
    
protected:
    
    bool event(QEvent* e);

};

bool Application::event(QEvent* e)
{
    switch (e->type()) {
        case QEvent::FileOpen:
        {
            assert(_app);
            QString file =  static_cast<QFileOpenEvent*>(e)->file();
#ifdef Q_OS_UNIX
            if (!file.isEmpty()) {
                file = AppManager::qt_tildeExpansion(file);
            }
#endif
            _app->setFileToOpen(file);
        }   return true;
        default:
            return QApplication::event(e);
    }
}


void GuiApplicationManager::initializeQApp(int &argc,char** argv) {
    QApplication* app = new Application(this,argc, argv);
	app->setQuitOnLastWindowClosed(true);
    Q_INIT_RESOURCE(GuiResources);
    app->setFont(QFont(NATRON_FONT, NATRON_FONT_SIZE_11));

    ///Register all the shortcuts.
    populateShortcuts();
    
    ///Restore user shortcuts
    loadShortcuts();
}

void GuiApplicationManager::setUndoRedoStackLimit(int limit)
{
    
    const std::map<int,AppInstanceRef>& apps = getAppInstances();
    for (std::map<int,AppInstanceRef>::const_iterator it = apps.begin(); it != apps.end(); ++it) {
        GuiAppInstance* guiApp = dynamic_cast<GuiAppInstance*>(it->second.app);
        if (guiApp) {
            guiApp->setUndoRedoStackLimit(limit);
        }
    }
}

void GuiApplicationManager::debugImage(const Natron::Image* image,const QString& filename) const
{
    Gui::debugImage(image,filename);
}

void GuiApplicationManager::setFileToOpen(const QString& str)
{
    _imp->_openFileRequest = str;
    if (isLoaded() && !_imp->_openFileRequest.isEmpty()) {
        handleOpenFileRequest();
    }
}

void GuiApplicationManager::handleOpenFileRequest()
{
    std::string fileUnPathed = _imp->_openFileRequest.toStdString();
    _imp->_openFileRequest.clear();
    std::string path = SequenceParsing::removePath(fileUnPathed);
    AppInstance* mainApp = getAppInstance(0);
    if (mainApp && mainApp->getProject()->isGraphWorthLess()) {
        mainApp->getProject()->loadProject(path.c_str(), fileUnPathed.c_str());
    } else {
        ///remove autosaves otherwise the new instance might try to load an autosave
        Project::removeAutoSaves();
        AppInstance* newApp = newAppInstance();
        newApp->getProject()->loadProject(path.c_str(), fileUnPathed.c_str());
    }
}

void GuiApplicationManager::onLoadCompleted()
{
    if (!_imp->_openFileRequest.isEmpty()) {
        handleOpenFileRequest();
    }
}

void GuiApplicationManager::exitApp()
{
    ///make a copy of the map because it will be modified when closing projects
    std::map<int,AppInstanceRef> instances = getAppInstances();
    for (std::map<int,AppInstanceRef>::const_iterator it = instances.begin(); it!=instances.end(); ++it) {
        GuiAppInstance* app = dynamic_cast<GuiAppInstance*>(it->second.app);
        if (!app->getGui()->closeInstance()) {
            return;
        }
    }
}

static bool matchesModifers(const Qt::KeyboardModifiers& lhs,const Qt::KeyboardModifiers& rhs,Qt::Key key)
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
    if (lhs == (Qt::ShiftModifier | Qt::AltModifier) && rhs == Qt::AltModifier && isDigit) {
        return true;
    }
    
    return false;
}

static bool matchesKey(Qt::Key lhs,Qt::Key rhs)
{
    if (lhs == rhs) {
        return true;
    }
    ///special case for the backspace and delete keys that mean the same thing generally
    else if ((lhs == Qt::Key_Backspace && rhs == Qt::Key_Delete) ||
             (lhs == Qt::Key_Delete && rhs == Qt::Key_Backspace)) {
        return true;
    }
    ///special case for the return and enter keys that mean the same thing generally
    else if ((lhs == Qt::Key_Return && rhs == Qt::Key_Enter) ||
             (lhs == Qt::Key_Enter && rhs == Qt::Key_Return)) {
        return true;
    }
    return false;
}

bool GuiApplicationManager::matchesKeybind(const QString& group,const QString& actionID,
                                           const Qt::KeyboardModifiers& modifiers,int symbol) const
{
    AppShortcuts::const_iterator it = _imp->_actionShortcuts.find(group);
    if (it == _imp->_actionShortcuts.end()) {
        // we didn't find the group
        return false;
    }
    
    GroupShortcuts::const_iterator it2 = it->second.find(actionID);
    if (it2 == it->second.end()) {
        // we didn't find the action
        return false;
    }
    
    const KeyBoundAction* keybind = dynamic_cast<const KeyBoundAction*>(it2->second);
    if (!keybind) {
        return false;
    }
    
    // the following macro only tests the Control, Alt, and Shift modifiers, and discards the others
    Qt::KeyboardModifiers onlyCAS = modifiers & (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier);
    
    if (matchesModifers(onlyCAS,keybind->modifiers,(Qt::Key)symbol)) {
        // modifiers are equal, now test symbol
        return matchesKey((Qt::Key)symbol, keybind->currentShortcut);
    }
    return false;
}


bool GuiApplicationManager::matchesMouseShortcut(const QString& group,const QString& actionID,
                                                 const Qt::KeyboardModifiers& modifiers,int button) const
{
    AppShortcuts::const_iterator it = _imp->_actionShortcuts.find(group);
    if (it == _imp->_actionShortcuts.end()) {
        // we didn't find the group
        return false;
    }
    
    GroupShortcuts::const_iterator it2 = it->second.find(actionID);
    if (it2 == it->second.end()) {
        // we didn't find the action
        return false;
    }
    
    const MouseAction* mAction = dynamic_cast<const MouseAction*>(it2->second);
    if (!mAction) {
        return false;
    }
    
    // the following macro only tests the Control, Alt, and Shift modifiers, and discards the others
    Qt::KeyboardModifiers onlyCAS = modifiers & (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier);
    
    if (onlyCAS == mAction->modifiers) {
        // modifiers are equal, now test symbol
        if ((Qt::MouseButton)button == mAction->button) {
            return true;
        }
    }
    ///Alt + left click is the same as middle button
    else if ((onlyCAS == Qt::AltModifier && (Qt::MouseButton)button == Qt::LeftButton && mAction->button == Qt::MiddleButton &&
               mAction->modifiers == Qt::NoModifier) ||
               (onlyCAS == Qt::NoModifier && (Qt::MouseButton)button == Qt::MiddleButton && mAction->button == Qt::LeftButton &&
                mAction->modifiers == Qt::AltModifier)) {
                   return true;
               }
        
    return false;

}


void GuiApplicationManager::saveShortcuts() const
{
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);
    for (AppShortcuts::const_iterator it = _imp->_actionShortcuts.begin(); it!=_imp->_actionShortcuts.end(); ++it) {
        settings.beginGroup(it->first);
        for (GroupShortcuts::const_iterator it2 = it->second.begin(); it2!=it->second.end(); ++it2) {
            const MouseAction* mAction = dynamic_cast<const MouseAction*>(it2->second);
            const KeyBoundAction* kAction = dynamic_cast<const KeyBoundAction*>(it2->second);
            settings.setValue(it2->first + "_Modifiers", (int)it2->second->modifiers);
            if (mAction) {
                settings.setValue(it2->first + "_Button", (int)mAction->button);
            } else if (mAction) {
                settings.setValue(it2->first + "_Symbol", (int)kAction->currentShortcut);
            }
        }
        settings.endGroup();
    }
}

void GuiApplicationManager::loadShortcuts()
{
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);
    for (AppShortcuts::iterator it = _imp->_actionShortcuts.begin(); it!=_imp->_actionShortcuts.end(); ++it) {
        settings.beginGroup(it->first);
        for (GroupShortcuts::iterator it2 = it->second.begin(); it2!=it->second.end(); ++it2) {
            MouseAction* mAction = dynamic_cast<MouseAction*>(it2->second);
            KeyBoundAction* kAction = dynamic_cast<KeyBoundAction*>(it2->second);
            
            if (settings.contains(it2->first + "_Modifiers")) {
                it2->second->modifiers = Qt::KeyboardModifiers(settings.value(it2->first + "_Modifiers").toInt());
            }
            if (mAction && settings.contains(it2->first + "_Button")) {
                mAction->button = (Qt::MouseButton)settings.value(it2->first + "_Button").toInt();
            }
            if (kAction && settings.contains(it2->first + "_Symbol")) {
                kAction->currentShortcut = (Qt::Key)settings.value(it2->first + "_Symbol").toInt();
            }
        }
        settings.endGroup();
    }

}

void GuiApplicationManager::restoreDefaultShortcuts()
{
    for (AppShortcuts::iterator it = _imp->_actionShortcuts.begin(); it!=_imp->_actionShortcuts.end(); ++it) {
        for (GroupShortcuts::iterator it2 = it->second.begin(); it2!=it->second.end(); ++it2) {
            KeyBoundAction* kAction = dynamic_cast<KeyBoundAction*>(it2->second);
            it2->second->modifiers = it2->second->defaultModifiers;
            if (kAction) {
                kAction->currentShortcut = kAction->defaultShortcut;
            }
            ///mouse actions cannot have their button changed
        }
    }
}



void GuiApplicationManager::populateShortcuts()
{
    ///General
    registerStandardKeybind(kShortcutGroupGlobal, kShortcutIDActionNewProject, kShortcutDescActionNewProject,QKeySequence::New);
    registerStandardKeybind(kShortcutGroupGlobal, kShortcutIDActionOpenProject, kShortcutDescActionOpenProject,QKeySequence::Open);
    registerStandardKeybind(kShortcutGroupGlobal, kShortcutIDActionSaveProject, kShortcutDescActionSaveProject,QKeySequence::Save);
    registerStandardKeybind(kShortcutGroupGlobal, kShortcutIDActionSaveAsProject, kShortcutDescActionSaveAsProject,QKeySequence::SaveAs);
    registerStandardKeybind(kShortcutGroupGlobal, kShortcutIDActionCloseProject, kShortcutDescActionCloseProject,QKeySequence::Close);
    registerStandardKeybind(kShortcutGroupGlobal, kShortcutIDActionPreferences, kShortcutDescActionPreferences,QKeySequence::Preferences);
    registerStandardKeybind(kShortcutGroupGlobal, kShortcutIDActionQuit, kShortcutDescActionQuit,QKeySequence::Quit);
    
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionProjectSettings, kShortcutDescActionProjectSettings, Qt::NoModifier, Qt::Key_S);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionNewViewer, kShortcutDescActionNewViewer, Qt::ControlModifier, Qt::Key_I);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionFullscreen, kShortcutDescActionFullscreen, Qt::ControlModifier | Qt::MetaModifier, Qt::Key_F);
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
    
    ///Viewer
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionLuminance, kShortcutDescActionLuminance, Qt::NoModifier, Qt::Key_Y);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionR, kShortcutDescActionR, Qt::NoModifier, Qt::Key_R);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionG, kShortcutDescActionG, Qt::NoModifier, Qt::Key_G);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionB, kShortcutDescActionB, Qt::NoModifier, Qt::Key_B);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionA, kShortcutDescActionA, Qt::NoModifier, Qt::Key_A);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionFitViewer, kShortcutDescActionFitViewer, Qt::NoModifier, Qt::Key_F);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionClipEnabled, kShortcutDescActionClipEnabled, Qt::ShiftModifier, Qt::Key_C);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionRefresh, kShortcutDescActionRefresh, Qt::NoModifier, Qt::Key_U);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionROIEnabled, kShortcutDescActionROIEnabled, Qt::ShiftModifier, Qt::Key_W);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionProxyEnabled, kShortcutDescActionProxyEnabled, Qt::ControlModifier, Qt::Key_P);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionProxyLevel2, kShortcutDescActionProxyLevel2, Qt::AltModifier, Qt::Key_1);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionProxyLevel4, kShortcutDescActionProxyLevel4, Qt::AltModifier, Qt::Key_2);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionProxyLevel8, kShortcutDescActionProxyLevel8, Qt::AltModifier, Qt::Key_3);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionProxyLevel16, kShortcutDescActionProxyLevel16, Qt::AltModifier, Qt::Key_4);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionProxyLevel32, kShortcutDescActionProxyLevel32, Qt::AltModifier, Qt::Key_5);
    
    registerMouseShortcut(kShortcutGroupViewer, kShortcutIDMouseZoom, kShortcutDescMouseZoom, Qt::NoModifier, Qt::MiddleButton);
    registerMouseShortcut(kShortcutGroupViewer, kShortcutIDMousePan, kShortcutDescMousePan, Qt::AltModifier, Qt::LeftButton);
    registerMouseShortcut(kShortcutGroupViewer, kShortcutIDMousePickColor, kShortcutDescMousePickColor, Qt::ControlModifier, Qt::LeftButton);
    registerMouseShortcut(kShortcutGroupViewer, kShortcutIDMouseRectanglePick, kShortcutDescMouseRectanglePick, Qt::ControlModifier | Qt::ShiftModifier, Qt::LeftButton);
    
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
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoTransformModifier, kShortcutDescActionRotoTransformModifier, Qt::NoModifier,Qt::Key_Control);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoSelectAll, kShortcutDescActionRotoSelectAll, Qt::ControlModifier, Qt::Key_A);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoSelectionTool, kShortcutDescActionRotoSelectionTool, Qt::NoModifier, Qt::Key_Q);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoAddTool, kShortcutDescActionRotoAddTool, Qt::NoModifier, Qt::Key_D);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoEditTool, kShortcutDescActionRotoEditTool, Qt::NoModifier, Qt::Key_V);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoNudgeLeft, kShortcutDescActionRotoNudgeLeft, Qt::NoModifier, Qt::Key_Left);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoNudgeBottom, kShortcutDescActionRotoNudgeBottom, Qt::NoModifier, Qt::Key_Down);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoNudgeRight, kShortcutDescActionRotoNudgeRight, Qt::NoModifier, Qt::Key_Right);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoNudgeTop, kShortcutDescActionRotoNudgeTop, Qt::NoModifier, Qt::Key_Up);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoSmooth, kShortcutDescActionRotoSmooth, Qt::NoModifier, Qt::Key_Z);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoCuspBezier, kShortcutDescActionRotoCuspBezier, Qt::ShiftModifier, Qt::Key_Z);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoRemoveFeather, kShortcutDescActionRotoRemoveFeather, Qt::ShiftModifier, Qt::Key_E);
    
    ///Tracking
    registerKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingSelectAll, kShortcutDescActionTrackingSelectAll, Qt::ControlModifier, Qt::Key_A);
    registerKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingDelete, kShortcutDescActionTrackingDelete, Qt::NoModifier, Qt::Key_Backspace);
    registerKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingBackward, kShortcutDescActionTrackingBackward, Qt::NoModifier, Qt::Key_Z);
    registerKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingPrevious, kShortcutDescActionTrackingPrevious, Qt::NoModifier, Qt::Key_X);
    registerKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingNext, kShortcutDescActionTrackingnext, Qt::NoModifier, Qt::Key_C);
    registerKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingForward, kShortcutDescActionTrackingForward, Qt::NoModifier, Qt::Key_V);
    
    ///Nodegraph
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphCreateReader, kShortcutDescActionGraphCreateReader, Qt::NoModifier, Qt::Key_R);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphCreateWriter, kShortcutDescActionGraphCreateWriter, Qt::NoModifier, Qt::Key_W);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphCreateTransform, kShortcutDescActionGraphCreateTransform, Qt::NoModifier, Qt::Key_T);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphCreateMerge, kShortcutDescActionGraphCreateMerge, Qt::NoModifier, Qt::Key_M);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphCreateColorCorrect, kShortcutDescActionGraphCreateColorCorrect, Qt::NoModifier, Qt::Key_C);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphCreateGrade, kShortcutDescActionGraphCreateGrade, Qt::NoModifier, Qt::Key_G);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphCreateRoto, kShortcutDescActionGraphCreateRoto, Qt::NoModifier, Qt::Key_O);
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
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphSwitchInputs, kShortcutDescActionGraphSwitchInputs, Qt::ShiftModifier, Qt::Key_X);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphCopy, kShortcutDescActionGraphCopy, Qt::ControlModifier, Qt::Key_C);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphPaste, kShortcutDescActionGraphPaste, Qt::ControlModifier, Qt::Key_V);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphCut, kShortcutDescActionGraphCut, Qt::ControlModifier, Qt::Key_X);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphClone, kShortcutDescActionGraphClone, Qt::AltModifier, Qt::Key_K);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphDeclone, kShortcutDescActionGraphDeclone, Qt::AltModifier | Qt::ShiftModifier, Qt::Key_K);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphDuplicate, kShortcutDescActionGraphDuplicate, Qt::AltModifier, Qt::Key_C);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphForcePreview, kShortcutDescActionGraphForcePreview, Qt::NoModifier, Qt::Key_P);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphFrameNodes, kShortcutDescActionGraphFrameNodes, Qt::NoModifier, Qt::Key_F);
 
    registerMouseShortcut(kShortcutGroupNodegraph, kShortcutIDMouseZoom, kShortcutDescMouseZoom, Qt::NoModifier, Qt::MiddleButton);
    registerMouseShortcut(kShortcutGroupNodegraph, kShortcutIDMousePan, kShortcutDescMousePan, Qt::AltModifier, Qt::LeftButton);
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
    
    registerMouseShortcut(kShortcutGroupCurveEditor, kShortcutIDMouseZoom, kShortcutDescMouseZoom, Qt::NoModifier, Qt::MiddleButton);
    registerMouseShortcut(kShortcutGroupCurveEditor, kShortcutIDMousePan, kShortcutDescMousePan, Qt::AltModifier, Qt::LeftButton);
    registerMouseShortcut(kShortcutGroupCurveEditor, kShortcutIDMouseZoomX, kShortcutDescMouseZoomX, Qt::ControlModifier, Qt::MiddleButton);
    registerMouseShortcut(kShortcutGroupCurveEditor, kShortcutIDMouseZoomY, kShortcutDescMouseZoomY, Qt::ControlModifier | Qt::ShiftModifier, Qt::MiddleButton);
}

void GuiApplicationManagerPrivate::addKeybind(const QString& grouping,const QString& id,
                const QString& description,
                const Qt::KeyboardModifiers& modifiers,Qt::Key symbol)
{
    KeyBoundAction* kA = new KeyBoundAction;
    kA->grouping = grouping;
    kA->description = description;
    kA->defaultModifiers = modifiers;
    kA->modifiers = modifiers;
    kA->defaultShortcut = symbol;
    kA->currentShortcut = symbol;
    AppShortcuts::iterator foundGroup = _actionShortcuts.find(grouping);
    if (foundGroup != _actionShortcuts.end()) {
        foundGroup->second.insert(std::make_pair(id, kA));
    } else {
        GroupShortcuts group;
        group.insert(std::make_pair(id, kA));
        _actionShortcuts.insert(std::make_pair(grouping, group));
    }
}

void GuiApplicationManagerPrivate::addMouseShortcut(const QString& grouping,const QString& id,
                      const QString& description,
                      const Qt::KeyboardModifiers& modifiers,Qt::MouseButton button)
{
    MouseAction* mA = new MouseAction;
    mA->grouping = grouping;
    mA->description = description;
    mA->defaultModifiers = modifiers;
    mA->modifiers = modifiers;
    mA->button = button;
    AppShortcuts::iterator foundGroup = _actionShortcuts.find(grouping);
    if (foundGroup != _actionShortcuts.end()) {
        foundGroup->second.insert(std::make_pair(id, mA));
    } else {
        GroupShortcuts group;
        group.insert(std::make_pair(id, mA));
        _actionShortcuts.insert(std::make_pair(grouping, group));
    }

}

static void extractSymbolsAndModifiers(QKeySequence::StandardKey key,Qt::KeyboardModifiers& modifiers,Qt::Key& symbol)
{
    QKeySequence seq(key);
    int count = (int)seq.count();
    for (int i = 0; i < count; ++i) {
        if (seq[i] == (int)Qt::CTRL) {
            modifiers |= Qt::ControlModifier;
        } else if (seq[i] == (int)Qt::SHIFT) {
            modifiers |= Qt::ShiftModifier;
        } else if (seq[i] == (int)Qt::ALT) {
            modifiers |= Qt::AltModifier;
        } else if (seq[i] != 0) {
            symbol = (Qt::Key)seq[i];
        }
    }
}

void GuiApplicationManagerPrivate::addStandardKeybind(const QString& grouping,const QString& id,
                        const QString& description,QKeySequence::StandardKey key)
{
    Qt::KeyboardModifiers modifiers;
    Qt::Key symbol;
    extractSymbolsAndModifiers(key, modifiers, symbol);
    KeyBoundAction* kA = new KeyBoundAction;
    kA->grouping = grouping;
    kA->editable = false;
    kA->description = description;
    kA->defaultModifiers = modifiers;
    kA->modifiers = modifiers;
    kA->defaultShortcut = symbol;
    kA->currentShortcut = symbol;
    AppShortcuts::iterator foundGroup = _actionShortcuts.find(grouping);
    if (foundGroup != _actionShortcuts.end()) {
        foundGroup->second.insert(std::make_pair(id, kA));
    } else {
        GroupShortcuts group;
        group.insert(std::make_pair(id, kA));
        _actionShortcuts.insert(std::make_pair(grouping, group));
    }

}

static QKeySequence makeKeySequence(const Qt::KeyboardModifiers& modifiers,Qt::Key key) {
    int keys = 0;
    if (modifiers.testFlag(Qt::ControlModifier)) {
        keys |= Qt::CTRL;
    }
    if (modifiers.testFlag(Qt::ShiftModifier)) {
        keys |= Qt::SHIFT;
    }
    if (modifiers.testFlag(Qt::AltModifier)) {
        keys |= Qt::ALT;
    }
    if (modifiers.testFlag(Qt::MetaModifier)) {
        keys |= Qt::META;
    }
    keys |= key;
    return QKeySequence(keys);
}

QKeySequence GuiApplicationManager::getKeySequenceForAction(const QString& group,const QString& actionID) const
{
    AppShortcuts::const_iterator foundGroup = _imp->_actionShortcuts.find(group);
    if (foundGroup != _imp->_actionShortcuts.end()) {
        GroupShortcuts::const_iterator found = foundGroup->second.find(actionID);
        if (found != foundGroup->second.end()) {
            const KeyBoundAction* ka = dynamic_cast<const KeyBoundAction*>(found->second);
            if (ka) {
                return makeKeySequence(found->second->modifiers, ka->currentShortcut);
            }
        }
    }
    return QKeySequence();
}

bool GuiApplicationManager::getModifiersAndKeyForAction(const QString& group,const QString& actionID,
                                                        Qt::KeyboardModifiers& modifiers,int& symbol) const
{
    AppShortcuts::const_iterator foundGroup = _imp->_actionShortcuts.find(group);
    if (foundGroup != _imp->_actionShortcuts.end()) {
        GroupShortcuts::const_iterator found = foundGroup->second.find(actionID);
        if (found != foundGroup->second.end()) {
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