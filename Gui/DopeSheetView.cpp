#include "DopeSheetView.h"

#include <algorithm>

// Qt includes
#include <QApplication>
#include <QMouseEvent>
#include <QThread>
#include <QToolButton>
#include <QTreeWidget>
#include <QUndoStack>

// Natron includes
#include "Engine/Curve.h"
#include "Engine/Image.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/DockablePanel.h"
#include "Gui/DopeSheet.h"
#include "Gui/DopeSheetEditorUndoRedo.h"
#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiMacros.h"
#include "Gui/Histogram.h"
#include "Gui/KnobGui.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"
#include "Gui/TextRenderer.h"
#include "Gui/ticks.h"
#include "Gui/ViewerGL.h"
#include "Gui/ZoomContext.h"


// Some macros to ensure the following operations always running in the main thread
#define RUNNING_IN_MAIN_THREAD() \
    assert(qApp && qApp->thread() == QThread::currentThread())

// Note : if you want to call the following functions from the pimpl of a class, ensure it have its
// parent as attribute and pass its name as parameter, or pass 'this' if
// it's called from a pimpled class.
#define RUNNING_IN_MAIN_CONTEXT(glContextOwner) \
    assert(QGLContext::currentContext() == glContextOwner->context())

#define RUNNING_IN_MAIN_THREAD_AND_CONTEXT(glContextOwner) \
    RUNNING_IN_MAIN_THREAD();\
    RUNNING_IN_MAIN_CONTEXT(glContextOwner)


typedef std::set<double> TimeSet;
typedef Knob<int> * KnobIntPtr;
typedef std::vector<DSSelectedKey> DSSelectedKeys;

const int KF_HEIGHT = 10;
const int KF_X_OFFSET = 3;
const int CLICK_DISTANCE_ACCEPTANCE = 5;


////////////////////////// DopeSheetView //////////////////////////

class DopeSheetViewPrivate
{
public:
    DopeSheetViewPrivate(DopeSheetView *qq);
    ~DopeSheetViewPrivate();

    /* functions */

    // Helpers
    QRectF rectToZoomCoordinates(const QRectF &rect) const;
    QRectF rectToWidgetCoordinates(const QRectF &rect) const;
    QRectF nameItemRectToSectionRect(const QRectF &rect) const;

    void findKeyframeBounds(int *minTime, int *maxTime);

    DSNode *getNodeUnderMouse(const QPointF &pos) const;
    DSKnob *getKnobUnderMouse(const QPointF &pos, int *dimension = NULL) const;

    bool keyframeIsAlreadyInSelection(DSSelectedKey *key) const;

    DopeSheetView::EventStateEnum getEventStateDuringHover(const QPointF &widgetCoords) const;
    Qt::CursorShape getCursorForEditingState(DopeSheetView::EventStateEnum es) const;

    bool isNearByClipRectLeft(double time, const QRectF &clipRect) const;
    bool isNearByClipRectRight(double time, const QRectF &clipRect) const;
    bool isNearByCurrentFrameIndicatorBottom(const QPointF &zoomCoords) const;

    DSSelectedKeys isNearByKeyframe(DSKnob *dsKnob, const QPointF &widgetCoords, int dimension) const;
    DSSelectedKeys isNearByKeyframe(DSNode *dsNode, const QPointF &widgetCoords) const;

    // Drawing
    void drawScale() const;

    void drawSections() const;

    void drawNodeSection(const DSNode *dsNode) const;
    void drawKnobSection(const DSKnob *dsKnob) const;

    void drawClip(const DSNode *dsNode) const;
    void drawKeyframes(DSNode *dsNode) const;

    void drawProjectBounds() const;
    void drawCurrentFrameIndicator();

    void drawSelectionRect() const;

    void drawSelectedKeysBRect() const;


    // After or during an user interaction
    void computeTimelinePositions();
    void computeSelectionRect(const QPointF &origin, const QPointF &current);

    // User interaction
    void createSelectionFromCursor(DSKnob *dsKnob, const QPointF &widgetCoords, int dimension, bool booleanOp);
    void createSelectionFromCursor(DSNode *dsNode, const QPointF &widgetCoords, bool booleanOp);
    void createSelectionFromRect(const QRectF &rect, bool booleanOp);

    void pushUndoCommand(QUndoCommand *cmd);

    void clearCurrentClipSelection();
    void clearCurrentKeyframeSelection();

    void moveCurrentFrameIndicator(double toTime);

    void deleteSelectedKeyframes();

    void frame();

    /* attributes */
    DopeSheetView *parent;

    Gui *gui;

    // necessary to retrieve some useful values for drawing
    boost::shared_ptr<TimeLine> timeline;

    // useful to retrieve the "model"
    DopeSheetEditor *dopeSheetEditor;

    // for rendering
    QFont *font;
    Natron::TextRenderer textRenderer;

    // for navigating
    ZoomContext zoomContext;
    bool zoomOrPannedSinceLastFit;

    // for current time indicator
    QPolygonF currentFrameIndicatorBottomPoly;

    // for keyframe selection
    DSKeyPtrList keyframeSelection;
    QRectF selectionRect;

    // keyframe selection rect
    QRectF selectedKeysBRect;

    // for various user interaction
    QPointF lastPosOnMousePress;
    QPointF lastPosOnMouseMove;
    double lastTimeOffsetOnMousePress;
    double keyDragLastMovement;

    DopeSheetView::EventStateEnum eventState;
    DopeSheetView::EventStateEnum couldBeNextEventState;

    // for clip (Reader, Time nodes) user interaction
    DSNode *currentNodeClipSelected;
    DSNode *currentEditedReader;

    // others
    boost::scoped_ptr<QUndoStack> undoStack;
    bool hasOpenGLVAOSupport;
};

DopeSheetViewPrivate::DopeSheetViewPrivate(DopeSheetView *qq) :
    parent(qq),
    gui(0),
    timeline(),
    dopeSheetEditor(0),
    font(new QFont(appFont,appFontSize)),
    textRenderer(),
    zoomContext(),
    zoomOrPannedSinceLastFit(false),
    keyframeSelection(),
    selectionRect(),
    selectedKeysBRect(),
    lastPosOnMousePress(),
    lastPosOnMouseMove(),
    lastTimeOffsetOnMousePress(),
    keyDragLastMovement(),
    eventState(DopeSheetView::esNoEditingState),
    couldBeNextEventState(DopeSheetView::esNoEditingState),
    currentNodeClipSelected(0),
    currentEditedReader(0),
    undoStack(new QUndoStack(parent)),
    hasOpenGLVAOSupport(true)
{}

DopeSheetViewPrivate::~DopeSheetViewPrivate()
{
    clearCurrentClipSelection();
}

/**
 * @brief DopeSheetViewPrivate::rectToZoomCoordinates
 *
 *
 */
QRectF DopeSheetViewPrivate::rectToZoomCoordinates(const QRectF &rect) const
{
    QPointF topLeft(rect.left(),
                    zoomContext.toZoomCoordinates(rect.left(),
                                                  rect.top()).y());
    QPointF bottomRight(rect.right(),
                        zoomContext.toZoomCoordinates(rect.right(),
                                                      rect.bottom()).y());

    return QRectF(topLeft, bottomRight);
}

QRectF DopeSheetViewPrivate::rectToWidgetCoordinates(const QRectF &rect) const
{
    QPointF topLeft(rect.left(),
                    zoomContext.toWidgetCoordinates(rect.left(),
                                                    rect.top()).y());
    QPointF bottomRight(rect.right(),
                        zoomContext.toWidgetCoordinates(rect.right(),
                                                        rect.bottom()).y());

    return QRectF(topLeft, bottomRight);
}

/**
 * @brief DopeSheetViewPrivate::drawScale
 *
 * Draws the dope sheet's grid and time indicators.
 */
