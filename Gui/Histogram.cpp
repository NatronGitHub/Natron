/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#include "Histogram.h"

#include <algorithm> // min, max
#include <stdexcept>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QSplitter>
#include <QDesktopWidget>
#include <QGLShaderProgram>
GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QMouseEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QDebug>
#include <QApplication>
#include <QToolButton>
#include <QActionGroup>

#include "Engine/HistogramCPU.h"
#include "Engine/Image.h"
#include "Engine/Node.h"
#include "Engine/ViewerInstance.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/ClickableLabel.h"
#include "Gui/ComboBox.h"
#include "Gui/CurveWidget.h"
#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/GuiMacros.h"
#include "Gui/Menu.h"
#include "Gui/NodeGraph.h"
#include "Gui/Shaders.h"
#include "Gui/TextRenderer.h"
#include "Gui/Texture.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"
#include "Gui/ZoomContext.h"
#include "Gui/ticks.h"

// warning: 'gluErrorString' is deprecated: first deprecated in OS X 10.9 [-Wdeprecated-declarations]
CLANG_DIAG_OFF(deprecated-declarations)
GCC_DIAG_OFF(deprecated-declarations)

NATRON_NAMESPACE_ENTER;

namespace { // protext local classes in anonymous namespace
enum EventStateEnum
{
    eEventStateDraggingView = 0,
    eEventStateZoomingView,
    eEventStateNone
};
}


struct HistogramPrivate
{
    HistogramPrivate(Histogram* widget)
    : mainLayout(NULL)
    , rightClickMenu(NULL)
    , histogramSelectionMenu(NULL)
    , histogramSelectionGroup(NULL)
    , viewerCurrentInputMenu(NULL)
    , viewerCurrentInputGroup(NULL)
    , modeActions(0)
    , modeMenu(NULL)
    , fullImage(NULL)
    , filterActions(0)
    , filterMenu(NULL)
    , widget(widget)
    , mode(Histogram::eDisplayModeRGB)
    , oldClick()
    , zoomCtx()
    , supportsGLSL(true)
    , hasOpenGLVAOSupport(true)
    , state(eEventStateNone)
    , hasBeenModifiedSinceResize(false)
    , _baseAxisColor(118,215,90,255)
    , _scaleColor(67,123,52,255)
    , _font(appFont,appFontSize)
    , textRenderer()
    , drawCoordinates(false)
    , xCoordinateStr()
    , rValueStr()
    , gValueStr()
    , bValueStr()
    , filterSize(0)
#ifdef NATRON_HISTOGRAM_USING_OPENGL
    , histogramComputingShader()
    , histogramMaximumShader()
    , histogramRenderingShader()
#else
    , histogramThread()
    , histogram1()
    , histogram2()
    , histogram3()
    , pixelsCount(0)
    , vmin(0)
    , vmax(0)
    , binsCount(0)
    , mipMapLevel(0)
    , hasImage(false)
#endif
    , sizeH()
    {
    }
    
    boost::shared_ptr<Image> getHistogramImage(RectI* imagePortion) const;
    
    
    void showMenu(const QPoint & globalPos);
    
    void drawScale();
    
    void drawPicker();
    
    void drawWarnings();
    
    void drawMissingImage();
    
    void updatePicker(double x);
    
#ifdef NATRON_HISTOGRAM_USING_OPENGL
    
    void resizeComputingVBO(int w,int h);
    
    
    ///For all these functions, mode refers to either R,G,B,A or Y
    void computeHistogram(Histogram::DisplayModeEnum mode);
    void renderHistogram(Histogram::DisplayModeEnum mode);
    void activateHistogramComputingShader(Histogram::DisplayModeEnum mode);
    void activateHistogramRenderingShader(Histogram::DisplayModeEnum mode);
    
#else
    void drawHistogramCPU();
#endif
    
    //////////////////////////////////
    // data members
    
    QVBoxLayout* mainLayout;
    
    ///////// OPTIONS
    Menu* rightClickMenu;
    QMenu* histogramSelectionMenu;
    QActionGroup* histogramSelectionGroup;
    Menu* viewerCurrentInputMenu;
    QActionGroup* viewerCurrentInputGroup;
    QActionGroup* modeActions;
    Menu* modeMenu;
    QAction* fullImage;
    QActionGroup* filterActions;
    Menu* filterMenu;
    Histogram* widget;
    Histogram::DisplayModeEnum mode;
    QPoint oldClick; /// the last click pressed, in widget coordinates [ (0,0) == top left corner ]
    ZoomContext zoomCtx;
    bool supportsGLSL;
    bool hasOpenGLVAOSupport;
    EventStateEnum state;
    bool hasBeenModifiedSinceResize; //< true if the user panned or zoomed since the last resize
    QColor _baseAxisColor;
    QColor _scaleColor;
    QFont _font;
    TextRenderer textRenderer;
    bool drawCoordinates;
    QString xCoordinateStr;
    QString rValueStr,gValueStr,bValueStr;
    int filterSize;

#ifdef NATRON_HISTOGRAM_USING_OPENGL
    /*texture ID of the input images*/
    boost::shared_ptr<Texture> leftImageTexture,rightImageTexture;


    /*The shader that computes the histogram:
       Takes in input the image, and writes to the 256x1 texture*/
    boost::shared_ptr<QGLShaderProgram> histogramComputingShader;

    /*The shader that computes the maximum of the histogram.
       Takes in input the reduction I and writes to the reduction I+1
       the local maximas of the reduction I. Each pixel of the texture I+1
       holds the maximum of 4 pixels in the texture I.*/
    boost::shared_ptr<QGLShaderProgram> histogramMaximumShader;

    /*The shader that renders the histogram. Takes in input the
       image and the histogram texture and produces the 256x256 image
       of the histogram.*/
    boost::shared_ptr<QGLShaderProgram> histogramRenderingShader;

    /*vbo ID, This vbo holds the texture coordinates to fetch
       to compute the histogram. It changes when the
       input image changes*/
    GLuint vboID;

    /*the vao encapsulating the vbo*/
    GLuint vaoID;

    /*The vbo used to render the histogram, it never changes*/
    GLuint vboHistogramRendering;

    /*The texture holding the histogram (256x1)
     */
    GLuint histogramTexture[3];

    /*The textures holding the histogram reductions.
       They're used to compute the maximum of the histogram (64x1)(16x1)(4x1);*/
    GLuint histogramReductionsTexture[3];

    /*The texture holding the maximum (1x1)*/
    GLuint histogramMaximumTexture[3];

    /*Texture holding the image of the histogram, the result (256x256)
     */
    GLuint histogramRenderTexture[3];

    /*The FBO's associated to the reductions textures.
       1 per texture as they all have a different size*/
    GLuint fboReductions[3];

    /*The FBO holding the maximum texture*/
    GLuint fboMaximum;

    /*The fbo holding the final rendered texture*/
    GLuint fboRendering;

    /*The fbo holding the histogram (256x1)*/
    GLuint fbohistogram;
#else // !NATRON_HISTOGRAM_USING_OPENGL
    HistogramCPU histogramThread;

    ///up to 3 histograms (in the RGB) case. FOr all other cases just histogram1 is used.
    std::vector<float> histogram1;
    std::vector<float> histogram2;
    std::vector<float> histogram3;
    unsigned int pixelsCount;
    double vmin,vmax; //< the x range of the histogram
    unsigned int binsCount;
    unsigned int mipMapLevel;
    bool hasImage;
#endif // !NATRON_HISTOGRAM_USING_OPENGL
    
    QSize sizeH;
};

Histogram::Histogram(Gui* gui,
                     const QGLWidget* shareWidget)
: QGLWidget(gui,shareWidget)
, PanelWidget(this,gui)
, _imp( new HistogramPrivate(this) )
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Expanding);
    setMouseTracking(true);
#ifndef NATRON_HISTOGRAM_USING_OPENGL
    QObject::connect( &_imp->histogramThread, SIGNAL(histogramProduced()), this, SLOT(onCPUHistogramComputed()) );
#endif

