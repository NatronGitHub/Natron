//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "Histogram.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QSplitter>
#include <QGLShaderProgram>
CLANG_DIAG_OFF(unused-private-field)
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QMouseEvent>
CLANG_DIAG_ON(unused-private-field)
#include <QDebug>
#include <QCoreApplication>
#include <QMenu>
#include <QActionGroup>

#include "Engine/Image.h"
#include "Engine/ViewerInstance.h"
#include "Engine/HistogramCPU.h"

#include "Gui/ticks.h"
#include "Gui/Shaders.h"
#include "Gui/Gui.h"
#include "Gui/ComboBox.h"
#include "Gui/ClickableLabel.h"
#include "Gui/ViewerTab.h"
#include "Gui/Texture.h"
#include "Gui/ViewerGL.h"
#include "Gui/TextRenderer.h"

// warning: 'gluErrorString' is deprecated: first deprecated in OS X 10.9 [-Wdeprecated-declarations]
CLANG_DIAG_OFF(deprecated-declarations)
GCC_DIAG_OFF(deprecated-declarations)

namespace { // protext local classes in anonymous namespace
    
    // a data container with only a constructor, a destructor, and a few utility functions is really just a struct
    // see ViewerGL.cpp for a full documentation of ZoomContext
    struct ZoomContext {
        
        ZoomContext()
        : bottom(0.)
        , left(0.)
        , zoomFactor(1.)
        , aspectRatio(1.)
        {}
        
        QPoint _oldClick; /// the last click pressed, in widget coordinates [ (0,0) == top left corner ]
        double bottom; /// the bottom edge of orthographic projection
        double left; /// the left edge of the orthographic projection
        double zoomFactor; /// the zoom factor applied to the current image
        double aspectRatio;///
        
        double _lastOrthoLeft,_lastOrthoBottom,_lastOrthoRight,_lastOrthoTop; //< remembers the last values passed to the glOrtho call
    };
    
    enum EventState {
        DRAGGING_VIEW = 0,
        NONE = 1
    };
}

struct HistogramPrivate
{
    
    
    Gui* gui; //< ptr to the gui
    
    QVBoxLayout* mainLayout;
    
    ///////// OPTIONS
    QMenu* rightClickMenu;
    
    QMenu* histogramSelectionMenu;
    QActionGroup* histogramSelectionGroup;
    
    QActionGroup* modeActions;
    QMenu* modeMenu;
    
    QAction* fullImage;
    
    QActionGroup* filterActions;
    QMenu* filterMenu;
    
    Histogram* widget;
    Histogram::DisplayMode mode;
    ZoomContext zoomCtx;
    bool supportsGLSL;
    bool hasOpenGLVAOSupport;
    EventState state;
    bool hasBeenModifiedSinceResize; //< true if the user panned or zoomed since the last resize
    
    QColor _baseAxisColor;
    QColor _scaleColor;
    QFont _font;
    Natron::TextRenderer textRenderer;
    
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
#else
    
    HistogramCPU histogramThread;
    
    ///up to 3 histograms (in the RGB) case. FOr all other cases just histogram1 is used.
    std::vector<float> histogram1;
    std::vector<float> histogram2;
    std::vector<float> histogram3;
    unsigned int pixelsCount;
    double vmin,vmax; //< the x range of the histogram
    unsigned int binsCount;
    
#endif
    HistogramPrivate(Gui* parent,Histogram* widget)
    : gui(parent)
    , mainLayout(NULL)
    , rightClickMenu(NULL)
    , histogramSelectionMenu(NULL)
    , histogramSelectionGroup(NULL)
    , modeMenu(NULL)
    , fullImage(NULL)
    , filterMenu(NULL)
    , widget(widget)
    , mode(Histogram::RGB)
    , zoomCtx()
    , supportsGLSL(true)
    , hasOpenGLVAOSupport(true)
    , state(NONE)
    , hasBeenModifiedSinceResize(false)
    , _baseAxisColor(118,215,90,255)
    , _scaleColor(67,123,52,255)
    , _font(NATRON_FONT, NATRON_FONT_SIZE_10)
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
#endif
    {
    }
    
    
    boost::shared_ptr<Natron::Image> getHistogramImage(RectI* imagePortion) const;

    
    void showMenu(const QPoint& globalPos);
        
    void drawScale();
    
    void drawPicker();
    
    void updatePicker(double x);
    
#ifdef NATRON_HISTOGRAM_USING_OPENGL
    
    void resizeComputingVBO(int w,int h);

    
    ///For all these functions, mode refers to either R,G,B,A or Y
    void computeHistogram(Histogram::DisplayMode mode);
    void renderHistogram(Histogram::DisplayMode mode);
    void activateHistogramComputingShader(Histogram::DisplayMode mode);
    void activateHistogramRenderingShader(Histogram::DisplayMode mode);
    
#else
    void drawHistogramCPU();
#endif
};

Histogram::Histogram(Gui* gui, const QGLWidget* shareWidget)
: QGLWidget(gui,shareWidget)
, _imp(new HistogramPrivate(gui,this))
{
    setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Expanding);
    setMouseTracking(true);
#ifndef NATRON_HISTOGRAM_USING_OPENGL
    QObject::connect(&_imp->histogramThread, SIGNAL(histogramProduced()), this, SLOT(onCPUHistogramComputed()));
