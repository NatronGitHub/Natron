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
#include "Global/GLIncludes.h" //!<must be included before QGLWidget
#include <QtOpenGL/QGLWidget>
#include <QtOpenGL/QGLShaderProgram>
#include "Global/GLObfuscate.h" //!<must be included after QGLWidget
GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QMouseEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QtCore/QDebug>
#include <QApplication>
#include <QToolButton>
#include <QActionGroup>

#include "Engine/HistogramCPU.h"
#include "Engine/Image.h"
#include "Engine/Node.h"
#include "Engine/OSGLFunctions.h"
#include "Engine/Texture.h"
#include "Engine/ViewerInstance.h"
#include "Engine/ViewerNode.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/ClickableLabel.h"
#include "Gui/ComboBox.h"
#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/GuiMacros.h"
#include "Gui/Menu.h"
#include "Gui/NodeGraph.h"
#include "Gui/Shaders.h"
#include "Gui/TabWidget.h"
#include "Gui/TextRenderer.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"
#include "Gui/ZoomContext.h"
#include "Gui/ticks.h"

#include "Serialization/WorkspaceSerialization.h"


NATRON_NAMESPACE_ENTER


NATRON_NAMESPACE_ANONYMOUS_ENTER

enum EventStateEnum
{
    eEventStateDraggingView = 0,
    eEventStateZoomingView,
    eEventStateNone
};

NATRON_NAMESPACE_ANONYMOUS_EXIT


struct HistogramPrivate
{
    Q_DECLARE_TR_FUNCTIONS(Histogram)

public:
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
        , state(eEventStateNone)
        , hasBeenModifiedSinceResize(false)
        , _baseAxisColor(118, 215, 90, 255)
        , _scaleColor(67, 123, 52, 255)
        , _font(appFont, appFontSize)
        , textRenderer()
        , drawCoordinates(false)
        , xCoordinateStr()
        , rValueStr()
        , gValueStr()
        , bValueStr()
        , filterSize(0)
        , histogramThread(HistogramCPUThread::create())
        , histogram1()
        , histogram2()
        , histogram3()
        , pixelsCount(0)
        , vmin(0)
        , vmax(0)
        , binsCount(0)
        , mipMapLevel(0)
        , hasImage(false)
        , sizeH()
        , showViewerPicker(false)
        , viewerPickerColor()
    {
    }


    void showMenu(const QPoint & globalPos);

    void drawScale();

    void drawPicker();

    void drawWarnings();

    void drawMissingImage();

    void updatePicker(double x);

    void drawViewerPicker();

    void drawHistogramCPU();

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
    mutable QMutex zoomContextMutex;
    ZoomContext zoomCtx;
    EventStateEnum state;
    bool hasBeenModifiedSinceResize; //< true if the user panned or zoomed since the last resize
    QColor _baseAxisColor;
    QColor _scaleColor;
    QFont _font;
    TextRenderer textRenderer;
    bool drawCoordinates;
    QString xCoordinateStr;
    QString rValueStr, gValueStr, bValueStr;
    int filterSize;

    HistogramCPUThreadPtr histogramThread;

    ///up to 3 histograms (in the RGB) case. FOr all other cases just histogram1 is used.
    std::vector<float> histogram1;
    std::vector<float> histogram2;
    std::vector<float> histogram3;
    unsigned int pixelsCount;
    double vmin, vmax; //< the x range of the histogram
    unsigned int binsCount;
    unsigned int mipMapLevel;
    bool hasImage;

    QSize sizeH;
    bool showViewerPicker;
    std::vector<double> viewerPickerColor;
};

Histogram::Histogram(const std::string& scriptName,
                     Gui* gui,
                     const QGLWidget* shareWidget)
    : QGLWidget(gui, shareWidget)
    , PanelWidget(scriptName, this, gui)
    , _imp( new HistogramPrivate(this) )
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setMouseTracking(true);
    QObject::connect( _imp->histogramThread.get(), SIGNAL(histogramProduced()), this, SLOT(onCPUHistogramComputed()) );

