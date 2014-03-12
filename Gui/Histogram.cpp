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
#include <QMouseEvent>
#include <QDebug>
#include <QCoreApplication>

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

//////////////////////////////// HISTOGRAM TAB ////////////////////////////////

struct HistogramContainer {
    Histogram* histogram;
    QWidget* container;
    QVBoxLayout* layout;
    
    QWidget* labelsContainer;
    QHBoxLayout* labelsLayout;
    QLabel* pickerLabel;
    QLabel* descriptionLabel;
    
    HistogramContainer(HistogramTab* tab,QWidget* parent)
    : histogram(0)
    , container(0)
    , layout(0)
    , pickerLabel(0)
    , descriptionLabel(0)
    {
        container = new QWidget(parent);
        layout = new QVBoxLayout(container);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        
        histogram = new Histogram(tab);
        layout->addWidget(histogram);
        
        labelsContainer = new QWidget(parent);
        labelsLayout = new QHBoxLayout(labelsContainer);
        labelsLayout->setContentsMargins(0, 0, 0, 0);
        labelsLayout->setSpacing(0);
        
        descriptionLabel = new QLabel(labelsContainer);
        labelsLayout->addWidget(descriptionLabel);
        
        pickerLabel = new QLabel(labelsContainer);
        labelsLayout->addWidget(pickerLabel);
        
        layout->addWidget(labelsContainer);
    }
};

struct HistogramTabPrivate
{
    
    Gui* gui; //< ptr to the gui
    
    QVBoxLayout* mainLayout;
    
    ///////// OPTIONS
    QWidget* optionsHeader;
    QHBoxLayout* optionsHeaderLayout;
    
    QLabel* leftHistogramSelectionLabel;
    ComboBox* leftHistogramSelection;
    
    QLabel* rightHistogramSelectionLabel;
    ComboBox* rightHistogramSelection;
    
    QLabel* modeLabel;
    ComboBox* modeSelection;
    
    QLabel* layoutLabel;
    ComboBox* layoutSelection;
    
    QCheckBox* fullImage;
    ClickableLabel* fullImageLabel;
    
    QWidget* headerSecondLine;
    QHBoxLayout* headerSecondLineLayout;
    QLabel* coordLabel;
    
    QLabel* filterLabel;
    ComboBox* filterSelection;
    
    ////////// HISTOGRAMS
    HistogramContainer* histogram1;
    HistogramContainer* histogram2;
    HistogramContainer* histogram3;
    QSplitter* splitter1_2;
    QSplitter* splitter2_3;
    
    HistogramTabPrivate(Gui* parent)
    : gui(parent)
    , mainLayout(NULL)
    , optionsHeader(NULL)
    , optionsHeaderLayout(NULL)
    , leftHistogramSelectionLabel(NULL)
    , leftHistogramSelection(NULL)
    , rightHistogramSelectionLabel(NULL)
    , rightHistogramSelection(NULL)
    , modeLabel(NULL)
    , modeSelection(NULL)
    , layoutLabel(NULL)
    , layoutSelection(NULL)
    , fullImage(NULL)
    , fullImageLabel(NULL)
    , headerSecondLine(NULL)
    , headerSecondLineLayout(NULL)
    , coordLabel(NULL)
    , filterLabel(NULL)
    , filterSelection(NULL)
    , histogram1(NULL)
    , histogram2(NULL)
    , histogram3(NULL)
    , splitter1_2(NULL)
    , splitter2_3(NULL)
    {
        
    }

    boost::shared_ptr<Natron::Image> getHistogramImageInternal(bool left,RectI* imagePortion) const;
};

