/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
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

#ifndef Natron_Gui_NodeGui_h
#define Natron_Gui_NodeGui_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <map>
#include <set>
#include <list>
#include <vector>
#include <utility>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QRectF>
#include <QtCore/QMutex>
#include <QtCore/QSize>
#include <QGraphicsItem>
#include <QDialog>
#include <QtCore/QMutex>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

#include "Engine/NodeGuiI.h"
#include "Engine/EngineFwd.h"
#include "Engine/ImagePlaneDesc.h"
#include "Engine/TimeLineKeys.h"

#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

struct NodeGuiIndicatorPrivate;

class NodeGuiIndicator
{
public:

    NodeGuiIndicator(NodeGraph* graph,
                     int depth,
                     const QString & text,
                     const QPointF & topLeft,
                     int width,
                     int height,
                     const QGradientStops & gradient,
                     const QColor & textColor,
                     QGraphicsItem* parent);

    ~NodeGuiIndicator();

    void setToolTip(const QString & tooltip);

    void setActive(bool active);

    bool isActive() const;

    void refreshPosition(const QPointF & center);

private:

    boost::scoped_ptr<NodeGuiIndicatorPrivate> _imp;
};

///A richer text item which allows text alignmenent
class TextItem
    : public QGraphicsTextItem
{
    Q_OBJECT

public:

    enum
    {
        Type = UserType + 1
    };

    TextItem(QGraphicsItem* parent = 0);
    TextItem(const QString & text,
             QGraphicsItem* parent = 0);

    void setAlignment(Qt::Alignment alignment);
    virtual int type() const;

public Q_SLOTS:

    void updateGeometry(int, int, int);
    void updateGeometry();

private:
    void init();

    Qt::Alignment _alignement;
};

class NodeGui
    : public QObject
      , public QGraphicsItem
      , public NodeGuiI
      , public boost::enable_shared_from_this<NodeGui>
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON
    Q_INTERFACES(QGraphicsItem)

public:

    typedef std::vector<Edge*> InputEdges;

protected: // parent of DotGui, BackdropGui,
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    NodeGui(QGraphicsItem *parent = 0);

public:
    static NodeGuiPtr create(QGraphicsItem *parent = 0) WARN_UNUSED_RETURN
    {
        return NodeGuiPtr( new NodeGui(parent) );
    }

    void initialize(NodeGraph* dag, const NodePtr &internalNode, const CreateNodeArgs& args);

    //Creates panel if needed, might be expensive
    void ensurePanelCreated();


    virtual void destroyGui() OVERRIDE FINAL;

    ~NodeGui() OVERRIDE;

    void restoreStateAfterCreation(const CreateNodeArgs& args);


    NodePtr getNode() const
    {
        return _internalNode.lock();
    }

    /*Returns a pointer to the dag gui*/
    NodeGraph* getDagGui()
    {
        return _graph;
    }

    virtual bool isSettingsPanelVisible() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isSettingsPanelMinimized() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    void getPosition(double *x, double* y) const;
    void getSize(double* w, double* h) const;
    void getColor(double* r, double *g, double* b) const;

    virtual void setPosition(double x, double y) OVERRIDE FINAL;
    virtual void setSize(double w, double h) OVERRIDE FINAL;
    virtual void setColor(double r, double g, double b) OVERRIDE FINAL;
    virtual void setOverlayColor(double r, double g, double b)  OVERRIDE FINAL;

    /*Returns true if the NodeGUI contains the point (in items coordinates)*/
    virtual bool contains(const QPointF &point) const OVERRIDE FINAL;

    /*Returns true if the bounding box of the node intersects the given rectangle in scene coordinates.*/
    bool intersects(const QRectF & rect) const;

    /*returns a QPainterPath indicating the global shape of the node.
       This must be provided so the QGraphicsView framework recognises the
       item correctly.*/
    virtual QPainterPath shape() const OVERRIDE;

    /*Returns the bouding box of the nodeGUI, must be derived if you
       plan on changing the shape of the node.*/
    virtual QRectF boundingRect() const OVERRIDE;

    QRectF boundingRectWithEdges() const;

    /*this function does the painting, using QPainter, you can overload it to change the aspect of
       the node.*/
    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* options, QWidget* parent) OVERRIDE;


    /*Returns a ref to the vector of all the input arrows. This can be used
       to query the src and dst of a specific arrow.*/
    const std::vector<Edge*> & getInputsArrows() const
    {
        return _inputEdges;
    }

    Edge* getOutputArrow() const;
    Edge* getInputArrow(int inputNb) const;

    /*Returns true if the point is included in the rectangle +10px on all edges.*/
    bool isNearby(QPointF &point);

    /*Returns a pointer to the settings panel of this node.*/
    NodeSettingsPanel* getSettingPanel() const
    {
        return _settingsPanel;
    }

    /*Returns a pointer to the layout containing settings panels.*/
    QVBoxLayout* getDockContainer() const;


    /* @brief toggles selected on/off. MT-Safe*/
    void setUserSelected(bool b);

    /* @brief Is the node selected ? MT-Safe */
    bool getIsSelected() const;

    virtual bool isUserSelected() const OVERRIDE FINAL
    {
        return getIsSelected();
    }

    /*Returns a pointer to the first available input. Otherwise returns NULL*/
    Edge* firstAvailableEdge();

    /*find the edge connecting this as dst and the parent as src.
       Return a valid ptr to the edge if it found it, otherwise returns NULL.*/
    Edge* findConnectedEdge(NodeGui* parent);


    void markInputNull(Edge* e);

    const std::list<std::pair<KnobIWPtr, KnobGuiPtr> > & getKnobs() const;
    static const int DEFAULT_OFFSET_BETWEEN_NODES = 30;


    QSize getSize() const;

    /*Returns an edge if the node has an edge close to the
       point pt. pt is in scene coord.*/
    Edge* hasEdgeNearbyPoint(const QPointF & pt);
    Edge* hasEdgeNearbyRect(const QRectF & rect);
    Edge* hasBendPointNearbyPoint(const QPointF & pt);

    void refreshPosition( double x, double y, bool skipMagnet = false, const QPointF & mouseScenePos = QPointF() );

    void changePosition(double dx, double dy);

    void removeSettingsPanel();

    QUndoStackPtr getUndoStack() const;

    void discardGraphPointer();