//    QDesktopWidget* desktop = QApplication::desktop();
//    _imp->sizeH = desktop->screenGeometry().size();
    _imp->sizeH = QSize(10000, 10000);

    _imp->rightClickMenu = new Menu(this);
    //_imp->rightClickMenu->setFont( QFont(appFont,appFontSize) );

    _imp->histogramSelectionMenu = new Menu(tr("Viewer target"), _imp->rightClickMenu);
    //_imp->histogramSelectionMenu->setFont( QFont(appFont,appFontSize) );
    _imp->rightClickMenu->addAction( _imp->histogramSelectionMenu->menuAction() );

    _imp->histogramSelectionGroup = new QActionGroup(_imp->histogramSelectionMenu);

    _imp->viewerCurrentInputMenu = new Menu(tr("Viewer input"), _imp->rightClickMenu);
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

    _imp->modeMenu = new Menu(tr("Display mode"), _imp->rightClickMenu);
    //_imp->modeMenu->setFont( QFont(appFont,appFontSize) );
    _imp->rightClickMenu->addAction( _imp->modeMenu->menuAction() );

    _imp->fullImage = new QAction(_imp->rightClickMenu);
    _imp->fullImage->setText( tr("Full image") );
    _imp->fullImage->setCheckable(true);
    _imp->fullImage->setChecked(false);
    QObject::connect( _imp->fullImage, SIGNAL(triggered()), this, SLOT(computeHistogramAndRefresh()) );
    _imp->rightClickMenu->addAction(_imp->fullImage);

    _imp->filterMenu = new Menu(tr("Smoothing"), _imp->rightClickMenu);
    //_imp->filterMenu->setFont( QFont(appFont,appFontSize) );
    _imp->rightClickMenu->addAction( _imp->filterMenu->menuAction() );

    _imp->modeActions = new QActionGroup(_imp->modeMenu);
    QAction* rgbAction = new QAction(_imp->modeMenu);
    rgbAction->setText( QString::fromUtf8("RGB") );
    rgbAction->setData(0);
    _imp->modeActions->addAction(rgbAction);

    QAction* aAction = new QAction(_imp->modeMenu);
    aAction->setText( QString::fromUtf8("A") );
    aAction->setData(1);
    _imp->modeActions->addAction(aAction);

    QAction* yAction = new QAction(_imp->modeMenu);
    yAction->setText( QString::fromUtf8("Y") );
    yAction->setData(2);
    _imp->modeActions->addAction(yAction);

    QAction* rAction = new QAction(_imp->modeMenu);
    rAction->setText( QString::fromUtf8("R") );
    rAction->setData(3);
    _imp->modeActions->addAction(rAction);

    QAction* gAction = new QAction(_imp->modeMenu);
    gAction->setText( QString::fromUtf8("G") );
    gAction->setData(4);
    _imp->modeActions->addAction(gAction);

    QAction* bAction = new QAction(_imp->modeMenu);
    bAction->setText( QString::fromUtf8("B") );
    bAction->setData(5);
    _imp->modeActions->addAction(bAction);
    QList<QAction*> actions = _imp->modeActions->actions();
    for (int i = 0; i < actions.size(); ++i) {
        _imp->modeMenu->addAction( actions.at(i) );
    }

    QObject::connect( _imp->modeActions, SIGNAL(triggered(QAction*)), this, SLOT(onDisplayModeChanged(QAction*)) );


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
}