HistogramTab::HistogramTab(Gui* gui)
: QWidget(gui)
, _imp(new HistogramTabPrivate(gui))
{
    _imp->mainLayout = new QVBoxLayout(this);
    _imp->mainLayout->setContentsMargins(0,0,0,0);
    _imp->mainLayout->setSpacing(0);
    setLayout(_imp->mainLayout);
    
    _imp->optionsHeader = new QWidget(this);
    _imp->optionsHeaderLayout = new QHBoxLayout(_imp->optionsHeader);
    _imp->optionsHeaderLayout->setContentsMargins(0, 0, 0, 0);
    
    _imp->leftHistogramSelectionLabel = new QLabel(tr("Left Histogram:"),_imp->optionsHeader);
    _imp->optionsHeaderLayout->addWidget(_imp->leftHistogramSelectionLabel);
    
    
    _imp->leftHistogramSelection = new ComboBox(_imp->optionsHeader);
    QObject::connect(_imp->leftHistogramSelection, SIGNAL(currentIndexChanged(int)), this, SLOT(makeHistogramsLayout(int)));
    _imp->optionsHeaderLayout->QLayout::addWidget(_imp->leftHistogramSelection);
    
    _imp->optionsHeaderLayout->addStretch();
    
    _imp->rightHistogramSelectionLabel = new QLabel(tr("Right Histogram:"),_imp->optionsHeader);
    _imp->optionsHeaderLayout->addWidget(_imp->rightHistogramSelectionLabel);
    
    _imp->rightHistogramSelection = new ComboBox(_imp->optionsHeader);
    QObject::connect(_imp->rightHistogramSelection, SIGNAL(currentIndexChanged(int)), this, SLOT(makeHistogramsLayout(int)));
    _imp->optionsHeaderLayout->QLayout::addWidget(_imp->rightHistogramSelection);
    
    QObject::connect(_imp->gui, SIGNAL(viewersChanged()), this, SLOT(populateViewersChoices()));
    populateViewersChoices();
    _imp->leftHistogramSelection->blockSignals(true);
    _imp->leftHistogramSelection->setCurrentIndex(1);
    _imp->leftHistogramSelection->blockSignals(false);
    
    _imp->optionsHeaderLayout->addStretch();

    _imp->modeLabel = new QLabel(tr("Mode:"),_imp->optionsHeader);
    _imp->optionsHeaderLayout->addWidget(_imp->modeLabel);
    
    _imp->modeSelection = new ComboBox(_imp->optionsHeader);
    _imp->modeSelection->addItem("RGB");
    _imp->modeSelection->addItem("A");
    _imp->modeSelection->addItem("Y");
    _imp->modeSelection->addItem("R");
    _imp->modeSelection->addItem("G");
    _imp->modeSelection->addItem("B");
    QObject::connect(_imp->modeSelection, SIGNAL(currentIndexChanged(int)), this, SLOT(makeHistogramsLayout(int)));
    _imp->optionsHeaderLayout->QLayout::addWidget(_imp->modeSelection);
    
    _imp->optionsHeaderLayout->addStretch();
    
    _imp->layoutLabel = new QLabel(tr("Layout:"),_imp->optionsHeader);
    _imp->optionsHeaderLayout->QLayout::addWidget(_imp->layoutLabel);
    
    _imp->layoutSelection = new ComboBox(_imp->optionsHeader);
    _imp->layoutSelection->addItem("Split",QIcon(),QKeySequence(),"Splits the left and right Viewer histograms side by side.");
    _imp->layoutSelection->addItem("Anaglyph",QIcon(),QKeySequence(),"Left Viewer histogram will be in red and right Viewer "
                                   "histogram in Cyan.");
    QObject::connect(_imp->layoutSelection, SIGNAL(currentIndexChanged(int)), this, SLOT(makeHistogramsLayout(int)));
    _imp->optionsHeaderLayout->QLayout::addWidget(_imp->layoutSelection);

    _imp->optionsHeaderLayout->addStretch();
    
    _imp->fullImageLabel = new ClickableLabel(tr("Full image:"),_imp->optionsHeader);
    _imp->fullImageLabel->setToolTip("When true, the full image of the targeted viewer \n"
                                     "is used to fill the histogram. When false, only the \n"
                                     "visible portion of the viewer will be taken into account.\n "
                                     "(This is affected by the viewer's region of interest.)");
    _imp->optionsHeaderLayout->addWidget(_imp->fullImageLabel);
    _imp->fullImage = new QCheckBox(_imp->optionsHeader);
    QObject::connect(_imp->fullImage, SIGNAL(clicked(bool)), _imp->fullImage, SLOT(setChecked(bool)));
    QObject::connect(_imp->fullImage, SIGNAL(clicked(bool)), this, SLOT(onFullImageCheckBoxChecked(bool)));
    _imp->fullImage->setChecked(false);
    _imp->optionsHeaderLayout->QLayout::addWidget(_imp->fullImage);
    
    _imp->mainLayout->addWidget(_imp->optionsHeader);

    
    _imp->headerSecondLine = new QWidget(this);
    _imp->headerSecondLineLayout = new QHBoxLayout(_imp->headerSecondLine);
    _imp->headerSecondLineLayout->setContentsMargins(0, 0, 0, 0);
    
    
    _imp->filterLabel = new QLabel(tr("Filter:"),_imp->headerSecondLine);
    _imp->headerSecondLineLayout->QLayout::addWidget(_imp->filterLabel);
    
    _imp->filterSelection = new ComboBox(_imp->headerSecondLine);
    _imp->headerSecondLineLayout->addWidget(_imp->filterSelection);
    _imp->filterSelection->setToolTip("The filter applied to the histogram.");
    _imp->filterSelection->addItem(tr("No smoothing"));
    _imp->filterSelection->addItem(tr("Size 3"));
    _imp->filterSelection->addItem(tr("Size 5"));
    QObject::connect(_imp->filterSelection, SIGNAL(currentIndexChanged(int)), this, SLOT(onFilterChanged(int)));
    
    _imp->headerSecondLineLayout->addStretch();
    
    _imp->coordLabel = new QLabel("",_imp->optionsHeader);
    _imp->headerSecondLineLayout->addWidget(_imp->coordLabel);
    
    _imp->mainLayout->addWidget(_imp->headerSecondLine);
    
    _imp->splitter1_2 = new QSplitter(Qt::Horizontal,this);
    _imp->splitter2_3 = new QSplitter(Qt::Horizontal,this);
    
    _imp->histogram1 = new HistogramContainer(this,this);
    _imp->histogram2 = new HistogramContainer(this,this);
    _imp->histogram3 = new HistogramContainer(this,this);
    
    makeHistogramsLayout(0);
}