#endif
    
    
    _imp->rightClickMenu = new QMenu(this);
    
    _imp->histogramSelectionMenu = new QMenu("Viewer target",_imp->rightClickMenu);
    _imp->rightClickMenu->addAction(_imp->histogramSelectionMenu->menuAction());
    
    _imp->histogramSelectionGroup = new QActionGroup(_imp->histogramSelectionMenu);
    
    _imp->modeMenu = new QMenu("Display mode",_imp->rightClickMenu);
    _imp->rightClickMenu->addAction(_imp->modeMenu->menuAction());
    
    _imp->fullImage = new QAction(_imp->rightClickMenu);
    _imp->fullImage->setText("Full image");
    _imp->fullImage->setCheckable(true);
    _imp->fullImage->setChecked(false);
    QObject::connect(_imp->fullImage, SIGNAL(triggered()), this, SLOT(computeHistogramAndRefresh()));
    _imp->rightClickMenu->addAction(_imp->fullImage);
    
    _imp->filterMenu = new QMenu("Filter",_imp->rightClickMenu);
    _imp->rightClickMenu->addAction(_imp->filterMenu->menuAction());
    
    _imp->modeActions = new QActionGroup(_imp->modeMenu);
    QAction* rgbAction = new QAction(_imp->modeMenu);
    rgbAction->setText(QString("RGB"));
    rgbAction->setData(0);
    _imp->modeActions->addAction(rgbAction);
    
    QAction* aAction = new QAction(_imp->modeMenu);
    aAction->setText(QString("A"));
    aAction->setData(1);
    _imp->modeActions->addAction(aAction);
    
    QAction* yAction = new QAction(_imp->modeMenu);
    yAction->setText(QString("Y"));
    yAction->setData(2);
    _imp->modeActions->addAction(yAction);
    
    QAction* rAction = new QAction(_imp->modeMenu);
    rAction->setText(QString("R"));
    rAction->setData(3);
    _imp->modeActions->addAction(rAction);
    
    QAction* gAction = new QAction(_imp->modeMenu);
    gAction->setText(QString("G"));
    gAction->setData(4);
    _imp->modeActions->addAction(gAction);
    
    QAction* bAction = new QAction(_imp->modeMenu);
    bAction->setText(QString("B"));
    bAction->setData(5);
    _imp->modeActions->addAction(bAction);
    QList<QAction*> actions = _imp->modeActions->actions();
    for (int i = 0; i < actions.size();++i) {
        _imp->modeMenu->addAction(actions.at(i));
    }
    
    QObject::connect(_imp->modeActions,SIGNAL(triggered(QAction*)),this,SLOT(onDisplayModeChanged(QAction*)));
    
    
    
    _imp->filterActions = new QActionGroup(_imp->filterMenu);
    QAction* noSmoothAction = new QAction(_imp->filterActions);
    noSmoothAction->setText("No smoothing");
    noSmoothAction->setData(0);
    noSmoothAction->setCheckable(true);
    noSmoothAction->setChecked(true);
    _imp->filterActions->addAction(noSmoothAction);
    
    QAction* size3Action = new QAction(_imp->filterActions);
    size3Action->setText("Size 3");
    size3Action->setData(3);
    size3Action->setCheckable(true);
    size3Action->setChecked(false);
    _imp->filterActions->addAction(size3Action);
    
    QAction* size5Action = new QAction(_imp->filterActions);
    size5Action->setText("Size 5");
    size5Action->setData(5);
    size5Action->setCheckable(true);
    size5Action->setChecked(false);
    _imp->filterActions->addAction(size5Action);
    
    actions = _imp->filterActions->actions();
    for (int i = 0; i < actions.size();++i) {
        _imp->filterMenu->addAction(actions.at(i));
    }
    
    QObject::connect(_imp->filterActions, SIGNAL(triggered(QAction*)), this, SLOT(onFilterChanged(QAction*)));
    
    QObject::connect(_imp->gui, SIGNAL(viewersChanged()), this, SLOT(populateViewersChoices()));
    populateViewersChoices();
}