int
Histogram::getViewerTextureInputDisplayed() const
{
    int textureIndex = 0;

    if (_imp->viewerCurrentInputGroup) {
        QAction* selectedInputAction = _imp->viewerCurrentInputGroup->checkedAction();
        if (selectedInputAction) {
            textureIndex = selectedInputAction->data().toInt();
        }
    }

    return textureIndex;
}

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
    noneAction->setText( QString::fromUtf8("-") );
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
        QAction* ac = new QAction(_imp->histogramSelectionGroup);
        ac->setText( QString::fromUtf8( (*it)->getInternalNode()->getScriptName_mt_safe().c_str() ) );
        ac->setCheckable(true);
        ac->setChecked(false);
        ac->setData(c);
        _imp->histogramSelectionGroup->addAction(ac);
        _imp->histogramSelectionMenu->addAction(ac);
        ++c;
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

    QObject::connect( _imp->histogramSelectionGroup, SIGNAL(triggered(QAction*)), this, SLOT(onCurrentViewerChanged(QAction*)) );
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
                                int texIndex)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    if (viewer) {
        QString viewerName = QString::fromUtf8( viewer->getInternalNode()->getScriptName_mt_safe().c_str() );
        ViewerTab* lastSelectedViewer = getGui()->getNodeGraph()->getLastSelectedViewer();
        QString currentViewerName;
        if (lastSelectedViewer) {
            currentViewerName = QString::fromUtf8( lastSelectedViewer->getInternalNode()->getScriptName_mt_safe().c_str() );
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

    _imp->hasImage = false;
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
    appPTR->initializeOpenGLFunctionsOnce();

    if ( !appPTR->isOpenGLLoaded() ) {
        return;
    }

    assert( QGLContext::currentContext() == context() );

} // initializeGL



void
Histogram::paintGL()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );

    if ( !appPTR->isOpenGLLoaded() ) {
        return;
    }


    glCheckError(GL_GPU);
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
        GL_GPU::ClearColor(0, 0, 0, 1);
        GL_GPU::Clear(GL_COLOR_BUFFER_BIT);
        glCheckErrorIgnoreOSXBug(GL_GPU);

        return;
    }

    {
        GLProtectAttrib<GL_GPU> a(GL_TRANSFORM_BIT | GL_COLOR_BUFFER_BIT);
        GLProtectMatrix<GL_GPU> p(GL_PROJECTION);
        GL_GPU::LoadIdentity();
        GL_GPU::Ortho(zoomLeft, zoomRight, zoomBottom, zoomTop, 1, -1);
        GLProtectMatrix<GL_GPU> m(GL_MODELVIEW);
        GL_GPU::LoadIdentity();
        glCheckError(GL_GPU);

        GL_GPU::ClearColor(0, 0, 0, 1);
        GL_GPU::Clear(GL_COLOR_BUFFER_BIT);
        glCheckErrorIgnoreOSXBug(GL_GPU);

        _imp->drawScale();

        if (_imp->hasImage) {
            _imp->drawHistogramCPU();
            if (_imp->drawCoordinates) {
                _imp->drawPicker();
            }

            _imp->drawWarnings();

            if (_imp->showViewerPicker) {
                _imp->drawViewerPicker();
            }
        } else {
            _imp->drawMissingImage();
        }


        glCheckError(GL_GPU);
    } // GLProtectAttrib a(GL_TRANSFORM_BIT | GL_COLOR_BUFFER_BIT);
} // paintGL

void
Histogram::resizeGL(int width,
                    int height)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == context() );

    if ( !appPTR->isOpenGLLoaded() ) {
        return;
    }

    if (width == 0) {
        width = 1;
    }
    if (height == 0) { // prevent division by 0
        height = 1;
    }
    GL_GPU::Viewport (0, 0, width, height);

    QMutexLocker k(&_imp->zoomContextMutex);
    _imp->zoomCtx.setScreenSize(width, height);
    if (!_imp->hasBeenModifiedSinceResize) {
        _imp->zoomCtx.fill(0., 1., 0., 10.);
        k.unlock();
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
    } else if ( ( (e->buttons() & Qt::MiddleButton) &&
                  ( ( buttonMetaAlt(e) == Qt::AltModifier) || (e->buttons() & Qt::LeftButton) ) ) ||
                ( (e->buttons() & Qt::LeftButton) &&
                  ( buttonMetaAlt(e) == (Qt::AltModifier | Qt::MetaModifier) ) ) ) {
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

    QPointF newClick_opengl = _imp->zoomCtx.toZoomCoordinates( e->x(), e->y() );
    QPointF oldClick_opengl = _imp->zoomCtx.toZoomCoordinates( _imp->oldClick.x(), _imp->oldClick.y() );


    _imp->oldClick = e->pos();
    _imp->drawCoordinates = true;
    //bool caught = true;
    double dx = ( oldClick_opengl.x() - newClick_opengl.x() );
    double dy = ( oldClick_opengl.y() - newClick_opengl.y() );

    switch (_imp->state) {
    case eEventStateDraggingView:
        {
            QMutexLocker k(&_imp->zoomContextMutex);
            _imp->zoomCtx.translate(dx, dy);
        }
        _imp->hasBeenModifiedSinceResize = true;
        computeHistogramAndRefresh();
        break;
    case eEventStateZoomingView: {
        int delta = 2 * ( ( e->x() - _imp->oldClick.x() ) - ( e->y() - _imp->oldClick.y() ) );
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

        {
            QMutexLocker k(&_imp->zoomContextMutex);
            _imp->zoomCtx.zoom(zoomCenter.x(), zoomCenter.y(), scaleFactor);
        }

        _imp->hasBeenModifiedSinceResize = true;

        computeHistogramAndRefresh();
        break;
    }
        case eEventStateNone:
            _imp->updatePicker( newClick_opengl.x() );
            update();
            break;
        default:
            //caught = false;
            break;
    }
 
    TabWidget* tab = getParentPane() ;
    if (tab) {
        // If the Viewer is in a tab, send the tab widget the event directly
        qApp->sendEvent(tab, e);
    } else {
        QGLWidget::mouseMoveEvent(e);
    }
    
} // Histogram::mouseMoveEvent