HistogramTab::~HistogramTab() {
    
}

void HistogramTab::onFilterChanged(int) {
    _imp->histogram1->histogram->computeHistogramAndRefresh();
    _imp->histogram2->histogram->computeHistogramAndRefresh();
    _imp->histogram3->histogram->computeHistogramAndRefresh();
}

int HistogramTab::getCurrentFilterSize() const {
    int index = _imp->filterSelection->activeIndex();
    switch (index) {
        case 0:
            return 0;
        case 1:
            return 3;
        case 2:
            return 5;
        default:
            assert(false);
            break;
    }
}

void HistogramTab::refreshCoordinatesLabel(double x,double y) {
  
    QString txt = QString("x=%1 y=%2").arg(x,0,'f',5).arg(y,0,'f',5);
    _imp->coordLabel->setText(txt);
    if (!_imp->coordLabel->isVisible()) {
        _imp->coordLabel->show();
    }
}

void HistogramTab::updateCoordPickedForHistogram(Histogram* histo,double x,double y) {
    QLabel* label;
    if (histo == _imp->histogram1->histogram) {
        label = _imp->histogram1->pickerLabel;
    } else if(histo == _imp->histogram2->histogram) {
        label = _imp->histogram2->pickerLabel;
    } else if(histo == _imp->histogram3->histogram) {
        label = _imp->histogram3->pickerLabel;
    } else {
        assert(false);
    }
    
    QString txt = QString("x=%1 y=%2").arg(x,0,'f',5).arg(y,0,'f',5);
    label->setText(txt);
}

void HistogramTab::updateDescriptionLabelForHistogram(Histogram* histo,const QString& text) {
    QLabel* label;
    if (histo == _imp->histogram1->histogram) {
        label = _imp->histogram1->descriptionLabel;
    } else if(histo == _imp->histogram2->histogram) {
        label = _imp->histogram2->descriptionLabel;
    } else if(histo == _imp->histogram3->histogram) {
        label = _imp->histogram3->descriptionLabel;
    } else {
        assert(false);
    }
    label->setText(text);
}

void HistogramTab::hideCoordinatesLabel() {
    _imp->coordLabel->hide();
}

void HistogramTab::onFullImageCheckBoxChecked(bool /*checked*/) {
    _imp->histogram1->histogram->computeHistogramAndRefresh();
    _imp->histogram2->histogram->computeHistogramAndRefresh();
    _imp->histogram3->histogram->computeHistogramAndRefresh();
}

void HistogramTab::populateViewersChoices() {
    
    int leftCurrentChoice = _imp->leftHistogramSelection->activeIndex();
    int rightCurrentChoice = _imp->rightHistogramSelection->activeIndex();
    
    _imp->leftHistogramSelection->clear();
    _imp->rightHistogramSelection->clear();
    
    _imp->leftHistogramSelection->addItem("-");
    _imp->leftHistogramSelection->addItem("Current Viewer");
    
    _imp->rightHistogramSelection->addItem("-");
    _imp->rightHistogramSelection->addItem("Current Viewer");
    
    const std::list<ViewerTab*>& viewerTabs = _imp->gui->getViewersList();
    for (std::list<ViewerTab*>::const_iterator it = viewerTabs.begin(); it != viewerTabs.end(); ++it) {
        _imp->leftHistogramSelection->addItem((*it)->getInternalNode()->getName().c_str());
        _imp->rightHistogramSelection->addItem((*it)->getInternalNode()->getName().c_str());
    }
    
    ///block the signals so only 1 combobox emits a current index changed, otherwise the
    ///receiving slot would be called twice.
    _imp->leftHistogramSelection->blockSignals(true);
    if (leftCurrentChoice < _imp->leftHistogramSelection->count()) {
        _imp->leftHistogramSelection->setCurrentIndex(leftCurrentChoice);
    } else {
        _imp->leftHistogramSelection->setCurrentIndex(1);
    }
    _imp->leftHistogramSelection->blockSignals(false);
    
    if (rightCurrentChoice < _imp->rightHistogramSelection->count()) {
        _imp->rightHistogramSelection->setCurrentIndex(rightCurrentChoice);
    } else {
        _imp->rightHistogramSelection->setCurrentIndex(0);
    }

}