//    QDesktopWidget* desktop = QApplication::desktop();
//    _imp->sizeH = desktop->screenGeometry().size();
    _imp->sizeH = QSize(10000,10000);
    
    _imp->rightClickMenu = new Menu(this);
    //_imp->rightClickMenu->setFont( QFont(appFont,appFontSize) );

    _imp->histogramSelectionMenu = new Menu(tr("Viewer target"),_imp->rightClickMenu);
    //_imp->histogramSelectionMenu->setFont( QFont(appFont,appFontSize) );
    _imp->rightClickMenu->addAction( _imp->histogramSelectionMenu->menuAction() );

    _imp->histogramSelectionGroup = new QActionGroup(_imp->histogramSelectionMenu);

    _imp->viewerCurrentInputMenu = new Menu(tr("Viewer input"),_imp->rightClickMenu);
    //_imp->viewerCurrentInputMenu->setFont( QFont(appFont,appFontSize) );
    _imp->rightClickMenu->addAction( _imp->viewerCurrentInputMenu->menuAction() );

    _imp->viewerCurrentInputGroup = new QActionGroup(_imp->viewerCurrentInputMenu);

    QAction* inputAAction = new QAction(_imp->viewerCurrentInputMenu);
    inputAAction->setText( tr("Input A") );
    inputAAction->setData(0);
    inputAAction->setCheckable(true);
    inputAAction->setChecked(true);
    QObject::connect( inputAAction, SIGNAL(triggered()), this, SLOT(computeHistogramAndRefresh()) );
    _imp->viewerCurrentInputGroup->addAction(inputAAction);
    _imp->viewerCurrentInputMenu->addAction(inputAAction);

    QAction* inputBAction = new QAction(_imp->viewerCurrentInputMenu);
    inputBAction->setText( tr("Input B") );
    inputBAction->setData(1);
    inputBAction->setCheckable(true);
    inputBAction->setChecked(false);
    QObject::connect( inputBAction, SIGNAL(triggered()), this, SLOT(computeHistogramAndRefresh()) );
    _imp->viewerCurrentInputGroup->addAction(inputBAction);
    _imp->viewerCurrentInputMenu->addAction(inputBAction);

    _imp->modeMenu = new Menu(tr("Display mode"),_imp->rightClickMenu);
    //_imp->modeMenu->setFont( QFont(appFont,appFontSize) );
    _imp->rightClickMenu->addAction( _imp->modeMenu->menuAction() );

    _imp->fullImage = new QAction(_imp->rightClickMenu);
    _imp->fullImage->setText("Full image");
    _imp->fullImage->setCheckable(true);
    _imp->fullImage->setChecked(false);
    QObject::connect( _imp->fullImage, SIGNAL(triggered()), this, SLOT(computeHistogramAndRefresh()) );
    _imp->rightClickMenu->addAction(_imp->fullImage);

    _imp->filterMenu = new Menu(tr("Smoothing"),_imp->rightClickMenu);
    //_imp->filterMenu->setFont( QFont(appFont,appFontSize) );
    _imp->rightClickMenu->addAction( _imp->filterMenu->menuAction() );

    _imp->modeActions = new QActionGroup(_imp->modeMenu);
    QAction* rgbAction = new QAction(_imp->modeMenu);
    rgbAction->setText( QString("RGB") );
    rgbAction->setData(0);
    _imp->modeActions->addAction(rgbAction);

    QAction* aAction = new QAction(_imp->modeMenu);
    aAction->setText( QString("A") );
    aAction->setData(1);
    _imp->modeActions->addAction(aAction);

    QAction* yAction = new QAction(_imp->modeMenu);
    yAction->setText( QString("Y") );
    yAction->setData(2);
    _imp->modeActions->addAction(yAction);

    QAction* rAction = new QAction(_imp->modeMenu);
    rAction->setText( QString("R") );
    rAction->setData(3);
    _imp->modeActions->addAction(rAction);

    QAction* gAction = new QAction(_imp->modeMenu);
    gAction->setText( QString("G") );
    gAction->setData(4);
    _imp->modeActions->addAction(gAction);

    QAction* bAction = new QAction(_imp->modeMenu);
    bAction->setText( QString("B") );
    bAction->setData(5);
    _imp->modeActions->addAction(bAction);
    QList<QAction*> actions = _imp->modeActions->actions();
    for (int i = 0; i < actions.size(); ++i) {
        _imp->modeMenu->addAction( actions.at(i) );
    }

    QObject::connect( _imp->modeActions,SIGNAL(triggered(QAction*)),this,SLOT(onDisplayModeChanged(QAction*)) );


    _imp->filterActions = new QActionGroup(_imp->filterMenu);
    QAction* noSmoothAction = new QAction(_imp->filterActions);
    noSmoothAction->setText( tr("Small") );
    noSmoothAction->setData(1);
    noSmoothAction->setCheckable(true);
    noSmoothAction->setChecked(true);
    _imp->filterActions->addAction(noSmoothAction);

    QAction* size3Action = new QAction(_imp->filterActions);
    size3Action->setText( tr("Medium") );
    size3Action->setData(3);
    size3Action->setCheckable(true);
    size3Action->setChecked(false);
    _imp->filterActions->addAction(size3Action);

    QAction* size5Action = new QAction(_imp->filterActions);
    size5Action->setText( tr("High") );
    size5Action->setData(5);
    size5Action->setCheckable(true);
    size5Action->setChecked(false);
    _imp->filterActions->addAction(size5Action);

    actions = _imp->filterActions->actions();
    for (int i = 0; i < actions.size(); ++i) {
        _imp->filterMenu->addAction( actions.at(i) );
    }

    QObject::connect( _imp->filterActions, SIGNAL(triggered(QAction*)), this, SLOT(onFilterChanged(QAction*)) );
    QObject::connect( getGui(), SIGNAL(viewersChanged()), this, SLOT(populateViewersChoices()) );
    populateViewersChoices();
}

Histogram::~Histogram()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    makeCurrent();

#ifdef NATRON_HISTOGRAM_USING_OPENGL
    glDeleteTextures(3,&_imp->histogramTexture[0]);
    glDeleteTextures(3,&_imp->histogramReductionsTexture[0]);
    glDeleteTextures(3,&_imp->histogramRenderTexture[0]);
    glDeleteTextures(3,&_imp->histogramMaximumTexture[0]);

    glDeleteFramebuffers(3,&_imp->fboReductions[0]);
    glDeleteFramebuffers(1,&_imp->fboMaximum);
    glDeleteFramebuffers(1,&_imp->fbohistogram);
    glDeleteFramebuffers(1,&_imp->fboRendering);

    glDeleteVertexArrays(1,&_imp->vaoID);
    glDeleteBuffers(1,&_imp->vboID);
    glDeleteBuffers(1,&_imp->vboHistogramRendering);

#endif
}

boost::shared_ptr<Image> HistogramPrivate::getHistogramImage(RectI* imagePortion) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    bool useImageRoD = fullImage->isChecked();
    int index = 0;
    std::string viewerName;
    QAction* selectedHistAction = histogramSelectionGroup->checkedAction();
    if (selectedHistAction) {
        index = selectedHistAction->data().toInt();
        viewerName = selectedHistAction->text().toStdString();
    }

    int textureIndex = 0;
    QAction* selectedInputAction = viewerCurrentInputGroup->checkedAction();
    if (selectedInputAction) {
        textureIndex = selectedInputAction->data().toInt();
    }
    
    ViewerTab* viewer = 0;
    if (index == 0) {
        //no viewer selected
        imagePortion->clear();
        return boost::shared_ptr<Image>();
    } else if (index == 1) {
        //current viewer
        viewer = widget->getGui()->getNodeGraph()->getLastSelectedViewer();
        
    } else {
        boost::shared_ptr<Image> ret;
        const std::list<ViewerTab*> & viewerTabs = widget->getGui()->getViewersList();
        for (std::list<ViewerTab*>::const_iterator it = viewerTabs.begin(); it != viewerTabs.end(); ++it) {
            if ( (*it)->getInternalNode()->getScriptName_mt_safe() == viewerName ) {
                viewer = *it;
                break;
            }
        }

    }
    
    std::list<boost::shared_ptr<Image> > tiles;
    if (viewer) {
        viewer->getViewer()->getLastRenderedImageByMipMapLevel(textureIndex,viewer->getInternalNode()->getMipMapLevelFromZoomFactor(),&tiles);
    }
    
    ///We must copy all tiles into an image of the whole size
    boost::shared_ptr<Image> ret;
    if (!tiles.empty()) {
        const    boost::shared_ptr<Image>& firstTile = tiles.front();
        RectI bounds;
        unsigned int mipMapLevel = 0;
        double par = 1.;
        ImageBitDepthEnum depth = eImageBitDepthFloat;
        ImageComponents comps;
        ImagePremultiplicationEnum premult;
        ImageFieldingOrderEnum fielding;
        for (std::list<boost::shared_ptr<Image> >::const_iterator it = tiles.begin(); it!=tiles.end(); ++it) {
            if (bounds.isNull()) {
                bounds = (*it)->getBounds();
                mipMapLevel = (*it)->getMipMapLevel();
                par = (*it)->getPixelAspectRatio();
                depth = (*it)->getBitDepth();
                comps = (*it)->getComponents();
                fielding = (*it)->getFieldingOrder();
                premult = (*it)->getPremultiplication();
            } else {
                bounds.merge((*it)->getBounds());
                assert(mipMapLevel == (*it)->getMipMapLevel());
                assert(depth == (*it)->getBitDepth());
                assert(comps == (*it)->getComponents());
                assert(par == (*it)->getPixelAspectRatio());
                assert(fielding == (*it)->getFieldingOrder());
                assert(premult == (*it)->getPremultiplication());
            }
        }
        if (bounds.isNull()) {
            return ret;
        }
        
        ret.reset(new Image(comps,
                            firstTile->getRoD(),
                            bounds,
                            mipMapLevel,
                            par,
                            depth,
                            premult,
                            fielding,
                            false));
        for (std::list<boost::shared_ptr<Image> >::const_iterator it = tiles.begin(); it!=tiles.end(); ++it) {
            ret->pasteFrom(**it, (*it)->getBounds(), false);
        }
        
        if (!useImageRoD) {
            if (viewer) {
                *imagePortion = viewer->getViewer()->getImageRectangleDisplayed(bounds,par, mipMapLevel);
            }
        } else {
            *imagePortion = ret->getBounds();
        }
    }
    
    return ret;
} // getHistogramImage