void
HistogramPrivate::updatePicker(double x)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    xCoordinateStr = QString::fromUtf8("x=") + QString::number(x, 'f', 6);
    double binSize = (vmax - vmin) / binsCount;
    int index = (int)( (x - vmin) / binSize );
    rValueStr.clear();
    gValueStr.clear();
    bValueStr.clear();
    if (mode == Histogram::eDisplayModeRGB) {
        float r = histogram1.empty() ? 0 :  histogram1[index];
        float g = histogram2.empty() ? 0 :  histogram2[index];
        float b = histogram3.empty() ? 0 :  histogram3[index];
        rValueStr = QString::fromUtf8("r=") + QString::number(r);
        gValueStr = QString::fromUtf8("g=") + QString::number(g);
        bValueStr = QString::fromUtf8("b=") + QString::number(b);
    } else if (mode == Histogram::eDisplayModeY) {
        float y = histogram1[index];
        rValueStr = QString::fromUtf8("y=") + QString::number(y);
    } else if (mode == Histogram::eDisplayModeA) {
        float a = histogram1[index];
        rValueStr = QString::fromUtf8("a=") + QString::number(a);
    } else if (mode == Histogram::eDisplayModeR) {
        float r = histogram1[index];
        rValueStr = QString::fromUtf8("r=") + QString::number(r);
    } else if (mode == Histogram::eDisplayModeG) {
        float g = histogram1[index];
        gValueStr = QString::fromUtf8("g=") + QString::number(g);
    } else if (mode == Histogram::eDisplayModeB) {
        float b = histogram1[index];
        bValueStr = QString::fromUtf8("b=") + QString::number(b);
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

        QMutexLocker k(&_imp->zoomContextMutex);
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

        QMutexLocker k(&_imp->zoomContextMutex);
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

        QMutexLocker k(&_imp->zoomContextMutex);
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

    Qt::Key key = (Qt::Key)e->key();
    bool accept = true;

    if ( key == Qt::Key_F && modCASIsNone(e) ) {
        _imp->hasBeenModifiedSinceResize = false;
        {
            QMutexLocker k(&_imp->zoomContextMutex);
            _imp->zoomCtx.fill(0., 1., 0., 10.);
        }
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
Histogram::enterEvent(QEvent* e)
{
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

    bool fullImage = _imp->fullImage->isChecked();
    int index = 0;
    int textureIndex = 0;
    {
        QAction* selectedInputAction = _imp->viewerCurrentInputGroup->checkedAction();
        if (selectedInputAction) {
            textureIndex = selectedInputAction->data().toInt();
        }
    }
    std::string viewerName;
    {
        QAction* selectedHistAction = _imp->histogramSelectionGroup->checkedAction();
        if (selectedHistAction) {
            index = selectedHistAction->data().toInt();
            viewerName = selectedHistAction->text().toStdString();
        }
    }
    

    ViewerTab* viewer = 0;
    if (index == 1) {
        //current viewer
        viewer = _imp->widget->getGui()->getActiveViewer();
    } else {
        ImagePtr ret;
        const std::list<ViewerTab*> & viewerTabs = _imp->widget->getGui()->getViewersList();
        for (std::list<ViewerTab*>::const_iterator it = viewerTabs.begin(); it != viewerTabs.end(); ++it) {
            if ( (*it)->getInternalNode()->getScriptName_mt_safe() == viewerName ) {
                viewer = *it;
                break;
            }
        }
    }

    if (!viewer) {
        return;
    }

    RectD roiParam;
    if (!fullImage) {
        roiParam = viewer->getInternalNode()->getViewerProcessNode(textureIndex)->getViewerRoI();
    }

    QPointF btmLeft = _imp->zoomCtx.toZoomCoordinates(0, height() - 1);
    QPointF topRight = _imp->zoomCtx.toZoomCoordinates(width() - 1, 0);
    double vmin = btmLeft.x();
    double vmax = topRight.x();
    _imp->histogramThread->computeHistogram(_imp->mode, viewer->getInternalNode(), textureIndex, roiParam, width(), vmin, vmax, _imp->filterSize);

    QPointF oldClick_opengl = _imp->zoomCtx.toZoomCoordinates( _imp->oldClick.x(), _imp->oldClick.y() );
    _imp->updatePicker( oldClick_opengl.x() );

    update();
}


void
Histogram::onCPUHistogramComputed()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    int mode;
    bool success = _imp->histogramThread->getMostRecentlyProducedHistogram(&_imp->histogram1, &_imp->histogram2, &_imp->histogram3, &_imp->binsCount, &_imp->pixelsCount, &mode, &_imp->vmin, &_imp->vmax, &_imp->mipMapLevel);
    assert(success);
    if (success) {
        _imp->hasImage = true;
        update();
    }
}


void
HistogramPrivate::drawScale()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == widget->context() );


    glCheckError(GL_GPU);
    QPointF btmLeft = zoomCtx.toZoomCoordinates(0, widget->height() - 1);
    QPointF topRight = zoomCtx.toZoomCoordinates(widget->width() - 1, 0);

    ///don't attempt to draw a scale on a widget with an invalid height
    if (widget->height() <= 1) {
        return;
    }

    QFontMetrics fontM(_font);
    const double smallestTickSizePixel = 5.; // tick size (in pixels) for alpha = 0.
    const double largestTickSizePixel = 1000.; // tick size (in pixels) for alpha = 1.


    {
        GLProtectAttrib<GL_GPU> a(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);

        GL_GPU::Enable(GL_BLEND);
        GL_GPU::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
            const double minTickSizeTextPixel = (axis == 0) ? fontM.width( QLatin1String("00") ) : fontM.height(); // AXIS-SPECIFIC
            const double minTickSizeText = range * minTickSizeTextPixel / rangePixel;
            for (int i = m1; i <= m2; ++i) {
                double value = i * smallTickSize + offset;
                const double tickSize = ticks[i - m1] * smallTickSize;
                const double alpha = ticks_alpha(smallestTickSize, largestTickSize, tickSize);

                GL_GPU::Color4f(_baseAxisColor.redF(), _baseAxisColor.greenF(), _baseAxisColor.blueF(), alpha);

                GL_GPU::Begin(GL_LINES);
                if (axis == 0) {
                    GL_GPU::Vertex2f( value, btmLeft.y() ); // AXIS-SPECIFIC
                    GL_GPU::Vertex2f( value, topRight.y() ); // AXIS-SPECIFIC
                } else {
                    GL_GPU::Vertex2f(btmLeft.x(), value); // AXIS-SPECIFIC
                    GL_GPU::Vertex2f(topRight.x(), value); // AXIS-SPECIFIC
                }
                GL_GPU::End();
                glCheckErrorIgnoreOSXBug(GL_GPU);

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
                        alphaText = std::min(alphaText, alpha); // don't draw more opaque than tcks
                        QColor c = _scaleColor;
                        c.setAlpha(255 * alphaText);
                        glCheckError(GL_GPU);
                        if (axis == 0) {
                            widget->renderText(value, btmLeft.y(), s, c, _font, Qt::AlignHCenter); // AXIS-SPECIFIC
                        } else {
                            widget->renderText(btmLeft.x(), value, s, c, _font, Qt::AlignVCenter); // AXIS-SPECIFIC
                        }
                    }
                }
            }
        }
    } // GLProtectAttrib a(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
    glCheckError(GL_GPU);
} // drawScale