void HistogramTab::onViewerImageChanged() {
    ViewerGL* viewer = qobject_cast<ViewerGL*>(sender());
    if (viewer) {
        QString viewerName = viewer->getInternalNode()->getName().c_str();
        ViewerTab* lastSelectedViewer = _imp->gui->getLastSelectedViewer();
        QString currentViewerName;
        if (lastSelectedViewer) {
            currentViewerName = lastSelectedViewer->getInternalNode()->getName().c_str();
        }
        
        bool refreshLeft = (_imp->leftHistogramSelection->activeIndex() == 1 && lastSelectedViewer == viewer->getViewerTab())
        || (_imp->leftHistogramSelection->activeIndex() > 1 && _imp->leftHistogramSelection->getCurrentIndexText() == viewerName);
        
        bool refreshRight = (_imp->rightHistogramSelection->activeIndex() == 1 && lastSelectedViewer == viewer->getViewerTab())
        || (_imp->rightHistogramSelection->activeIndex() > 1 && _imp->rightHistogramSelection->getCurrentIndexText() == viewerName);
        
        if (refreshLeft) {
            if (_imp->histogram1->histogram->isVisible() && (_imp->histogram1->histogram->getInterest() == Histogram::LEFT_AND_RIGHT ||
                _imp->histogram1->histogram->getInterest() == Histogram::LEFT_IMAGE)) {
                _imp->histogram1->histogram->computeHistogramAndRefresh();
            }
            if (_imp->histogram2->histogram->isVisible() && (_imp->histogram2->histogram->getInterest() == Histogram::LEFT_AND_RIGHT ||
                                                  _imp->histogram2->histogram->getInterest() == Histogram::LEFT_IMAGE)) {
                _imp->histogram2->histogram->computeHistogramAndRefresh();
            }
            if (_imp->histogram3->histogram->isVisible() && (_imp->histogram3->histogram->getInterest() == Histogram::LEFT_AND_RIGHT ||
                                                  _imp->histogram3->histogram->getInterest() == Histogram::LEFT_IMAGE)) {
                _imp->histogram3->histogram->computeHistogramAndRefresh();
            }
        }
        
        if (refreshRight) {
            if (_imp->histogram1->histogram->isVisible() && (_imp->histogram1->histogram->getInterest() == Histogram::LEFT_AND_RIGHT ||
                                                  _imp->histogram1->histogram->getInterest() == Histogram::RIGHT_IMAGE)) {
                _imp->histogram1->histogram->computeHistogramAndRefresh();
            }
            if (_imp->histogram2->histogram->isVisible() && (_imp->histogram2->histogram->getInterest() == Histogram::LEFT_AND_RIGHT ||
                                                  _imp->histogram2->histogram->getInterest() == Histogram::RIGHT_IMAGE)) {
                _imp->histogram2->histogram->computeHistogramAndRefresh();
            }
            if (_imp->histogram3->histogram->isVisible() && (_imp->histogram3->histogram->getInterest() == Histogram::LEFT_AND_RIGHT ||
                                                  _imp->histogram3->histogram->getInterest() == Histogram::RIGHT_IMAGE)) {
                _imp->histogram3->histogram->computeHistogramAndRefresh();
            }
        }
    }
}