void DopeSheetViewPrivate::drawScale() const
{
    RUNNING_IN_MAIN_THREAD_AND_CONTEXT(parent);

    QPointF bottomLeft = zoomContext.toZoomCoordinates(0, parent->height() - 1);
    QPointF topRight = zoomContext.toZoomCoordinates(parent->width() - 1, 0);

    // Don't attempt to draw a scale on a widget with an invalid height
    if (parent->height() <= 1) {
        return;
    }

    QFontMetrics fontM(*font);
    const double smallestTickSizePixel = 5.; // tick size (in pixels) for alpha = 0.
    const double largestTickSizePixel = 1000.; // tick size (in pixels) for alpha = 1.
    std::vector<double> acceptedDistances;
    acceptedDistances.push_back(1.);
    acceptedDistances.push_back(5.);
    acceptedDistances.push_back(10.);
    acceptedDistances.push_back(50.);

    // Retrieve the appropriate settings for drawing
    boost::shared_ptr<Settings> settings = appPTR->getCurrentSettings();
    double scaleR, scaleG, scaleB;
    //FIXME Use getDopeSheetEditorScaleColor instead
    settings->getDopeSheetEditorGridColor(&scaleR, &scaleG, &scaleB);

    QColor scaleColor;
    scaleColor.setRgbF(Natron::clamp(scaleR),
                       Natron::clamp(scaleG),
                       Natron::clamp(scaleB));

    // Perform drawing
    {
        GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        const double rangePixel = parent->width();
        const double range_min = bottomLeft.x();
        const double range_max = topRight.x();
        const double range = range_max - range_min;

        double smallTickSize;
        bool half_tick;

        ticks_size(range_min, range_max, rangePixel, smallestTickSizePixel, &smallTickSize, &half_tick);

        int m1, m2;
        const int ticks_max = 1000;
        double offset;

        ticks_bounds(range_min, range_max, smallTickSize, half_tick, ticks_max, &offset, &m1, &m2);
        std::vector<int> ticks;
        ticks_fill(half_tick, ticks_max, m1, m2, &ticks);

        const double smallestTickSize = range * smallestTickSizePixel / rangePixel;
        const double largestTickSize = range * largestTickSizePixel / rangePixel;
        const double minTickSizeTextPixel = fontM.width( QString("00") );
        const double minTickSizeText = range * minTickSizeTextPixel / rangePixel;

        for (int i = m1; i <= m2; ++i) {

            double value = i * smallTickSize + offset;
            const double tickSize = ticks[i - m1] * smallTickSize;
            const double alpha = ticks_alpha(smallestTickSize, largestTickSize, tickSize);

            glColor4f(scaleColor.redF(), scaleColor.greenF(), scaleColor.blueF(), alpha);

            // Draw the vertical lines belonging to the grid
            glBegin(GL_LINES);
            glVertex2f(value, bottomLeft.y());
            glVertex2f(value, topRight.y());
            glEnd();

            glCheckError();

            // Draw the time indicators
            if (tickSize > minTickSizeText) {
                const int tickSizePixel = rangePixel * tickSize / range;
                const QString s = QString::number(value);
                const int sSizePixel = fontM.width(s);

                if (tickSizePixel > sSizePixel) {
                    const int sSizeFullPixel = sSizePixel + minTickSizeTextPixel;
                    double alphaText = 1.0; //alpha;

                    if (tickSizePixel < sSizeFullPixel) {
                        // when the text size is between sSizePixel and sSizeFullPixel,
                        // draw it with a lower alpha
                        alphaText *= (tickSizePixel - sSizePixel) / (double)minTickSizeTextPixel;
                    }

                    QColor c = scaleColor;
                    c.setAlpha(255 * alphaText);

                    parent->renderText(value, bottomLeft.y(), s, c, *font);

                    // Uncomment the line below to draw the indicator on top too
                    // parent->renderText(value, topRight.y() - 20, s, c, *font);
                }
            }
        }
    }
}

QRectF DopeSheetViewPrivate::nameItemRectToSectionRect(const QRectF &rect) const
{
    QRectF r = rectToZoomCoordinates(rect);

    double sectionTop = r.topLeft().y();
    double sectionBottom = r.bottomRight().y() - 1;

    return QRectF(QPointF(zoomContext.left(), sectionTop),
                  QPointF(zoomContext.right(), sectionBottom));
}

/**
 * @brief DopeSheetViewPrivate::drawKnobSection
 *
 *
 */
void DopeSheetViewPrivate::drawKnobSection(const DSKnob *dsKnob) const
{
    GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

    boost::shared_ptr<Settings> settings = appPTR->getCurrentSettings();

    if (dsKnob->isMultiDim()) {
        // Draw root section
        QRectF nameItemRect = dsKnob->getNameItemRect();
        QRectF sectionRect = nameItemRectToSectionRect(nameItemRect);

        double rootR, rootG, rootB, rootA;
        settings->getDopeSheetEditorRootSectionBackgroundColor(&rootR, &rootG, &rootB, &rootA);

        glColor4f(rootR, rootG, rootB, rootA);

        glBegin(GL_QUADS);
        glVertex2f(sectionRect.topLeft().x(), sectionRect.topLeft().y());
        glVertex2f(sectionRect.bottomLeft().x(), sectionRect.bottomLeft().y());
        glVertex2f(sectionRect.bottomRight().x(), sectionRect.bottomRight().y());
        glVertex2f(sectionRect.topRight().x(), sectionRect.topRight().y());
        glEnd();

        // Draw child sections
        double knobR, knobG, knobB, knobA;
        settings->getDopeSheetEditorKnobSectionBackgroundColor(&knobR, &knobG, &knobB, &knobA);

        glColor4f(knobR, knobG, knobB, knobA);

        for (int i = 0; i < dsKnob->getKnobGui()->getKnob()->getDimension(); ++i) {
            QRectF nameChildItemRect = dsKnob->getNameItemRectForDim(i);
            QRectF childSectionRect = nameItemRectToSectionRect(nameChildItemRect);

            // Draw child section
            glBegin(GL_QUADS);
            glVertex2f(childSectionRect.topLeft().x(), childSectionRect.topLeft().y());
            glVertex2f(childSectionRect.bottomLeft().x(), childSectionRect.bottomLeft().y());
            glVertex2f(childSectionRect.bottomRight().x(), childSectionRect.bottomRight().y());
            glVertex2f(childSectionRect.topRight().x(), childSectionRect.topRight().y());
            glEnd();
        }
    }
    else {
        QRectF nameItemRect = dsKnob->getNameItemRect();
        QRectF sectionRect = nameItemRectToSectionRect(nameItemRect);

        double knobR, knobG, knobB, knobA;
        settings->getDopeSheetEditorKnobSectionBackgroundColor(&knobR, &knobG, &knobB, &knobA);

        glColor4f(knobR, knobG, knobB, knobA);

        glBegin(GL_QUADS);
        glVertex2f(sectionRect.topLeft().x(), sectionRect.topLeft().y());
        glVertex2f(sectionRect.bottomLeft().x(), sectionRect.bottomLeft().y());
        glVertex2f(sectionRect.bottomRight().x(), sectionRect.bottomRight().y());
        glVertex2f(sectionRect.topRight().x(), sectionRect.topRight().y());
        glEnd();

    }
}

/**
 * @brief DopeSheetViewPrivate::drawNodeSection
 *
 *
 */
void DopeSheetViewPrivate::drawNodeSection(const DSNode *dsNode) const
{
    GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

    QRectF nameItemRect = dsNode->getNameItemRect();

    QRectF sectionRect = nameItemRectToSectionRect(nameItemRect);

    boost::shared_ptr<Settings> settings = appPTR->getCurrentSettings();
    double rootR, rootG, rootB, rootA;
    settings->getDopeSheetEditorRootSectionBackgroundColor(&rootR, &rootG, &rootB, &rootA);

    glColor4f(rootR, rootG, rootB, rootA);

    glBegin(GL_QUADS);
    glVertex2f(sectionRect.topLeft().x(), sectionRect.topLeft().y());
    glVertex2f(sectionRect.bottomLeft().x(), sectionRect.bottomLeft().y());
    glVertex2f(sectionRect.bottomRight().x(), sectionRect.bottomRight().y());
    glVertex2f(sectionRect.topRight().x(), sectionRect.topRight().y());
    glEnd();
}

/**
 * @brief DopeSheetViewPrivate::drawKeyframes
 *
 *
 */