void
HistogramPrivate::drawWarnings()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == widget->context() );
    if (mipMapLevel > 0) {
        QFontMetrics m(_font);
        QString str( tr("Image downscaled") );
        int strWidth = m.width(str);
        QPointF pos = zoomCtx.toZoomCoordinates(widget->width() - strWidth - 10, 5 * m.height() + 30);
        glCheckError(GL_GPU);
        widget->renderText(pos.x(), pos.y(), str, QColor(220, 220, 0), _font);
        glCheckError(GL_GPU);
    }
}

void
HistogramPrivate::drawMissingImage()
{
    QPointF topLeft = zoomCtx.toZoomCoordinates(0, 0);
    QPointF btmRight = zoomCtx.toZoomCoordinates( widget->width(), widget->height() );
    QPointF topRight( btmRight.x(), topLeft.y() );
    QPointF btmLeft( topLeft.x(), btmRight.y() );
    {
        GLProtectAttrib<GL_GPU> a(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);

        GL_GPU::Color4f(0.9, 0.9, 0, 1);
        GL_GPU::LineWidth(1.5);
        GL_GPU::Begin(GL_LINES);
        GL_GPU::Vertex2f( topLeft.x(), topLeft.y() );
        GL_GPU::Vertex2f( btmRight.x(), btmRight.y() );
        GL_GPU::Vertex2f( btmLeft.x(), btmLeft.y() );
        GL_GPU::Vertex2f( topRight.x(), topRight.y() );
        GL_GPU::End();
        GL_GPU::LineWidth(1.);
    }
    QString txt( tr("Missing image") );
    QFontMetrics m(_font);
    int strWidth = m.width(txt);
    QPointF pos = zoomCtx.toZoomCoordinates(widget->width() / 2. - strWidth / 2., m.height() + 10);

    glCheckError(GL_GPU);
    widget->renderText(pos.x(), pos.y(), txt, QColor(220, 0, 0), _font);
    glCheckError(GL_GPU);
}

