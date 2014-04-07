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

#ifndef NATRON_GUI_GUI_H_
#define NATRON_GUI_GUI_H_

#ifndef Q_MOC_RUN
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
#include <QMainWindow>
CLANG_DIAG_ON(deprecated)

#include "Global/GlobalDefines.h"

//boost
namespace boost {
    namespace archive {
        class xml_iarchive;
        class xml_oarchive;
    }
}

//QtGui
class Splitter;
class QUndoStack;
class QScrollArea;

//Natron gui
class GuiAppInstance;
class NodeGui;
class TabWidget;
class ToolButton;
class ViewerTab;
class DockablePanel;
class NodeGraph;
class CurveEditor;
class Histogram;

//Natron engine
class ViewerInstance;
class PluginGroupNode;
class Color_Knob;
class ProcessHandler;
namespace Natron {
    class Node;
    class Image;
    class OutputEffectInstance;
}


struct GuiPrivate;
class Gui : public QMainWindow , public boost::noncopyable
{
    Q_OBJECT
    
public:
    explicit Gui(GuiAppInstance* app,QWidget* parent=0);
    
    virtual ~Gui() OVERRIDE;
    
    void createGui();
    
    boost::shared_ptr<NodeGui> createNodeGUI(boost::shared_ptr<Natron::Node> node,bool requestedByLoad);

    void addNodeGuiToCurveEditor(boost::shared_ptr<NodeGui> node);
        
    boost::shared_ptr<NodeGui> getSelectedNode() const;
    
    void setLastSelectedViewer(ViewerTab* tab);
    
    ViewerTab* getLastSelectedViewer() const;
    
    bool eventFilter(QObject *target, QEvent *event);

    void createViewerGui(boost::shared_ptr<Natron::Node> viewer);
    
    /*Called internally by the viewer node. It adds
     a new Viewer tab GUI and returns a pointer to it.*/
    ViewerTab* addNewViewerTab(ViewerInstance* node,TabWidget* where);
    
    void addViewerTab(ViewerTab* tab,TabWidget* where);
    
    /*Called internally by the viewer node when
     it gets deleted. This removes the 
     associated GUI. It may also be called from the TabWidget
     that wants to close. The deleteData flag tells whether we actually want
     to destroy the tab/node or just hide them.*/
    void removeViewerTab(ViewerTab* tab,bool initiatedFromNode,bool deleteData);
    
    Histogram* addNewHistogram();
    
    void removeHistogram(Histogram* h);
    
    const std::list<Histogram*>& getHistograms() const;
    
    std::list<Histogram*> getHistograms_mt_safe() const;
    
    void setNewViewerAnchor(TabWidget* where);
        
    void maximize(TabWidget* what);
    
    void minimize();
    
    void loadStyleSheet();
    
    ToolButton* findExistingToolButton(const QString& name) const;
    
    ToolButton* findOrCreateToolButton(PluginGroupNode* plugin);
    
    const std::vector<ToolButton*>& getToolButtons() const;

    void registerNewUndoStack(QUndoStack* stack);
    
    void registerNewUndoStack(QUndoStack* stack,QAction* undoAction,QAction* redoAction);
    
    void removeUndoStack(QUndoStack* stack);
        
    /**
     * @brief An error dialog with title and text customizable
     **/
    void errorDialog(const std::string& title,const std::string& text);
    
    void warningDialog(const std::string& title,const std::string& text);
    
    void informationDialog(const std::string& title,const std::string& text);
    
    Natron::StandardButton questionDialog(const std::string& title,const std::string& message,Natron::StandardButtons buttons =
                                           Natron::StandardButtons(Natron::Yes | Natron::No),
                                           Natron::StandardButton defaultButton = Natron::NoButton);
    
    void selectNode(boost::shared_ptr<NodeGui> node);
    
    GuiAppInstance* getApp() const;
        
    void updateViewsActions(int viewsCount);
    
    static QKeySequence keySequenceForView(int v);
    
    /*set the curve editor as the active widget of its pane*/
    void setCurveEditorOnTop();
    
    const std::list<TabWidget*>& getPanes() const;
    
    std::list<TabWidget*> getPanes_mt_safe() const;
    
    void removePane(TabWidget* pane);
    
    void registerPane(TabWidget* pane);
    
    void registerTab(QWidget* tab);
    
    void unregisterTab(QWidget* tab);
    
    /*Returns a valid tab if a tab with a matching name has been
     found. Otherwise returns NULL.*/
    QWidget* findExistingTab(const std::string& name) const;
    

    std::list<Splitter*> getSplitters() const;
    
    void removeSplitter(Splitter* s);
    
    void registerSplitter(Splitter* s);

    std::vector<std::string> popOpenFileDialog(bool sequenceDialog,
                                               const std::vector<std::string>& initialfilters,const std::string& initialDir);
    
    std::string popSaveFileDialog(bool sequenceDialog,const std::vector<std::string>& initialfilters,const std::string& initialDir);
    