void DopeSheetViewPrivate::drawKeyframes(DSNode *dsNode) const
{
    RUNNING_IN_MAIN_THREAD_AND_CONTEXT(parent);

    // Perform drawing
    {
        GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_POINT_SMOOTH);

        TreeItemsAndDSKnobs knobItems = dsNode->getTreeItemsAndDSKnobs();
        TimeSet nodeKeyframes;

        for (TreeItemsAndDSKnobs::const_iterator it = knobItems.begin();
             it != knobItems.end();
             ++it) {

            DSKnob *dsKnob = (*it).second;

            // The knob is no longer animated
            if (dsKnob->getNameItem()->isHidden()) {
                continue;
            }

            KnobGui *knobGui = dsKnob->getKnobGui();
            TimeSet multiDimKnobKeyframes;

            glColor3f(1.f, 0.f, 0.f);

            // Draw keyframes for each dimension of the knob
            for (int dim = 0; dim < knobGui->getKnob()->getDimension(); ++dim) {
                KeyFrameSet keyframes = knobGui->getCurve(dim)->getKeyFrames_mt_safe();

                for (KeyFrameSet::const_iterator kIt = keyframes.begin();
                     kIt != keyframes.end();
                     ++kIt) {
                    KeyFrame kf = (*kIt);

                    double keyTime = kf.getTime();

                    double y = (dsKnob->isMultiDim()) ? dsKnob->getNameItemRectForDim(dim).center().y()
                                                      : dsKnob->getNameItemRect().center().y();
                    QPointF p = zoomContext.toZoomCoordinates(keyTime, y);

                    QRectF kfRect;
                    kfRect.setHeight(KF_HEIGHT);
                    kfRect.setLeft(zoomContext.toZoomCoordinates(keyTime - KF_X_OFFSET, y).x());
                    kfRect.setRight(zoomContext.toZoomCoordinates(keyTime + KF_X_OFFSET, y).x());
                    kfRect.moveCenter(zoomContext.toWidgetCoordinates(p.x(), p.y()));

                    QRectF zoomKfRect = rectToZoomCoordinates(kfRect);

                    // Draw keyframe in the knob dim section only if it's visible
                    if (dsNode->getNameItem()->isExpanded() && dsKnob->getNameItem()->isExpanded()) {
                        DSKeyPtrList::const_iterator isSelected = keyframeSelection.end();

                        for (DSKeyPtrList::const_iterator it2 = keyframeSelection.begin();
                             it2 != keyframeSelection.end(); it2++) {
                            DSKeyPtr selectedKey = (*it2);

                            if (selectedKey->dimension != dim) {
                                continue;
                            }

                            if (selectedKey->key.getTime() == keyTime && selectedKey->dsKnob == dsKnob) {
                                if (selectedKey->key == kf) {
                                    isSelected = it2;
                                    glColor3f(0.961f, 0.961f, 0.047f);
                                    break;
                                }
                            }
                        }

                        glBegin(GL_QUADS);
                        glVertex2f(zoomKfRect.left(), zoomKfRect.top());
                        glVertex2f(zoomKfRect.left(), zoomKfRect.bottom());
                        glVertex2f(zoomKfRect.right(), zoomKfRect.bottom());
                        glVertex2f(zoomKfRect.right(), zoomKfRect.top());
                        glEnd();

                        if (isSelected != keyframeSelection.end()) {
                            glColor3f(1.f, 0.f, 0.f);
                        }
                    }

                    // Draw keyframe in multidim root knob section too
                    if (dsKnob->isMultiDim()) {
                        TimeSet::const_iterator multiDimKnobKeysIt = multiDimKnobKeyframes.find(keyTime);

                        if (multiDimKnobKeysIt == multiDimKnobKeyframes.end()) {
                            p = zoomContext.toZoomCoordinates(keyTime,
                                                              dsKnob->getNameItemRect().center().y());

                            kfRect.moveCenter(zoomContext.toWidgetCoordinates(p.x(), p.y()));
                            zoomKfRect = rectToZoomCoordinates(kfRect);

                            // Draw only if the section is visible
                            if (dsNode->getNameItem()->isExpanded()) {
                                glBegin(GL_QUADS);
                                glVertex2f(zoomKfRect.left(), zoomKfRect.top());
                                glVertex2f(zoomKfRect.left(), zoomKfRect.bottom());
                                glVertex2f(zoomKfRect.right(), zoomKfRect.bottom());
                                glVertex2f(zoomKfRect.right(), zoomKfRect.top());
                                glEnd();
                            }

                            multiDimKnobKeyframes.insert(keyTime);
                        }
                    }

                    // Draw keyframe in node section
                    TimeSet::iterator nodeKeysIt = nodeKeyframes.find(keyTime);

                    if (nodeKeysIt == nodeKeyframes.end()) {
                        p = zoomContext.toZoomCoordinates(keyTime,
                                                          dsNode->getNameItemRect().center().y());

                        kfRect.moveCenter(zoomContext.toWidgetCoordinates(p.x(), p.y()));
                        zoomKfRect = rectToZoomCoordinates(kfRect);

                        glBegin(GL_QUADS);
                        glVertex2f(zoomKfRect.left(), zoomKfRect.top());
                        glVertex2f(zoomKfRect.left(), zoomKfRect.bottom());
                        glVertex2f(zoomKfRect.right(), zoomKfRect.bottom());
                        glVertex2f(zoomKfRect.right(), zoomKfRect.top());
                        glEnd();

                        nodeKeyframes.insert(keyTime);
                    }
                }
            }
        }
    }
}

/**
 * @brief DopeSheetViewPrivate::drawSections
 *
 *
 *
 * These rows have the same height as an item from the hierarchy view.
 */
void DopeSheetViewPrivate::drawSections() const
{
    RUNNING_IN_MAIN_THREAD_AND_CONTEXT(parent);

    TreeItemsAndDSNodes treeItemsAndDSNodes = dopeSheetEditor->getTreeItemsAndDSNodes();

    // Perform drawing
    {
        GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

        for (TreeItemsAndDSNodes::const_iterator it = treeItemsAndDSNodes.begin();
             it != treeItemsAndDSNodes.end();
             ++it) {
            DSNode *dsNode = (*it).second;

            if(dsNode->getNameItem()->isHidden()) {
                continue;
            }

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            drawNodeSection(dsNode);

            TreeItemsAndDSKnobs knobItems = dsNode->getTreeItemsAndDSKnobs();
            for (TreeItemsAndDSKnobs::const_iterator it2 = knobItems.begin();
                 it2 != knobItems.end();
                 ++it2) {

                DSKnob *dsKnob = (*it2).second;

                drawKnobSection(dsKnob);
            }

            if (dsNode->isReaderNode() || dsNode->isGroupNode()) {
                drawClip(dsNode);
            }
            else {
                drawKeyframes(dsNode);
            }
        }
    }
}

void DopeSheetViewPrivate::drawClip(const DSNode *dsNode) const
{
    // Draw the clip
    {
        QColor readerFillColor = dsNode->getClipColor(DSNode::ClipFill);
        QColor readerOutlineColor = dsNode->getClipColor(DSNode::ClipOutline);

        QRectF clipRect = dsNode->getClipRect();
        QRectF clipRectZoomCoords = rectToZoomCoordinates(dsNode->getClipRect());

        GLProtectAttrib a(GL_CURRENT_BIT);

        // Fill the reader rect
        glColor4f(readerFillColor.redF(), readerFillColor.greenF(),
                  readerFillColor.blueF(), readerFillColor.alphaF());

        glBegin(GL_QUADS);
        glVertex2f(clipRect.topLeft().x(), clipRectZoomCoords.topLeft().y());
        glVertex2f(clipRect.bottomLeft().x(), clipRectZoomCoords.bottomLeft().y() + 2);
        glVertex2f(clipRect.bottomRight().x(), clipRectZoomCoords.bottomRight().y() + 2);
        glVertex2f(clipRect.topRight().x(), clipRectZoomCoords.topRight().y());
        glEnd();

        glLineWidth(2);

        // Draw the outline
        glColor4f(readerOutlineColor.redF(), readerOutlineColor.greenF(),
                  readerOutlineColor.blueF(), readerOutlineColor.alphaF());

        glBegin(GL_LINE_LOOP);
        glVertex2f(clipRect.topLeft().x(), clipRectZoomCoords.topLeft().y());
        glVertex2f(clipRect.bottomLeft().x(), clipRectZoomCoords.bottomLeft().y() + 2);
        glVertex2f(clipRect.bottomRight().x(), clipRectZoomCoords.bottomRight().y() + 2);
        glVertex2f(clipRect.topRight().x(), clipRectZoomCoords.topRight().y());
        glEnd();

        // If necessary, draw the original frame range line
        if (dsNode->isReaderNode()) {
            KnobIntPtr originalFrameRangeKnob = dynamic_cast<KnobIntPtr>
                    (dsNode->getNodeGui()->getNode()->getKnobByName("originalFrameRange").get());
            KnobIntPtr firstFrameKnob = dynamic_cast<KnobIntPtr>
                    (dsNode->getNodeGui()->getNode()->getKnobByName("firstFrame").get());
            KnobIntPtr lastFrameKnob = dynamic_cast<KnobIntPtr>
                    (dsNode->getNodeGui()->getNode()->getKnobByName("lastFrame").get());

            int framesFromEndToTotal = (originalFrameRangeKnob->getValue(1) - originalFrameRangeKnob->getValue(0))
                    - lastFrameKnob->getValue();

            float clipRectCenterY = clipRectZoomCoords.center().y();

            glLineWidth(1);

            glColor4f(readerOutlineColor.redF(), readerOutlineColor.greenF(),
                      readerOutlineColor.blueF(), readerOutlineColor.alphaF());

            glBegin(GL_LINES);
            glVertex2f(clipRect.left() - firstFrameKnob->getValue(), clipRectCenterY);
            glVertex2f(clipRect.left(), clipRectCenterY);

            glVertex2f(clipRect.right(), clipRectCenterY);
            glVertex2f(clipRect.right() + framesFromEndToTotal, clipRectCenterY);
            glEnd();
        }
    }

    // Draw the preview
    //    {
    //        if ( node->isRenderingPreview() ) {
    //            return;
    //        }

    //        int w = readerRect.width();
    //        int h = readerRect.height();

    //        size_t dataSize = 4 * w * h;
    //        {
    //#ifndef __NATRON_WIN32__
    //            unsigned int* buf = (unsigned int*)calloc(dataSize, 1);
    //#else
    //            unsigned int* buf = (unsigned int*)malloc(dataSize);
    //            for (int i = 0; i < w * h; ++i) {
    //                buf[i] = qRgba(0,0,0,255);
    //            }
    //#endif
    //            bool success = node->makePreviewImage((startingTime - lastFrame) / 2, &w, &h, buf);

    //            if (success) {
    //                QImage img(reinterpret_cast<const uchar *>(buf), w, h, QImage::Format_ARGB32);
    //                GLuint textureId = parent->bindTexture(img);

    //                parent->drawTexture(rectToZoomCoordinates(QRectF(readerRect.left(),
    //                                                                 readerRect.top(),
    //                                                                 w, h)),
    //                                    textureId);
    //            }

    //            free(buf);
    //        }
    //    }
}