void
HistogramPrivate::drawViewerPicker()
{
    // always running in the main thread

    GL_GPU::LineWidth(2.);

    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == widget->context() );

    double wHeight = widget->height();
    QPointF topLeft = zoomCtx.toZoomCoordinates(0, 0);
    QPointF btmRight = zoomCtx.toZoomCoordinates(widget->width(), wHeight);

    //QFontMetrics m(_font, 0);
    //double yPos = zoomCtx.toZoomCoordinates(0,wHeight - m.height() * 2.).y();
    QColor color;
    double imgColor[4] = {0., 0., 0., 0.};
    for (std::size_t i = 0; i < (std::size_t)std::min( (int)viewerPickerColor.size(), 3 ); ++i) {
        imgColor[i] = viewerPickerColor[i];
    }

    if (mode == Histogram::eDisplayModeY) {
        GL_GPU::Color3f(0.398979, 0.398979, 0.398979);
        double luminance = 0.299 * imgColor[0] + 0.587 * imgColor[1] + 0.114 * imgColor[2];
        GL_GPU::Begin(GL_LINES);
        GL_GPU::Vertex2d( luminance, topLeft.y() );
        GL_GPU::Vertex2d( luminance, btmRight.y() );
        GL_GPU::End();
    } else if (mode == Histogram::eDisplayModeR) {
        GL_GPU::Color3f(0.398979, 0.398979, 0.398979);
        GL_GPU::Begin(GL_LINES);
        GL_GPU::Vertex2d( imgColor[0], topLeft.y() );
        GL_GPU::Vertex2d( imgColor[0], btmRight.y() );
        GL_GPU::End();
    } else if (mode == Histogram::eDisplayModeG) {
        GL_GPU::Color3f(0.398979, 0.398979, 0.398979);
        GL_GPU::Begin(GL_LINES);
        GL_GPU::Vertex2d( imgColor[1], topLeft.y() );
        GL_GPU::Vertex2d( imgColor[1], btmRight.y() );
        GL_GPU::End();
    } else if (mode == Histogram::eDisplayModeB) {
        GL_GPU::Color3f(0.398979, 0.398979, 0.398979);
        GL_GPU::Begin(GL_LINES);
        GL_GPU::Vertex2d( imgColor[2], topLeft.y() );
        GL_GPU::Vertex2d( imgColor[2], btmRight.y() );
        GL_GPU::End();
    } else if (mode == Histogram::eDisplayModeA) {
        GL_GPU::Color3f(0.398979, 0.398979, 0.398979);
        GL_GPU::Begin(GL_LINES);
        GL_GPU::Vertex2d( imgColor[3], topLeft.y() );
        GL_GPU::Vertex2d( imgColor[3], btmRight.y() );
        GL_GPU::End();
    } else if (mode == Histogram::eDisplayModeRGB) {
        GL_GPU::Color3f(0.851643, 0.196936, 0.196936);
        GL_GPU::Begin(GL_LINES);
        GL_GPU::Vertex2d( imgColor[0], topLeft.y() );
        GL_GPU::Vertex2d( imgColor[0], btmRight.y() );

        GL_GPU::Color3f(0, 0.654707, 0);
        GL_GPU::Vertex2d( imgColor[1], topLeft.y() );
        GL_GPU::Vertex2d( imgColor[1], btmRight.y() );

        GL_GPU::Color3f(0.345293, 0.345293, 1);
        GL_GPU::Vertex2d( imgColor[2], topLeft.y() );
        GL_GPU::Vertex2d( imgColor[2], btmRight.y() );
        GL_GPU::End();

        GL_GPU::Color3f(0.398979, 0.398979, 0.398979);
        GL_GPU::Vertex2d( imgColor[3], topLeft.y() );
        GL_GPU::Vertex2d( imgColor[3], btmRight.y() );
        GL_GPU::End();
    }

    GL_GPU::LineWidth(1.);
} // HistogramPrivate::drawViewerPicker