void
HistogramPrivate::showMenu(const QPoint & globalPos)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    rightClickMenu->exec(globalPos);
}

void
Histogram::populateViewersChoices()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    QString currentSelection;
    assert(_imp->histogramSelectionGroup);
    QAction* checkedAction = _imp->histogramSelectionGroup->checkedAction();
    if (checkedAction) {
        currentSelection = checkedAction->text();
    }
    delete _imp->histogramSelectionGroup;
    _imp->histogramSelectionGroup = new QActionGroup(_imp->histogramSelectionMenu);

    _imp->histogramSelectionMenu->clear();

    QAction* noneAction = new QAction(_imp->histogramSelectionGroup);
    noneAction->setText("-");
    noneAction->setData(0);
    noneAction->setCheckable(true);
    noneAction->setChecked(false);
    _imp->histogramSelectionGroup->addAction(noneAction);
    _imp->histogramSelectionMenu->addAction(noneAction);

    QAction* currentAction = new QAction(_imp->histogramSelectionGroup);
    currentAction->setText( tr("Current Viewer") );
    currentAction->setData(1);
    currentAction->setCheckable(true);
    currentAction->setChecked(false);
    _imp->histogramSelectionGroup->addAction(currentAction);
    _imp->histogramSelectionMenu->addAction(currentAction);


    const std::list<ViewerTab*> & viewerTabs = getGui()->getViewersList();
    int c = 2;
    for (std::list<ViewerTab*>::const_iterator it = viewerTabs.begin(); it != viewerTabs.end(); ++it) {
        if ( (*it)->getInternalNode()->getNode()->isActivated() ) {
            QAction* ac = new QAction(_imp->histogramSelectionGroup);
            ac->setText( (*it)->getInternalNode()->getScriptName_mt_safe().c_str() );
            ac->setCheckable(true);
            ac->setChecked(false);
            ac->setData(c);
            _imp->histogramSelectionGroup->addAction(ac);
            _imp->histogramSelectionMenu->addAction(ac);
            ++c;
        }
    }

    _imp->histogramSelectionGroup->blockSignals(true);
    if ( !currentSelection.isEmpty() ) {
        QList<QAction*> actions = _imp->histogramSelectionGroup->actions();
        for (int i = 0; i < actions.size(); ++i) {
            if (actions.at(i)->text() == currentSelection) {
                actions.at(i)->setChecked(true);
            }
        }
    } else {
        currentAction->setChecked(true);
    }
    _imp->histogramSelectionGroup->blockSignals(false);

    QObject::connect( _imp->histogramSelectionGroup,SIGNAL(triggered(QAction*)),this,SLOT(onCurrentViewerChanged(QAction*)) );
} // populateViewersChoices

void
Histogram::onCurrentViewerChanged(QAction*)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    computeHistogramAndRefresh();
}

void
Histogram::onViewerImageChanged(ViewerGL* viewer,
                                int texIndex,
                                bool hasImageBackend)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    if (viewer && hasImageBackend) {
        QString viewerName = viewer->getInternalNode()->getScriptName_mt_safe().c_str();
        ViewerTab* lastSelectedViewer = getGui()->getNodeGraph()->getLastSelectedViewer();
        QString currentViewerName;
        if (lastSelectedViewer) {
            currentViewerName = lastSelectedViewer->getInternalNode()->getScriptName_mt_safe().c_str();
        }

        QAction* selectedHistAction = _imp->histogramSelectionGroup->checkedAction();
        if (selectedHistAction) {
            int actionIndex = selectedHistAction->data().toInt();

            if ( ( (actionIndex == 1) && ( lastSelectedViewer == viewer->getViewerTab() ) )
                 || ( ( actionIndex > 1) && ( selectedHistAction->text() == viewerName) ) ) {
                QAction* currentInput = _imp->viewerCurrentInputGroup->checkedAction();
                if ( currentInput && (currentInput->data().toInt() == texIndex) ) {
                    computeHistogramAndRefresh();
                    return;
                } else {
                    return;
                }
            }
        }
    }
    
    _imp->hasImage =false;
    update();
    
}

QSize
Histogram::sizeHint() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _imp->sizeH;
}

void
Histogram::onFilterChanged(QAction* action)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->filterSize = action->data().toInt();
    computeHistogramAndRefresh();
}

void
Histogram::onDisplayModeChanged(QAction* action)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->mode = (Histogram::DisplayModeEnum)action->data().toInt();
    computeHistogramAndRefresh();
}

void
Histogram::initializeGL()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );

    GLenum err = glewInit();
    if (GLEW_OK != err) {
        /* Problem: glewInit failed, something is seriously wrong. */
        Dialogs::errorDialog( tr("OpenGL/GLEW error").toStdString(),
                             (const char*)glewGetErrorString(err) );
    }

    if ( !QGLShaderProgram::hasOpenGLShaderPrograms( context() ) ) {
        _imp->supportsGLSL = false;
    } else {
#ifdef NATRON_HISTOGRAM_USING_OPENGL
        _imp->histogramComputingShader.reset( new QGLShaderProgram( context() ) );
        if ( !_imp->histogramComputingShader->addShaderFromSourceCode(QGLShader::Vertex,histogramComputationVertex_vert) ) {
            qDebug() << _imp->histogramComputingShader->log();
        }
        if ( !_imp->histogramComputingShader->addShaderFromSourceCode(QGLShader::Fragment,histogramComputation_frag) ) {
            qDebug() << _imp->histogramComputingShader->log();
        }
        if ( !_imp->histogramComputingShader->link() ) {
            qDebug() << _imp->histogramComputingShader->log();
        }

        _imp->histogramMaximumShader.reset( new QGLShaderProgram( context() ) );
        if ( !_imp->histogramMaximumShader->addShaderFromSourceCode(QGLShader::Fragment,histogramMaximum_frag) ) {
            qDebug() << _imp->histogramMaximumShader->log();
        }
        if ( !_imp->histogramMaximumShader->link() ) {
            qDebug() << _imp->histogramMaximumShader->log();
        }

        _imp->histogramRenderingShader.reset( new QGLShaderProgram( context() ) );
        if ( !_imp->histogramRenderingShader->addShaderFromSourceCode(QGLShader::Vertex,histogramRenderingVertex_vert) ) {
            qDebug() << _imp->histogramRenderingShader->log();
        }
        if ( !_imp->histogramRenderingShader->addShaderFromSourceCode(QGLShader::Fragment,histogramRendering_frag) ) {
            qDebug() << _imp->histogramRenderingShader->log();
        }
        if ( !_imp->histogramRenderingShader->link() ) {
            qDebug() << _imp->histogramRenderingShader->log();
        }
#endif
    }

    if ( !glewIsSupported("GL_ARB_vertex_array_object "  // BindVertexArray, DeleteVertexArrays, GenVertexArrays, IsVertexArray (VAO), core since 3.0
                          ) ) {
        _imp->hasOpenGLVAOSupport = false;
    }