void HistogramTab::makeHistogramsLayout(int) {
    ///remove the splitters from the layout and hide them
    
    _imp->histogram1->container->hide();
    _imp->histogram2->container->hide();
    _imp->histogram3->container->hide();

    _imp->mainLayout->removeWidget(_imp->splitter1_2);
    _imp->splitter1_2->hide();
    _imp->mainLayout->removeWidget(_imp->splitter2_3);
    _imp->splitter2_3->hide();

    int mode = _imp->modeSelection->activeIndex();
    int layout = _imp->layoutSelection->activeIndex();
    
    if (layout == 0) { //< Split mode
        
        
        if (_imp->leftHistogramSelection->activeIndex() != 0) {
            _imp->histogram1->container->show();
            _imp->splitter1_2->addWidget(_imp->histogram1->container);
        }
        
        if (_imp->rightHistogramSelection->activeIndex() != 0) {
            _imp->histogram2->container->show();
            _imp->splitter1_2->addWidget(_imp->histogram2->container);
        }
        
        _imp->mainLayout->addWidget(_imp->splitter1_2);
        _imp->splitter1_2->show();

        if (mode == 0) { //< RGB
            _imp->histogram1->histogram->setDisplayModeAndInterest(Histogram::RGB,Histogram::LEFT_IMAGE);
            _imp->histogram2->histogram->setDisplayModeAndInterest(Histogram::RGB,Histogram::RIGHT_IMAGE);
        } else if (mode == 1) { //< A
            _imp->histogram1->histogram->setDisplayModeAndInterest(Histogram::A,Histogram::LEFT_IMAGE);
            _imp->histogram2->histogram->setDisplayModeAndInterest(Histogram::A,Histogram::RIGHT_IMAGE);
        } else if (mode == 2) { //< Y
            _imp->histogram1->histogram->setDisplayModeAndInterest(Histogram::Y,Histogram::LEFT_IMAGE);
            _imp->histogram2->histogram->setDisplayModeAndInterest(Histogram::Y,Histogram::RIGHT_IMAGE);
        } else if (mode == 3) { //< R
            _imp->histogram1->histogram->setDisplayModeAndInterest(Histogram::R,Histogram::LEFT_IMAGE);
            _imp->histogram2->histogram->setDisplayModeAndInterest(Histogram::R,Histogram::RIGHT_IMAGE);
        } else if (mode == 4) { //< G
            _imp->histogram1->histogram->setDisplayModeAndInterest(Histogram::G,Histogram::LEFT_IMAGE);
            _imp->histogram2->histogram->setDisplayModeAndInterest(Histogram::G,Histogram::RIGHT_IMAGE);
        } else if (mode == 5) { //< B
            _imp->histogram1->histogram->setDisplayModeAndInterest(Histogram::B,Histogram::LEFT_IMAGE);
            _imp->histogram2->histogram->setDisplayModeAndInterest(Histogram::B,Histogram::RIGHT_IMAGE);
        } else {
            assert(false);
        }
        
    } else if (layout ==1) { // anaglyph mode
        bool useOnlyFirstHisto = false;
        Histogram::HistogramInterest interest;
        bool useLeft = _imp->leftHistogramSelection->activeIndex() != 0;
        bool useRight = _imp->rightHistogramSelection->activeIndex() != 0;
        if (useLeft && useRight) {
            interest = Histogram::LEFT_AND_RIGHT;
        } else if (useLeft && !useRight) {
            interest = Histogram::LEFT_IMAGE;
        } else if (!useLeft && useRight) {
            interest = Histogram::RIGHT_IMAGE;
        } else {
            interest = Histogram::NO_IMAGE;
        }
        if (mode == 0) { //< RGB
            _imp->splitter1_2->addWidget(_imp->histogram1->container);
            _imp->splitter1_2->addWidget(_imp->histogram2->container);
            _imp->splitter2_3->addWidget(_imp->splitter1_2);
            _imp->splitter2_3->addWidget(_imp->histogram3->container);
            _imp->mainLayout->addWidget(_imp->splitter2_3);
            _imp->splitter1_2->show();
            _imp->splitter2_3->show();
            _imp->histogram1->container->show();
            _imp->histogram2->container->show();
            _imp->histogram3->container->show();
            _imp->histogram1->histogram->setDisplayModeAndInterest(Histogram::R,interest);
            _imp->histogram2->histogram->setDisplayModeAndInterest(Histogram::G,interest);
            _imp->histogram3->histogram->setDisplayModeAndInterest(Histogram::B,interest);
        } else if (mode == 1) { //< A
            useOnlyFirstHisto = true;
            _imp->histogram1->histogram->setDisplayModeAndInterest(Histogram::A,interest);
        } else if (mode == 2) { //< Y
            useOnlyFirstHisto = true;
            _imp->histogram1->histogram->setDisplayModeAndInterest(Histogram::Y,interest);
        } else if (mode == 3) { //< R
            useOnlyFirstHisto = true;
            _imp->histogram1->histogram->setDisplayModeAndInterest(Histogram::R,interest);
        } else if (mode == 4) { //< G
            _imp->histogram1->histogram->setDisplayModeAndInterest(Histogram::G,interest);
            useOnlyFirstHisto = true;
        } else if (mode == 5) { //< B
            _imp->histogram1->histogram->setDisplayModeAndInterest(Histogram::B,interest);
            useOnlyFirstHisto = true;
        } else {
            assert(false);
        }
        if (useOnlyFirstHisto) {
            _imp->histogram1->container->show();
            _imp->splitter1_2->addWidget(_imp->histogram1->container);
            _imp->splitter1_2->show();
            _imp->mainLayout->addWidget(_imp->splitter1_2);
        }
    } else {
        assert(false);
    }
}

boost::shared_ptr<Natron::Image> HistogramTabPrivate::getHistogramImageInternal(bool left,RectI* imagePortion) const {
    bool useImageRoD = fullImage->isChecked();
    
    int index;
    std::string viewerName ;
    if (left) {
        index = leftHistogramSelection->activeIndex();
        viewerName = leftHistogramSelection->getCurrentIndexText().toStdString();
    } else {
        index = rightHistogramSelection->activeIndex();
        viewerName = rightHistogramSelection->getCurrentIndexText().toStdString();
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
        if (!useImageRoD) {
            if (lastSelectedViewer) {
                *imagePortion = lastSelectedViewer->getViewer()->getImageRectangleDisplayed(ret->getRoD());
            }
        } else {
            *imagePortion = ret->getRoD();
        }
        return ret;
    } else {
        boost::shared_ptr<Natron::Image> ret;
        const std::list<ViewerTab*>& viewerTabs = gui->getViewersList();
        for (std::list<ViewerTab*>::const_iterator it = viewerTabs.begin(); it != viewerTabs.end(); ++it) {
            if ((*it)->getInternalNode()->getName() == viewerName) {
                ret = (*it)->getInternalNode()->getLastRenderedImage();
                if (!useImageRoD) {
                    *imagePortion = (*it)->getViewer()->getImageRectangleDisplayed(ret->getRoD());
                } else {
                    *imagePortion = ret->getRoD();
                }
                return ret;
            }
        }
        return ret;
    }

}