    void setUserScrubbingTimeline(bool b);
    
    bool isUserScrubbingTimeline() const;
    
    /*Refresh all previews if the project's preview mode is auto*/
    void refreshAllPreviews();
    
    /*force a refresh on all previews no matter what*/
    void forceRefreshAllPreviews();
    
    void startDragPanel(QWidget* panel)  ;
    
    QWidget* stopDragPanel()  ;
    
    bool isDraggingPanel() const;
    
    void updateRecentFileActions();
    
    static QPixmap screenShot(QWidget* w);
    
    void loadProjectGui(boost::archive::xml_iarchive& obj) const;
    
    void saveProjectGui(boost::archive::xml_oarchive& archive);
    
    void setColorPickersColor(const QColor& c);
    
    void registerNewColorPicker(boost::shared_ptr<Color_Knob> knob);
    
    void removeColorPicker(boost::shared_ptr<Color_Knob> knob);

    void initProjectGuiKnobs();
    
    void updateViewersViewsMenu(int viewsCount);
    
    void setViewersCurrentView(int view);
    
    const std::list<ViewerTab*>& getViewersList() const;
    
    std::list<ViewerTab*> getViewersList_mt_safe() const;
    
    void activateViewerTab(ViewerInstance* viewer);
    
    void deactivateViewerTab(ViewerInstance* viewer);
    
    ViewerTab* getViewerTabForInstance(ViewerInstance* node) const;
    
    const std::list<boost::shared_ptr<NodeGui> >& getVisibleNodes() const;
    
    std::list<boost::shared_ptr<NodeGui> > getVisibleNodes_mt_safe() const;
    
    void deselectAllNodes() const;
    
    void onProcessHandlerStarted(const QString& sequenceName,int firstFrame,int lastFrame,
                                 ProcessHandler* process);
    
    void onWriterRenderStarted(const QString& sequenceName,int firstFrame,int lastFrame,
                               Natron::OutputEffectInstance* writer);
    
    NodeGraph* getNodeGraph() const;
    
    CurveEditor* getCurveEditor() const;
    
    QScrollArea* getPropertiesScrollArea() const;
    
    TabWidget* getWorkshopPane() const;

    const std::map<std::string,QWidget*>& getRegisteredTabs() const;
    
    void updateLastSequenceOpenedPath(const QString& path);
    
    void updateLastSequenceSavedPath(const QString& path);
    
    bool isClosing() const;
    
    void clearExceedingUndoRedoEvents();
    
    void setGlewVersion(const QString& version);
    
    void setOpenGLVersion(const QString & version);
    
    const QString& getGlewVersion() const;
    
    const QString& getOpenGLVersion() const;

    const QString& getBoostVersion() const;

    const QString& getQtVersion() const;

    
    /*Useful function that saves on disk the image in png format.
     The name of the image will be the hash key of the image.*/
    static void debugImage(Natron::Image* image,const QString& filename = QString());
    
signals:
    
    void doDialog(int type,const QString& title,const QString& content,Natron::StandardButtons buttons,int defaultB);

    ///emitted when a viewer changes its name or is deleted/added
    void viewersChanged();
    
public slots:

    bool exitGui(); //< exit, asking questions to the user
    void quit(); //< exit right away, without any user interaction
    void toggleFullScreen();
    void closeEvent(QCloseEvent *e);
    void newProject();
    void openProject();
    bool saveProject();
    bool saveProjectAs();
    void autoSave();
    
    void connectInput1();
    void connectInput2();
    void connectInput3();
    void connectInput4();
    void connectInput5();
    void connectInput6();
    void connectInput7();
    void connectInput8();
    void connectInput9();
    void connectInput10();
    
    void showView0();
    void showView1();
    void showView2();
    void showView3();
    void showView4();
    void showView5();
    void showView6();
    void showView7();
    void showView8();
    void showView9();
    
    void onDoDialog(int type,const QString& title,const QString& content,Natron::StandardButtons buttons,int defaultB);
    
    /*Returns a code from the save dialog:
     * -1  = unrecognized code
     * 0 = Save
     * 1 = Discard
     * 2 = Cancel or Escape
     **/
    int saveWarning();
    
    void setVisibleProjectSettingsPanel();
    
    void putSettingsPanelFirst(DockablePanel* panel);
    
    void addToolButttonsToToolBar();

    void onCurrentUndoStackChanged(QUndoStack* stack);
    
    void showSettings();
    
    void showAbout();

    void openRecentFile();
    
    void onProjectNameChanged(const QString& name);
    
    void onNodeNameChanged(const QString& name);
    
    void onViewerImageChanged();
        
    boost::shared_ptr<Natron::Node> createReader();
    
    boost::shared_ptr<Natron::Node> createWriter();
    
    void renderAllWriters();
        
    void renderSelectedNode();

private:

    void setupUi();
    
    boost::scoped_ptr<GuiPrivate> _imp;
    
};




#endif // NATRON_GUI_GUI_H_