#ifdef NATRON_HISTOGRAM_USING_OPENGL

    // enable globally : no glPushAttrib()
    glEnable(GL_TEXTURE_RECTANGLE_ARB);


    glGenTextures(3,&_imp->histogramTexture[0]);
    glGenTextures(3,&_imp->histogramReductionsTexture[0]);
    glGenTextures(3,&_imp->histogramRenderTexture[0]);
    glGenTextures(3,&_imp->histogramMaximumTexture[0]);

    _imp->leftImageTexture.reset( new Texture(GL_TEXTURE_RECTANGLE_ARB,GL_NEAREST,GL_NEAREST) );
    _imp->rightImageTexture.reset( new Texture(GL_TEXTURE_RECTANGLE_ARB,GL_NEAREST,GL_NEAREST) );

    glGenFramebuffers(3,&_imp->fboReductions[0]);
    glGenFramebuffers(1,&_imp->fboMaximum);
    glGenFramebuffers(1,&_imp->fbohistogram);
    glGenFramebuffers(1,&_imp->fboRendering);

    glGenVertexArrays(1,&_imp->vaoID);
    glGenBuffers(1,&_imp->vboID);
    glGenBuffers(1,&_imp->vboHistogramRendering);

    /*initializing histogram fbo and attaching the 3 textures */
    glBindFramebuffer(GL_FRAMEBUFFER,_imp->fbohistogram);
    GLenum attachments[3] = {
        GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1,GL_COLOR_ATTACHMENT2
    };
    for (unsigned int i = 0; i < 3; ++i) {
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB,_imp->histogramTexture[i]);
        glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,0,GL_R32F,256,1,0,GL_RED,GL_FLOAT,0);
        glTexParameteri (GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri (GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER,attachments[i],GL_TEXTURE_RECTANGLE_ARB,_imp->histogramTexture[i],0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER,0);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB,0);

    /*initializing fbo for parallel reductions and textures*/
    for (unsigned int i = 0; i < 3; ++i) {
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB,_imp->histogramReductionsTexture[i]);
        glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,0,GL_R32F,256.f / pow( 4.f,(float)(i + 1) ),1,0,GL_RED,GL_FLOAT,0);
        glTexParameteri (GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri (GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER,_imp->fboReductions[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_RECTANGLE_ARB,_imp->histogramReductionsTexture[i],0);
        glBindFramebuffer(GL_FRAMEBUFFER,0);
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB,0);
    }

    /*initializing fbo holding maximums and textures*/
    glBindFramebuffer(GL_FRAMEBUFFER,_imp->fboMaximum);
    for (unsigned int i = 0; i < 3; ++i) {
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB,_imp->histogramMaximumTexture[i]);
        glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,0,GL_R32F,1,1,0,GL_RED,GL_FLOAT,0);
        glTexParameteri (GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri (GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER,attachments[i],GL_TEXTURE_RECTANGLE_ARB,_imp->histogramMaximumTexture[i],0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER,0);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB,0);

    /*initializing fbo holding rendering textures for the histogram*/
    glBindFramebuffer(GL_FRAMEBUFFER,_imp->fboRendering);
    for (unsigned int i = 0; i < 3; ++i) {
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB,_imp->histogramRenderTexture[i]);
        glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,0,GL_RGBA8,256,256,0,GL_RGBA,GL_UNSIGNED_BYTE,0);
        glTexParameteri (GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri (GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER,attachments[i],GL_TEXTURE_RECTANGLE_ARB,_imp->histogramRenderTexture[i],0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER,0);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB,0);


    /*initializing the vbo used to render the histogram*/
    glBindBuffer(GL_ARRAY_BUFFER,_imp->vboHistogramRendering);
    int vertexCount = 512; // we draw the bins as lines (2 vertex) hence 512 vertices
    glBufferData(GL_ARRAY_BUFFER,vertexCount * 3 * sizeof(float),NULL,GL_DYNAMIC_DRAW);
    float *gpuVertexBuffer = reinterpret_cast<float*>( glMapBuffer(GL_ARRAY_BUFFER,GL_WRITE_ONLY) );
    int j = 0;
    int x = 0;
    while (j < vertexCount * 3) {
        gpuVertexBuffer[j] = x;
        gpuVertexBuffer[j + 1] = 0.f;
        gpuVertexBuffer[j + 2] = 0.f;
        gpuVertexBuffer[j + 3] = x;
        gpuVertexBuffer[j + 4] = 0.f;
        gpuVertexBuffer[j + 5] = 1.f;
        x++;
        j += 6;
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER,0);

#endif // ifdef NATRON_HISTOGRAM_USING_OPENGL
} // initializeGL

#ifdef NATRON_HISTOGRAM_USING_OPENGL
void
HistogramPrivate::resizeComputingVBO(int w,
                                     int h)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );

    glBindBuffer(GL_ARRAY_BUFFER,vboID);
    int vertexCount = w * h;
    glBufferData(GL_ARRAY_BUFFER,vertexCount * 2 * sizeof(float),NULL,GL_DYNAMIC_DRAW);
    float* gpuVertexBuffer = reinterpret_cast<float*>( glMapBuffer(GL_ARRAY_BUFFER,GL_WRITE_ONLY) );
    int i = 0;
    float x = 0;
    float y = 0;
    while ( i < (vertexCount * 2) ) {
        if ( (x != 0) && ( (int)x % w == 0 ) ) {
            x = 0;
            ++y;
        }
        gpuVertexBuffer[i] = x;
        gpuVertexBuffer[i + 1] = y;
        ++x;
        i += 2;
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER,0);
}

static void
textureMap_Polygon(int fromX,
                   int fromY,
                   int fromW,
                   int fromH,
                   int x,
                   int y,
                   int w,
                   int h)
{
    glBegin(GL_POLYGON);
    glTexCoord2i (fromX, fromH); glVertex2i (x,h);
    glTexCoord2i (fromW, fromH); glVertex2i (w,h);
    glTexCoord2i (fromW, fromY); glVertex2i (w,y);
    glTexCoord2i (fromX, fromY); glVertex2i (x,y);
    glEnd ();
}

static void
startRenderingTo(GLuint fboId,
                 GLenum attachment,
                 int w,
                 int h)
{
    glBindFramebuffer(GL_FRAMEBUFFER,fboId);
    glDrawBuffer(attachment);
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);
    glCheckProjectionStack();
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, w, 0, h, 1, -1);
    glMatrixMode(GL_MODELVIEW);
    glCheckModelviewStack();
    glPushMatrix();
    glLoadIdentity();
}

static void
stopRenderingTo()
{
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glBindFramebuffer(GL_FRAMEBUFFER,0);
}

static int
shaderChannelFromDisplayMode(Histogram::DisplayModeEnum channel)
{
    switch (channel) {
    case Histogram::eDisplayModeR:

        return 1;
        break;
    case Histogram::eDisplayModeG:

        return 2;
        break;
    case Histogram::eDisplayModeB:

        return 3;
        break;
    case Histogram::eDisplayModeA:

        return 4;
        break;
    case Histogram::eDisplayModeY:

        return 0;
        break;
    case Histogram::eDisplayModeRGB:
    default:
        assert(false);
        break;
    }
}

void
HistogramPrivate::activateHistogramComputingShader(Histogram::DisplayModeEnum channel)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    histogramComputingShader->bind();
    histogramComputingShader->setUniformValue("Tex",0);
    glCheckError();
    histogramComputingShader->setUniformValue( "channel",shaderChannelFromDisplayMode(channel) );
    glBindAttribLocation(histogramComputingShader->programId(),0,"TexCoord");
}

void
HistogramPrivate::activateHistogramRenderingShader(Histogram::DisplayModeEnum channel)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );

    histogramRenderingShader->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB,histogramTexture[channel]);
    histogramRenderingShader->setUniformValue("HistogramTex",0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB,histogramMaximumTexture[0]);
    histogramRenderingShader->setUniformValue("MaximumRedTex",1);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB,histogramMaximumTexture[1]);
    histogramRenderingShader->setUniformValue("MaximumGreenTex",2);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB,histogramMaximumTexture[2]);
    histogramRenderingShader->setUniformValue("MaximumBlueTex",3);
    histogramRenderingShader->setUniformValue( "channel",shaderChannelFromDisplayMode(channel) );
    glBindAttribLocation(histogramRenderingShader->programId(),0,"TexCoord");
}