void
HistogramPrivate::drawPicker()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == widget->context() );

    glCheckError(GL_GPU);
    QFontMetrics m(_font, 0);
    int strWidth = std::max( std::max( std::max( m.width(rValueStr), m.width(gValueStr) ), m.width(bValueStr) ), m.width(xCoordinateStr) );
    QPointF xPos = zoomCtx.toZoomCoordinates(widget->width() - strWidth - 10, m.height() + 10);
    QPointF rPos = zoomCtx.toZoomCoordinates(widget->width() - strWidth - 10, 2 * m.height() + 15);
    QPointF gPos = zoomCtx.toZoomCoordinates(widget->width() - strWidth - 10, 3 * m.height() + 20);
    QPointF bPos = zoomCtx.toZoomCoordinates(widget->width() - strWidth - 10, 4 * m.height() + 25);
    QColor xColor, rColor, gColor, bColor;

    // Text-aware Magic colors (see recipe below):
    // - they all have the same luminance
    // - red, green and blue are complementary, but they sum up to more than 1
    xColor.setRgbF(0.398979, 0.398979, 0.398979);
    switch (mode) {
    case Histogram::eDisplayModeRGB:
        rColor.setRgbF(0.851643, 0.196936, 0.196936);
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

    gColor.setRgbF(0, 0.654707, 0);

    bColor.setRgbF(0.345293, 0.345293, 1);

    glCheckError(GL_GPU);
    widget->renderText(xPos.x(), xPos.y(), xCoordinateStr, xColor, _font);
    widget->renderText(rPos.x(), rPos.y(), rValueStr, rColor, _font);
    widget->renderText(gPos.x(), gPos.y(), gValueStr, gColor, _font);
    widget->renderText(bPos.x(), bPos.y(), bValueStr, bColor, _font);
    glCheckError(GL_GPU);
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

void
HistogramPrivate::drawHistogramCPU()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert( QGLContext::currentContext() == widget->context() );

    glCheckError(GL_GPU);
    {
        GLProtectAttrib<GL_GPU> a(GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);

        GL_GPU::Enable(GL_BLEND);
        GL_GPU::BlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
        GL_GPU::BlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE);

        // see the code above to compute the magic colors

        double binSize = (vmax - vmin) / binsCount;

        GL_GPU::Begin(GL_LINES);
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
                GL_GPU::Color3f(0.711519527404004, 0.164533420851110, 0.164533420851110);
                GL_GPU::Vertex2d(binMinX, 0);
                GL_GPU::Vertex2d(binMinX,  rTotNormalized);

                //glColor3d(0, 1, 0);
                GL_GPU::Color3f(0., 0.546986106552894, 0.);
                GL_GPU::Vertex2d(binMinX, 0);
                GL_GPU::Vertex2d(binMinX,  gTotNormalized);

                //glColor3d(0, 0, 1);
                GL_GPU::Color3f(0.288480472595996, 0.288480472595996, 0.835466579148890);
                GL_GPU::Vertex2d(binMinX, 0);
                GL_GPU::Vertex2d(binMinX,  bTotNormalized);
            } else {
                if ( histogram1.empty() ) {
                    break;
                }

                double vTotNormalized = (double)histogram1[i] / (double)pixelsCount / binSize;

                // all the following colors have the same luminance (~0.4)
                switch (mode) {
                case Histogram::eDisplayModeR:
                    //glColor3f(1, 0, 0);
                    GL_GPU::Color3f(0.851643, 0.196936, 0.196936);
                    break;
                case Histogram::eDisplayModeG:
                    //glColor3f(0, 1, 0);
                    GL_GPU::Color3f(0, 0.654707, 0);
                    break;
                case Histogram::eDisplayModeB:
                    //glColor3f(0, 0, 1);
                    GL_GPU::Color3f(0.345293, 0.345293, 1);
                    break;
                case Histogram::eDisplayModeA:
                    //glColor3f(1, 1, 1);
                    GL_GPU::Color3f(0.398979, 0.398979, 0.398979);
                    break;
                case Histogram::eDisplayModeY:
                    //glColor3f(0.7, 0.7, 0.7);
                    GL_GPU::Color3f(0.398979, 0.398979, 0.398979);
                    break;
                default:
                    assert(false);
                    break;
                }
                GL_GPU::Vertex2f(binMinX, 0);
                GL_GPU::Vertex2f(binMinX,  vTotNormalized);
            }
        }
        GL_GPU::End(); // GL_LINES
        glCheckErrorIgnoreOSXBug(GL_GPU);
    } // GLProtectAttrib a(GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
    glCheckError(GL_GPU);
} // drawHistogramCPU