void DopeSheetViewPrivate::drawProjectBounds() const
{
    RUNNING_IN_MAIN_THREAD_AND_CONTEXT(parent);

    double bottom = zoomContext.toZoomCoordinates(0, parent->height() - 1).y();
    double top = zoomContext.toZoomCoordinates(parent->width() - 1, 0).y();

    int projectStart, projectEnd;
    gui->getApp()->getFrameRange(&projectStart, &projectEnd);

    // Perform drawing
    {
        GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

        glColor4f(1.f, 1.f, 1.f, 1.f);

        // Draw start bound
        glBegin(GL_LINES);
        glVertex2f(projectStart, top);
        glVertex2f(projectStart, bottom);
        glEnd();

        // Draw end bound
        glBegin(GL_LINES);
        glVertex2f(projectEnd, top);
        glVertex2f(projectEnd, bottom);
        glEnd();
    }
}

/**
 * @brief DopeSheetViewPrivate::drawIndicator
 *
 *
 */
void DopeSheetViewPrivate::drawCurrentFrameIndicator()
{
    RUNNING_IN_MAIN_THREAD_AND_CONTEXT(parent);

    computeTimelinePositions();

    int top = zoomContext.toZoomCoordinates(0, 0).y();
    int bottom = zoomContext.toZoomCoordinates(parent->width() - 1,
                                               parent->height() - 1).y();

    int currentFrame = timeline->currentFrame();

    // Retrieve settings for drawing
    boost::shared_ptr<Settings> settings = appPTR->getCurrentSettings();
    double gridR, gridG, gridB;
    settings->getDopeSheetEditorGridColor(&gridR, &gridG, &gridB);

    // Perform drawing
    {
        GLProtectAttrib a(GL_CURRENT_BIT | GL_HINT_BIT | GL_ENABLE_BIT |
                          GL_LINE_BIT | GL_POLYGON_BIT | GL_COLOR_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);

        glColor3f(gridR, gridG, gridB);

        glBegin(GL_LINES);
        glVertex2f(currentFrame, top);
        glVertex2f(currentFrame, bottom);
        glEnd();

        glEnable(GL_POLYGON_SMOOTH);
        glHint(GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE);

        // Draw top polygon
        //        glBegin(GL_POLYGON);
        //        glVertex2f(currentTime - polyHalfWidth, top);
        //        glVertex2f(currentTime + polyHalfWidth, top);
        //        glVertex2f(currentTime, top - polyHeight);
        //        glEnd();

        // Draw bottom polygon
        glBegin(GL_POLYGON);
        glVertex2f(currentFrameIndicatorBottomPoly.at(0).x(), currentFrameIndicatorBottomPoly.at(0).y());
        glVertex2f(currentFrameIndicatorBottomPoly.at(1).x(), currentFrameIndicatorBottomPoly.at(1).y());
        glVertex2f(currentFrameIndicatorBottomPoly.at(2).x(), currentFrameIndicatorBottomPoly.at(2).y());
        glEnd();
    }
}

/**
 * @brief DopeSheetViewPrivate::drawSelectionRect
 *
 *
 */
void DopeSheetViewPrivate::drawSelectionRect() const
{
    RUNNING_IN_MAIN_THREAD_AND_CONTEXT(parent);

    QPointF topLeft = selectionRect.topLeft();
    QPointF bottomRight = selectionRect.bottomRight();

    // Perform drawing
    {
        GLProtectAttrib a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);

        glColor4f(0.3, 0.3, 0.3, 0.2);

        // Draw rect
        glBegin(GL_QUADS);
        glVertex2f(topLeft.x(), bottomRight.y());
        glVertex2f(topLeft.x(), topLeft.y());
        glVertex2f(bottomRight.x(), topLeft.y());
        glVertex2f(bottomRight.x(), bottomRight.y());
        glEnd();

        glLineWidth(1.5);

        // Draw outline
        glColor4f(0.5,0.5,0.5,1.);
        glBegin(GL_LINE_LOOP);
        glVertex2f(topLeft.x(), bottomRight.y());
        glVertex2f(topLeft.x(), topLeft.y());
        glVertex2f(bottomRight.x(), topLeft.y());
        glVertex2f(bottomRight.x(), bottomRight.y());
        glEnd();

        glCheckError();
    }
}

/**
 * @brief DopeSheetViewPrivate::drawSelectedKeysBRect
 *
 *
 */
void DopeSheetViewPrivate::drawSelectedKeysBRect() const
{
    RUNNING_IN_MAIN_THREAD_AND_CONTEXT(parent);

    QRectF bRect = rectToZoomCoordinates(selectedKeysBRect);

    // Perform drawing
    {
        GLProtectAttrib a(GL_HINT_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);

        glLineWidth(1.5);

        glColor4f(0.5, 0.5, 0.5, 1.);

        // Draw outline
        glBegin(GL_LINE_LOOP);
        glVertex2f(selectedKeysBRect.left(), bRect.bottom());
        glVertex2f(selectedKeysBRect.left(), bRect.top());
        glVertex2f(selectedKeysBRect.right(), bRect.top());
        glVertex2f(selectedKeysBRect.right(), bRect.bottom());
        glEnd();

        // Draw center cross lines
        const int CROSS_LINE_OFFSET = 10;

        QPointF bRectCenter = bRect.center();
        QPointF bRectCenterWidgetCoords = zoomContext.toWidgetCoordinates(bRectCenter.x(), bRectCenter.y());

        QLineF horizontalLine(zoomContext.toZoomCoordinates(bRectCenterWidgetCoords.x() - CROSS_LINE_OFFSET, bRectCenterWidgetCoords.y()),
                              zoomContext.toZoomCoordinates(bRectCenterWidgetCoords.x() + CROSS_LINE_OFFSET, bRectCenterWidgetCoords.y()));

        QLineF verticalLine(zoomContext.toZoomCoordinates(bRectCenterWidgetCoords.x(), bRectCenterWidgetCoords.y() - CROSS_LINE_OFFSET),
                            zoomContext.toZoomCoordinates(bRectCenterWidgetCoords.x(), bRectCenterWidgetCoords.y() + CROSS_LINE_OFFSET));

        glBegin(GL_LINES);
        glVertex2f(horizontalLine.p1().x(), horizontalLine.p1().y());
        glVertex2f(horizontalLine.p2().x(), horizontalLine.p2().y());

        glVertex2f(verticalLine.p1().x(), verticalLine.p1().y());
        glVertex2f(verticalLine.p2().x(), verticalLine.p2().y());
        glEnd();

        glCheckError();
    }
}

DSNode *DopeSheetViewPrivate::getNodeUnderMouse(const QPointF &pos) const
{
    QTreeWidgetItem *treeItem = dopeSheetEditor->getHierarchyView()->itemAt(0, pos.y());

    if (!treeItem) {
        return 0;
    }

    TreeItemsAndDSNodes dsNodeItems = dopeSheetEditor->getTreeItemsAndDSNodes();
    DSNode *dsNode = dsNodeItems[treeItem];

    return dsNode;
}

DSKnob *DopeSheetViewPrivate::getKnobUnderMouse(const QPointF &pos, int *dimension) const
{
    DSKnob *dsKnob = 0;

    QTreeWidgetItem *clickedTreeItem = dopeSheetEditor->getHierarchyView()->itemAt(0, pos.y());

    DSNode *dsNode = dopeSheetEditor->findDSNode(clickedTreeItem);

    TreeItemsAndDSKnobs treeItemsAndKnobs = dsNode->getTreeItemsAndDSKnobs();
    TreeItemsAndDSKnobs::const_iterator knobIt = treeItemsAndKnobs.find(clickedTreeItem);

    if (knobIt == treeItemsAndKnobs.end()) {
        QTreeWidgetItem *knobTreeItem = clickedTreeItem->parent();
        knobIt = treeItemsAndKnobs.find(knobTreeItem);

        if (knobIt != treeItemsAndKnobs.end()) {
            dsKnob = knobIt->second;
        }

        if (dimension) {
            if (dsKnob->isMultiDim()) {
                *dimension = knobTreeItem->indexOfChild(clickedTreeItem);
            }
        }
    }
    else {
        dsKnob = knobIt->second;

        if (dsKnob->getKnobGui()->getKnob()->getDimension() > 1) {
            *dimension = -1;
        }
        else {
            *dimension = 0;
        }
    }

    return dsKnob;
}

void DopeSheetViewPrivate::createSelectionFromCursor(DSKnob *dsKnob, const QPointF &widgetCoords, int dimension, bool booleanOp)
{
    if (!booleanOp) {
        clearCurrentKeyframeSelection();
    }

    DSSelectedKeys keys = isNearByKeyframe(dsKnob, widgetCoords, dimension);

    for (DSSelectedKeys::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        DSSelectedKey temp = (*it);

//        if (booleanOp) {
//            for (DSKeyPtrList::const_iterator it2 = keyframeSelection.begin();
//                 it2 != keyframeSelection.end(); ++it2) {
//                if (*(*it2).get() == temp) {
//                    keyframeSelection.remove(*it2);
//                }
//            }
//        }

        DSKeyPtr selected(new DSSelectedKey(temp));
        keyframeSelection.push_back(selected);
    }
}