Histogram::~Histogram()
{
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


boost::shared_ptr<Natron::Image> HistogramPrivate::getHistogramImage(RectI* imagePortion) const
{
    bool useImageRoD = fullImage->isChecked();
    
    int index = 0;
    std::string viewerName ;
    QAction* selectedHistAction = histogramSelectionGroup->checkedAction();
    if (selectedHistAction) {
        index = selectedHistAction->data().toInt();
        viewerName = selectedHistAction->text().toStdString();
    }
    
    if (index == 0) {
        //no viewer selected
        imagePortion->clear();
        return boost::shared_ptr<Natron::Image>();
    } else if (index == 1) {
        //current viewer
        ViewerTab* lastSelectedViewer = gui->getLastSelectedViewer();
        boost::shared_ptr<Natron::Image> ret;
        if (lastSelectedViewer) {
            ret = lastSelectedViewer->getInternalNode()->getLastRenderedImage();
        }
        if (ret) {
            if (!useImageRoD) {
                if (lastSelectedViewer) {
                    *imagePortion = lastSelectedViewer->getViewer()->getImageRectangleDisplayed(ret->getRoD());
                }
            } else {
                *imagePortion = ret->getRoD();
            }
        }
        return ret;
    } else {
        boost::shared_ptr<Natron::Image> ret;
        const std::list<ViewerTab*>& viewerTabs = gui->getViewersList();
        for (std::list<ViewerTab*>::const_iterator it = viewerTabs.begin(); it != viewerTabs.end(); ++it) {
            if ((*it)->getInternalNode()->getName() == viewerName) {
                ret = (*it)->getInternalNode()->getLastRenderedImage();
                if (ret) {
                    if (!useImageRoD) {
                        *imagePortion = (*it)->getViewer()->getImageRectangleDisplayed(ret->getRoD());
                    } else {
                        *imagePortion = ret->getRoD();
                    }
                }
                return ret;
            }
        }
        return ret;
    }
    
}

void HistogramPrivate::showMenu(const QPoint& globalPos)
{
    rightClickMenu->exec(globalPos);
}


void Histogram::populateViewersChoices()
{
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
    currentAction->setText("Current Viewer");
    currentAction->setData(1);
    currentAction->setCheckable(true);
    currentAction->setChecked(false);
    _imp->histogramSelectionGroup->addAction(currentAction);
    _imp->histogramSelectionMenu->addAction(currentAction);
    
    
    const std::list<ViewerTab*>& viewerTabs = _imp->gui->getViewersList();
    int c = 2;
    for (std::list<ViewerTab*>::const_iterator it = viewerTabs.begin(); it != viewerTabs.end(); ++it) {
        if ((*it)->getInternalNode()->getNode()->isActivated()) {
            QAction* ac = new QAction(_imp->histogramSelectionGroup);
            ac->setText((*it)->getInternalNode()->getName().c_str());
            ac->setCheckable(true);
            ac->setChecked(false);
            ac->setData(c);
            _imp->histogramSelectionGroup->addAction(ac);
            _imp->histogramSelectionMenu->addAction(ac);
            ++c;
        }
    }
    
    _imp->histogramSelectionGroup->blockSignals(true);
    if (!currentSelection.isEmpty()) {
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
    
    QObject::connect(_imp->histogramSelectionGroup,SIGNAL(triggered(QAction*)),this,SLOT(onCurrentViewerChanged(QAction*)));
    
}

void Histogram::onCurrentViewerChanged(QAction*)
{
    computeHistogramAndRefresh();
}

void Histogram::onViewerImageChanged(ViewerGL* viewer)
{
    if (viewer) {
        QString viewerName = viewer->getInternalNode()->getName().c_str();
        ViewerTab* lastSelectedViewer = _imp->gui->getLastSelectedViewer();
        QString currentViewerName;
        if (lastSelectedViewer) {
            currentViewerName = lastSelectedViewer->getInternalNode()->getName().c_str();
        }
        
        QAction* selectedHistAction = _imp->histogramSelectionGroup->checkedAction();
        if (selectedHistAction) {
            int actionIndex = selectedHistAction->data().toInt();
            
            if ((actionIndex == 1 && lastSelectedViewer == viewer->getViewerTab())
                || (actionIndex > 1 && selectedHistAction->text() == viewerName)) {
                    computeHistogramAndRefresh();
                }
        
        }
    }
}


QSize Histogram::sizeHint() const
{
    return QSize(500,1000);
}


void Histogram::onFilterChanged(QAction* action)
{
    _imp->filterSize = action->data().toInt();
    computeHistogramAndRefresh();
}


void Histogram::onDisplayModeChanged(QAction* action)
{
    _imp->mode = (Histogram::DisplayMode)action->data().toInt();
    computeHistogramAndRefresh();
}


void Histogram::initializeGL()
{
    
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        /* Problem: glewInit failed, something is seriously wrong. */
        Natron::errorDialog("OpenGL/GLEW error",
                            (const char*)glewGetErrorString(err));
    }
    
    if (!QGLShaderProgram::hasOpenGLShaderPrograms(context())) {
        _imp->supportsGLSL = false;
    } else {
        
#ifdef NATRON_HISTOGRAM_USING_OPENGL
        _imp->histogramComputingShader.reset(new QGLShaderProgram(context()));
        if(!_imp->histogramComputingShader->addShaderFromSourceCode(QGLShader::Vertex,histogramComputationVertex_vert))
            qDebug() << _imp->histogramComputingShader->log();
        if(!_imp->histogramComputingShader->addShaderFromSourceCode(QGLShader::Fragment,histogramComputation_frag))
            qDebug() << _imp->histogramComputingShader->log();
        if(!_imp->histogramComputingShader->link()){
            qDebug() << _imp->histogramComputingShader->log();
        }
        
        _imp->histogramMaximumShader.reset(new QGLShaderProgram(context()));
        if(!_imp->histogramMaximumShader->addShaderFromSourceCode(QGLShader::Fragment,histogramMaximum_frag))
            qDebug() << _imp->histogramMaximumShader->log();
        if(!_imp->histogramMaximumShader->link()){
            qDebug() << _imp->histogramMaximumShader->log();
        }
        
        _imp->histogramRenderingShader.reset(new QGLShaderProgram(context()));
        if(!_imp->histogramRenderingShader->addShaderFromSourceCode(QGLShader::Vertex,histogramRenderingVertex_vert))
            qDebug() << _imp->histogramRenderingShader->log();
        if(!_imp->histogramRenderingShader->addShaderFromSourceCode(QGLShader::Fragment,histogramRendering_frag))
            qDebug() << _imp->histogramRenderingShader->log();
        if(!_imp->histogramRenderingShader->link()){
            qDebug() << _imp->histogramRenderingShader->log();
        }
#endif
    }
    
    if (!glewIsSupported("GL_ARB_vertex_array_object "  // BindVertexArray, DeleteVertexArrays, GenVertexArrays, IsVertexArray (VAO), core since 3.0
                         )) {
        _imp->hasOpenGLVAOSupport = false;
    }
    
#ifdef NATRON_HISTOGRAM_USING_OPENGL
    
    glEnable(GL_TEXTURE_RECTANGLE_ARB);
    

    glGenTextures(3,&_imp->histogramTexture[0]);
    glGenTextures(3,&_imp->histogramReductionsTexture[0]);
    glGenTextures(3,&_imp->histogramRenderTexture[0]);
    glGenTextures(3,&_imp->histogramMaximumTexture[0]);
    
    _imp->leftImageTexture.reset(new Texture(GL_TEXTURE_RECTANGLE_ARB,GL_NEAREST,GL_NEAREST));
    _imp->rightImageTexture.reset(new Texture(GL_TEXTURE_RECTANGLE_ARB,GL_NEAREST,GL_NEAREST));
    
    glGenFramebuffers(3,&_imp->fboReductions[0]);
    glGenFramebuffers(1,&_imp->fboMaximum);
    glGenFramebuffers(1,&_imp->fbohistogram);
    glGenFramebuffers(1,&_imp->fboRendering);
    
    glGenVertexArrays(1,&_imp->vaoID);
    glGenBuffers(1,&_imp->vboID);
    glGenBuffers(1,&_imp->vboHistogramRendering);
    
    /*initializing histogram fbo and attaching the 3 textures */
    glBindFramebuffer(GL_FRAMEBUFFER,_imp->fbohistogram);
    GLenum attachments[3] = {GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1,GL_COLOR_ATTACHMENT2};
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
        glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,0,GL_R32F,256.f/pow(4.f,(float)(i+1)),1,0,GL_RED,GL_FLOAT,0);
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
    int vertexCount = 512;// we draw the bins as lines (2 vertex) hence 512 vertices
    glBufferData(GL_ARRAY_BUFFER,vertexCount*3*sizeof(float),NULL,GL_DYNAMIC_DRAW);
    float *gpuVertexBuffer = reinterpret_cast<float*>(glMapBuffer(GL_ARRAY_BUFFER,GL_WRITE_ONLY));
    int j=0;
    int x=0;
    while (j< vertexCount*3) {
        gpuVertexBuffer[j]=x;
        gpuVertexBuffer[j+1]=0.f;
        gpuVertexBuffer[j+2]=0.f;
        gpuVertexBuffer[j+3]=x;
        gpuVertexBuffer[j+4]=0.f;
        gpuVertexBuffer[j+5]=1.f;
        x++;
        j+=6;
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER,0);
    
#endif
}