static GLenum
colorAttachmentFromDisplayMode(Histogram::DisplayModeEnum channel)
{
    switch (channel) {
    case Histogram::eDisplayModeR:

        return GL_COLOR_ATTACHMENT0;
        break;
    case Histogram::eDisplayModeG:

        return GL_COLOR_ATTACHMENT1;
        break;
    case Histogram::eDisplayModeB:

        return GL_COLOR_ATTACHMENT2;
        break;
    case Histogram::eDisplayModeA:

        return GL_COLOR_ATTACHMENT0;
        break;
    case Histogram::eDisplayModeY:

        return GL_COLOR_ATTACHMENT0;
        break;
    case Histogram::eDisplayModeRGB:
    default:
        ///isn't meant for other components than the one handled here
        assert(false);
        break;
    }
}

void
HistogramPrivate::computeHistogram(Histogram::DisplayModeEnum channel)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );

    GLenum attachment = colorAttachmentFromDisplayMode(channel);

#ifdef DEBUG
#pragma message WARN("TODO: ave currently bound VA, Buffer, and bound texture")
#endif
    /*binding the VAO holding managing the VBO*/
    glBindVertexArray(vaoID);
    /*binding the VBO sending vertices to the vertex shader*/
    glBindBuffer(GL_ARRAY_BUFFER,vboID);
    /*each attribute will be a vec2f*/
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,0);
    /*enabling the VBO 0 of the VAO*/
    glEnableVertexAttribArray(0);

    GLuint savedTexture;
    glGetIntegerv(GL_TEXTURE_BINDING_RECTANGLE_ARB, (GLint*)&savedTexture);
    {
        GLProtectAttrib a(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TRANSFORM_BIT | GL_VIEWPORT_BIT);

        /*start rendering to the histogram texture held by the _fboHistogram*/
        startRenderingTo(fbohistogram,attachment,256,1); // modifies GL_TRANSFORM & GL_VIEWPORT

        /*clearing out the texture from previous computations*/
        glClearColor(0.f,0.f,0.f,0.f);
        glClear(GL_COLOR_BUFFER_BIT);
        /*enabling blending to add up the colors in texels :
         this results in pixels suming up */
        glEnable(GL_BLEND);
        glBlendEquationSeparate(GL_FUNC_ADD,GL_FUNC_ADD);
        glBlendFuncSeparate(GL_ONE,GL_ONE,GL_ONE,GL_ONE);
        /*binding the input image*/
        glActiveTexture(GL_TEXTURE0);

        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, leftImageTexture->getTexID());
        /*making current the shader computing the histogram*/
        activateHistogramComputingShader(channel);
        /*the number of vertices in the VBO*/
        int vertexCount = leftImageTexture->w() * leftImageTexture->h();
        /*sending vertices to the GPU, they're handled by the vertex shader*/
        glDrawArrays(GL_POINTS,0,vertexCount);
        /*stop computing*/
        histogramComputingShader->release();
        /*reset our context state*/
        glDisable(GL_BLEND);
        glBindVertexArray(0);
        stopRenderingTo();

        /*At this point we have the Histogram filled. From now on we can compute the maximum
         of the histogram with parallel reductions. 4 passes are needed : 256 -> 64 ,
         64 -> 16 , 16 -> 4, 4 -> 1
         The last pass is done after the loop as it is done in a separate FBO.
         ---------------------------------------------------------------------
         ** One pass does the following :
         - binds the fbo holding the I'th reduction.`
         - activates the shader with in input the I-1'th texture, the one resulting
         from the previous reduction.
         - does the rendering to the I'th reduced texture.*/
        GLuint inputTex = histogramTexture[channel];
        for (unsigned int i = 0; i < 3; i++) {
            int wTarget = (int)( 256.f / pow( 4.f,(float)(i + 1) ) );
            startRenderingTo(fboReductions[i],GL_COLOR_ATTACHMENT0,wTarget,1);
            histogramMaximumShader->bind();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_RECTANGLE_ARB,inputTex);
            histogramMaximumShader->setUniformValue("Tex",0);
            textureMap_Polygon(0,0,wTarget,1,0,0,wTarget,1);
            histogramMaximumShader->release();
            stopRenderingTo();
            inputTex = histogramReductionsTexture[i];
        }
        /*This part is similar to the loop above, but it is a special case since
         we do not render in a fboReductions but in the fboMaximum.
         The color attachment might change if we need to compute 3 histograms
         In this case only the red histogram is computed.*/
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, histogramReductionsTexture[2]);
        histogramMaximumShader->bind();
        histogramMaximumShader->setUniformValue("Tex",0);
        startRenderingTo(fboMaximum,attachment,1,1);
        textureMap_Polygon(0,0,1,1,0,0,1,1);
        histogramMaximumShader->release();
        stopRenderingTo(); // modifies GL_TRANSFORM
        glCheckError();
    } // GLProtectAttrib a(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, savedTexture);
    glCheckError();
} // computeHistogram

void
HistogramPrivate::renderHistogram(Histogram::DisplayModeEnum channel)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );

    GLenum attachment = colorAttachmentFromDisplayMode(channel);


    /*start rendering the histogram(256x1) to the rendering texture(256x256) that is attached
       to the rendering fbo.*/
    startRenderingTo(fboRendering,attachment,256,256);
    /*clearing out the texture from previous rendering*/
    /*binding the VAO holding the VBO*/
    glBindVertexArray(vaoID);
    /*binding the VBO holding the vertices that will be send to
       the GPU*/
    glBindBuffer(GL_ARRAY_BUFFER,vboHistogramRendering);
    /*each attribute in the VBO is a vec3f*/
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,0);
    /*enabling the VBO 0 of the VAO*/
    glEnableVertexAttribArray(0);
    /*activating the rendering shader*/
    activateHistogramRenderingShader(channel);
    glClearColor(0.0,0.0,0.0,0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    /*sending vertices to the GPU so the vertex
       shader start computing. Note that there're 512
       vertices because we render lines: 1 vertex for the
       bottom, another for the top of the bin.*/
    glDrawArrays(GL_LINES,0,512);
    histogramRenderingShader->release();
    /*reseting opengl context*/
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB,0);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB,0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB,0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB,0);
    glBindVertexArray(0);
    stopRenderingTo();
    glCheckError();
}

#endif // ifdef NATRON_HISTOGRAM_USING_OPENGL

void
Histogram::paintGL()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );

    glCheckError();
    double w = (double)width();
    double h = (double)height();

    ///don't bother painting an invisible widget, this may be caused to a splitter collapsed.
    if ( (w <= 1) || (h <= 1) ) {
        return;
    }

    assert(_imp->zoomCtx.factor() > 0.);

    double zoomLeft = _imp->zoomCtx.left();
    double zoomRight = _imp->zoomCtx.right();
    double zoomBottom = _imp->zoomCtx.bottom();
    double zoomTop = _imp->zoomCtx.top();
    if ( (zoomLeft == zoomRight) || (zoomTop == zoomBottom) ) {
        glClearColor(0,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT);
        glCheckErrorIgnoreOSXBug();

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
        
        glClearColor(0,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT);
        glCheckErrorIgnoreOSXBug();
        
        _imp->drawScale();
        
        if (_imp->hasImage) {
#ifndef NATRON_HISTOGRAM_USING_OPENGL
            _imp->drawHistogramCPU();
#endif
            if (_imp->drawCoordinates) {
                _imp->drawPicker();
            }
            
            _imp->drawWarnings();
            
        } else {
            _imp->drawMissingImage();
        }
        
        glCheckError();
    } // GLProtectAttrib a(GL_TRANSFORM_BIT | GL_COLOR_BUFFER_BIT);
} // paintGL

void
Histogram::resizeGL(int width,
                    int height)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );

    if (height == 0) { // prevent division by 0
        height = 1;
    }
    glViewport (0, 0, width, height);
    _imp->zoomCtx.setScreenSize(width, height);
    if (!_imp->hasBeenModifiedSinceResize) {
        _imp->zoomCtx.fill(0., 1., 0., 10.);
        computeHistogramAndRefresh();
    }
}