///same as setScale() but also scales the arrows
    void setScale_natron(double scale);

    void removeHighlightOnAllEdges();

    QColor getCurrentColor() const;

    void setCurrentColor(const QColor & c);

    void setOverlayColor(const QColor& c);

    void refreshKnobsAfterTimeChange(bool onlyTimeEvaluationKnobs, TimeValue time);

    void setParentMultiInstance(const NodeGuiPtr & parent);

    NodeGuiPtr getParentMultiInstance() const
    {
        return _parentMultiInstance.lock();
    }

    std::list<KnobItemsTableGuiPtr> getAllKnobItemsTables() const;

    KnobItemsTableGuiPtr getKnobItemsTable(const std::string& tableName) const;

    void setMergeHintActive(bool active);


    void setOverlayLocked(bool locked);

    virtual bool isOverlayLocked() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual void refreshStateIndicator();

    bool isNearbyNameFrame(const QPointF& pos) const;

    bool isNearbyResizeHandle(const QPointF& pos) const;

    int getFrameNameHeight() const;


    bool wasBeginEditCalled()
    {
        return _wasBeginEditCalled;
    }

    virtual void onIdentityStateChanged(int inputNb) OVERRIDE FINAL;

    void copyPreviewImageBuffer(const std::vector<unsigned int>& data, int width, int height);


    /**
     * @brief Set the cursor to be one of the default cursor.
     * @returns True if it successfully set the cursor, false otherwise.
     * Note: this can only be called during an overlay interact action.
     **/
    virtual void setCurrentCursor(CursorEnum defaultCursor) OVERRIDE FINAL;
    virtual bool setCurrentCursor(const QString& customCursorFilePath) OVERRIDE FINAL;
    virtual void showGroupKnobAsDialog(const KnobGroupPtr& group) OVERRIDE FINAL;

    void getAllVisibleKnobsKeyframes(TimeLineKeysSet* keys) const;

    virtual bool addComponentsWithDialog(const KnobChoicePtr& knob) OVERRIDE FINAL;

    void refreshLinkIndicators(const std::list<std::pair<NodePtr, bool> >& links);

    struct RightClickMenuAction
    {
        std::string id, label, hint;
        std::vector<RightClickMenuAction> subOptions;
    };

    /**
     * @brief Builds the given GUI menu from a list of knob actions to fetch on the given KnobHolder. 
     * Actions are assigned a shortcut from the knob name to be found in the pluginShortcutGroup.
     * @param knobsToFetch The list of knob names that should be mapped to actions in the menu. 
     * If an action is triggered, the corresponding knobChanged handler will be called for the given knob.
     * The special value of kNatronRightClickMenuSeparator indicates that a separator should be added in the menu.
     * Note that a menu can be cascading by providing more than 1 sub-list by main entry items
     **/
    static void populateMenuRecursive(const std::vector<RightClickMenuAction>& knobsToFetch,  const KnobHolderPtr& holder, const std::string& pluginShortcutGroup, const QObject* actionSlotReceiver, Menu* m);

    /**
     * @brief Convenience function where the menu is read from the choice menu knob. If an item of KnobChoice points to another KnobChoice, it will be used as a sub-menu 
     **/
    static void populateMenuRecursive(const KnobChoicePtr& choiceKnob, const KnobHolderPtr& holder, const std::string& pluginShortcutGroup, const QObject* actionSlotReceiver, Menu* m);