boost::shared_ptr<Natron::Image> HistogramTab::getLeftHistogramImage(RectI* imagePortion) const {
    return _imp->getHistogramImageInternal(true, imagePortion);
}

boost::shared_ptr<Natron::Image> HistogramTab::getRightHistogramImage(RectI* imagePortion) const {
    return _imp->getHistogramImageInternal(false, imagePortion);
}

//////////////////////////////// HISTOGRAM ////////////////////////////////


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
    
    
    HistogramTab* uiContext;
    Histogram* widget;
    Histogram::DisplayMode mode;
    Histogram::HistogramInterest interest;
    ZoomContext zoomCtx;
    bool supportsGLSL;
    bool hasOpenGLVAOSupport;
    EventState state;
    bool hasBeenModifiedSinceResize; //< true if the user panned or zoomed since the last resize
    
    QColor _baseAxisColor;
    QColor _scaleColor;
    QFont _font;
    Natron::TextRenderer textRenderer;
    
    
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
    std::vector<unsigned int> histogram1;
    std::vector<unsigned int> histogram2;
    std::vector<unsigned int> histogram3;
    unsigned int pixelsCount;
    double vmin,vmax; //< the x range of the histogram
    unsigned int binsCount;
    
#endif
    HistogramPrivate(HistogramTab* parent,Histogram* widget)
    : uiContext(parent)
    , widget(widget)
    , mode(Histogram::RGB)
    , interest(Histogram::LEFT_AND_RIGHT)
    , zoomCtx()
    , supportsGLSL(true)
    , hasOpenGLVAOSupport(true)
    , state(NONE)
    , hasBeenModifiedSinceResize(false)
    , _baseAxisColor(118,215,90,255)
    , _scaleColor(67,123,52,255)
    , _font(NATRON_FONT, NATRON_FONT_SIZE_10)
    , textRenderer()
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
    
    void drawScale();
    
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

Histogram::Histogram(HistogramTab* parent, const QGLWidget* shareWidget)
: QGLWidget(parent,shareWidget)
, _imp(new HistogramPrivate(parent,this))
{
    setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Expanding);
    setMouseTracking(true);
#ifndef NATRON_HISTOGRAM_USING_OPENGL
    QObject::connect(&_imp->histogramThread, SIGNAL(histogramProduced()), this, SLOT(onCPUHistogramComputed()));
#endif
}