void
Histogram::mousePressEvent(QMouseEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    takeClickFocus();
    
    ////
    // middle button: scroll view
    if ( buttonDownIsMiddle(e) ) {
        _imp->state = eEventStateDraggingView;
        _imp->oldClick = e->pos();
    } else if ( buttonDownIsRight(e) ) {
        _imp->showMenu( e->globalPos() );
    } else if (((e->buttons() & Qt::MiddleButton) &&
                (buttonMetaAlt(e) == Qt::AltModifier || (e->buttons() & Qt::LeftButton))) ||
               ((e->buttons() & Qt::LeftButton) &&
                (buttonMetaAlt(e) == (Qt::AltModifier|Qt::MetaModifier)))) {
        // Alt + middle or left + middle or Meta + Alt + left = zoom
        _imp->state = eEventStateZoomingView;
        _imp->oldClick = e->pos();
        return;
    }
}

void
Histogram::mouseMoveEvent(QMouseEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    QPointF newClick_opengl = _imp->zoomCtx.toZoomCoordinates( e->x(),e->y() );
    QPointF oldClick_opengl = _imp->zoomCtx.toZoomCoordinates( _imp->oldClick.x(),_imp->oldClick.y() );


    _imp->oldClick = e->pos();
    _imp->drawCoordinates = true;

    double dx = ( oldClick_opengl.x() - newClick_opengl.x() );
    double dy = ( oldClick_opengl.y() - newClick_opengl.y() );

    switch (_imp->state) {
    case eEventStateDraggingView:
        _imp->zoomCtx.translate(dx, dy);
        _imp->hasBeenModifiedSinceResize = true;
            computeHistogramAndRefresh();
            break;
        case eEventStateZoomingView: {
            
            int delta = 2*((e->x() - _imp->oldClick.x()) - (e->y() - _imp->oldClick.y()));
            
            const double zoomFactor_min = 0.000001;
            const double zoomFactor_max = 1000000.;
            double zoomFactor;
            double scaleFactor = std::pow( NATRON_WHEEL_ZOOM_PER_DELTA, delta);
            QPointF zoomCenter = _imp->zoomCtx.toZoomCoordinates( e->x(), e->y() );
            
            
            
            // Wheel: zoom values and time, keep point under mouse
            zoomFactor = _imp->zoomCtx.factor() * scaleFactor;
            if (zoomFactor <= zoomFactor_min) {
                zoomFactor = zoomFactor_min;
                scaleFactor = zoomFactor / _imp->zoomCtx.factor();
            } else if (zoomFactor > zoomFactor_max) {
                zoomFactor = zoomFactor_max;
                scaleFactor = zoomFactor / _imp->zoomCtx.factor();
            }
            _imp->zoomCtx.zoom(zoomCenter.x(), zoomCenter.y(), scaleFactor);
            
            _imp->hasBeenModifiedSinceResize = true;
            
            computeHistogramAndRefresh();
    } break;
    case eEventStateNone:
        _imp->updatePicker( newClick_opengl.x() );
        update();
        break;
    }
    QGLWidget::mouseMoveEvent(e);
}

void
HistogramPrivate::updatePicker(double x)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    xCoordinateStr = QString("x=") + QString::number(x,'f',6);
    double binSize = (vmax - vmin) / binsCount;
    int index = (int)( (x - vmin) / binSize );
    rValueStr.clear();
    gValueStr.clear();
    bValueStr.clear();
    if (mode == Histogram::eDisplayModeRGB) {
        float r = histogram1.empty() ? 0 :  histogram1[index];
        float g = histogram2.empty() ? 0 :  histogram2[index];
        float b = histogram3.empty() ? 0 :  histogram3[index];
        rValueStr = QString("r=") + QString::number(r);
        gValueStr = QString("g=") + QString::number(g);
        bValueStr = QString("b=") + QString::number(b);
    } else if (mode == Histogram::eDisplayModeY) {
        float y = histogram1[index];
        rValueStr = QString("y=") + QString::number(y);
    } else if (mode == Histogram::eDisplayModeA) {
        float a = histogram1[index];
        rValueStr = QString("a=") + QString::number(a);
    } else if (mode == Histogram::eDisplayModeR) {
        float r = histogram1[index];
        rValueStr = QString("r=") + QString::number(r);
    } else if (mode == Histogram::eDisplayModeG) {
        float g = histogram1[index];
        gValueStr = QString("g=") + QString::number(g);
    } else if (mode == Histogram::eDisplayModeB) {
        float b = histogram1[index];
        bValueStr = QString("b=") + QString::number(b);
    } else {
        assert(false);
    }
}

void
Histogram::mouseReleaseEvent(QMouseEvent* /*e*/)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->state = eEventStateNone;
}

void
Histogram::wheelEvent(QWheelEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    // don't handle horizontal wheel (e.g. on trackpad or Might Mouse)
    if (e->orientation() != Qt::Vertical) {
        return;
    }
    const double zoomFactor_min = 0.000001;
    const double zoomFactor_max = 1000000.;
    const double par_min = 0.000001;
    const double par_max = 1000000.;
    double zoomFactor;
    double par;
    double scaleFactor = std::pow( NATRON_WHEEL_ZOOM_PER_DELTA, e->delta() );
    QPointF zoomCenter = _imp->zoomCtx.toZoomCoordinates( e->x(), e->y() );

    if ( modCASIsControlShift(e) ) {
        // Alt + Shift + Wheel: zoom values only, keep point under mouse
        zoomFactor = _imp->zoomCtx.factor() * scaleFactor;
        if (zoomFactor <= zoomFactor_min) {
            zoomFactor = zoomFactor_min;
            scaleFactor = zoomFactor / _imp->zoomCtx.factor();
        } else if (zoomFactor > zoomFactor_max) {
            zoomFactor = zoomFactor_max;
            scaleFactor = zoomFactor / _imp->zoomCtx.factor();
        }
        par = _imp->zoomCtx.aspectRatio() / scaleFactor;
        if (par <= par_min) {
            par = par_min;
            scaleFactor = par / _imp->zoomCtx.aspectRatio();
        } else if (par > par_max) {
            par = par_max;
            scaleFactor = par / _imp->zoomCtx.factor();
        }
        _imp->zoomCtx.zoomy(zoomCenter.x(), zoomCenter.y(), scaleFactor);
    } else if ( modCASIsControl(e) ) {
        // Alt + Wheel: zoom time only, keep point under mouse
        par = _imp->zoomCtx.aspectRatio() * scaleFactor;
        if (par <= par_min) {
            par = par_min;
            scaleFactor = par / _imp->zoomCtx.aspectRatio();
        } else if (par > par_max) {
            par = par_max;
            scaleFactor = par / _imp->zoomCtx.factor();
        }
        _imp->zoomCtx.zoomx(zoomCenter.x(), zoomCenter.y(), scaleFactor);
    } else {
        // Wheel: zoom values and time, keep point under mouse
        zoomFactor = _imp->zoomCtx.factor() * scaleFactor;
        if (zoomFactor <= zoomFactor_min) {
            zoomFactor = zoomFactor_min;
            scaleFactor = zoomFactor / _imp->zoomCtx.factor();
        } else if (zoomFactor > zoomFactor_max) {
            zoomFactor = zoomFactor_max;
            scaleFactor = zoomFactor / _imp->zoomCtx.factor();
        }
        _imp->zoomCtx.zoom(zoomCenter.x(), zoomCenter.y(), scaleFactor);
    }

    _imp->hasBeenModifiedSinceResize = true;

    computeHistogramAndRefresh();
} // wheelEvent

void
Histogram::keyPressEvent(QKeyEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    
    Qt::KeyboardModifiers modifiers = e->modifiers();
    Qt::Key key = (Qt::Key)e->key();
    
    bool accept = true;
    
    if (isKeybind(kShortcutGroupViewer, kShortcutIDActionFitViewer, modifiers, key)) {
        _imp->hasBeenModifiedSinceResize = false;
        _imp->zoomCtx.fill(0., 1., 0., 10.);
        computeHistogramAndRefresh();
    } else {
        accept = false;
    }
    if (accept) {
        takeClickFocus();
        e->accept();
    } else {
        handleUnCaughtKeyPressEvent(e);
        QGLWidget::keyPressEvent(e);
    }
}

void
Histogram::keyReleaseEvent(QKeyEvent* e)
{
    handleUnCaughtKeyUpEvent(e);
    QGLWidget::keyReleaseEvent(e);
}

void
Histogram::enterEvent(QEvent* e) {
    enterEventBase();
    QGLWidget::enterEvent(e);
}

void
Histogram::leaveEvent(QEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    leaveEventBase();
    _imp->drawCoordinates = false;
    QGLWidget::leaveEvent(e);
}