#ifdef NATRON_HISTOGRAM_USING_OPENGL
void HistogramPrivate::resizeComputingVBO(int w,int h)
{
    glBindBuffer(GL_ARRAY_BUFFER,vboID);
    int vertexCount = w * h;
    glBufferData(GL_ARRAY_BUFFER,vertexCount*2*sizeof(float),NULL,GL_DYNAMIC_DRAW);
    float* gpuVertexBuffer = reinterpret_cast<float*>(glMapBuffer(GL_ARRAY_BUFFER,GL_WRITE_ONLY));
    int i = 0;
    float x = 0;
    float y = 0;
    while (i < (vertexCount * 2)) {
        if(x != 0 && (int)x % w == 0){
            x = 0;
            ++y;
        }
        gpuVertexBuffer[i]=x;
        gpuVertexBuffer[i+1]=y;
        ++x;
        i+=2;
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER,0);
}



static void textureMap_Polygon(int fromX,int fromY,int fromW,int fromH,int x,int y,int w,int h)
{
    glBegin(GL_POLYGON);
    glTexCoord2i (fromX, fromH);glVertex2i (x,h);
    glTexCoord2i (fromW, fromH);glVertex2i (w,h);
    glTexCoord2i (fromW, fromY);glVertex2i (w,y);
    glTexCoord2i (fromX, fromY);glVertex2i (x,y);
    glEnd ();
}

static void startRenderingTo(GLuint fboId,GLenum attachment,int w,int h)
{
    glBindFramebuffer(GL_FRAMEBUFFER,fboId);
    glDrawBuffer(attachment);
    glPushAttrib(GL_VIEWPORT_BIT | GL_COLOR_BUFFER_BIT);
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0,w,0,h,-1,1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
}
static void stopRenderingTo()
{
    glPopAttrib();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glBindFramebuffer(GL_FRAMEBUFFER,0);
}

static int shaderChannelFromDisplayMode(Histogram::DisplayMode channel)
{
    switch (channel) {
        case Histogram::R:
            return 1;
            break;
        case Histogram::G:
            return 2;
            break;
        case Histogram::B:
            return 3;
            break;
        case Histogram::A:
            return 4;
            break;
        case Histogram::Y:
            return 0;
            break;
        case Histogram::RGB:
        default:
            assert(false);
            break;
    }
}

void HistogramPrivate::activateHistogramComputingShader(Histogram::DisplayMode channel)
{
    histogramComputingShader->bind();
    histogramComputingShader->setUniformValue("Tex",0);
    glCheckError();
    histogramComputingShader->setUniformValue("channel",shaderChannelFromDisplayMode(channel));
    glBindAttribLocation(histogramComputingShader->programId(),0,"TexCoord");
}

void HistogramPrivate::activateHistogramRenderingShader(Histogram::DisplayMode channel)
{
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
    histogramRenderingShader->setUniformValue("channel",shaderChannelFromDisplayMode(channel));
    glBindAttribLocation(histogramRenderingShader->programId(),0,"TexCoord");
}

static GLenum colorAttachmentFromDisplayMode(Histogram::DisplayMode channel)
{
    switch (channel) {
        case Histogram::R:
            return GL_COLOR_ATTACHMENT0;
            break;
        case Histogram::G:
            return GL_COLOR_ATTACHMENT1;
            break;
        case Histogram::B:
            return GL_COLOR_ATTACHMENT2;
            break;
        case Histogram::A:
            return GL_COLOR_ATTACHMENT0;
            break;
        case Histogram::Y:
            return GL_COLOR_ATTACHMENT0;
            break;
        case Histogram::RGB:
        default:
            ///isn't meant for other components than the one handled here
            assert(false);
            break;
    }
}