void DopeSheetViewPrivate::createSelectionFromCursor(DSNode *dsNode, const QPointF &widgetCoords, bool booleanOp)
{
    if (!booleanOp) {
        clearCurrentKeyframeSelection();
    }

    DSSelectedKeys keys = isNearByKeyframe(dsNode, widgetCoords);

    for (DSSelectedKeys::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        DSSelectedKey temp = (*it);

        DSKeyPtr selected(new DSSelectedKey(temp));
        keyframeSelection.push_back(selected);
    }
}

void DopeSheetViewPrivate::createSelectionFromRect(const QRectF& rect, bool booleanOp)
{
    if (!booleanOp) {
        clearCurrentKeyframeSelection();
    }

    TreeItemsAndDSNodes dsNodes = dopeSheetEditor->getTreeItemsAndDSNodes();

    for (TreeItemsAndDSNodes::const_iterator it = dsNodes.begin(); it != dsNodes.end(); ++it) {
        DSNode *dsNode = (*it).second;

        TreeItemsAndDSKnobs dsKnobs = dsNode->getTreeItemsAndDSKnobs();

        for (TreeItemsAndDSKnobs::const_iterator it2 = dsKnobs.begin(); it2 != dsKnobs.end(); ++it2) {
            DSKnob *dsKnob = (*it2).second;
            KnobGui *knobGui = dsKnob->getKnobGui();

            for (int i = 0; i < knobGui->getKnob()->getDimension(); ++i) {
                KeyFrameSet keyframes = knobGui->getCurve(i)->getKeyFrames_mt_safe();

                for (KeyFrameSet::const_iterator kIt = keyframes.begin();
                     kIt != keyframes.end();
                     ++kIt) {
                    KeyFrame kf = (*kIt);

                    double sectionCenterY = (dsKnob->isMultiDim()) ? dsKnob->getNameItemRectForDim(i).center().y()
                                                                   : dsKnob->getNameItemRect().center().y();

                    double x = kf.getTime();

                    if ((rect.left() <= x) && (rect.right() >= x)
                            && (rect.top() >= sectionCenterY) && (rect.bottom() <= sectionCenterY)) {
                        DSKeyPtr selected(new DSSelectedKey(dsKnob, kf, i));
                        keyframeSelection.push_back(selected);
                    }
                }
            }
        }
    }
}

bool DopeSheetViewPrivate::keyframeIsAlreadyInSelection(DSSelectedKey *key) const
{

}

DopeSheetView::EventStateEnum DopeSheetViewPrivate::getEventStateDuringHover(const QPointF &widgetCoords) const
{
    DopeSheetView::EventStateEnum es = DopeSheetView::esNoEditingState;

    QPointF zoomCoords = zoomContext.toZoomCoordinates(widgetCoords.x(), widgetCoords.y());

    // Does the user hovering the keyframe selection bounding rect ?
    QRectF selectedKeysBRectZoomCoords = rectToZoomCoordinates(selectedKeysBRect);

    if (selectedKeysBRectZoomCoords.isValid() && selectedKeysBRectZoomCoords.contains(zoomCoords)) {
        es = DopeSheetView::esMoveKeyframeSelection;
    }
    // Or does he hovering the current frame indicator ?
    else if (isNearByCurrentFrameIndicatorBottom(zoomCoords)) {
        es = DopeSheetView::esMoveCurrentFrameIndicator;
    }
    else if (QTreeWidgetItem *treeItem = dopeSheetEditor->getHierarchyView()->itemAt(0, widgetCoords.y())) {
        TreeItemsAndDSNodes dsNodeItems = dopeSheetEditor->getTreeItemsAndDSNodes();
        TreeItemsAndDSNodes::const_iterator dsNodeIt = dsNodeItems.find(treeItem);

        if (dsNodeIt != dsNodeItems.end()) {
            DSNode *dsNode = (*dsNodeIt).second;
            QRectF nodeClipRect = rectToZoomCoordinates(dsNode->getClipRect());

            if (dsNode->isReaderNode() || dsNode->isGroupNode()) {
                if (nodeClipRect.contains(zoomCoords.x(), zoomCoords.y())) {
                    if (dsNode->isReaderNode()) {
                        if (isNearByClipRectLeft(zoomCoords.x(), nodeClipRect)) {
                            es = DopeSheetView::esReaderLeftTrim;
                        }
                        else if (isNearByClipRectRight(zoomCoords.x(), nodeClipRect)) {
                            es = DopeSheetView::esReaderRightTrim;
                        }
                        //                        else if (zoomCoords.y() <= nodeClipRect.bottom() + 10) {
                        //                            es = DopeSheetView::esReaderSlip;
                        //                        }
                        else {
                            es = DopeSheetView::esClipRepos;
                        }
                    }
                }
            }
            else {
                DSSelectedKeys keysUnderMouse = isNearByKeyframe(dsNode, widgetCoords);

                if (!keysUnderMouse.empty()) {
                    es = DopeSheetView::esMoveSingleKeyframe;
                }
            }
        }
        else {
            int knobDim;
            DSKnob *dsKnob = getKnobUnderMouse(zoomContext.toWidgetCoordinates(zoomCoords.x(), zoomCoords.y()), &knobDim);

            DSSelectedKeys keysUnderMouse = isNearByKeyframe(dsKnob, widgetCoords, knobDim);

            if (!keysUnderMouse.empty()) {
                es = DopeSheetView::esMoveSingleKeyframe;
            }
        }
    }

    if (es == DopeSheetView::esNoEditingState) {
        es = DopeSheetView::esSelectionByRect;
    }

    return es;
}

bool DopeSheetViewPrivate::isNearByClipRectLeft(double time, const QRectF &clipRect) const
{
    return ((time >= clipRect.left() - CLICK_DISTANCE_ACCEPTANCE)
            && (time <= clipRect.left() + CLICK_DISTANCE_ACCEPTANCE));
}

bool DopeSheetViewPrivate::isNearByClipRectRight(double time, const QRectF &clipRect) const
{
    return ((time >= clipRect.right() - CLICK_DISTANCE_ACCEPTANCE)
            && (time <= clipRect.right() + CLICK_DISTANCE_ACCEPTANCE));
}

bool DopeSheetViewPrivate::isNearByCurrentFrameIndicatorBottom(const QPointF &zoomCoords) const
{
    return (currentFrameIndicatorBottomPoly.containsPoint(zoomCoords, Qt::OddEvenFill));
}

DSSelectedKeys DopeSheetViewPrivate::isNearByKeyframe(DSKnob *dsKnob, const QPointF &widgetCoords, int dimension) const
{
    DSSelectedKeys ret;

    boost::shared_ptr<KnobI> knob = dsKnob->getKnobGui()->getKnob();

    int startDim = 0;
    int endDim = knob->getDimension();

    if (dimension > -1) {
        startDim = dimension;
        endDim = dimension + 1;
    }

    for (int i = startDim; i < endDim; ++i) {
        KeyFrameSet keyframes = knob->getCurve(i)->getKeyFrames_mt_safe();

        for (KeyFrameSet::const_iterator kIt = keyframes.begin();
             kIt != keyframes.end();
             ++kIt) {
            KeyFrame kf = (*kIt);

            QPointF keyframeWidgetPos = zoomContext.toWidgetCoordinates(kf.getTime(), 0);

            if (std::abs(widgetCoords.x() - keyframeWidgetPos.x()) < CLICK_DISTANCE_ACCEPTANCE) {
                DSSelectedKey key(dsKnob, kf, i);
                ret.push_back(key);
            }
        }
    }

    return ret;
}

DSSelectedKeys DopeSheetViewPrivate::isNearByKeyframe(DSNode *dsNode, const QPointF &widgetCoords) const
{
    DSSelectedKeys ret;

    TreeItemsAndDSKnobs dsKnobs = dsNode->getTreeItemsAndDSKnobs();

    for (TreeItemsAndDSKnobs::const_iterator it = dsKnobs.begin(); it != dsKnobs.end(); ++it) {
        DSKnob *dsKnob = (*it).second;
        KnobGui *knobGui = dsKnob->getKnobGui();

        for (int i = 0; i < knobGui->getKnob()->getDimension(); ++i) {
            KeyFrameSet keyframes = knobGui->getCurve(i)->getKeyFrames_mt_safe();

            for (KeyFrameSet::const_iterator kIt = keyframes.begin();
                 kIt != keyframes.end();
                 ++kIt) {
                KeyFrame kf = (*kIt);

                QPointF keyframeWidgetPos = zoomContext.toWidgetCoordinates(kf.getTime(), 0);

                if (std::abs(widgetCoords.x() - keyframeWidgetPos.x()) < CLICK_DISTANCE_ACCEPTANCE) {
                    DSSelectedKey key(dsKnob, kf, i);
                    ret.push_back(key);
                }
            }
        }
    }

    return ret;
}