void
Histogram::showEvent(QShowEvent* e)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    QGLWidget::showEvent(e);
    if ( (width() != 0) && (height() != 0) ) {
        computeHistogramAndRefresh(true);
    }
}

void
Histogram::computeHistogramAndRefresh(bool forceEvenIfNotVisible)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    if (!isVisible() && !forceEvenIfNotVisible) {
        return;
    }

    QPointF btmLeft = _imp->zoomCtx.toZoomCoordinates(0,height() - 1);
    QPointF topRight = _imp->zoomCtx.toZoomCoordinates(width() - 1, 0);
    double vmin = btmLeft.x();
    double vmax = topRight.x();

#ifndef NATRON_HISTOGRAM_USING_OPENGL

    RectI rect;
    boost::shared_ptr<Image> image = _imp->getHistogramImage(&rect);
    if (image) {
        _imp->histogramThread.computeHistogram(_imp->mode, image, rect, width(),vmin,vmax,_imp->filterSize);
    } else {
        _imp->hasImage = false;
    }

#endif

    QPointF oldClick_opengl = _imp->zoomCtx.toZoomCoordinates( _imp->oldClick.x(),_imp->oldClick.y() );
    _imp->updatePicker( oldClick_opengl.x() );

    update();
}

#ifndef NATRON_HISTOGRAM_USING_OPENGL

void
Histogram::onCPUHistogramComputed()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    int mode;
    bool success = _imp->histogramThread.getMostRecentlyProducedHistogram(&_imp->histogram1, &_imp->histogram2, &_imp->histogram3, &_imp->binsCount, &_imp->pixelsCount, &mode,&_imp->vmin,&_imp->vmax,&_imp->mipMapLevel);
    assert(success);
    if (success) {
        _imp->hasImage = true;
        update();
    }
}

#endif

void
HistogramPrivate::drawScale()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == widget->context() );

    {
        GLenum _glerror_ = glGetError();
        if (_glerror_ != GL_NO_ERROR) {
            std::cout << "GL_ERROR :" << __FILE__ << " " << __LINE__ << " " << gluErrorString(_glerror_) << std::endl;
        }
    }

    glCheckError();
    QPointF btmLeft = zoomCtx.toZoomCoordinates(0,widget->height() - 1);
    QPointF topRight = zoomCtx.toZoomCoordinates(widget->width() - 1, 0);

    ///don't attempt to draw a scale on a widget with an invalid height
    if (widget->height() <= 1) {
        return;
    }

    QFontMetrics fontM(_font);
    const double smallestTickSizePixel = 5.; // tick size (in pixels) for alpha = 0.
    const double largestTickSizePixel = 1000.; // tick size (in pixels) for alpha = 1.


    {
        GLProtectAttrib a(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        for (int axis = 0; axis < 2; ++axis) {
            const double rangePixel = (axis == 0) ? widget->width() : widget->height(); // AXIS-SPECIFIC
            const double range_min = (axis == 0) ? btmLeft.x() : btmLeft.y(); // AXIS-SPECIFIC
            const double range_max = (axis == 0) ? topRight.x() : topRight.y(); // AXIS-SPECIFIC
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
            const double minTickSizeTextPixel = (axis == 0) ? fontM.width( QString("00") ) : fontM.height(); // AXIS-SPECIFIC
            const double minTickSizeText = range * minTickSizeTextPixel / rangePixel;
            for (int i = m1; i <= m2; ++i) {
                double value = i * smallTickSize + offset;
                const double tickSize = ticks[i - m1] * smallTickSize;
                const double alpha = ticks_alpha(smallestTickSize, largestTickSize, tickSize);

                glColor4f(_baseAxisColor.redF(), _baseAxisColor.greenF(), _baseAxisColor.blueF(), alpha);

                glBegin(GL_LINES);
                if (axis == 0) {
                    glVertex2f( value, btmLeft.y() ); // AXIS-SPECIFIC
                    glVertex2f( value, topRight.y() ); // AXIS-SPECIFIC
                } else {
                    glVertex2f(btmLeft.x(), value); // AXIS-SPECIFIC
                    glVertex2f(topRight.x(), value); // AXIS-SPECIFIC
                }
                glEnd();
                glCheckErrorIgnoreOSXBug();

                if (tickSize > minTickSizeText) {
                    const int tickSizePixel = rangePixel * tickSize / range;
                    const QString s = QString::number(value);
                    const int sSizePixel = (axis == 0) ? fontM.width(s) : fontM.height(); // AXIS-SPECIFIC
                    if (tickSizePixel > sSizePixel) {
                        const int sSizeFullPixel = sSizePixel + minTickSizeTextPixel;
                        double alphaText = 1.0; //alpha;
                        if (tickSizePixel < sSizeFullPixel) {
                            // when the text size is between sSizePixel and sSizeFullPixel,
                            // draw it with a lower alpha
                            alphaText *= (tickSizePixel - sSizePixel) / (double)minTickSizeTextPixel;
                        }
                        QColor c = _scaleColor;
                        c.setAlpha(255 * alphaText);
                        glCheckError();
                        if (axis == 0) {
                            widget->renderText(value, btmLeft.y(), s, c, _font); // AXIS-SPECIFIC
                        } else {
                            widget->renderText(btmLeft.x(), value, s, c, _font); // AXIS-SPECIFIC
                        }
                    }
                }
            }
        }
    } // GLProtectAttrib a(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
    glCheckError();
} // drawScale

void
HistogramPrivate::drawWarnings()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == widget->context() );
    if (mipMapLevel > 0) {
        QFontMetrics m(_font);
        QString str(QObject::tr("Image downscaled"));
        int strWidth = m.width(str);
        QPointF pos = zoomCtx.toZoomCoordinates(widget->width() - strWidth - 10,5 * m.height() + 30);
        glCheckError();
        widget->renderText(pos.x(), pos.y(), str,QColor(220,220,0), _font);
        glCheckError();
    }

}

void
HistogramPrivate::drawMissingImage()
{
    QPointF topLeft = zoomCtx.toZoomCoordinates(0, 0);
    QPointF btmRight = zoomCtx.toZoomCoordinates(widget->width(), widget->height());
    QPointF topRight(btmRight.x(),topLeft.y());
    QPointF btmLeft(topLeft.x(),btmRight.y());
    {
        GLProtectAttrib a(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
        
        glColor4f(0.9,0.9,0,1);
        glLineWidth(1.5);
        glBegin(GL_LINES);
        glVertex2f(topLeft.x(),topLeft.y());
        glVertex2f(btmRight.x(),btmRight.y());
        glVertex2f(btmLeft.x(),btmLeft.y());
        glVertex2f(topRight.x(),topRight.y());
        glEnd();
        glLineWidth(1.);
    }
    
    QString txt(QObject::tr("Missing image"));
    QFontMetrics m(_font);
    int strWidth = m.width(txt);
    QPointF pos = zoomCtx.toZoomCoordinates(widget->width() / 2. - strWidth / 2., m.height() + 10);
    glCheckError();
    widget->renderText(pos.x(), pos.y(), txt,QColor(220,0,0), _font);
    glCheckError();
}

void
HistogramPrivate::drawPicker()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == widget->context() );

    glCheckError();
    QFontMetrics m(_font);
    int strWidth = std::max( std::max( std::max( m.width(rValueStr),m.width(gValueStr) ),m.width(bValueStr) ),m.width(xCoordinateStr) );
    QPointF xPos = zoomCtx.toZoomCoordinates(widget->width() - strWidth - 10,m.height() + 10);
    QPointF rPos = zoomCtx.toZoomCoordinates(widget->width() - strWidth - 10,2 * m.height() + 15);
    QPointF gPos = zoomCtx.toZoomCoordinates(widget->width() - strWidth - 10,3 * m.height() + 20);
    QPointF bPos = zoomCtx.toZoomCoordinates(widget->width() - strWidth - 10,4 * m.height() + 25);
    QColor xColor, rColor,gColor,bColor;

    // Text-aware Magic colors (see recipe below):
    // - they all have the same luminance
    // - red, green and blue are complementary, but they sum up to more than 1
    xColor.setRgbF(0.398979,0.398979,0.398979);
    switch (mode) {
    case Histogram::eDisplayModeRGB:
        rColor.setRgbF(0.851643,0.196936,0.196936);
        break;
    case Histogram::eDisplayModeY:
    case Histogram::eDisplayModeA:
    case Histogram::eDisplayModeR:
    case Histogram::eDisplayModeG:
    case Histogram::eDisplayModeB:
    default:
        rColor = xColor;
        break;
    }

    gColor.setRgbF(0,0.654707,0);

    bColor.setRgbF(0.345293,0.345293,1);

    glCheckError();
    widget->renderText(xPos.x(), xPos.y(), xCoordinateStr,xColor, _font);
    widget->renderText(rPos.x(), rPos.y(), rValueStr, rColor, _font);
    widget->renderText(gPos.x(), gPos.y(), gValueStr, gColor, _font);
    widget->renderText(bPos.x(), bPos.y(), bValueStr, bColor, _font);
    glCheckError();
}