void
Histogram::renderText(double x,
                      double y,
                      const QString & text,
                      const QColor & color,
                      const QFont & font,
                      int flags) const
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
    if ( (w <= 0) || (h <= 0) || (right <= left) || (top <= bottom) ) {
        return;
    }
    double scalex = (right - left) / w;
    double scaley = (top - bottom) / h;
    _imp->textRenderer.renderText(x, y, scalex, scaley, text, color, font, flags);
    glCheckError(GL_GPU);
}

void
Histogram::setViewerCursor(const std::vector<double>& pickerColor)
{
    _imp->showViewerPicker = true;
    _imp->viewerPickerColor = pickerColor;
    update();
}

void
Histogram::hideViewerCursor()
{
    if (!_imp->showViewerPicker) {
        return;
    }
    _imp->showViewerPicker = false;
    update();
}


bool
Histogram::saveProjection(SERIALIZATION_NAMESPACE::ViewportData* data)
{
    QMutexLocker k(&_imp->zoomContextMutex);
    data->left = _imp->zoomCtx.left();
    data->bottom = _imp->zoomCtx.bottom();
    data->zoomFactor = _imp->zoomCtx.factor();
    data->par = _imp->zoomCtx.aspectRatio();
    return true;
}

bool
Histogram::loadProjection(const SERIALIZATION_NAMESPACE::ViewportData& data)
{
    QMutexLocker k(&_imp->zoomContextMutex);
    _imp->zoomCtx.setZoom(data.left, data.bottom, data.zoomFactor, data.par);
    return true;
}


NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_Histogram.cpp"