void DopeSheetViewPrivate::clearCurrentClipSelection()
{
    if (currentNodeClipSelected) {
        currentNodeClipSelected->setSelected(false);
        currentNodeClipSelected = 0;
    }
}

void DopeSheetViewPrivate::clearCurrentKeyframeSelection()
{
    keyframeSelection.clear();
}

Qt::CursorShape DopeSheetViewPrivate::getCursorForEditingState(DopeSheetView::EventStateEnum es) const
{
    Qt::CursorShape cursorShape;

    switch (es) {
    case DopeSheetView::esClipRepos:
    case DopeSheetView::esMoveSingleKeyframe:
    case DopeSheetView::esMoveKeyframeSelection:
        cursorShape = Qt::OpenHandCursor;
        break;
    case DopeSheetView::esReaderLeftTrim:
    case DopeSheetView::esMoveCurrentFrameIndicator:
        cursorShape = Qt::SplitHCursor;
        break;
    case DopeSheetView::esReaderRightTrim:
        cursorShape = Qt::SplitHCursor;
        break;
    case DopeSheetView::esReaderSlip:
        cursorShape = Qt::SizeHorCursor;
        break;
    default:
        cursorShape = Qt::ArrowCursor;
        break;
    }

    return cursorShape;
}

void DopeSheetViewPrivate::computeTimelinePositions()
{
    RUNNING_IN_MAIN_THREAD();

    double polyHalfWidth = 7.5;
    double polyHeight = 7.5;

    int bottom = zoomContext.toZoomCoordinates(parent->width() - 1,
                                               parent->height() - 1).y();

    int currentFrame = timeline->currentFrame();

    QPointF bottomCursorBottom(currentFrame, bottom);
    QPointF bottomCursorBottomWidgetCoords = zoomContext.toWidgetCoordinates(bottomCursorBottom.x(), bottomCursorBottom.y());

    QPointF leftPoint = zoomContext.toZoomCoordinates(bottomCursorBottomWidgetCoords.x() - polyHalfWidth, bottomCursorBottomWidgetCoords.y());
    QPointF rightPoint = zoomContext.toZoomCoordinates(bottomCursorBottomWidgetCoords.x() + polyHalfWidth, bottomCursorBottomWidgetCoords.y());
    QPointF topPoint = zoomContext.toZoomCoordinates(bottomCursorBottomWidgetCoords.x(), bottomCursorBottomWidgetCoords.y() - polyHeight);

    currentFrameIndicatorBottomPoly.clear();

    currentFrameIndicatorBottomPoly << leftPoint
                                    << rightPoint
                                    << topPoint;
}

void DopeSheetViewPrivate::computeSelectionRect(const QPointF &origin, const QPointF &current)
{
    double xmin = std::min(origin.x(), current.x());
    double xmax = std::max(origin.x(), current.x());
    double ymin = std::min(origin.y(), current.y());
    double ymax = std::max(origin.y(), current.y());

    selectionRect.setTopLeft(QPointF(xmin, ymin));
    selectionRect.setBottomRight(QPointF(xmax, ymax));
}

void DopeSheetViewPrivate::pushUndoCommand(QUndoCommand *cmd)
{
    undoStack->setActive();
    undoStack->push(cmd);
}

void DopeSheetViewPrivate::moveCurrentFrameIndicator(double toTime)
{
    gui->getApp()->setLastViewerUsingTimeline(boost::shared_ptr<Natron::Node>());

    timeline->seekFrame(SequenceTime(toTime), false, 0, Natron::eTimelineChangeReasonDopeSheetEditorSeek);
}

void DopeSheetViewPrivate::deleteSelectedKeyframes()
{
    RUNNING_IN_MAIN_THREAD();

    if (keyframeSelection.empty()) {
        return;
    }

    selectedKeysBRect = QRectF();

    std::vector<DSSelectedKey> toRemove;
    for (DSKeyPtrList::iterator it = keyframeSelection.begin(); it != keyframeSelection.end(); ++it) {
        toRemove.push_back(DSSelectedKey(**it));
    }

    pushUndoCommand(new DSRemoveKeysCommand(toRemove, parent));

    keyframeSelection.clear();

    parent->update();
}

void DopeSheetViewPrivate::findKeyframeBounds(int *minTime, int *maxTime)
{
    int min = 0;
    int max = 0;

    TreeItemsAndDSNodes dsNodeItems = dopeSheetEditor->getTreeItemsAndDSNodes();

    for (TreeItemsAndDSNodes::const_iterator it = dsNodeItems.begin(); it != dsNodeItems.end(); ++it) {
        if ((*it).first->isHidden()) {
            continue;
        }

        DSNode *dsNode = (*it).second;

        TreeItemsAndDSKnobs dsKnobItems = dsNode->getTreeItemsAndDSKnobs();

        for (TreeItemsAndDSKnobs::const_iterator itKnob = dsKnobItems.begin(); itKnob != dsKnobItems.end(); ++itKnob) {
            if ((*itKnob).first->isHidden()) {
                continue;
            }

            DSKnob *dsKnob = (*itKnob).second;

            for (int i = 0; i < dsKnob->getKnobGui()->getKnob()->getDimension(); ++i) {
                KeyFrameSet keyframes = dsKnob->getKnobGui()->getCurve(i)->getKeyFrames_mt_safe();

                int firstTime = keyframes.begin()->getTime();
                int lastTime = keyframes.rbegin()->getTime();

                min = std::min(min, firstTime);
                max = std::max(max, lastTime);
            }
        }
    }

    *minTime = min;
    *maxTime = max;
}

void DopeSheetViewPrivate::frame()
{
    RUNNING_IN_MAIN_THREAD();

    if (keyframeSelection.size() == 1) {
        return;
    }

    int left, right;

    // frame on project bounds
    if (keyframeSelection.empty()) {
        findKeyframeBounds(&left, &right);
    }
    // or frame on current selection
    else {
        left = selectedKeysBRect.left();
        right = selectedKeysBRect.right();
    }

    zoomContext.fill(left, right,
                     zoomContext.bottom(), zoomContext.top());

    computeTimelinePositions();

    if (keyframeSelection.size() > 1) {
        parent->computeSelectedKeysBRect();
    }

    parent->update();
}

/**
 * @class DopeSheetView
 *
 *
 */

/**
 * @brief DopeSheetView::DopeSheetView
 *
 * Constructs a DopeSheetView object.
 */
DopeSheetView::DopeSheetView(DopeSheetEditor *dopeSheetEditor, Gui *gui, boost::shared_ptr<TimeLine> timeline, QWidget *parent) :
    QGLWidget(parent),
    _imp(new DopeSheetViewPrivate(this))
{
    _imp->dopeSheetEditor = dopeSheetEditor;

    _imp->gui = gui;
    _imp->gui->registerNewUndoStack(_imp->undoStack.get());

    _imp->timeline = timeline;

    setMouseTracking(true);

    if (timeline) {
        boost::shared_ptr<Natron::Project> project = gui->getApp()->getProject();
        assert(project);

        connect(timeline.get(), SIGNAL(frameChanged(SequenceTime,int)), this, SLOT(onTimeLineFrameChanged(SequenceTime,int)));
        connect(project.get(), SIGNAL(frameRangeChanged(int,int)), this, SLOT(onTimeLineBoundariesChanged(int,int)));

        onTimeLineFrameChanged(timeline->currentFrame(), Natron::eValueChangedReasonNatronGuiEdited);

        int left,right;
        project->getFrameRange(&left, &right);
        onTimeLineBoundariesChanged(left, right);
    }
}

/**
 * @brief DopeSheetView::~DopeSheetView
 *
 * Destroys the DopeSheetView object.
 */
DopeSheetView::~DopeSheetView()
{

}

/**
 * @brief DopeSheetView::swapOpenGLBuffers
 *
 *
 */
void DopeSheetView::swapOpenGLBuffers()
{
    RUNNING_IN_MAIN_THREAD();

    swapBuffers();
}

/**
 * @brief DopeSheetView::redraw
 *
 *
 */
void DopeSheetView::redraw()
{
    RUNNING_IN_MAIN_THREAD();

    update();
}

/**
 * @brief DopeSheetView::getViewportSize
 *
 *
 */
void DopeSheetView::getViewportSize(double &width, double &height) const
{
    RUNNING_IN_MAIN_THREAD();

    width = this->width();
    height = this->height();
}

/**
 * @brief DopeSheetView::getPixelScale
 *
 *
 */
void DopeSheetView::getPixelScale(double &xScale, double &yScale) const
{
    RUNNING_IN_MAIN_THREAD();

    xScale = _imp->zoomContext.screenPixelWidth();
    yScale = _imp->zoomContext.screenPixelHeight();
}

/**
 * @brief DopeSheetView::getBackgroundColour
 *
 *
 */
void DopeSheetView::getBackgroundColour(double &r, double &g, double &b) const
{
    RUNNING_IN_MAIN_THREAD();

    // use the same settings as the curve editor
    appPTR->getCurrentSettings()->getCurveEditorBGColor(&r, &g, &b);
}

/**
 * @brief DopeSheetView::saveOpenGLContext
 *
 *
 */
void DopeSheetView::saveOpenGLContext()
{
    RUNNING_IN_MAIN_THREAD();


}

