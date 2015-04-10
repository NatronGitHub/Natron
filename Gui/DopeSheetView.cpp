#include "DopeSheetView.h"

#include <algorithm>

// Qt includes
#include <QApplication>
#include <QThread>
#include <QTreeWidget>

// Natron includes
#include "Engine/Curve.h"
#include "Engine/Image.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"

#include "Gui/DockablePanel.h"
#include "Gui/DopeSheet.h"
#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/KnobGui.h"
#include "Gui/NodeGui.h"
#include "Gui/TextRenderer.h"
#include "Gui/ticks.h"
#include "Gui/ZoomContext.h"


// Some macros to ensure the following operations always running in the main thread
#define RUNNING_IN_MAIN_THREAD() \
    assert(qApp && qApp->thread() == QThread::currentThread())

// Note : if you want to call the following functions from a pimpl class, ensure it have its
// parent as attribute and pass its name as parameter, or pass 'this' if
// it's called from a pimpled class.
#define RUNNING_IN_MAIN_CONTEXT(glContextOwner) \
    assert(QGLContext::currentContext() == glContextOwner->context())

#define RUNNING_IN_MAIN_THREAD_AND_CONTEXT(glContextOwner) \
    RUNNING_IN_MAIN_THREAD();\
    RUNNING_IN_MAIN_CONTEXT(glContextOwner)

typedef std::set<double> TimeSet;

////////////////////////// Helpers //////////////////////////

namespace {


} // anon namespace


////////////////////////// DopeSheetView //////////////////////////

class DopeSheetViewPrivate
{
public:
    DopeSheetViewPrivate(DopeSheetView *qq);

    /* functions */
    QRectF rectToZoomCoordinates(const QRectF &rect) const;

    void drawScale() const;

    // Helpers
    QRectF nameItemRectToSectionRect(const QRectF &rect) const;
    bool curveHasKeyframeAtTime(const boost::shared_ptr<Curve> &curve, double time) const;

    // Drawing
    void drawSections() const;

    void drawNodeSection(const DSNode *dsNode) const;
    void drawKnobSection(const DSKnob *dsKnob) const;

    void drawReader(const DSNode *dsNode) const;
    void drawGroup(const DSNode *dsNode) const;
    void drawKeyframes(DSNode *dsNode) const;

    void drawProjectBounds() const;
    void drawCurrentFrameIndicator() const;

    void drawSelectionRect() const;

    void drawSelectedKeysRect() const;

    /* attributes */
    DopeSheetView *parent;

    Gui *gui;

    // Necessary to retrieve some useful values for drawing
    boost::shared_ptr<TimeLine> timeline;
    DopeSheetEditor *dopeSheetEditor;

    // for rendering
    QFont *font;
    Natron::TextRenderer textRenderer;

    // for navigating
    ZoomContext zoomContext;
    bool zoomOrPannedSinceLastFit;

    // for keyframe selection
    QRectF selectionRect;
    QRectF currentSelectedKeysRect;