void HistogramPrivate::computeHistogram(Histogram::DisplayMode channel)
{
    GLenum attachment = colorAttachmentFromDisplayMode(channel);
    
    /*binding the VAO holding managing the VBO*/
    glBindVertexArray(vaoID);
    /*binding the VBO sending vertices to the vertex shader*/
    glBindBuffer(GL_ARRAY_BUFFER,vboID);
    /*each attribute will be a vec2f*/
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,0);
    /*enabling the VBO 0 of the VAO*/
    glEnableVertexAttribArray(0);
    /*start rendering to the histogram texture held by the _fboHistogram*/
    startRenderingTo(fbohistogram,attachment,256,1);
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
    
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB,leftImageTexture->getTexID());
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
    for(unsigned int i = 0 ; i < 3 ; i++){
        int wTarget = (int)(256.f/pow(4.f,(float)(i+1)));
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
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB,histogramReductionsTexture[2]);
    histogramMaximumShader->bind();
    histogramMaximumShader->setUniformValue("Tex",0);
    startRenderingTo(fboMaximum,attachment,1,1);
    textureMap_Polygon(0,0,1,1,0,0,1,1);
    histogramMaximumShader->release();
    stopRenderingTo();
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB,0);
    glCheckError();
}

void HistogramPrivate::renderHistogram(Histogram::DisplayMode channel)
{
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

#endif

void Histogram::paintGL()
{
    double w = (double)width();
    double h = (double)height();
    
    ///don't bother painting an invisible widget, this may be caused to a splitter collapsed.
    if (w <= 1 || h <= 1) {
        return;
    }
    
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
    if(_imp->zoomCtx.zoomFactor <= 0){
        return;
    }
    double bottom = _imp->zoomCtx.bottom;
    double left = _imp->zoomCtx.left;
    double top = bottom +  h / (double)_imp->zoomCtx.zoomFactor * _imp->zoomCtx.aspectRatio ;
    double right = left +  (w / (double)_imp->zoomCtx.zoomFactor);
    if(left == right || top == bottom){
        glClearColor(0,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT);
        return;
    }
    _imp->zoomCtx._lastOrthoLeft = left;
    _imp->zoomCtx._lastOrthoRight = right;
    _imp->zoomCtx._lastOrthoBottom = bottom;
    _imp->zoomCtx._lastOrthoTop = top;
    glOrtho(left , right, bottom, top, -1, 1);
    glCheckError();
    
    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity();
    
    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT);

    
    _imp->drawScale();
    
#ifndef NATRON_HISTOGRAM_USING_OPENGL
    _imp->drawHistogramCPU();
#endif
    if (_imp->drawCoordinates) {
        _imp->drawPicker();
    }
}

void Histogram::resizeGL(int w, int h)
{
    if (h == 0) {
        h = 1;
    }
    glViewport (0, 0, w , h);
    if (!_imp->hasBeenModifiedSinceResize) {
        centerOn(0, 1, 0, 1);
    }
}

QPointF Histogram::toHistogramCoordinates(double x,double y) const
{
    double w = (double)width() ;
    double h = (double)height();
    double bottom = _imp->zoomCtx.bottom;
    double left = _imp->zoomCtx.left;
    double top =  bottom +  h / _imp->zoomCtx.zoomFactor * _imp->zoomCtx.aspectRatio ;
    double right = left +  w / _imp->zoomCtx.zoomFactor;
    return QPointF((((right - left)*x)/w)+left,(((bottom - top)*y)/h)+top);
}

QPointF Histogram::toWidgetCoordinates(double x, double y) const
{
    double w = (double)width() ;
    double h = (double)height();
    double bottom = _imp->zoomCtx.bottom;
    double left = _imp->zoomCtx.left;
    double top =  bottom +  h / _imp->zoomCtx.zoomFactor * _imp->zoomCtx.aspectRatio ;
    double right = left +  w / _imp->zoomCtx.zoomFactor;
    return QPointF(((x - left)/(right - left))*w,((y - top)/(bottom - top))*h);
}

void Histogram::centerOn(double xmin,double xmax,double ymin,double ymax)
{
    double curveWidth = xmax - xmin;
    double curveHeight = (ymax - ymin);
    double w = width();
    double h = height() * _imp->zoomCtx.aspectRatio ;
    if (w / h < curveWidth / curveHeight) {
        _imp->zoomCtx.left = xmin;
        _imp->zoomCtx.zoomFactor = w / curveWidth;
        _imp->zoomCtx.bottom = (ymax + ymin) / 2. - ((h / w) * curveWidth / 2.);
    } else {
        _imp->zoomCtx.bottom = ymin;
        _imp->zoomCtx.zoomFactor = h / curveHeight;
        _imp->zoomCtx.left = (xmax + xmin) / 2. - ((w / h) * curveHeight / 2.);
    }
        
    computeHistogramAndRefresh();
}

void Histogram::mousePressEvent(QMouseEvent* event)
{
    ////
    // middle button: scroll view
    if (event->button() == Qt::MiddleButton || event->modifiers().testFlag(Qt::AltModifier) ) {
        _imp->state = DRAGGING_VIEW;
        _imp->zoomCtx._oldClick = event->pos();
    } else if (event->button() == Qt::RightButton) {
        _imp->showMenu(event->globalPos());
    }
    
}

void Histogram::mouseMoveEvent(QMouseEvent* event)
{
    QPointF newClick_opengl = toHistogramCoordinates(event->x(),event->y());
    QPointF oldClick_opengl = toHistogramCoordinates(_imp->zoomCtx._oldClick.x(),_imp->zoomCtx._oldClick.y());
    

    _imp->zoomCtx._oldClick = event->pos();
    _imp->drawCoordinates = true;
    _imp->updatePicker(newClick_opengl.x());
    
    switch (_imp->state) {
        case DRAGGING_VIEW:
            _imp->zoomCtx.bottom += (oldClick_opengl.y() - newClick_opengl.y());
            _imp->zoomCtx.left += (oldClick_opengl.x() - newClick_opengl.x());
            _imp->hasBeenModifiedSinceResize = true;
            computeHistogramAndRefresh();
            break;
        case NONE:
            update();
            break;
    }
    
    
}