/**
 * @brief DopeSheetView::restoreOpenGLContext
 *
 *
 */
void DopeSheetView::restoreOpenGLContext()
{
    RUNNING_IN_MAIN_THREAD();

}

/**
 * @brief DopeSheetView::getCurrentRenderScale
 *
 *
 */
unsigned int DopeSheetView::getCurrentRenderScale() const
{
    return 0;
}

void DopeSheetView::computeSelectedKeysBRect()
{
    if (_imp->keyframeSelection.size() <= 1) {
        _imp->selectedKeysBRect = QRectF();

        return;
    }

    const int SELECTED_KF_BBOX_BOUNDS_OFFSET = 4;

    QRectF rect;

    for (DSKeyPtrList::const_iterator it = _imp->keyframeSelection.begin();
         it != _imp->keyframeSelection.end();
         ++it) {
        DSKeyPtr selected = (*it);

        double x = selected->key.getTime();
        double y = (selected->dsKnob->isMultiDim()) ? selected->dsKnob->getNameItemRectForDim(selected->dimension).center().y()
                                                    : selected->dsKnob->getNameItemRect().center().y();

        if (it != _imp->keyframeSelection.begin()) {
            if (x < rect.left()) {
                rect.setLeft(x);
            }

            if (x > rect.right()) {
                rect.setRight(x);
            }

            if (y > rect.top()) {
                rect.setTop(y);
            }

            if (y < rect.bottom()) {
                rect.setBottom(y);
            }
        }
        else {
            rect.setLeft(x);
            rect.setRight(x);
            rect.setTop(y);
            rect.setBottom(y);
        }
    }

    QPointF topLeft(rect.left(), rect.top());
    QPointF bottomRight(rect.right(), rect.bottom());

    _imp->selectedKeysBRect.setTopLeft(topLeft);
    _imp->selectedKeysBRect.setBottomRight(bottomRight);

    if (!_imp->selectedKeysBRect.isNull()) {
        _imp->selectedKeysBRect.adjust(-SELECTED_KF_BBOX_BOUNDS_OFFSET, SELECTED_KF_BBOX_BOUNDS_OFFSET,
                                       SELECTED_KF_BBOX_BOUNDS_OFFSET, -SELECTED_KF_BBOX_BOUNDS_OFFSET);
    }
}

void DopeSheetView::onTimeLineFrameChanged(SequenceTime sTime, int reason)
{
    Q_UNUSED(sTime);
    Q_UNUSED(reason);

    RUNNING_IN_MAIN_THREAD();

    if (_imp->gui->isGUIFrozen()) {
        return;
    }

    _imp->computeTimelinePositions();

    update();
}

void DopeSheetView::onTimeLineBoundariesChanged(int, int)
{
    RUNNING_IN_MAIN_THREAD();

    update();
}

/**
 * @brief DopeSheetView::initializeGL
 *
 *
 */
void DopeSheetView::initializeGL()
{
    RUNNING_IN_MAIN_THREAD();

    if ( !glewIsSupported("GL_ARB_vertex_array_object ")) {
        _imp->hasOpenGLVAOSupport = false;
    }
}

/**
 * @brief DopeSheetView::resizeGL
 *
 *
 */
void DopeSheetView::resizeGL(int w, int h)
{
    RUNNING_IN_MAIN_THREAD_AND_CONTEXT(this);

    if (h == 0) {

    }

    glViewport(0, 0, w, h);

    _imp->zoomContext.setScreenSize(w, h);

    // Don't do the following when the height of the widget is irrelevant
    if (h == 1) {
        return;
    }

    // Find out what are the selected keyframes and center on them
    if (!_imp->zoomOrPannedSinceLastFit) {
        //TODO see CurveWidget::resizeGL
    }
}

/**
 * @brief DopeSheetView::paintGL
 *
 *
 */
void DopeSheetView::paintGL()
{
    RUNNING_IN_MAIN_THREAD_AND_CONTEXT(this);

    glCheckError();

    if (_imp->zoomContext.factor() <= 0) {
        return;
    }

    double zoomLeft, zoomRight, zoomBottom, zoomTop;
    zoomLeft = _imp->zoomContext.left();
    zoomRight = _imp->zoomContext.right();
    zoomBottom = _imp->zoomContext.bottom();
    zoomTop = _imp->zoomContext.top();

    // Retrieve the appropriate settings for drawing
    boost::shared_ptr<Settings> settings = appPTR->getCurrentSettings();
    double bgR, bgG, bgB;
    settings->getDopeSheetEditorBackgroundColor(&bgR, &bgG, &bgB);

    if ((zoomLeft == zoomRight) || (zoomTop == zoomBottom)) {
        glClearColor(bgR, bgG, bgB, 1.);
        glClear(GL_COLOR_BUFFER_BIT);

        return;
    }

    {
        GLProtectAttrib a(GL_TRANSFORM_BIT | GL_COLOR_BUFFER_BIT);
        GLProtectMatrix p(GL_PROJECTION);

        glLoadIdentity();
        glOrtho(zoomLeft, zoomRight, zoomBottom, zoomTop, 1, -1);

        GLProtectMatrix m(GL_MODELVIEW);

        glLoadIdentity();

        glCheckError();

        glClearColor(bgR, bgG, bgB, 1.);
        glClear(GL_COLOR_BUFFER_BIT);

        _imp->drawScale();
        _imp->drawSections();

        if (_imp->eventState == DopeSheetView::esSelectionByRect) {
            _imp->drawSelectionRect();
        }

        if (_imp->rectToZoomCoordinates(_imp->selectedKeysBRect).isValid()) {
            _imp->drawSelectedKeysBRect();
        }

        _imp->drawProjectBounds();
        _imp->drawCurrentFrameIndicator();
    }
}

void DopeSheetView::mousePressEvent(QMouseEvent *e)
{
    RUNNING_IN_MAIN_THREAD();

    if (buttonDownIsMiddle(e)) {
        _imp->couldBeNextEventState = DopeSheetView::esDraggingView;
    }

    QPointF clickZoomCoords = _imp->zoomContext.toZoomCoordinates(e->x(), e->y());

    bool leftClickedInBackground = true;

    if (_imp->couldBeNextEventState == DopeSheetView::esMoveKeyframeSelection) {
        leftClickedInBackground = false;
    }
    else if (buttonDownIsLeft(e)) {
        if (QTreeWidgetItem *treeItem = _imp->dopeSheetEditor->getHierarchyView()->itemAt(0, e->y())) {
            TreeItemsAndDSNodes dsNodeItems = _imp->dopeSheetEditor->getTreeItemsAndDSNodes();
            DSNode *dsNode = dsNodeItems[treeItem];

            if (dsNode) {
                // Clicked on a reader
                if (_imp->couldBeNextEventState == DopeSheetView::esClipRepos
                        || _imp->couldBeNextEventState == DopeSheetView::esReaderLeftTrim
                        || _imp->couldBeNextEventState == DopeSheetView::esReaderRightTrim
                        || _imp->couldBeNextEventState == DopeSheetView::esReaderSlip) {
                    _imp->currentEditedReader = dsNode;

                    KnobIntPtr timeOffsetKnob = dynamic_cast<KnobIntPtr>
                            (_imp->currentEditedReader->getNodeGui()->getNode()->getKnobByName("timeOffset").get());

                    _imp->lastTimeOffsetOnMousePress = timeOffsetKnob->getValue();

                    leftClickedInBackground = false;
                }

                // Clicked on a keyframe in the section of a common node
                if (dsNode->isCommonNode()) {
                    _imp->createSelectionFromCursor(dsNode, e->pos(), modCASIsControl(e));

                    computeSelectedKeysBRect();

                    if (!_imp->keyframeSelection.empty() && !modCASIsControl(e)) {
                        leftClickedInBackground = false;
                    }
                }
            }
            else {
                QRectF selectedKeysBRectZoomCoords = _imp->rectToZoomCoordinates(_imp->selectedKeysBRect);

                if (!selectedKeysBRectZoomCoords.contains(clickZoomCoords)) {
                    int knobDim;
                    DSKnob *dsKnob = _imp->getKnobUnderMouse(e->pos(), &knobDim);

                    if (dsKnob) {
                        _imp->createSelectionFromCursor(dsKnob, e->pos(), knobDim, modCASIsControl(e));

                        computeSelectedKeysBRect();

                        if (!_imp->keyframeSelection.empty() && !modCASIsControl(e)) {
                            leftClickedInBackground = false;
                        }
                    }
                }
            }
        }
    }
    else if (_imp->couldBeNextEventState == DopeSheetView::esMoveCurrentFrameIndicator ||
            _imp->couldBeNextEventState == DopeSheetView::esDraggingView) {
        leftClickedInBackground = false;
    }

    if (leftClickedInBackground) {
        if (!modCASIsControl(e)) {
            _imp->clearCurrentClipSelection();
            _imp->clearCurrentKeyframeSelection();

            _imp->selectedKeysBRect = QRectF();
        }

        _imp->couldBeNextEventState = DopeSheetView::esSelectionByRect;
        _imp->selectionRect.setTopLeft(clickZoomCoords);
        _imp->selectionRect.setBottomRight(clickZoomCoords);
    }

    if (_imp->couldBeNextEventState != DopeSheetView::esNoEditingState) {
        _imp->eventState = _imp->couldBeNextEventState;
    }

    _imp->lastPosOnMousePress = e->pos();
    _imp->keyDragLastMovement = 0.;

    update();
}