#if 0
/*
   // compute magic colors, by F. Devernay
   // - they are red, green and blue
   // - they all have the same luminance
   // - their sum is white
   // OpenGL luminance formula:
   // L = r*.3086 + g*.6094 + b*0.0820
 */
/* Maple code:
 # compute magic colors:
 # - they are red, green and blue
 # - they all have the same luminance
 # - their sum is white
 #
 # magic red, magic green, and magic blue are:
 # R = [R_r, R_gb, R_gb]
 # G = [G_rb, G_g, G_rb]
 # B = [B_rg, B_rg, B_b]
 #
 # columns of M are coefficients of [R_r, R_gb, G_g, B_b, B_gb]
 # G_rb is supposed to be zero (or there is an infinity of solutions)
 # The lines mean:
 # - the sum of all red components is 1
 # - the sum of all green components is 1
 # - the sum of all blue components is 1
 # - the luminance of magic red is 1/3
 # - the luminance of magic green is 1/3
 # - the luminance of magic blue is 1/3

 # OpenGL luminance coefficients
   r:=0.3086;g:=0.6094;b:=0.0820;

   with(LinearAlgebra):
   M := Matrix([[1, 0, 0, 0, 1], [0, 1, 1, 0, 1], [0, 1, 0, 1, 0], [3*r, 3*(g+b), 0, 0, 0], [0, 0, 3*g, 0, 0], [0, 0, 0, 3*b, 3*(r+g)]]):
   b := Vector([1,1,1,1,1,1]):
   LinearSolve(M, b);
 */
#include <stdio.h>

int
tochar(double d)
{
    return (int)(d * 255 + 0.5);
}

int
main(int argc,
     char**argv)
{
    const double Rr = 0.711519527404004;
    const double Rgb = 0.164533420851110;
    const double Gg = 0.546986106552894;
    const double Grb = 0.;
    const double Bb = 0.835466579148890;
    const double Brg = 0.288480472595996;
    const double R[3] = {
        Rr, Rgb, Rgb
    };
    const double G[3] = {
        Grb, Gg, Grb
    };
    const double B[3] = {
        Brg, Brg, Bb
    };
    const double maxval = Bb;

    printf("OpenGL colors, luminance = 1/3:\n");
    printf( "red=(%g,%g,%g) green=(%g,%g,%g) blue=(%g,%g,%g)\n",
            (R[0]), (R[1]), (R[2]),
            (G[0]), (G[1]), (G[2]),
            (B[0]), (B[1]), (B[2]) );
    printf("OpenGL colors luminance=%g:\n", (1. / 3.) / maxval);
    printf( "red=(%g,%g,%g) green=(%g,%g,%g) blue=(%g,%g,%g)\n",
            (R[0] / maxval), (R[1] / maxval), (R[2] / maxval),
            (G[0] / maxval), (G[1] / maxval), (G[2] / maxval),
            (B[0] / maxval), (B[1] / maxval), (B[2] / maxval) );
    printf("HTML colors, luminance=1/3:\n");
    printf( "red=#%02x%02x%02x green=#%02x%02x%02x blue=#%02x%02x%02x\n",
            tochar(R[0]), tochar(R[1]), tochar(R[2]),
            tochar(G[0]), tochar(G[1]), tochar(G[2]),
            tochar(B[0]), tochar(B[1]), tochar(B[2]) );
    printf("HTML colors, luminance=%g:\n", (1. / 3.) / maxval);
    printf( "red=#%02x%02x%02x green=#%02x%02x%02x blue=#%02x%02x%02x\n",
            tochar(R[0] / maxval), tochar(R[1] / maxval), tochar(R[2] / maxval),
            tochar(G[0] / maxval), tochar(G[1] / maxval), tochar(G[2] / maxval),
            tochar(B[0] / maxval), tochar(B[1] / maxval), tochar(B[2] / maxval) );
}

#endif // if 0

#ifndef NATRON_HISTOGRAM_USING_OPENGL
void
HistogramPrivate::drawHistogramCPU()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == widget->context() );

    glCheckError();
    {
        GLProtectAttrib a(GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);

        glEnable(GL_BLEND);
        glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
        glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE);

        // see the code above to compute the magic colors

        double binSize = (vmax - vmin) / binsCount;

        glBegin(GL_LINES);
        for (unsigned int i = 0; i < binsCount; ++i) {
            double binMinX = vmin + i * binSize;
            if (mode == Histogram::eDisplayModeRGB) {
                if ( histogram1.empty() || histogram2.empty() || histogram3.empty() ) {
                    break;
                }
                double rTotNormalized = ( (double)histogram1[i] / (double)pixelsCount ) / binSize;
                double gTotNormalized = ( (double)histogram2[i] / (double)pixelsCount ) / binSize;
                double bTotNormalized = ( (double)histogram3[i] / (double)pixelsCount ) / binSize;

                // use three colors with equal luminance (0.33), so that the blue is visible and their sum is white
                //glColor3d(1, 0, 0);
                glColor3f(0.711519527404004, 0.164533420851110, 0.164533420851110);
                glVertex2d(binMinX, 0);
                glVertex2d(binMinX,  rTotNormalized);

                //glColor3d(0, 1, 0);
                glColor3f(0., 0.546986106552894, 0.);
                glVertex2d(binMinX, 0);
                glVertex2d(binMinX,  gTotNormalized);

                //glColor3d(0, 0, 1);
                glColor3f(0.288480472595996, 0.288480472595996, 0.835466579148890);
                glVertex2d(binMinX, 0);
                glVertex2d(binMinX,  bTotNormalized);
            } else {
                if ( histogram1.empty() ) {
                    break;
                }

                double vTotNormalized = (double)histogram1[i] / (double)pixelsCount / binSize;

                // all the following colors have the same luminance (~0.4)
                switch (mode) {
                    case Histogram::eDisplayModeR:
                        //glColor3f(1, 0, 0);
                        glColor3f(0.851643,0.196936,0.196936);
                        break;
                    case Histogram::eDisplayModeG:
                        //glColor3f(0, 1, 0);
                        glColor3f(0,0.654707,0);
                        break;
                    case Histogram::eDisplayModeB:
                        //glColor3f(0, 0, 1);
                        glColor3f(0.345293,0.345293,1);
                        break;
                    case Histogram::eDisplayModeA:
                        //glColor3f(1, 1, 1);
                        glColor3f(0.398979,0.398979,0.398979);
                        break;
                    case Histogram::eDisplayModeY:
                        //glColor3f(0.7, 0.7, 0.7);
                        glColor3f(0.398979,0.398979,0.398979);
                        break;
                    default:
                        assert(false);
                        break;
                }
                glVertex2f(binMinX, 0);
                glVertex2f(binMinX,  vTotNormalized);
            }
        }
        glEnd(); // GL_LINES
        glCheckErrorIgnoreOSXBug();
    } // GLProtectAttrib a(GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
    glCheckError();
} // drawHistogramCPU

#endif // ifndef NATRON_HISTOGRAM_USING_OPENGL

void
Histogram::renderText(double x,
                      double y,
                      const QString & text,
                      const QColor & color,
                      const QFont & font) const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );

    if ( text.isEmpty() ) {
        return;
    }

    double w = (double)width();
    double h = (double)height();
    double bottom = _imp->zoomCtx.bottom();
    double left = _imp->zoomCtx.left();
    double top =  _imp->zoomCtx.top();
    double right = _imp->zoomCtx.right();
    if (w <= 0 || h <= 0 || right <= left || top <= bottom) {
        return;
    }
    double scalex = (right-left) / w;
    double scaley = (top-bottom) / h;
    _imp->textRenderer.renderText(x, y, scalex, scaley, text, color, font);
    glCheckError();
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_Histogram.cpp"