void HistogramPrivate::updatePicker(double x)
{
    xCoordinateStr = QString("x=") + QString::number(x,'f',6);
    double binSize = (vmax - vmin) / binsCount;
    int index = (int)((x - vmin) / binSize);
    rValueStr.clear();
    gValueStr.clear();
    bValueStr.clear();
    if (mode == Histogram::RGB) {
        float r = histogram1.empty() ? 0 :  histogram1[index];
        float g = histogram2.empty() ? 0 :  histogram2[index];
        float b = histogram3.empty() ? 0 :  histogram3[index];
        rValueStr = QString("r=") + QString::number(r);
        gValueStr = QString("g=") + QString::number(g);
        bValueStr = QString("b=") + QString::number(b);
    } else if (mode == Histogram::Y) {
        float y = histogram1[index];
        rValueStr = QString("y=") + QString::number(y);
    } else if (mode == Histogram::A) {
        float a = histogram1[index];
        rValueStr = QString("a=") + QString::number(a);
    } else if (mode == Histogram::R) {
        float r = histogram1[index];
        rValueStr = QString("r=") + QString::number(r);
    } else if (mode == Histogram::G) {
        float g = histogram1[index];
        gValueStr = QString("g=") + QString::number(g);
    } else if (mode == Histogram::B) {
        float b = histogram1[index];
        bValueStr = QString("b=") + QString::number(b);
    } else {
        assert(false);
    }
}

void Histogram::mouseReleaseEvent(QMouseEvent* /*event*/)
{
    _imp->state = NONE;
}

void Histogram::wheelEvent(QWheelEvent *event)
{
    // don't handle horizontal wheel (e.g. on trackpad or Might Mouse)
    if (event->orientation() != Qt::Vertical) {
        return;
    }
    
    const double oldAspectRatio = _imp->zoomCtx.aspectRatio;
    const double oldZoomFactor = _imp->zoomCtx.zoomFactor;
    double newAspectRatio = oldAspectRatio;
    double newZoomFactor = oldZoomFactor;
    const double scaleFactor = std::pow(NATRON_WHEEL_ZOOM_PER_DELTA, event->delta());
    
    if (event->modifiers().testFlag(Qt::ControlModifier) && event->modifiers().testFlag(Qt::ShiftModifier)) {
        // Alt + Shift + Wheel: zoom values only, keep point under mouse
        newAspectRatio *= scaleFactor;
        if (newAspectRatio <= 0.000001) {
            newAspectRatio = 0.000001;
        } else if (newAspectRatio > 1000000.) {
            newAspectRatio = 1000000.;
        }
    } else if (event->modifiers().testFlag(Qt::ControlModifier)) {
        // Alt + Wheel: zoom time only, keep point under mouse
        newAspectRatio *= scaleFactor;
        newZoomFactor *= scaleFactor;
        if (newZoomFactor <= 0.000001) {
            newAspectRatio *= 0.000001/newZoomFactor;
            newZoomFactor = 0.000001;
        } else if (newZoomFactor > 1000000.) {
            newAspectRatio *= 1000000./newZoomFactor;
            newZoomFactor = 1000000.;
        }
        if (newAspectRatio <= 0.000001) {
            newZoomFactor *= 0.000001/newAspectRatio;
            newAspectRatio = 0.000001;
        } else if (newAspectRatio > 1000000.) {
            newZoomFactor *= 1000000./newAspectRatio;
            newAspectRatio = 1000000.;
        }
    } else {
        // Wheel: zoom values and time, keep point under mouse
        newZoomFactor *= scaleFactor;
        if (newZoomFactor <= 0.000001) {
            newZoomFactor = 0.000001;
        } else if (newZoomFactor > 1000000.) {
            newZoomFactor = 1000000.;
        }
    }
    QPointF zoomCenter = toHistogramCoordinates(event->x(), event->y());
    double zoomRatio =  oldZoomFactor / newZoomFactor;
    double aspectRatioRatio =  oldAspectRatio / newAspectRatio;
    _imp->zoomCtx.left = zoomCenter.x() - (zoomCenter.x() - _imp->zoomCtx.left)*zoomRatio ;
    _imp->zoomCtx.bottom = zoomCenter.y() - (zoomCenter.y() - _imp->zoomCtx.bottom)*zoomRatio/aspectRatioRatio;
    
    _imp->zoomCtx.aspectRatio = newAspectRatio;
    _imp->zoomCtx.zoomFactor = newZoomFactor;
    
    _imp->hasBeenModifiedSinceResize = true;
    
    computeHistogramAndRefresh();
    
}

void Histogram::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Space) {
        QKeyEvent* ev = new QKeyEvent(QEvent::KeyPress,Qt::Key_Space,Qt::NoModifier);
        QCoreApplication::postEvent(parentWidget(),ev);
    } else if (e->key() == Qt::Key_F) {
        _imp->hasBeenModifiedSinceResize = false;
        //reset the pixel aspect to 1
        _imp->zoomCtx.aspectRatio = 1.;
        centerOn(0, 1, 0, 1);
    }
}

void Histogram::enterEvent(QEvent* e)
{
    setFocus();
    QGLWidget::enterEvent(e);
}

void Histogram::leaveEvent(QEvent* e)
{
    _imp->drawCoordinates = false;
    QGLWidget::leaveEvent(e);
}

