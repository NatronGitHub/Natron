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

#ifndef NATRON_GUI_KNOBGUI_H_
#define NATRON_GUI_KNOBGUI_H_

#include <QtCore/QObject>

#include <boost/shared_ptr.hpp>

#include "Global/GlobalDefines.h"

// Qt
class QUndoCommand; //used by KnobGui
class QGridLayout; //used by KnobGui
class QVBoxLayout; //used by KnobGui
class QHBoxLayout; //used by KnobGui
class QMenu;
class QComboBox;


// Engine
class Knob; //used by KnobGui
class Variant; //used by KnobGui
class KeyFrame;

// Gui
class Button; //used by KnobGui
class DockablePanel; //used by KnobGui

class KnobGui : public QObject
{
    Q_OBJECT

public:
    
    
    friend class KnobMultipleUndosCommand;
    friend class KnobUndoCommand;
    
    KnobGui(boost::shared_ptr<Knob> knob,DockablePanel* container);
    
    virtual ~KnobGui();
        
    bool triggerNewLine() const { return _triggerNewLine; }
    
    void turnOffNewLine() { _triggerNewLine = false; }
    
    boost::shared_ptr<Knob> getKnob() const { return _knob; }
    
    /*Set the spacing between items in the layout*/
    void setSpacingBetweenItems(int spacing){ _spacingBetweenItems = spacing; }
    
    int getSpacingBetweenItems() const { return _spacingBetweenItems; }
    
    void createGUI(QGridLayout* layout,int row);
    
    void moveToLayout(QVBoxLayout* layout);
    
    /*Used by Tab_KnobGui to insert already existing widgets
     in a tab.*/
    virtual void addToLayout(QHBoxLayout* layout)=0;
    
    
    void pushUndoCommand(QUndoCommand* cmd);
    
    bool hasWidgetBeenCreated() const {return _widgetCreated;}
    
    void setKeyframe(SequenceTime time,int dimension);

    void removeKeyFrame(SequenceTime time,int dimension);
public slots:
    /*Called when the value held by the knob is changed internally.
     This should in turn update the GUI but not emit the valueChanged()
     signal.*/
    void onInternalValueChanged(int dimension);
    
    void onInternalKeySet(SequenceTime time,int dimension);

    void onInternalKeyRemoved(SequenceTime time,int dimension);

    void setSecret();

    void showAnimationMenu();
    
    void setEnabledSlot();
    
    void hide();
    
    void show();
    
    /******Animations slots for all dimensions at once*****/
    
    void onSetKeyActionTriggered();
    
    void onRemoveKeyActionTriggered();
    
    void onShowInCurveEditorActionTriggered();
    
    void onRemoveAnyAnimationActionTriggered();

    void onConstantInterpActionTriggered();
    
    void onLinearInterpActionTriggered();
    
    void onSmoothInterpActionTriggered();
    
    void onCatmullromInterpActionTriggered();
    
    void onCubicInterpActionTriggered();
    
    void onHorizontalInterpActionTriggered();
    
    void onCopyValuesActionTriggered();
    
    void onCopyAnimationActionTriggered();
    
    void onPasteActionTriggered();
    
    void onLinkToActionTriggered();
    /******************************************************/

signals:
    
    void knobUndoneChange();
    
    void knobRedoneChange();

    void keyFrameSetByUser(SequenceTime,int);

    void keyFrameRemovedByUser(SequenceTime,int);

    /**
     *@brief Emitted whenever a keyframe is set by the user or by the plugin.
    **/
    void keyFrameSet();

    /**
     *@brief Emitted whenever a keyframe is removed by the user or by the plugin.
    **/
    void keyFrameRemoved();
    
    /**
     *@brief Emitted whenever a keyframe's interpolation method is changed by the user or by the plugin.
     **/
    void keyInterpolationChanged();
    
private:

    virtual void _hide() = 0;

    virtual void _show() = 0;

    virtual void setEnabled() = 0;
    
    /*Must create the GUI and insert it in the grid layout at the index "row".*/
    virtual void createWidget(QGridLayout* layout,int row) = 0;
    
   
    /*Called by the onInternalValueChanged slot. This should update
     the widget to reflect the new internal value held by variant.*/
    virtual void updateGUI(int dimension,const Variant& variant) = 0;
    
    /*Called right away after updateGUI(). Depending in the animation level
     the widget for the knob could display its gui a bit differently.
     */
    virtual void reflectAnimationLevel(int /*dimension*/,Natron::AnimationLevel /*level*/) {}

    /*Calls reflectAnimationLevel with good parameters. Called right away after updateGUI() */
    void checkAnimationLevel(int dimension);
    
    void createAnimationMenu();
    
    void createAnimationButton(QGridLayout* layout,int row);
    
    /*This function is used by KnobUndoCommand. Calling this in a onInternalValueChanged/valueChanged
     signal/slot sequence can cause an infinite loop.*/
    bool setValue(int dimension,const Variant& variant,KeyFrame* newKey);
    
    void setInterpolationForDimensions(const std::vector<int>& dimensions,Natron::KeyframeType interp);
    
private:
    boost::shared_ptr<Knob> _knob;
    bool _triggerNewLine;
    int _spacingBetweenItems;
    bool _widgetCreated;
    DockablePanel* const _container;
    QMenu* _animationMenu;
    Button* _animationButton;
};


class LinkToKnobDialog : public QDialog {
    
    Q_OBJECT
    
    QVBoxLayout* _mainLayout;
    QHBoxLayout* _firstLineLayout;
    QWidget* _firstLine;
    QLabel* _selectKnobLabel;
    QComboBox* _selectionCombo;
    
    QWidget* _buttonsWidget;
    Button* _cancelButton;
    Button* _okButton;
    QHBoxLayout* _buttonsLayout;
    
    std::map<QString,boost::shared_ptr<Knob> > _allKnobs;
public:
    
    LinkToKnobDialog(KnobGui* from,QWidget* parent);
    
    virtual ~LinkToKnobDialog(){ _allKnobs.clear(); }
    
    boost::shared_ptr<Knob> getSelectedKnobs() const;
    
};

#endif // NATRON_GUI_KNOBGUI_H_
