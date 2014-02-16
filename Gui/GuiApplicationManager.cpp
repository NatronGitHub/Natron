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
#include <QPixmapCache>
#include <QBitmap>
#include <QImage>
#include <Qapplication>
#include <QFontDatabase>
#include <QIcon>

//core
#include <QDebug>


//engine
#include "Global/Enums.h"
#include "Engine/KnobSerialization.h"
#include "Engine/Plugin.h"
#include "Engine/LibraryBinary.h"
#include "Engine/ViewerInstance.h"

//gui
#include "Gui/Gui.h"
#include "Gui/SplashScreen.h"
#include "Gui/KnobGuiFactory.h"
#include "Gui/QtDecoder.h"
#include "Gui/QtEncoder.h"
#include "Gui/GuiAppInstance.h"

using namespace Natron;

struct KnobsClipBoard {
    KnobSerialization k; //< the serialized knob to copy
    bool isEmpty; //< is the clipboard empty
    bool copyAnimation; //< should we copy all the animation or not
};

struct GuiApplicationManagerPrivate {
    std::vector<PluginGroupNode*> _toolButtons;
    
    boost::scoped_ptr<KnobsClipBoard> _knobsClipBoard;
    
    boost::scoped_ptr<KnobGuiFactory> _knobGuiFactory;
    
    QCursor* _colorPickerCursor;
    
    SplashScreen* _splashScreen;
    
    GuiApplicationManagerPrivate()
    : _toolButtons()
    , _knobsClipBoard(new KnobsClipBoard)
    , _knobGuiFactory(new KnobGuiFactory())
    , _colorPickerCursor(NULL)
    , _splashScreen(NULL)
    {
    }
    
    void createColorPickerCursor();

};

GuiApplicationManager::GuiApplicationManager()
: AppManager()
, _imp(new GuiApplicationManagerPrivate)
{
}

GuiApplicationManager::~GuiApplicationManager() {
    foreach(PluginGroupNode* p,_imp->_toolButtons){
        delete p;
    }
}