void Histogram::showEvent(QShowEvent* e)
{
    QGLWidget::showEvent(e);
    computeHistogramAndRefresh(true);
}

void Histogram::computeHistogramAndRefresh(bool forceEvenIfNotVisible)
{
    if (!isVisible() && !forceEvenIfNotVisible) {
        return;
    }
    
    QPointF btmLeft = toHistogramCoordinates(0,height()-1);
    QPointF topRight = toHistogramCoordinates(width()-1, 0);
    double vmin = btmLeft.x();
    double vmax = topRight.x();
    
#ifndef NATRON_HISTOGRAM_USING_OPENGL

    RectI rect;
    boost::shared_ptr<Natron::Image> image = _imp->getHistogramImage(&rect);

    _imp->histogramThread.computeHistogram(_imp->mode, image, rect, width(),vmin,vmax,_imp->filterSize);
    
#endif
    
    update();
}

#ifndef NATRON_HISTOGRAM_USING_OPENGL

void Histogram::onCPUHistogramComputed()
{
    assert(qApp && qApp->thread() == QThread::currentThread());
    
    int mode;
    bool success = _imp->histogramThread.getMostRecentlyProducedHistogram(&_imp->histogram1, &_imp->histogram2, &_imp->histogram3, &_imp->binsCount, &_imp->pixelsCount, &mode,&_imp->vmin,&_imp->vmax);
    assert(success);
    update();
}


#endif

void HistogramPrivate::drawScale()
{
    assert(QGLContext::currentContext() == widget->context());
    
    QPointF btmLeft = widget->toHistogramCoordinates(0,widget->height()-1);
    QPointF topRight = widget->toHistogramCoordinates(widget->width()-1, 0);
    
    ///don't attempt to draw a scale on a widget with an invalid height
    if (widget->height() <= 1) {
        return;
    }
    
    QFontMetrics fontM(_font);
    const double smallestTickSizePixel = 5.; // tick size (in pixels) for alpha = 0.
    const double largestTickSizePixel = 1000.; // tick size (in pixels) for alpha = 1.
    std::vector<double> acceptedDistances;
    acceptedDistances.push_back(1.);
    acceptedDistances.push_back(5.);
    acceptedDistances.push_back(10.);
    acceptedDistances.push_back(50.);
    
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
        const double minTickSizeTextPixel = (axis == 0) ? fontM.width(QString("00")) : fontM.height(); // AXIS-SPECIFIC
        const double minTickSizeText = range * minTickSizeTextPixel/rangePixel;
        for (int i = m1 ; i <= m2; ++i) {
            double value = i * smallTickSize + offset;
            const double tickSize = ticks[i-m1]*smallTickSize;
            const double alpha = ticks_alpha(smallestTickSize, largestTickSize, tickSize);
            
            glColor4f(_baseAxisColor.redF(), _baseAxisColor.greenF(), _baseAxisColor.blueF(), alpha);
            
            glBegin(GL_LINES);
            if (axis == 0) {
                glVertex2f(value, btmLeft.y()); // AXIS-SPECIFIC
                glVertex2f(value, topRight.y()); // AXIS-SPECIFIC
            } else {
                glVertex2f(btmLeft.x(), value); // AXIS-SPECIFIC
                glVertex2f(topRight.x(), value); // AXIS-SPECIFIC
            }
            glEnd();
            
            if (tickSize > minTickSizeText) {
                const int tickSizePixel = rangePixel * tickSize/range;
                const QString s = QString::number(value);
                const int sSizePixel = (axis == 0) ? fontM.width(s) : fontM.height(); // AXIS-SPECIFIC
                if (tickSizePixel > sSizePixel) {
                    const int sSizeFullPixel = sSizePixel + minTickSizeTextPixel;
                    double alphaText = 1.0;//alpha;
                    if (tickSizePixel < sSizeFullPixel) {
                        // when the text size is between sSizePixel and sSizeFullPixel,
                        // draw it with a lower alpha
                        alphaText *= (tickSizePixel - sSizePixel)/(double)minTickSizeTextPixel;
                    }
                    QColor c = _scaleColor;
                    c.setAlpha(255*alphaText);
                    if (axis == 0) {
                        widget->renderText(value, btmLeft.y(), s, c, _font); // AXIS-SPECIFIC
                    } else {
                        widget->renderText(btmLeft.x(), value, s, c, _font); // AXIS-SPECIFIC
                    }
                }
            }
        }
        
    }
    
    glDisable(GL_BLEND);
    //reset back the color
    glColor4f(1., 1., 1., 1.);
    
}

void HistogramPrivate::drawPicker()
{
    QFontMetrics m(_font);
    int strWidth = std::max(std::max(std::max(m.width(rValueStr),m.width(gValueStr)),m.width(bValueStr)),m.width(xCoordinateStr));
    
    QPointF xPos = widget->toHistogramCoordinates(widget->width() - strWidth - 10 ,m.height() + 10);
    QPointF rPos = widget->toHistogramCoordinates(widget->width() - strWidth - 10 ,2 * m.height() + 15);
    QPointF gPos = widget->toHistogramCoordinates(widget->width() - strWidth - 10 ,3 * m.height() + 20);
    QPointF bPos = widget->toHistogramCoordinates(widget->width() - strWidth - 10 ,4 * m.height() + 25);
    
    QColor xColor(200,200,200);
    QColor rColor,gColor,bColor;
    
    switch (mode) {
        case Histogram::RGB:
            rColor.setRedF(0.711519527404004);
            rColor.setGreenF(0.164533420851110);
            rColor.setBlueF(0.164533420851110);
            break;
        case Histogram::Y:
        case Histogram::A:
        case Histogram::R:
        case Histogram::G:
        case Histogram::B:
        default:
            rColor = xColor;
            break;
    }
    
    gColor.setRedF(0);
    gColor.setGreenF(0.546986106552894);
    gColor.setBlueF(0);
    
    bColor.setRedF(0.288480472595996);
    bColor.setGreenF(0.288480472595996);
    bColor.setBlueF(0.8354665791488900);
    
    widget->renderText(xPos.x(), xPos.y(), xCoordinateStr,xColor , _font);
    widget->renderText(rPos.x(), rPos.y(), rValueStr, rColor, _font);
    widget->renderText(gPos.x(), gPos.y(), gValueStr, gColor, _font);
    widget->renderText(bPos.x(), bPos.y(), bValueStr, bColor, _font);
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
    return (int)(d*255 + 0.5);
}

