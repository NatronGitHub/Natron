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

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QtCore/QObject>
#include <QDialog>
CLANG_DIAG_ON(deprecated)

#include <boost/shared_ptr.hpp>

#include "Global/GlobalDefines.h"

// Qt
class QUndoCommand; //used by KnobGui
class QVBoxLayout; //used by KnobGui
class QHBoxLayout; //used by KnobGui
class QFormLayout;
class QMenu;
class QLabel;

// Engine
class Knob; //used by KnobGui
class Variant; //used by KnobGui
class KeyFrame;

// Gui
class ComboBox;
class Button;
class AnimationButton; //used by KnobGui
class DockablePanel; //used by KnobGui
class Gui;

class KnobGui : public QObject
{
    Q_OBJECT

public:
    
    
    friend class KnobMultipleUndosCommand;
    friend class KnobUndoCommand;
    
    KnobGui(boost::shared_ptr<Knob> knob,DockablePanel* container);
    
    virtual ~KnobGui() OVERRIDE;
        
    bool triggerNewLine() const { return _triggerNewLine; }
    
    void turnOffNewLine() { _triggerNewLine = false; }
    
    boost::shared_ptr<Knob> getKnob() const { return _knob; }
    
    /*Set the spacing between items in the layout*/
    void setSpacingBetweenItems(int spacing){ _spacingBetweenItems = spacing; }
    
    int getSpacingBetweenItems() const { return _spacingBetweenItems; }
    
    void createGUI(QFormLayout* containerLayout,QWidget* fieldContainer,QWidget* label,QHBoxLayout* layout,int row,bool isOnNewLine);
        
    void pushUndoCommand(QUndoCommand* cmd);
    
    const QUndoCommand* getLastUndoCommand() const;
    
    bool hasWidgetBeenCreated() const {return _widgetCreated;}
    
    void setKeyframe(SequenceTime time,int dimension);

    void removeKeyFrame(SequenceTime time,int dimension);
    
    QString toolTip() const;
    
    bool hasToolTip() const;
    
    Gui* getGui() const;
    
    void enableRightClickMenu(QWidget* widget,int dimension);

    virtual bool showDescriptionLabel() const { return true; }
    
    QWidget* getFieldContainer() const;

    int getActualIndexInLayout() const;
    
    bool isOnNewLine() const;
    
    ////calls setReadOnly and also set the label black
    void setReadOnly_(bool readOnly,int dimension);
 
public slots:
    /*Called when the value held by the knob is changed internally.
     This should in turn update the GUI but not emit the valueChanged()
     signal.*/
    void onInternalValueChanged(int dimension);
    
    void onInternalKeySet(SequenceTime time,int dimension);

    void onInternalKeyRemoved(SequenceTime time,int dimension);

    void setSecret();

    void showAnimationMenu();
    
    void onRightClickClicked(const QPoint& pos);
    
    void showRightClickMenuForDimension(const QPoint& pos,int dimension);
    
    void setEnabledSlot();
    
    void hide();
    
    ///if index is != -1 then it will reinsert the knob at this index in the layout
    void show(int index = -1);
    
    void onRestorationComplete();
    
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
    void copyValues(int dimension);
    
    void onPasteValuesActionTriggered();
    void pasteValues(int dimension);
    
    
    void onCopyAnimationActionTriggered();
    
    void onPasteAnimationActionTriggered();
    
    void onLinkToActionTriggered();
    void linkTo(int dimension);
    
    void onUnlinkActionTriggered();
    void unlink(int dimension);
    
    void onResetDefaultValuesActionTriggered();
    void resetDefault(int dimension);

    
    void onReadOnlyChanged(bool b,int d);
    
    void onKnobSlavedChanged(int dimension,bool b);

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
    
    ///emitted when the description label is clicked
    void labelClicked(bool);
    

protected:
    
    
    void pushValueChangedCommand(const std::vector<Variant>& newValues);
    
    void pushValueChangedCommand(const Variant& v, int dimension = 0);
    
private:

    virtual void _hide() = 0;

    virtual void _show() = 0;

    virtual void setEnabled() = 0;
    

    
    virtual void setReadOnly(bool readOnly,int dimension) = 0;
    
    /**
     * @brief Must fill the horizontal layout with all the widgets composing the knob.
     **/
    virtual void createWidget(QHBoxLayout* layout) = 0;
    
    
   
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
    
    void createAnimationButton(QHBoxLayout* layout);
    
    /*This function is used by KnobUndoCommand. Calling this in a onInternalValueChanged/valueChanged
     signal/slot sequence can cause an infinite loop.*/
     int setValue(int dimension,const Variant& variant,KeyFrame* newKey);
    
    void setInterpolationForDimensions(const std::vector<int>& dimensions,Natron::KeyframeType interp);
    
private:
    // FIXME: PIMPL
    boost::shared_ptr<Knob> _knob;
    bool _triggerNewLine;
    int _spacingBetweenItems;
    bool _widgetCreated;
    DockablePanel* const _container;
    QMenu* _animationMenu;
    AnimationButton* _animationButton;
    QMenu* _copyRightClickMenu;
    QHBoxLayout* _fieldLayout; //< the layout containing the widgets of the knob
    int _row;
    
    QFormLayout* _containerLayout;
    QWidget* _field;
    QWidget* _descriptionLabel;
    bool _isOnNewLine;
};


class LinkToKnobDialog : public QDialog {
    
    Q_OBJECT
public:
    
    LinkToKnobDialog(KnobGui* from,QWidget* parent);
    
    virtual ~LinkToKnobDialog() OVERRIDE { _allKnobs.clear(); }
    
    std::pair<int ,boost::shared_ptr<Knob> > getSelectedKnobs() const;

private:
    // FIXME: PIMPL
    QVBoxLayout* _mainLayout;
    QHBoxLayout* _firstLineLayout;
    QWidget* _firstLine;
    QLabel* _selectKnobLabel;
    ComboBox* _selectionCombo;

    QWidget* _buttonsWidget;
    Button* _cancelButton;
    Button* _okButton;
    QHBoxLayout* _buttonsLayout;

    std::map<QString,std::pair<int,boost::shared_ptr<Knob > > > _allKnobs;
};

#endif // NATRON_GUI_KNOBGUI_H_