void DopeSheetView::mouseMoveEvent(QMouseEvent *e)
{
    RUNNING_IN_MAIN_THREAD();

    QPointF mouseZoomCoords = _imp->zoomContext.toZoomCoordinates(e->x(), e->y());

    if (e->buttons() == Qt::NoButton) {
        DopeSheetView::EventStateEnum es = _imp->getEventStateDuringHover(e->pos());

        setCursor(_imp->getCursorForEditingState(es));

        _imp->couldBeNextEventState = es;
    }
    else if (buttonDownIsLeft(e)) {
        mouseDragEvent(e);
    }
    else if (buttonDownIsMiddle(e)) {
        double dx = _imp->zoomContext.toZoomCoordinates(_imp->lastPosOnMouseMove.x(),
                                                        _imp->lastPosOnMouseMove.y()).x() - mouseZoomCoords.x();
        _imp->zoomContext.translate(dx, 0);
    }

    _imp->lastPosOnMouseMove = e->pos();

    update();
}

void DopeSheetView::mouseReleaseEvent(QMouseEvent *e)
{
    Q_UNUSED(e);

    if (_imp->eventState == DopeSheetView::esSelectionByRect) {
        if (_imp->keyframeSelection.size() > 1) {
            computeSelectedKeysBRect();
        }

        _imp->selectionRect = QRectF();
    }

    if (_imp->eventState != esNoEditingState) {
        _imp->eventState = esNoEditingState;

        if (_imp->currentEditedReader) {
            _imp->currentEditedReader = 0;
        }
    }

    update();
}

void DopeSheetView::mouseDragEvent(QMouseEvent *e)
{
    QPointF mouseZoomCoords = _imp->zoomContext.toZoomCoordinates(e->x(), e->y());
    QPointF lastZoomCoordsOnMousePress = _imp->zoomContext.toZoomCoordinates(_imp->lastPosOnMousePress.x(),
                                                                             _imp->lastPosOnMousePress.y());
    double currentTime = mouseZoomCoords.x();

    switch (_imp->eventState) {
    case DopeSheetView::esMoveSingleKeyframe:
    case DopeSheetView::esMoveKeyframeSelection:
    {
        double totalMovement = currentTime - lastZoomCoordsOnMousePress.x();
        // Clamp the motion to the nearet integer
        totalMovement = std::floor(totalMovement + 0.5);

        double dt = totalMovement - _imp->keyDragLastMovement;

        _imp->pushUndoCommand(new DSMoveKeysCommand(_imp->keyframeSelection, dt, this));

        // Update the last drag movement
        _imp->keyDragLastMovement = totalMovement;

        break;
    }
    case DopeSheetView::esMoveCurrentFrameIndicator:
        _imp->moveCurrentFrameIndicator(mouseZoomCoords.x());

        break;
    case DopeSheetView::esSelectionByRect:
        _imp->computeSelectionRect(lastZoomCoordsOnMousePress, mouseZoomCoords);
        _imp->createSelectionFromRect(_imp->rectToZoomCoordinates(_imp->selectionRect), modCASIsControl(e));

        break;
    case DopeSheetView::esReaderLeftTrim:
    {
        KnobIntPtr timeOffsetKnob = dynamic_cast<KnobIntPtr>
                (_imp->currentEditedReader->getNodeGui()->getNode()->getKnobByName("timeOffset").get());
        KnobIntPtr firstFrameKnob = dynamic_cast<KnobIntPtr>
                (_imp->currentEditedReader->getNodeGui()->getNode()->getKnobByName("firstFrame").get());

        double newTime = (currentTime - timeOffsetKnob->getValue());

        _imp->pushUndoCommand(new DSLeftTrimReaderCommand(_imp->currentEditedReader, firstFrameKnob->getValue(), newTime, this));

        break;
    }
    case DopeSheetView::esReaderRightTrim:
    {
        KnobIntPtr timeOffsetKnob = dynamic_cast<KnobIntPtr>
                (_imp->currentEditedReader->getNodeGui()->getNode()->getKnobByName("timeOffset").get());
        KnobIntPtr lastFrameKnob = dynamic_cast<KnobIntPtr>
                (_imp->currentEditedReader->getNodeGui()->getNode()->getKnobByName("lastFrame").get());

        double newTime = (currentTime - timeOffsetKnob->getValue());

        _imp->pushUndoCommand(new DSRightTrimReaderCommand(_imp->currentEditedReader, lastFrameKnob->getValue(), newTime, this));

        break;
    }
    case DopeSheetView::esClipRepos:
    {
        KnobIntPtr timeOffsetKnob = dynamic_cast<KnobIntPtr>
                (_imp->currentEditedReader->getNodeGui()->getNode()->getKnobByName("timeOffset").get());

        int mouseOffset = (lastZoomCoordsOnMousePress.x() - _imp->lastTimeOffsetOnMousePress);
        double newTime = (currentTime - mouseOffset);

        _imp->pushUndoCommand(new DSMoveReaderCommand(_imp->currentEditedReader, timeOffsetKnob->getValue(), newTime, this));

        break;
    }
    default:
        break;
    }
}

void DopeSheetView::wheelEvent(QWheelEvent *e)
{
    RUNNING_IN_MAIN_THREAD();

    // don't handle horizontal wheel (e.g. on trackpad or Might Mouse)
    if (e->orientation() != Qt::Vertical) {
        return;
    }

    const double zoomFactor_min = 0.0001;
    const double zoomFactor_max = 10000.;
    const double par_min = 0.0001;
    const double par_max = 10000.;
    double zoomFactor;
    double par;
    double scaleFactor = std::pow(NATRON_WHEEL_ZOOM_PER_DELTA, e->delta());
    QPointF zoomCenter = _imp->zoomContext.toZoomCoordinates(e->x(), e->y());

    _imp->zoomOrPannedSinceLastFit = true;
    // Alt + Wheel: zoom time only, keep point under mouse
    par = _imp->zoomContext.aspectRatio() * scaleFactor;

    if (par <= par_min) {
        par = par_min;
        scaleFactor = par / _imp->zoomContext.aspectRatio();
    }
    else if (par > par_max) {
        par = par_max;
        scaleFactor = par / _imp->zoomContext.factor();
    }

    _imp->zoomContext.zoomx(zoomCenter.x(), zoomCenter.y(), scaleFactor);

    _imp->dopeSheetEditor->refreshClipRects();
    computeSelectedKeysBRect();

    update();
}

void DopeSheetView::enterEvent(QEvent *e)
{
    RUNNING_IN_MAIN_THREAD();

    QWidget *currentFocus = qApp->focusWidget();

    bool canSetFocus = !currentFocus ||
            dynamic_cast<ViewerGL *>(currentFocus) ||
            dynamic_cast<DopeSheetView *>(currentFocus) ||
            dynamic_cast<Histogram *>(currentFocus) ||
            dynamic_cast<NodeGraph *>(currentFocus) ||
            dynamic_cast<QToolButton *>(currentFocus) ||
            currentFocus->objectName() == "Properties" ||
            currentFocus->objectName() == "tree" ||
            currentFocus->objectName() == "SettingsPanel" ||
            currentFocus->objectName() == "qt_tabwidget_tabbar";

    if (canSetFocus) {
        setFocus();
    }

    QGLWidget::enterEvent(e);
}

void DopeSheetView::focusInEvent(QFocusEvent *e)
{
    QGLWidget::focusInEvent(e);

    _imp->undoStack->setActive();
}

void DopeSheetView::keyPressEvent(QKeyEvent *e)
{
    RUNNING_IN_MAIN_THREAD();

    Qt::KeyboardModifiers modifiers = e->modifiers();
    Qt::Key key = Qt::Key(e->key());

    if (isKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionDopeSheetEditorDeleteKey, modifiers, key)) {
        _imp->deleteSelectedKeyframes();
    }
    else if (isKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionDopeSheetEditorFrameSelection, modifiers, key)) {
        _imp->frame();
    }
}

/**
* @brief DopeSheetView::renderText
*
*
*/
void DopeSheetView::renderText(double x, double y,
                               const QString &text,
                               const QColor &color,
                               const QFont &font) const
{
    RUNNING_IN_MAIN_THREAD_AND_CONTEXT(this);

    if ( text.isEmpty() ) {
        return;
    }

    double w = double(width());
    double h = double(height());

    double bottom = _imp->zoomContext.bottom();
    double left = _imp->zoomContext.left();
    double top =  _imp->zoomContext.top();
    double right = _imp->zoomContext.right();

    if (w <= 0 || h <= 0 || right <= left || top <= bottom) {
        return;
    }

    double scalex = (right-left) / w;
    double scaley = (top-bottom) / h;

    _imp->textRenderer.renderText(x, y, scalex, scaley, text, color, font);

    glCheckError();
}