Histogram::~Histogram() {
    
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

QSize Histogram::sizeHint() const {
    return QSize(500,1000);
}

void Histogram::setDisplayModeAndInterest(DisplayMode mode,HistogramInterest interest) {
    _imp->mode = mode;
    _imp->interest = interest;
    QString txt;
    switch (mode) {
        case RGB:
            txt.append("RGB");
            break;
        case Y:
            txt.append("Y");
            break;
        case A:
            txt.append("A");
            break;
        case R:
            txt.append("R");
            break;
        case G:
            txt.append("G");
            break;
        case B:
            txt.append("B");
            break;
        default:
            break;
    }
    
    txt.append(": ");
    switch (interest) {
        case LEFT_AND_RIGHT:
            txt.append("<font color=\"##FF0000\">Left</font> / <font color=\"##00FFFF\">Right</font>");
            break;
        case LEFT_IMAGE:
            txt.append("Left");
            break;
        case RIGHT_IMAGE:
            txt.append("Right");
            break;
        case NO_IMAGE:
            txt.append("None");
            break;
        default:
            break;
    }
    
    _imp->uiContext->updateDescriptionLabelForHistogram(this, txt);
    computeHistogramAndRefresh();
}

Histogram::HistogramInterest Histogram::getInterest() const {
    return _imp->interest;
}


void Histogram::initializeGL() {
    
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
    for(unsigned int i = 0; i < 3; i++){
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB,_imp->histogramTexture[i]);
        glTexImage2D(GL_TEXTURE_RECTANGLE_ARB,0,GL_R32F,256,1,0,GL_RED,GL_FLOAT,0);
        glTexParameteri (GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri (GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER,attachments[i],GL_TEXTURE_RECTANGLE_ARB,_imp->histogramTexture[i],0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER,0);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB,0);
    
    /*initializing fbo for parallel reductions and textures*/
    for(unsigned int i = 0; i < 3; i++){
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
    for(unsigned int i = 0; i < 3; i++){
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
    for(unsigned int i = 0; i < 3; i++){
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
void HistogramPrivate::resizeComputingVBO(int w,int h) {
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



static void textureMap_Polygon(int fromX,int fromY,int fromW,int fromH,int x,int y,int w,int h){
    glBegin(GL_POLYGON);
    glTexCoord2i (fromX, fromH);glVertex2i (x,h);
    glTexCoord2i (fromW, fromH);glVertex2i (w,h);
    glTexCoord2i (fromW, fromY);glVertex2i (w,y);
    glTexCoord2i (fromX, fromY);glVertex2i (x,y);
    glEnd ();
}

static void startRenderingTo(GLuint fboId,GLenum attachment,int w,int h){
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
static void stopRenderingTo(){
    glPopAttrib();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glBindFramebuffer(GL_FRAMEBUFFER,0);
}

static int shaderChannelFromDisplayMode(Histogram::DisplayMode channel) {
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

void HistogramPrivate::activateHistogramComputingShader(Histogram::DisplayMode channel){
    histogramComputingShader->bind();
    histogramComputingShader->setUniformValue("Tex",0);
    checkGLErrors();
    histogramComputingShader->setUniformValue("channel",shaderChannelFromDisplayMode(channel));
    glBindAttribLocation(histogramComputingShader->programId(),0,"TexCoord");
}

void HistogramPrivate::activateHistogramRenderingShader(Histogram::DisplayMode channel){
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

static GLenum colorAttachmentFromDisplayMode(Histogram::DisplayMode channel) {
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

void HistogramPrivate::computeHistogram(Histogram::DisplayMode channel) {
    
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
    checkGLErrors();
}
void HistogramPrivate::renderHistogram(Histogram::DisplayMode channel) {
    
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
    checkGLErrors();
}

#endif

void Histogram::paintGL() {
    
    if (!isVisible()) {
        return;
    }
    
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
    checkGLErrors();
    
    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity();
    
    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT);

    
    _imp->drawScale();
    
#ifndef NATRON_HISTOGRAM_USING_OPENGL
    _imp->drawHistogramCPU();
#endif
    
}

void Histogram::resizeGL(int w, int h) {
    if(h == 0)
        h = 1;
    glViewport (0, 0, w , h);
    if (!_imp->hasBeenModifiedSinceResize) {
        centerOn(0, 1, 0, 1);
    }
}

QPointF Histogram::toHistogramCoordinates(double x,double y) const {
    double w = (double)width() ;
    double h = (double)height();
    double bottom = _imp->zoomCtx.bottom;
    double left = _imp->zoomCtx.left;
    double top =  bottom +  h / _imp->zoomCtx.zoomFactor * _imp->zoomCtx.aspectRatio ;
    double right = left +  w / _imp->zoomCtx.zoomFactor;
    return QPointF((((right - left)*x)/w)+left,(((bottom - top)*y)/h)+top);
}

QPointF Histogram::toWidgetCoordinates(double x, double y) const {
    double w = (double)width() ;
    double h = (double)height();
    double bottom = _imp->zoomCtx.bottom;
    double left = _imp->zoomCtx.left;
    double top =  bottom +  h / _imp->zoomCtx.zoomFactor * _imp->zoomCtx.aspectRatio ;
    double right = left +  w / _imp->zoomCtx.zoomFactor;
    return QPointF(((x - left)/(right - left))*w,((y - top)/(bottom - top))*h);
}

void Histogram::centerOn(double xmin,double xmax,double ymin,double ymax) {
    double curveWidth = xmax - xmin;
    double curveHeight = (ymax - ymin);
    double w = width();
    double h = height() * _imp->zoomCtx.aspectRatio ;
    if(w / h < curveWidth / curveHeight){
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

void Histogram::mousePressEvent(QMouseEvent* event) {
    ////
    // middle button: scroll view
    if (event->button() == Qt::MiddleButton || event->modifiers().testFlag(Qt::AltModifier) ) {
        _imp->state = DRAGGING_VIEW;
        _imp->zoomCtx._oldClick = event->pos();
    } else if (event->button() == Qt::LeftButton) {
        QPointF newClick_opengl = toHistogramCoordinates(event->x(),event->y());
        _imp->uiContext->updateCoordPickedForHistogram(this,newClick_opengl.x(),newClick_opengl.y());
    }
    
}

void Histogram::mouseMoveEvent(QMouseEvent* event) {
    
    
    QPointF newClick_opengl = toHistogramCoordinates(event->x(),event->y());
    QPointF oldClick_opengl = toHistogramCoordinates(_imp->zoomCtx._oldClick.x(),_imp->zoomCtx._oldClick.y());
    

    _imp->zoomCtx._oldClick = event->pos();

    switch (_imp->state) {
        case DRAGGING_VIEW:
            _imp->zoomCtx.bottom += (oldClick_opengl.y() - newClick_opengl.y());
            _imp->zoomCtx.left += (oldClick_opengl.x() - newClick_opengl.x());
            _imp->hasBeenModifiedSinceResize = true;
            computeHistogramAndRefresh();
            break;
        case NONE:
            break;
    }
    _imp->uiContext->refreshCoordinatesLabel(newClick_opengl.x(), newClick_opengl.y());

}

void Histogram::mouseReleaseEvent(QMouseEvent* /*event*/) {
    _imp->state = NONE;
}

void Histogram::wheelEvent(QWheelEvent *event) {
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

void Histogram::keyPressEvent(QKeyEvent *e) {
    if(e->key() == Qt::Key_Space){
        QKeyEvent* ev = new QKeyEvent(QEvent::KeyPress,Qt::Key_Space,Qt::NoModifier);
        QCoreApplication::postEvent(parentWidget()->parentWidget(),ev);
    } else if (e->key() == Qt::Key_F) {
        _imp->hasBeenModifiedSinceResize = false;
        
        //reset the pixel aspect to 1
        _imp->zoomCtx.aspectRatio = 1.;
        centerOn(0, 1, 0, 1);
    }
}

void Histogram::enterEvent(QEvent* e) {
    setFocus();
    QGLWidget::enterEvent(e);
}

void Histogram::leaveEvent(QEvent* e) {
    _imp->uiContext->hideCoordinatesLabel();
    QGLWidget::leaveEvent(e);
}

void Histogram::showEvent(QShowEvent* e) {
    computeHistogramAndRefresh();
    QGLWidget::showEvent(e);
}

void Histogram::computeHistogramAndRefresh() {
    
    if (!isVisible()) {
        return;
    }
    
    QPointF btmLeft = toHistogramCoordinates(0,height()-1);
    QPointF topRight = toHistogramCoordinates(width()-1, 0);
    double vmin = btmLeft.x();
    double vmax = topRight.x();
    
#ifndef NATRON_HISTOGRAM_USING_OPENGL

    RectI leftRect,rightRect;
    boost::shared_ptr<Natron::Image> leftImage;
    if (_imp->interest == Histogram::LEFT_IMAGE || _imp->interest == Histogram::LEFT_AND_RIGHT) {
        leftImage = _imp->uiContext->getLeftHistogramImage(&leftRect);
    }
    boost::shared_ptr<Natron::Image> rightImage;
    if (_imp->interest == Histogram::RIGHT_IMAGE || _imp->interest == Histogram::LEFT_AND_RIGHT) {
        rightImage = _imp->uiContext->getRightHistogramImage(&rightRect);
    }

    _imp->histogramThread.computeHistogram(_imp->mode, leftImage, rightImage, leftRect,rightRect, width(),vmin,vmax,
                                           _imp->uiContext->getCurrentFilterSize());
    
#endif
    
    update();
}

#ifndef NATRON_HISTOGRAM_USING_OPENGL

void Histogram::onCPUHistogramComputed() {
    
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

#if 0
/*
 // compute magic colors, by F. Devernay
 // - they are red, green and blue
 // - they all have the same luminance
 // - their sum is white
 // OpenGL luminance formula:
 // L = r*.3086 + g*.6094 + b*0.0820
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
            if (interest == Histogram::LEFT_AND_RIGHT) {
                if (histogram1.empty() || histogram2.empty()) {
                    break;
                }
                
                double vLeftTotNormalized = (double)histogram1[i] / (double)pixelsCount;
                double vRightTotNormalized = (double)histogram2[i] / (double)pixelsCount;

                glColor3f(1, 0, 0);
                glVertex2f(binMinX, 0);
                glVertex2f(binMinX,  vLeftTotNormalized);
                
                
                glColor3f(0, 1, 1);
                glVertex2f(binMinX, 0);
                glVertex2f(binMinX,  vRightTotNormalized);
            } else {
                if ((interest == Histogram::LEFT_IMAGE && histogram1.empty()) ||
                    (interest == Histogram::RIGHT_IMAGE && histogram2.empty())) {
                    break;
                }
                
                double vTotNormalized;
                
                if (interest == Histogram::LEFT_IMAGE) {
                    vTotNormalized = (double)histogram1[i] / (double)pixelsCount;
                } else {
                    vTotNormalized = (double)histogram2[i] / (double)pixelsCount;
                }
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
    glLoadIdentity();
    double h = (double)height();
    double w = (double)width();
    /*we put the ortho proj to the widget coords, draw the elements and revert back to the old orthographic proj.*/
    glOrtho(0,w,0,h,-1,1);
    glMatrixMode(GL_MODELVIEW);
    QPointF pos = toWidgetCoordinates(x, y);
    _imp->textRenderer.renderText(pos.x(),h-pos.y(),text,color,font);
    checkGLErrors();
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
    glOrtho(_imp->zoomCtx._lastOrthoLeft,_imp->zoomCtx._lastOrthoRight,_imp->zoomCtx._lastOrthoBottom,_imp->zoomCtx._lastOrthoTop,-1,1);
    glMatrixMode(GL_MODELVIEW);
    
}