int main(int argc, char**argv)
{
    const double Rr = 0.711519527404004;
    const double Rgb = 0.164533420851110;
    const double Gg = 0.546986106552894;
    const double Grb = 0.;
    const double Bb = 0.835466579148890;
    const double Brg = 0.288480472595996;
    const double R[3] = {Rr, Rgb, Rgb};
    const double G[3] = {Grb, Gg, Grb};
    const double B[3] = {Brg, Brg, Bb};
    const double maxval = Bb;

    printf("OpenGL colors, luminance = 1/3:\n");
    printf("red=(%g,%g,%g) green=(%g,%g,%g) blue=(%g,%g,%g)\n",
           (R[0]), (R[1]), (R[2]),
           (G[0]), (G[1]), (G[2]),
           (B[0]), (B[1]), (B[2]));
    printf("OpenGL colors luminance=%g:\n", (1./3.)/maxval);
    printf("red=(%g,%g,%g) green=(%g,%g,%g) blue=(%g,%g,%g)\n",
           (R[0]/maxval), (R[1]/maxval), (R[2]/maxval),
           (G[0]/maxval), (G[1]/maxval), (G[2]/maxval),
           (B[0]/maxval), (B[1]/maxval), (B[2]/maxval));
    printf("HTML colors, luminance=1/3:\n");
    printf("red=#%02x%02x%02x green=#%02x%02x%02x blue=#%02x%02x%02x\n",
           tochar(R[0]), tochar(R[1]), tochar(R[2]),
           tochar(G[0]), tochar(G[1]), tochar(G[2]),
           tochar(B[0]), tochar(B[1]), tochar(B[2]));
    printf("HTML colors, luminance=%g:\n", (1./3.)/maxval);
    printf("red=#%02x%02x%02x green=#%02x%02x%02x blue=#%02x%02x%02x\n",
           tochar(R[0]/maxval), tochar(R[1]/maxval), tochar(R[2]/maxval),
           tochar(G[0]/maxval), tochar(G[1]/maxval), tochar(G[2]/maxval),
           tochar(B[0]/maxval), tochar(B[1]/maxval), tochar(B[2]/maxval));
    
}
#endif

#ifndef NATRON_HISTOGRAM_USING_OPENGL
void HistogramPrivate::drawHistogramCPU()
{
    glEnable(GL_BLEND);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE);
    
    // see the code above to compute the magic colors

    double binSize = (vmax - vmin) / binsCount;
    
    glBegin(GL_LINES);
    for (unsigned int i = 0; i < binsCount; ++i) {
        double binMinX = vmin + i * binSize;
        if (mode == Histogram::RGB) {
            if (histogram1.empty() || histogram2.empty() || histogram3.empty()) {
                break;
            }
            double rTotNormalized = ((double)histogram1[i] / (double)pixelsCount) / binSize;
            double gTotNormalized = ((double)histogram2[i] / (double)pixelsCount) / binSize;
            double bTotNormalized = ((double)histogram3[i] / (double)pixelsCount) / binSize;

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
            if (histogram1.empty()) {
                break;
            }
            
            double vTotNormalized = (double)histogram1[i] / (double)pixelsCount / binSize;

            // all the following colors have the same luminance (~0.4)
            switch (mode) {
                case Histogram::R:
                    //glColor3f(1, 0, 0);
                    glColor3f(0.851643,0.196936,0.196936);
                    break;
                case Histogram::G:
                    //glColor3f(0, 1, 0);
                    glColor3f(0,0.654707,0);
                    break;
                case Histogram::B:
                    //glColor3f(0, 0, 1);
                    glColor3f(0.345293,0.345293,1);
                    break;
                case Histogram::A:
                    //glColor3f(1, 1, 1);
                    glColor3f(0.398979,0.398979,0.398979);
                    break;
                case Histogram::Y:
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

    glDisable(GL_BLEND);
    glColor4f(1, 1, 1, 1);
}
#endif
void Histogram::renderText(double x,double y,const QString& text,const QColor& color,const QFont& font) const
{
    assert(QGLContext::currentContext() == context());
    
    if(text.isEmpty())
        return;
    
    glMatrixMode (GL_PROJECTION);
    glPushMatrix(); // save GL_PROJECTION
    glLoadIdentity();
    double h = (double)height();
    double w = (double)width();
    /*we put the ortho proj to the widget coords, draw the elements and revert back to the old orthographic proj.*/
    glOrtho(0,w,0,h,-1,1);
    glMatrixMode(GL_MODELVIEW);
    QPointF pos = toWidgetCoordinates(x, y);
    glCheckError();
    _imp->textRenderer.renderText(pos.x(),h-pos.y(),text,color,font);
    glCheckError();
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
    glPopMatrix(); // restore GL_PROJECTION
    glMatrixMode(GL_MODELVIEW);
    
}