void GuiApplicationManager::getIcon(Natron::PixmapEnum e,QPixmap* pix) const {
    if(!QPixmapCache::find(QString::number(e),pix)){
        QImage img;
        switch(e){
            case NATRON_PIXMAP_PLAYER_PREVIOUS:
                img.load(NATRON_IMAGES_PATH"back1.png");
                *pix = QPixmap::fromImage(img).scaled(20,20);
                break;
            case NATRON_PIXMAP_PLAYER_FIRST_FRAME:
                img.load(NATRON_IMAGES_PATH"firstFrame.png");
                *pix = QPixmap::fromImage(img).scaled(20,20);
                break;
            case NATRON_PIXMAP_PLAYER_NEXT:
                img.load(NATRON_IMAGES_PATH"forward1.png");
                *pix = QPixmap::fromImage(img).scaled(20,20);
                break;
            case NATRON_PIXMAP_PLAYER_LAST_FRAME:
                img.load(NATRON_IMAGES_PATH"lastFrame.png");
                *pix = QPixmap::fromImage(img).scaled(20,20);
                break;
            case NATRON_PIXMAP_PLAYER_NEXT_INCR:
                img.load(NATRON_IMAGES_PATH"nextIncr.png");
                *pix = QPixmap::fromImage(img).scaled(20,20);
                break;
            case NATRON_PIXMAP_PLAYER_NEXT_KEY:
                img.load(NATRON_IMAGES_PATH"nextKF.png");
                *pix = QPixmap::fromImage(img).scaled(20,20);
                break;
            case NATRON_PIXMAP_PLAYER_PREVIOUS_INCR:
                img.load(NATRON_IMAGES_PATH"previousIncr.png");
                *pix = QPixmap::fromImage(img).scaled(20,20);
                break;
            case NATRON_PIXMAP_PLAYER_PREVIOUS_KEY:
                img.load(NATRON_IMAGES_PATH"prevKF.png");
                *pix = QPixmap::fromImage(img).scaled(20,20);
                break;
            case NATRON_PIXMAP_PLAYER_REWIND:
                img.load(NATRON_IMAGES_PATH"rewind.png");
                *pix = QPixmap::fromImage(img).scaled(20,20);
                break;
            case NATRON_PIXMAP_PLAYER_PLAY:
                img.load(NATRON_IMAGES_PATH"play.png");
                *pix = QPixmap::fromImage(img).scaled(20,20);
                break;
            case NATRON_PIXMAP_PLAYER_STOP:
                img.load(NATRON_IMAGES_PATH"stop.png");
                *pix = QPixmap::fromImage(img).scaled(20,20);
                break;
            case NATRON_PIXMAP_PLAYER_LOOP_MODE:
                img.load(NATRON_IMAGES_PATH"loopmode.png");
                *pix = QPixmap::fromImage(img).scaled(20,20);
                break;
            case NATRON_PIXMAP_MAXIMIZE_WIDGET:
                img.load(NATRON_IMAGES_PATH"maximize.png");
                *pix = QPixmap::fromImage(img).scaled(25,25);
                break;
            case NATRON_PIXMAP_MINIMIZE_WIDGET:
                img.load(NATRON_IMAGES_PATH"minimize.png");
                *pix = QPixmap::fromImage(img).scaled(15,15);
                break;
            case NATRON_PIXMAP_CLOSE_WIDGET:
                img.load(NATRON_IMAGES_PATH"close.png");
                *pix = QPixmap::fromImage(img).scaled(15,15);
                break;
            case NATRON_PIXMAP_HELP_WIDGET:
                img.load(NATRON_IMAGES_PATH"help.png");
                *pix = QPixmap::fromImage(img).scaled(15,15);
                break;
            case NATRON_PIXMAP_GROUPBOX_FOLDED:
                img.load(NATRON_IMAGES_PATH"groupbox_folded.png");
                *pix = QPixmap::fromImage(img).scaled(20, 20);
                break;
            case NATRON_PIXMAP_GROUPBOX_UNFOLDED:
                img.load(NATRON_IMAGES_PATH"groupbox_unfolded.png");
                *pix = QPixmap::fromImage(img).scaled(20, 20);
                break;
            case NATRON_PIXMAP_UNDO:
                img.load(NATRON_IMAGES_PATH"undo.png");
                *pix = QPixmap::fromImage(img).scaled(15, 15);
                break;
            case NATRON_PIXMAP_REDO:
                img.load(NATRON_IMAGES_PATH"redo.png");
                *pix = QPixmap::fromImage(img).scaled(15, 15);
                break;
            case NATRON_PIXMAP_UNDO_GRAYSCALE:
                img.load(NATRON_IMAGES_PATH"undo_grayscale.png");
                *pix = QPixmap::fromImage(img).scaled(15, 15);
                break;
            case NATRON_PIXMAP_REDO_GRAYSCALE:
                img.load(NATRON_IMAGES_PATH"redo_grayscale.png");
                *pix = QPixmap::fromImage(img).scaled(15, 15);
                break;
            case NATRON_PIXMAP_TAB_WIDGET_LAYOUT_BUTTON:
                img.load(NATRON_IMAGES_PATH"layout.png");
                *pix = QPixmap::fromImage(img).scaled(12,12);
                break;
            case NATRON_PIXMAP_TAB_WIDGET_SPLIT_HORIZONTALLY:
                img.load(NATRON_IMAGES_PATH"splitHorizontally.png");
                *pix = QPixmap::fromImage(img).scaled(12,12);
                break;
            case NATRON_PIXMAP_TAB_WIDGET_SPLIT_VERTICALLY:
                img.load(NATRON_IMAGES_PATH"splitVertically.png");
                *pix = QPixmap::fromImage(img).scaled(12,12);
                break;
            case NATRON_PIXMAP_VIEWER_CENTER:
                img.load(NATRON_IMAGES_PATH"centerViewer.png");
                *pix = QPixmap::fromImage(img).scaled(50, 50);
                break;
            case NATRON_PIXMAP_VIEWER_CLIP_TO_PROJECT:
                img.load(NATRON_IMAGES_PATH"cliptoproject.png");
                *pix = QPixmap::fromImage(img).scaled(50, 50);
                break;
            case NATRON_PIXMAP_VIEWER_REFRESH:
                img.load(NATRON_IMAGES_PATH"refresh.png");
                *pix = QPixmap::fromImage(img).scaled(20,20);
                break;
            case NATRON_PIXMAP_VIEWER_ROI:
                img.load(NATRON_IMAGES_PATH"viewer_roi.png");
                *pix = QPixmap::fromImage(img).scaled(20,20);
                break;
            case NATRON_PIXMAP_OPEN_FILE:
                img.load(NATRON_IMAGES_PATH"open-file.png");
                *pix = QPixmap::fromImage(img).scaled(15,15);
                break;
            case NATRON_PIXMAP_RGBA_CHANNELS:
                img.load(NATRON_IMAGES_PATH"RGBAchannels.png");
                *pix = QPixmap::fromImage(img).scaled(10,10);
                break;
            case NATRON_PIXMAP_COLORWHEEL:
                img.load(NATRON_IMAGES_PATH"colorwheel.png");
                *pix = QPixmap::fromImage(img).scaled(25, 20);
                break;
            case NATRON_PIXMAP_COLOR_PICKER:
                img.load(NATRON_IMAGES_PATH"color_picker.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_IO_GROUPING:
                img.load(NATRON_IMAGES_PATH"io_low.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_COLOR_GROUPING:
                img.load(NATRON_IMAGES_PATH"color_low.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_TRANSFORM_GROUPING:
                img.load(NATRON_IMAGES_PATH"transform_low.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_DEEP_GROUPING:
                img.load(NATRON_IMAGES_PATH"deep_low.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_FILTER_GROUPING:
                img.load(NATRON_IMAGES_PATH"filter_low.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MULTIVIEW_GROUPING:
                img.load(NATRON_IMAGES_PATH"multiview_low.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MISC_GROUPING:
                img.load(NATRON_IMAGES_PATH"misc_low.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_OPEN_EFFECTS_GROUPING:
                img.load(NATRON_IMAGES_PATH"openeffects.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_TIME_GROUPING:
                img.load(NATRON_IMAGES_PATH"time_low.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_PAINT_GROUPING:
                img.load(NATRON_IMAGES_PATH"paint_low.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_COMBOBOX:
                img.load(NATRON_IMAGES_PATH"combobox.png");
                *pix = QPixmap::fromImage(img).scaled(10, 10);
                break;
            case NATRON_PIXMAP_COMBOBOX_PRESSED:
                img.load(NATRON_IMAGES_PATH"pressed_combobox.png");
                *pix = QPixmap::fromImage(img).scaled(10, 10);
                break;
            case NATRON_PIXMAP_APP_ICON:
                img.load(NATRON_APPLICATION_ICON_PATH);
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


void GuiApplicationManager::loadExtra() {
    
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

void GuiApplicationManager::addPluginToolButtons(const QStringList& groups,
                          const QString& pluginID,
                          const QString& pluginLabel,
                          const QString& pluginIconPath,
                          const QString& groupIconPath) {
    PluginGroupNode* parent = NULL;
    for(int i = 0; i < groups.size();++i){
        PluginGroupNode* child = findPluginToolButtonOrCreate(groups.at(i),groups.at(i),groupIconPath);
        if(parent){
            parent->tryAddChild(child);
            child->setParent(parent);
        }
        parent = child;
        
    }
    PluginGroupNode* lastChild = findPluginToolButtonOrCreate(pluginID,pluginLabel,pluginIconPath);
    if(parent){
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


void GuiApplicationManager::setKnobClipBoard(const KnobSerialization& s,bool copyAnimation) {
    _imp->_knobsClipBoard->k = s;
    _imp->_knobsClipBoard->copyAnimation = copyAnimation;
    _imp->_knobsClipBoard->isEmpty = false;
}

bool GuiApplicationManager::isClipBoardEmpty() const{
    return  _imp->_knobsClipBoard->isEmpty;
}

void GuiApplicationManager::getKnobClipBoard(KnobSerialization* k,bool* copyAnimation) const{
    *k = _imp->_knobsClipBoard->k;
    *copyAnimation = copyAnimation;
}

void GuiApplicationManager::updateAllRecentFileMenus() {
    
    const std::map<int,AppInstance*>& instances = getAppInstances();
    for (std::map<int,AppInstance*>::const_iterator it = instances.begin(); it!= instances.end(); ++it) {
        dynamic_cast<GuiAppInstance*>(it->second)->getGui()->updateRecentFileActions();
    }
}

void GuiApplicationManager::setLoadingStatus(const QString& str) {
    if (isLoaded()) {
        return;
    }
    _imp->_splashScreen->updateText(str);

}

void GuiApplicationManager::loadBuiltinNodePlugins(std::vector<Natron::Plugin*>* plugins,
                            std::map<std::string,std::vector<std::string> >* readersMap,
                            std::map<std::string,std::vector<std::string> >* writersMap) {
    
    // these  are built-in nodes
    QStringList grouping;
    grouping.push_back("Image"); // Readers, Writers, and Generators are in the "Image" group in Nuke
    {
        QtReader* reader = dynamic_cast<QtReader*>(QtReader::BuildEffect(NULL));
        assert(reader);
        std::map<std::string,void*> readerFunctions;
        readerFunctions.insert(std::make_pair("BuildEffect", (void*)&QtReader::BuildEffect));
        LibraryBinary *readerPlugin = new LibraryBinary(readerFunctions);
        assert(readerPlugin);
        Natron::Plugin* plugin = new Natron::Plugin(readerPlugin,reader->pluginID().c_str(),reader->pluginLabel().c_str(),
                                                    (QMutex*)NULL,reader->majorVersion(),reader->minorVersion());
        plugins->push_back(plugin);
        addPluginToolButtons(grouping,reader->pluginID().c_str(),reader->pluginLabel().c_str(), "","");
        
        std::vector<std::string> extensions;
        reader->supportedFileFormats(&extensions);
        for(U32 k = 0; k < extensions.size();++k){
            std::map<std::string,std::vector<std::string> >::iterator it;
            it = readersMap->find(extensions[k]);
            
            if(it != readersMap->end()){
                it->second.push_back(reader->pluginID());
            }else{
                std::vector<std::string> newVec(1);
                newVec[0] = reader->pluginID();
                readersMap->insert(std::make_pair(extensions[k], newVec));
            }
        }
        
        
        delete reader;
    }
    
    {
        QtWriter* writer = dynamic_cast<QtWriter*>(QtWriter::BuildEffect(NULL));
        assert(writer);
        std::map<std::string,void*> writerFunctions;
        writerFunctions.insert(std::make_pair("BuildEffect", (void*)&QtWriter::BuildEffect));
        LibraryBinary *writerPlugin = new LibraryBinary(writerFunctions);
        assert(writerPlugin);
        Natron::Plugin* plugin = new Natron::Plugin(writerPlugin,writer->pluginID().c_str(),writer->pluginLabel().c_str(),
                                                    (QMutex*)NULL,writer->majorVersion(),writer->minorVersion());
        plugins->push_back(plugin);
        addPluginToolButtons(grouping,writer->pluginID().c_str(),writer->pluginLabel().c_str(), "","");
        
        std::vector<std::string> extensions;
        writer->supportedFileFormats(&extensions);
        for(U32 k = 0; k < extensions.size();++k){
            std::map<std::string,std::vector<std::string> >::iterator it;
            it = writersMap->find(extensions[k]);
            
            if(it != writersMap->end()){
                it->second.push_back(writer->pluginID());
            }else{
                std::vector<std::string> newVec(1);
                newVec[0] = writer->pluginID();
                writersMap->insert(std::make_pair(extensions[k], newVec));
            }
        }
        
        delete writer;
    }

    
    {
        EffectInstance* viewer = ViewerInstance::BuildEffect(NULL);
        assert(viewer);
        std::map<std::string,void*> viewerFunctions;
        viewerFunctions.insert(std::make_pair("BuildEffect", (void*)&ViewerInstance::BuildEffect));
        LibraryBinary *viewerPlugin = new LibraryBinary(viewerFunctions);
        assert(viewerPlugin);
        Natron::Plugin* plugin = new Natron::Plugin(viewerPlugin,viewer->pluginID().c_str(),viewer->pluginLabel().c_str(),
                                                    (QMutex*)NULL,viewer->majorVersion(),viewer->minorVersion());
        plugins->push_back(plugin);
        addPluginToolButtons(grouping,viewer->pluginID().c_str(),viewer->pluginLabel().c_str(), "","");
        delete viewer;
    }

}

AppInstance* GuiApplicationManager::makeNewInstance(int appID) const {
    return new GuiAppInstance(appID);
}

KnobGui* GuiApplicationManager::createGuiForKnob(boost::shared_ptr<Knob> knob, DockablePanel *container) const {
    return _imp->_knobGuiFactory->createGuiForKnob(knob,container);
}