    // others
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
    selectionRect(),
    currentSelectedKeysRect(),
    hasOpenGLVAOSupport(true)
{}

/**
 * @brief DopeSheetViewPrivate::rectToZoomCoordinates
 *
 *
 */
QRectF DopeSheetViewPrivate::rectToZoomCoordinates(const QRectF &rect) const
{
    QPointF topLeft = zoomContext.toZoomCoordinates(rect.topLeft().x(),
                                                    rect.topLeft().y());
    QPointF bottomRight = zoomContext.toZoomCoordinates(rect.bottomRight().x(),
                                                        rect.bottomRight().y());

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
    double gridR,gridG,gridB;
    boost::shared_ptr<Settings> sett = appPTR->getCurrentSettings();
    sett->getCurveEditorGridColor(&gridR, &gridG, &gridB); // use the same settings as the curve editor

    double scaleR,scaleG,scaleB;
    sett->getCurveEditorScaleColor(&scaleR, &scaleG, &scaleB); // use the same settings as the curve editor

    QColor scaleColor;
    scaleColor.setRgbF(Natron::clamp(scaleR), Natron::clamp(scaleG), Natron::clamp(scaleB));

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

            glColor4f(gridR, gridG, gridB, alpha);

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

bool DopeSheetViewPrivate::curveHasKeyframeAtTime(const boost::shared_ptr<Curve> &curve, double time) const
{
    KeyFrame k;

    return curve->getKeyFrameWithTime(time, &k);
}

QRectF DopeSheetViewPrivate::nameItemRectToSectionRect(const QRectF &rect) const
{
    QRectF r = rectToZoomCoordinates(rect);

    float sectionTop = r.topLeft().y();
    float sectionBottom = r.bottomRight().y() - 1;

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

    QColor sectionColor(Qt::darkGray);

    if (dsKnob->isMultiDim()) {
        sectionColor.setAlphaF(0.2f);
    }
    else {
        sectionColor.setAlphaF(0.5f);
    }

    QRectF nameItemRect = dsKnob->getNameItemRect();
    QRectF sectionRect = nameItemRectToSectionRect(nameItemRect);

    glColor4f(sectionColor.redF(), sectionColor.greenF(),
              sectionColor.blueF(), sectionColor.alphaF());

    glBegin(GL_QUADS);
    glVertex2f(sectionRect.topLeft().x(), sectionRect.topLeft().y());
    glVertex2f(sectionRect.bottomLeft().x(), sectionRect.bottomLeft().y());
    glVertex2f(sectionRect.bottomRight().x(), sectionRect.bottomRight().y());
    glVertex2f(sectionRect.topRight().x(), sectionRect.topRight().y());
    glEnd();
}

/**
 * @brief DopeSheetViewPrivate::drawNodeSection
 *
 *
 */
void DopeSheetViewPrivate::drawNodeSection(const DSNode *dsNode) const
{
    GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

    QColor sectionColor(Qt::darkGray);
    sectionColor.setAlphaF(0.1f);

    QRectF nameItemRect = dsNode->getNameItemRect();

    QRectF sectionRect = nameItemRectToSectionRect(nameItemRect);

    glColor4f(sectionColor.redF(), sectionColor.greenF(),
              sectionColor.blueF(), sectionColor.alphaF());

    glBegin(GL_QUADS);
    glVertex2f(sectionRect.topLeft().x(), sectionRect.topLeft().y());
    glVertex2f(sectionRect.bottomLeft().x(), sectionRect.bottomLeft().y());
    glVertex2f(sectionRect.bottomRight().x(), sectionRect.bottomRight().y());
    glVertex2f(sectionRect.topRight().x(), sectionRect.topRight().y());
    glEnd();
}

/**
 * @brief DopeSheetViewPrivate::drawKeyFramesFor
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

        glPointSize(7.f);
        glEnable(GL_POINT_SMOOTH);
        glColor3f(1.f, 0.f, 0.f);

        DSKnobList knobItems = dsNode->getDSKnobs();
        TimeSet nodeKeyframes;

        for (DSKnobList::const_iterator it = knobItems.begin();
             it != knobItems.end();
             ++it) {

            DSKnob *dsKnob = (*it);

            if (!(dsNode->isExpanded()) || dsKnob->isHidden() ||
                    dsKnob->nodeItemIsCollapsed()) {
                continue;
            }

            KnobGui *knobGui = dsKnob->getKnobGui();
            TimeSet multiDimKnobKeyframes;

            // Draw keyframes for each dimension of the knob
            for (int dim = 0; dim < knobGui->getKnob()->getDimension(); ++dim) {
                KeyFrameSet keyframes = knobGui->getCurve(dim)->getKeyFrames_mt_safe();

                for (KeyFrameSet::const_iterator kIt = keyframes.begin();
                     kIt != keyframes.end();
                     ++kIt) {
                    double keyTime = (*kIt).getTime();

                    double y = (dsKnob->isMultiDim()) ? dsKnob->getNameItemRectForDim(dim).center().y()
                                                      : dsKnob->getNameItemRect().center().y();
                    QPointF p = zoomContext.toZoomCoordinates(keyTime, y);

                    glBegin(GL_POINTS);
                    glVertex2f(p.x(), p.y());
                    glEnd();

                    if (dsKnob->isMultiDim()) {
                        // Draw keyframe in multidim root knob section too
                        TimeSet::iterator multiDimKnobKeysIt = multiDimKnobKeyframes.find(keyTime);

                        if (multiDimKnobKeysIt == multiDimKnobKeyframes.end()) {
                            p = zoomContext.toZoomCoordinates(keyTime,
                                                              dsKnob->getNameItemRect().center().y());

                            glBegin(GL_POINTS);
                            glVertex2f(p.x(), p.y());
                            glEnd();

                            multiDimKnobKeyframes.insert(keyTime);
                        }
                    }

                    // Draw keyframe in node section
                    TimeSet::iterator nodeKeysIt = nodeKeyframes.find(keyTime);

                    if (nodeKeysIt == nodeKeyframes.end()) {
                        p = zoomContext.toZoomCoordinates(keyTime,
                                                          dsNode->getNameItemRect().center().y());

                        glBegin(GL_POINTS);
                        glVertex2f(p.x(), p.y());
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

    const DSNodeList &dsNodeItems = dopeSheetEditor->getDSNodeItems();

    // Perform drawing
    {
        GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);

        for (DSNodeList::const_iterator it = dsNodeItems.begin();
             it != dsNodeItems.end();
             ++it) {

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            DSNode *dsNode = (*it);

            drawNodeSection(dsNode);

            DSKnobList knobItems = (*it)->getDSKnobs();
            for (DSKnobList::const_iterator it2 = knobItems.begin();
                 it2 != knobItems.end();
                 ++it2) {

                drawKnobSection(*it2);
            }

            if (dsNode->isReaderNode()) {
                drawReader(dsNode);
            }
            else if (dsNode->isGroupNode()) {
                drawGroup(dsNode);
            }
            else {
                drawKeyframes(dsNode);
            }
        }
    }
}

void DopeSheetViewPrivate::drawReader(const DSNode *dsNode) const
{
    boost::shared_ptr<Natron::Node> node = dsNode->getNodeGui()->getNode();

    int firstFrame = dynamic_cast<Knob<int> *>(node->getKnobByName("firstFrame").get())->getValue();
    int lastFrame = dynamic_cast<Knob<int> *>(node->getKnobByName("lastFrame").get())->getValue();
    int startingTime = dynamic_cast<Knob<int> *>(node->getKnobByName("startingTime").get())->getValue();

    QRectF nameItemRect = dsNode->getNameItemRect();
    QRectF readerRect;
    readerRect.setLeft(firstFrame);
    readerRect.setRight(lastFrame);
    readerRect.setTop(nameItemRect.top() + 1);
    readerRect.setBottom(nameItemRect.bottom() + 1);

    // Draw the reader rect
    {
        QColor readerColor(Qt::darkRed);
        QRectF convertedReaderRect = rectToZoomCoordinates(readerRect);

        GLProtectAttrib a(GL_CURRENT_BIT);

        glColor4f(readerColor.redF(), readerColor.greenF(),
                  readerColor.blueF(), readerColor.alphaF());

        glBegin(GL_QUADS);
        glVertex2f(convertedReaderRect.topLeft().x(), convertedReaderRect.topLeft().y());
        glVertex2f(convertedReaderRect.bottomLeft().x(), convertedReaderRect.bottomLeft().y());
        glVertex2f(convertedReaderRect.bottomRight().x(), convertedReaderRect.bottomRight().y());
        glVertex2f(convertedReaderRect.topRight().x(), convertedReaderRect.topRight().y());
        glEnd();
    }

    // Draw the preview
    {
        if ( node->isRenderingPreview() ) {
            return;
        }

        int w = readerRect.width();
        int h = readerRect.height();

        size_t dataSize = 4 * w * h;
        {
#ifndef __NATRON_WIN32__
            unsigned int* buf = (unsigned int*)calloc(dataSize, 1);
#else
            unsigned int* buf = (unsigned int*)malloc(dataSize);
            for (int i = 0; i < w * h; ++i) {
                buf[i] = qRgba(0,0,0,255);
            }
#endif
            bool success = node->makePreviewImage((startingTime - lastFrame) / 2, &w, &h, buf);

            if (success) {
                QImage img(reinterpret_cast<const uchar *>(buf), w, h, QImage::Format_ARGB32);
                GLuint textureId = parent->bindTexture(img);

                parent->drawTexture(rectToZoomCoordinates(QRectF(readerRect.left(),
                                                                 readerRect.top(),
                                                                 w, h)),
                                    textureId);
            }

            free(buf);
        }
    }

    // Draw the starting frame indicator
    QPointF topPoint = zoomContext.toZoomCoordinates(startingTime, nameItemRect.top() + 1);
    QPointF bottomPoint = zoomContext.toZoomCoordinates(startingTime, nameItemRect.bottom() + 1);

    {
        GLProtectAttrib a(GL_CURRENT_BIT);

        QColor startingFrameColor(Qt::green);

        glColor4f(startingFrameColor.redF(), startingFrameColor.greenF(),
                  startingFrameColor.blueF(), startingFrameColor.alphaF());

        glBegin(GL_LINES);
        glVertex2f(topPoint.x(), topPoint.y());
        glVertex2f(bottomPoint.x(), bottomPoint.y());
        glEnd();
    }
}

void DopeSheetViewPrivate::drawGroup(const DSNode *dsNode) const
{
    // Get the frame range of all child nodes
    std::vector<double> dimFirstKeyframes;
    std::vector<double> dimLastKeyframes;

    NodeList nodes = dynamic_cast<NodeGroup *>(dsNode->getNodeGui()->getNode()->getLiveInstance())->getNodes();

    for (NodeList::const_iterator it = nodes.begin();
         it != nodes.end();
         ++it) {
        NodePtr node = (*it);
        boost::shared_ptr<NodeGui> nodeGui = boost::dynamic_pointer_cast<NodeGui>(node->getNodeGui());

        if (!nodeGui->getSettingPanel() || !nodeGui->getSettingPanel()->isVisible()) {
            continue;
        }

        const std::vector<boost::shared_ptr<KnobI> > &knobs = node->getKnobs();

        for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it = knobs.begin();
             it != knobs.end();
             ++it) {
            boost::shared_ptr<KnobI> knob = (*it);

            if (!knob->canAnimate() || !knob->hasAnimation()) {
                continue;
            }
            else {
                for (int i = 0; i < knob->getDimension(); ++i) {
                    double time = -1;
                    knob->getFirstKeyFrameTime(i, &time);

                    if (time != -1) {
                        dimFirstKeyframes.push_back(time);
                    }

                    knob->getLastKeyFrameTime(i, &time);

                    if (time != -1) {
                        dimLastKeyframes.push_back(time);
                    }

                }
            }
        }
    }

    if (!dimFirstKeyframes.size() || !dimLastKeyframes.size()) {
        return;
    }

    double groupFirstKeyframe = *std::min_element(dimFirstKeyframes.begin(), dimFirstKeyframes.end());
    double groupLastKeyframe = *std::max_element(dimLastKeyframes.begin(), dimLastKeyframes.end());

    QRectF nameItemRect = dsNode->getNameItemRect();
    QRectF readerRect;
    readerRect.setLeft(groupFirstKeyframe);
    readerRect.setRight(groupLastKeyframe);
    readerRect.setTop(nameItemRect.top() + 1);
    readerRect.setBottom(nameItemRect.bottom() + 1);

    // Draw the reader rect
    {
        QColor readerColor(Qt::darkGreen);
        QRectF convertedReaderRect = rectToZoomCoordinates(readerRect);

        GLProtectAttrib a(GL_CURRENT_BIT);

        glColor4f(readerColor.redF(), readerColor.greenF(),
                  readerColor.blueF(), readerColor.alphaF());

        glBegin(GL_QUADS);
        glVertex2f(convertedReaderRect.topLeft().x(), convertedReaderRect.topLeft().y());
        glVertex2f(convertedReaderRect.bottomLeft().x(), convertedReaderRect.bottomLeft().y());
        glVertex2f(convertedReaderRect.bottomRight().x(), convertedReaderRect.bottomRight().y());
        glVertex2f(convertedReaderRect.topRight().x(), convertedReaderRect.topRight().y());
        glEnd();
    }
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
void DopeSheetViewPrivate::drawCurrentFrameIndicator() const
{
    RUNNING_IN_MAIN_THREAD_AND_CONTEXT(parent);

    float polyHalfWidth = 7.5;
    float polyHeight = 7.5;

    // Retrieve settings for drawing
    double colorR, colorG, colorB;
    boost::shared_ptr<Settings> settings = appPTR->getCurrentSettings();
    settings->getTimelinePlayheadColor(&colorR, &colorG, &colorB);

    int top = zoomContext.toZoomCoordinates(0, 0).y();
    int bottom = zoomContext.toZoomCoordinates(parent->width() - 1,
                                               parent->height() - 1).y();
    int currentTime = timeline->currentFrame();

    // Perform drawing
    {
        GLProtectAttrib a(GL_CURRENT_BIT | GL_HINT_BIT | GL_ENABLE_BIT |
                          GL_LINE_BIT | GL_POLYGON_BIT | GL_COLOR_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);

        QColor color = QColor::fromRgbF(colorR, colorG, colorB);

        glColor4f(color.redF(), color.greenF(),
                  color.blueF(), color.alphaF());

        glBegin(GL_LINES);
        glVertex2f(currentTime, top);
        glVertex2f(currentTime, bottom);
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
        glVertex2f(currentTime - polyHalfWidth, bottom);
        glVertex2f(currentTime + polyHalfWidth, bottom);
        glVertex2f(currentTime, bottom + polyHeight);
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

    // Perform drawing
    {

    }
}

/**
 * @brief DopeSheetViewPrivate::drawSelectedKeysRect
 *
 *
 */
void DopeSheetViewPrivate::drawSelectedKeysRect() const
{
    RUNNING_IN_MAIN_THREAD_AND_CONTEXT(parent);

    // Perform drawing
    {

    }
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
    _imp->timeline = timeline;
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

    double bgR,bgG,bgB;
    appPTR->getCurrentSettings()->getCurveEditorBGColor(&bgR, &bgG, &bgB);

    if ((zoomLeft == zoomRight) || (zoomTop == zoomBottom)) {
        glClearColor(bgR,bgG,bgB,1.);
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

        glClearColor(bgR,bgG,bgB,1.);
        glClear(GL_COLOR_BUFFER_BIT);

        _imp->drawScale();
        _imp->drawSections();
        _imp->drawProjectBounds();
        _imp->drawCurrentFrameIndicator();
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

    double w = (double)width();
    double h = (double)height();

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