protected:

    virtual int getBaseDepth() const { return 20; }

    virtual bool canResize() { return true; }

    virtual bool mustFrameName() const { return false; }

    virtual bool mustAddResizeHandle() const { return false; }

    virtual void getInitialSize(int *w, int *h) const;

    void getSizeWithPreview(int *w, int *h) const;

    virtual void adjustSizeToContent(int *w, int *h, bool adjustToTextSize);
    virtual void resizeExtraContent(int /*w*/,
                                    int /*h*/,
                                    bool /*forceResize*/) {}

public Q_SLOTS:

    void refreshPluginInfo();

    void onNodePresetsChanged();

    void onInputLabelChanged(int inputNb,const QString& label);

    void onInputVisibilityChanged(int inputNb);

    void onRightClickActionTriggered();

    void onRightClickMenuKnobPopulated();

    bool getColorFromGrouping(QColor* color);

    void onHideInputsKnobValueChanged(bool hidden);

    void onAvailableViewsChanged();

    void onOutputLayerChanged();

    void onSettingsPanelClosed(bool closed);

    void onSettingsPanelColorChanged(const QColor & color);

    void togglePreview();

    void onPreviewKnobToggled();

    void onDisabledKnobToggled(bool disabled);

    /**
     * @brief Updates the position of the items contained by the node to fit into
     * the new width and height.
     **/
    void resize(int width, int height, bool forceSize = false, bool adjustToTextSize = false);

    void refreshSize();

    /*Updates the preview image, only if the project is in auto-preview mode*/
    void updatePreviewImage();

    /*Updates the preview image no matter what*/
    void forceComputePreview();

    void setName(const QString & _nameItem);

    void onInternalNameChanged(const QString &oldLabel, const QString& newLabel);

    void onPersistentMessageChanged();

    void refreshEdges();

    /**
     * @brief Specific for the Viewer to  have inputs that are not used in A or B be dashed
     **/
    void refreshDashedStateOfEdges();

    void refreshAnimationIcon();

    /*initialises the input edges*/
    void initializeInputs();

    void initializeKnobs();

    //void activate();
    //void deactivate();

    void hideGui();

    void showGui();

    /*Use NULL for src to disconnect.*/
    bool connectEdge(int edgeNumber);

    void setVisibleSettingsPanel(bool b);

    void refreshRenderingIndicator();

    void beginEditKnobs();

    void centerGraphOnIt();

    void refreshOutputEdgeVisibility();

    void onStreamWarningsChanged();

    void refreshNodeText();

    void onSwitchInputActionTriggered();

    void onSettingsPanelClosedChanged(bool closed);

    void onParentMultiInstancePositionChanged(int x, int y);

    void refreshEdgesVisility(bool hovered);
    void refreshEdgesVisility();

    void onPreviewImageComputed();

    void onKeepInAnimationModuleKnobChanged();

private Q_SLOTS:


    void updatePreviewNow();

Q_SIGNALS:

    void colorChanged(QColor);

    void positionChanged(int x, int y);

    void settingsPanelClosed(bool b);

    void previewImageComputed();

protected:

    virtual void createGui();
    virtual NodeSettingsPanel* createPanel(QVBoxLayout* container, const NodeGuiPtr & thisAsShared);
    virtual bool canMakePreview()
    {
        return true;
    }

    virtual void applyBrush(const QBrush & brush);

