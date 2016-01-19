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

#ifndef Gui_ViewerTabPrivate_h
#define Gui_ViewerTabPrivate_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <map>
#include <set>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QMutex>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/Enums.h"

#include "Engine/EngineFwd.h"

#include "Gui/ComboBox.h"
#include "Gui/GuiFwd.h"


#define NATRON_TRANSFORM_AFFECTS_OVERLAYS

NATRON_NAMESPACE_ENTER;

struct ViewerTabPrivate
{
    struct InputName
    {
        QString name;
        boost::weak_ptr<Node> input;
    };

    typedef std::map<int,InputName> InputNamesMap;

    ViewerTab* publicInterface;
    
    /*OpenGL viewer*/
    ViewerGL* viewer;
    QWidget* viewerContainer;
    QHBoxLayout* viewerLayout;
    QWidget* viewerSubContainer;
    QVBoxLayout* viewerSubContainerLayout;
    QVBoxLayout* mainLayout;

    /*Viewer Settings*/
    QWidget* firstSettingsRow, *secondSettingsRow;
    QHBoxLayout* firstRowLayout, *secondRowLayout;

    /*1st row*/
    //ComboBox* viewerLayers;
    ComboBox* layerChoice;
    ComboBox* alphaChannelChoice;
    ChannelsComboBox* viewerChannels;
    ComboBox* zoomCombobox;
    Button* syncViewerButton;
    Button* centerViewerButton;
    Button* clipToProjectFormatButton;
    Button* enableViewerRoI;
    Button* refreshButton;
    QIcon iconRefreshOff, iconRefreshOn;
    int ongoingRenderCount;
    
    Button* activateRenderScale;
    bool renderScaleActive;
    ComboBox* renderScaleCombo;
    Natron::Label* firstInputLabel;
    ComboBox* firstInputImage;
    Natron::Label* compositingOperatorLabel;
    ComboBox* compositingOperator;
    Natron::Label* secondInputLabel;
    ComboBox* secondInputImage;

    /*2nd row*/
    Button* toggleGainButton;
    SpinBox* gainBox;
    ScaleSliderQWidget* gainSlider;
    double lastFstopValue;
    Button* autoContrast;
    SpinBox* gammaBox;
    double lastGammaValue;
    Button* toggleGammaButton;
    ScaleSliderQWidget* gammaSlider;
    ComboBox* viewerColorSpace;
    Button* checkerboardButton;
    Button* pickerButton;
    ComboBox* viewsComboBox;
    int currentViewIndex;
    QMutex currentViewMutex;
    /*Info*/
    InfoViewerWidget* infoWidget[2];


    /*TimeLine buttons*/
    QWidget* playerButtonsContainer;
    QHBoxLayout* playerLayout;
    SpinBox* currentFrameBox;
    Button* firstFrame_Button;
    Button* previousKeyFrame_Button;
    Button* play_Backward_Button;
    Button* previousFrame_Button;
    Button* stop_Button;
    Button* nextFrame_Button;
    Button* play_Forward_Button;
    Button* nextKeyFrame_Button;
    Button* lastFrame_Button;
    Button* previousIncrement_Button;
    SpinBox* incrementSpinBox;
    Button* nextIncrement_Button;
    Button* playbackMode_Button;
    
    mutable QMutex playbackModeMutex;
    PlaybackModeEnum playbackMode;
    
    LineEdit* frameRangeEdit;

    Natron::ClickableLabel* canEditFrameRangeLabel;
    Button* tripleSyncButton;
    
    QCheckBox* canEditFpsBox;
    Natron::ClickableLabel* canEditFpsLabel;
    mutable QMutex fpsLockedMutex;
    bool fpsLocked;
    SpinBox* fpsBox;
    double userFps;
    Button* turboButton;

    /*frame seeker*/
    TimeLineGui* timeLineGui;
    std::map<NodeGui*,RotoGui*> rotoNodes;
    std::pair<NodeGui*,RotoGui*> currentRoto;
    std::map<NodeGui*,TrackerGui*> trackerNodes;
    std::pair<NodeGui*,TrackerGui*> currentTracker;
    InputNamesMap inputNamesMap;
    mutable QMutex compOperatorMutex;
    ViewerCompositingOperatorEnum compOperator;
    ViewerInstance* viewerNode; // < pointer to the internal node
    
    mutable QMutex visibleToolbarsMutex; //< protects the 4 bool below
    bool infobarVisible;
    bool playerVisible;
    bool timelineVisible;
    bool leftToolbarVisible;
    bool rightToolbarVisible;
    bool topToolbarVisible;
    
    bool isFileDialogViewer;
    
    mutable QMutex checkerboardMutex;
    bool checkerboardEnabled;

    mutable QMutex fpsMutex;
    double fps;
    
    //The last node that took the penDown/motion/keyDown/keyRelease etc...
    boost::weak_ptr<Node> lastOverlayNode;
    bool hasPenDown;
    bool hasCaughtPenMotionWhileDragging;
    
    ViewerTabPrivate(ViewerTab* publicInterface,ViewerInstance* node);

#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
    // return the tronsform to apply to the overlay as a 3x3 homography in canonical coordinates
    bool getOverlayTransform(double time,
                             int view,
                             const boost::shared_ptr<Node>& target,
                             Natron::EffectInstance* currentNode,
                             Transform::Matrix3x3* transform) const;

    bool getTimeTransform(double time,
                          int view,
                          const boost::shared_ptr<Node>& target,
                          Natron::EffectInstance* currentNode,
                          double *newTime) const;

#endif

    void getComponentsAvailabel(std::set<ImageComponents>* comps) const;

};

NATRON_NAMESPACE_EXIT;

#endif // Gui_ViewerTabPrivate_h