private:

    int getPluginIconWidth() const;

    double refreshPreviewAndLabelPosition(const QRectF& bbox);

    void refreshEdgesVisibilityInternal(bool hovered);

    void refreshPositionEnd(double x, double y);

    void togglePreview_internal(bool refreshPreview = true);

    void ensurePreviewCreated();

    void setAboveItem(QGraphicsItem* item);

    void populateMenu();

    void refreshCurrentBrush();

    void initializeInputsForInspector();

    void initializeInputsLayeredComp();

    /*pointer to the dag*/
    NodeGraph* _graph;

    /*pointer to the internal node*/
    NodeWPtr _internalNode;

    /*true if the node is selected by the user*/

    /*A pointer to the graphical text displaying the name.*/
    bool _settingNameFromGui;
    bool _panelOpenedBeforeDeactivate;
    NodeGraphPixmapItem* _pluginIcon;
    QGraphicsRectItem* _pluginIconFrame;
    QGraphicsPixmapItem* _presetIcon;
    NodeGraphTextItem *_nameItem;
    QGraphicsRectItem *_nameFrame;
    QGraphicsPolygonItem* _resizeHandle;

    /*A pointer to the rectangle of the node.*/
    NodeGraphRectItem* _boundingBox;

    /*A pointer to the preview pixmap displayed for readers/*/
    QGraphicsPixmapItem* _previewPixmap;
    mutable QMutex _previewDataMutex;
    std::vector<unsigned int> _previewData;
    int _previewW, _previewH;
    QGraphicsSimpleTextItem* _persistentMessage;
    NodeGraphRectItem* _stateIndicator;
    bool _mergeHintActive;
    NodeGuiIndicatorPtr _streamIssuesWarning;
    QGraphicsLineItem* _disabledTopLeftBtmRight;
    QGraphicsLineItem* _disabledBtmLeftTopRight;
    /*the graphical input arrows*/
    std::vector<Edge*> _inputEdges;
    Edge* _outputEdge;
    NodeSettingsPanel* _settingsPanel;

    ///This is valid only if the node is a multi-instance and this is the main instance.
    ///The "real" panel showed on the gui will be the _settingsPanel, but we still need to create
    ///another panel for the main-instance (hidden) knobs to function properly
    NodeSettingsPanel* _mainInstancePanel;

    //True when the settings panel has been  created
    bool _panelCreated;
    bool _wasBeginEditCalled;

    boost::shared_ptr<NodeGuiIndicator> _expressionIndicator;
    boost::shared_ptr<NodeGuiIndicator> _cloneIndicator;
    boost::shared_ptr<NodeGuiIndicator> _animationIndicator;
    QPoint _magnecEnabled; //<enabled in X or/and Y
    QPointF _magnecDistance; //for x and for  y
    QPoint _updateDistanceSinceLastMagnec; //for x and for y
    QPointF _distanceSinceLastMagnec; //for x and for y
    QPointF _magnecStartingPos; //for x and for y
    QString _channelsExtraLabel;
    boost::weak_ptr<NodeGui> _parentMultiInstance;
    boost::shared_ptr<QUndoStack> _undoStack; /*!< undo/redo stack*/
    mutable QMutex _overlayLockedMutex;
    bool _overlayLocked;
    NodeGuiIndicatorPtr _availableViewsIndicator;
    NodeGuiIndicatorPtr _passThroughIndicator;
    NodeWPtr _identityInput;
    bool identityStateSet;
    boost::shared_ptr<GroupKnobDialog> _activeNodeCustomModalDialog;

};

inline NodeGuiPtr
toNodeGui(const NodeGuiIPtr& nodeGuiI)
{
    return boost::dynamic_pointer_cast<NodeGui>(nodeGuiI);
}

struct GroupKnobDialogPrivate;
class GroupKnobDialog : public QDialog
{

    GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
    GCC_DIAG_SUGGEST_OVERRIDE_ON

public:


    GroupKnobDialog(Gui* gui,
                    const KnobGroupConstPtr& group);

    virtual ~GroupKnobDialog();

public Q_SLOTS:

    void onDialogBoxButtonClicked(QAbstractButton* button);
    
private:

    boost::scoped_ptr<GroupKnobDialogPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // Natron_Gui_NodeGui_h
