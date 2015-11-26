/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#include "AddKnobDialog.h"

#include <cfloat> // floor

#include <QCheckBox>
#include <QTextEdit>
#include <QTextStream>
#include <QFormLayout>
#include <QDialogButtonBox>

#include "Engine/EffectInstance.h"
#include "Engine/KnobFile.h" // KnobFile
#include "Engine/KnobSerialization.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/ComboBox.h"
#include "Gui/DockablePanel.h"
#include "Gui/Gui.h"
#include "Gui/GuiDefines.h"
#include "Gui/KnobGui.h"
#include "Gui/Label.h"
#include "Gui/LineEdit.h"
#include "Gui/SpinBox.h"
#include "Gui/Utils.h" // convertFromPlainText


using namespace Natron;


struct AddKnobDialogPrivate
{
    boost::shared_ptr<KnobI> knob;
    boost::shared_ptr<KnobSerialization> originalKnobSerialization;
    boost::shared_ptr<KnobI> isKnobAlias;
    
    DockablePanel* panel;
    
    QVBoxLayout* vLayout;
    
    QWidget* mainContainer;
    QFormLayout* mainLayout;
    
    Natron::Label* typeLabel;
    ComboBox* typeChoice;
    Natron::Label* nameLabel;
    LineEdit* nameLineEdit;

    
    Natron::Label* labelLabel;
    LineEdit* labelLineEdit;
    
    Natron::Label* hideLabel;
    QCheckBox* hideBox;
    
    Natron::Label* startNewLineLabel;
    QCheckBox* startNewLineBox;
    
    Natron::Label* animatesLabel;
    QCheckBox* animatesCheckbox;
    
    Natron::Label* evaluatesLabel;
    QCheckBox* evaluatesOnChange;
    
    Natron::Label* tooltipLabel;
    QTextEdit* tooltipArea;
    
    Natron::Label* minLabel;
    SpinBox* minBox;
    
    Natron::Label* maxLabel;
    SpinBox* maxBox;
    
    Natron::Label* dminLabel;
    SpinBox* dminBox;
    
    Natron::Label* dmaxLabel;
    SpinBox* dmaxBox;
    
    enum DefaultValueType
    {
        eDefaultValueTypeInt,
        eDefaultValueTypeDouble,
        eDefaultValueTypeBool,
        eDefaultValueTypeString
    };
    
    
    Natron::Label* defaultValueLabel;
    SpinBox* default0;
    SpinBox* default1;
    SpinBox* default2;
    SpinBox* default3;
    LineEdit* defaultStr;
    QCheckBox* defaultBool;
    
    Natron::Label* menuItemsLabel;
    QTextEdit* menuItemsEdit;
    
    Natron::Label* multiLineLabel;
    QCheckBox* multiLine;
    
    Natron::Label* richTextLabel;
    QCheckBox* richText;
    
    Natron::Label* sequenceDialogLabel;
    QCheckBox* sequenceDialog;
    
    Natron::Label* multiPathLabel;
    QCheckBox* multiPath;
    
    Natron::Label* groupAsTabLabel;
    QCheckBox* groupAsTab;
    
    Natron::Label* parentGroupLabel;
    ComboBox* parentGroup;
    
    Natron::Label* parentPageLabel;
    ComboBox* parentPage;
    
    std::list<KnobGroup*> userGroups;
    std::list<KnobPage*> userPages; //< all user pages except the "User" one
    
    AddKnobDialogPrivate(DockablePanel* panel)
    : knob()
    , originalKnobSerialization()
    , isKnobAlias()
    , panel(panel)
    , vLayout(0)
    , mainContainer(0)
    , mainLayout(0)
    , typeLabel(0)
    , typeChoice(0)
    , nameLabel(0)
    , nameLineEdit(0)
    , labelLabel(0)
    , labelLineEdit(0)
    , hideLabel(0)
    , hideBox(0)
    , startNewLineLabel(0)
    , startNewLineBox(0)
    , animatesLabel(0)
    , animatesCheckbox(0)
    , evaluatesLabel(0)
    , evaluatesOnChange(0)
    , tooltipLabel(0)
    , tooltipArea(0)
    , minLabel(0)
    , minBox(0)
    , maxLabel(0)
    , maxBox(0)
    , dminLabel(0)
    , dminBox(0)
    , dmaxLabel(0)
    , dmaxBox(0)
    , defaultValueLabel(0)
    , default0(0)
    , default1(0)
    , default2(0)
    , default3(0)
    , defaultStr(0)
    , defaultBool(0)
    , menuItemsLabel(0)
    , menuItemsEdit(0)
    , multiLineLabel(0)
    , multiLine(0)
    , richTextLabel(0)
    , richText(0)
    , sequenceDialogLabel(0)
    , sequenceDialog(0)
    , multiPathLabel(0)
    , multiPath(0)
    , groupAsTabLabel(0)
    , groupAsTab(0)
    , parentGroupLabel(0)
    , parentGroup(0)
    , parentPageLabel(0)
    , parentPage(0)
    , userGroups()
    , userPages()
    {
        
    }
    
    void setVisibleMinMax(bool visible);
    
    void setVisibleMenuItems(bool visible);
    
    void setVisibleAnimates(bool visible);
    
    void setVisibleEvaluate(bool visible);
    
    void setVisibleStartNewLine(bool visible);
    
    void setVisibleHide(bool visible);
    
    void setVisibleMultiLine(bool visible);
    
    void setVisibleRichText(bool visible);
    
    void setVisibleSequence(bool visible);
    
    void setVisibleMultiPath(bool visible);
    
    void setVisibleGrpAsTab(bool visible);
    
    void setVisibleParent(bool visible);
    
    void setVisiblePage(bool visible);
    
    void setVisibleDefaultValues(bool visible,AddKnobDialogPrivate::DefaultValueType type,int dimensions);
    
    void createKnobFromSelection(int type,int optionalGroupIndex = -1);
    
    KnobGroup* getSelectedGroup() const;
};

static int getChoiceIndexFromKnobType(KnobI* knob)
{
    
    int dim = knob->getDimension();
    
    KnobInt* isInt = dynamic_cast<KnobInt*>(knob);
    KnobDouble* isDbl = dynamic_cast<KnobDouble*>(knob);
    KnobColor* isColor = dynamic_cast<KnobColor*>(knob);
    KnobChoice* isChoice = dynamic_cast<KnobChoice*>(knob);
    KnobBool* isBool = dynamic_cast<KnobBool*>(knob);
    KnobString* isStr = dynamic_cast<KnobString*>(knob);
    KnobFile* isFile = dynamic_cast<KnobFile*>(knob);
    KnobOutputFile* isOutputFile = dynamic_cast<KnobOutputFile*>(knob);
    KnobPath* isPath = dynamic_cast<KnobPath*>(knob);
    KnobGroup* isGrp = dynamic_cast<KnobGroup*>(knob);
    KnobPage* isPage = dynamic_cast<KnobPage*>(knob);
    KnobButton* isBtn = dynamic_cast<KnobButton*>(knob);
    
    if (isInt) {
        if (dim == 1) {
            return 0;
        } else if (dim == 2) {
            return 1;
        } else if (dim == 3) {
            return 2;
        }
    } else if (isDbl) {
        if (dim == 1) {
            return 3;
        } else if (dim == 2) {
            return 4;
        } else if (dim == 3) {
            return 5;
        }
    } else if (isColor) {
        if (dim == 3) {
            return 6;
        } else if (dim == 4) {
            return 7;
        }
    } else if (isChoice) {
        return 8;
    } else if (isBool) {
        return 9;
    } else if (isStr) {
        if (isStr->isLabel()) {
            return 10;
        } else  {
            return 11;
        }
    } else if (isFile) {
        return 12;
    } else if (isOutputFile) {
        return 13;
    } else if (isPath) {
        return 14;
    } else if (isGrp) {
        return 15;
    } else if (isPage) {
        return 16;
    } else if (isBtn) {
        return 17;
    }
    return -1;
}

AddKnobDialog::AddKnobDialog(DockablePanel* panel,const boost::shared_ptr<KnobI>& knob,QWidget* parent)
: QDialog(parent)
, _imp(new AddKnobDialogPrivate(panel))
{
    
    _imp->knob = knob;
    assert(!knob || knob->isUserKnob());
    
    //QFont font(NATRON_FONT,NATRON_FONT_SIZE_11);
    
    _imp->vLayout = new QVBoxLayout(this);
    _imp->vLayout->setContentsMargins(0, 0, 15, 0);
    
    _imp->mainContainer = new QWidget(this);
    _imp->mainLayout = new QFormLayout(_imp->mainContainer);
    _imp->mainLayout->setLabelAlignment(Qt::AlignVCenter | Qt::AlignRight);
    _imp->mainLayout->setFormAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    _imp->mainLayout->setSpacing(3);
    _imp->mainLayout->setContentsMargins(0, 0, 15, 0);
    
    _imp->vLayout->addWidget(_imp->mainContainer);
    
    {
        boost::shared_ptr<KnobI> isAlias;
        boost::shared_ptr<KnobI> listener;
        if (knob) {
            std::list<boost::shared_ptr<KnobI> > listeners;
            knob->getListeners(listeners);
            if (!listeners.empty()) {
                listener = listeners.front();
                isAlias = listener->getAliasMaster();
                if (isAlias != knob) {
                    listener.reset();
                }
            }
        }
        _imp->isKnobAlias = listener;
    }
    
    
    {
        QWidget* firstRowContainer = new QWidget(this);
        QHBoxLayout* firstRowLayout = new QHBoxLayout(firstRowContainer);
        firstRowLayout->setContentsMargins(0, 0, 0, 0);
        
        _imp->nameLabel = new Natron::Label(tr("Script name:"),this);
        _imp->nameLineEdit = new LineEdit(firstRowContainer);
        _imp->nameLineEdit->setToolTip(Natron::convertFromPlainText(tr("The name of the parameter as it will be used in Python scripts"), Qt::WhiteSpaceNormal));
        
        if (knob) {
            _imp->nameLineEdit->setText(knob->getName().c_str());
        }
        firstRowLayout->addWidget(_imp->nameLineEdit);
        firstRowLayout->addStretch();

        _imp->mainLayout->addRow(_imp->nameLabel, firstRowContainer);
        
    }
    
    {
        QWidget* secondRowContainer = new QWidget(this);
        QHBoxLayout* secondRowLayout = new QHBoxLayout(secondRowContainer);
        secondRowLayout->setContentsMargins(0, 0, 15, 0);
        _imp->labelLabel = new Natron::Label(tr("Label:"),secondRowContainer);
        _imp->labelLineEdit = new LineEdit(secondRowContainer);
        _imp->labelLineEdit->setToolTip(Natron::convertFromPlainText(tr("The label of the parameter as displayed on the graphical user interface"), Qt::WhiteSpaceNormal));
        if (knob) {
            _imp->labelLineEdit->setText(knob->getLabel().c_str());
        }
        secondRowLayout->addWidget(_imp->labelLineEdit);
        _imp->hideLabel = new Natron::Label(tr("Hide:"),secondRowContainer);
        secondRowLayout->addWidget(_imp->hideLabel);
        _imp->hideBox = new QCheckBox(secondRowContainer);
        _imp->hideBox->setToolTip(Natron::convertFromPlainText(tr("If checked the parameter will not be visible on the user interface"), Qt::WhiteSpaceNormal));
        if (knob) {
            _imp->hideBox->setChecked(knob->getIsSecret());
        }

        secondRowLayout->addWidget(_imp->hideBox);
        _imp->startNewLineLabel = new Natron::Label(tr("Start new line:"),secondRowContainer);
        secondRowLayout->addWidget(_imp->startNewLineLabel);
        _imp->startNewLineBox = new QCheckBox(secondRowContainer);
        _imp->startNewLineBox->setToolTip(Natron::convertFromPlainText(tr("If unchecked the parameter will be on the same line as the previous parameter"), Qt::WhiteSpaceNormal));
        if (knob) {
            
            // get the flag on the previous knob
            bool startNewLine = true;
            boost::shared_ptr<KnobI> parentKnob = _imp->knob->getParentKnob();
            if (parentKnob) {
                KnobGroup* parentIsGrp = dynamic_cast<KnobGroup*>(parentKnob.get());
                KnobPage* parentIsPage = dynamic_cast<KnobPage*>(parentKnob.get());
                assert(parentIsGrp || parentIsPage);
                std::vector<boost::shared_ptr<KnobI> > children;
                if (parentIsGrp) {
                    children = parentIsGrp->getChildren();
                } else if (parentIsPage) {
                    children = parentIsPage->getChildren();
                }
                for (U32 i = 0; i < children.size(); ++i) {
                    if (children[i] == _imp->knob) {
                        if (i > 0) {
                            startNewLine = children[i - 1]->isNewLineActivated();
                        }
                        break;
                    }
                }
            }

            
            _imp->startNewLineBox->setChecked(startNewLine);
        }
        secondRowLayout->addWidget(_imp->startNewLineBox);
        secondRowLayout->addStretch();
        
        _imp->mainLayout->addRow(_imp->labelLabel, secondRowContainer);
    }
    
    {
        QWidget* thirdRowContainer = new QWidget(this);
        QHBoxLayout* thirdRowLayout = new QHBoxLayout(thirdRowContainer);
        thirdRowLayout->setContentsMargins(0, 0, 15, 0);
        
        if (!knob) {
            _imp->typeLabel = new Natron::Label(tr("Type:"),thirdRowContainer);
            _imp->typeChoice = new ComboBox(thirdRowContainer);
            _imp->typeChoice->setToolTip(Natron::convertFromPlainText(tr("The data type of the parameter."), Qt::WhiteSpaceNormal));
            _imp->typeChoice->addItem("Integer");
            _imp->typeChoice->addItem("Integer 2D");
            _imp->typeChoice->addItem("Integer 3D");
            _imp->typeChoice->addItem("Floating point");
            _imp->typeChoice->addItem("Floating point 2D");
            _imp->typeChoice->addItem("Floating point 3D");
            _imp->typeChoice->addItem("Color RGB");
            _imp->typeChoice->addItem("Color RGBA");
            _imp->typeChoice->addItem("Choice (Pulldown)");
            _imp->typeChoice->addItem("Checkbox");
            _imp->typeChoice->addItem("Label");
            _imp->typeChoice->addItem("Text input");
            _imp->typeChoice->addItem("Input file");
            _imp->typeChoice->addItem("Output file");
            _imp->typeChoice->addItem("Directory");
            _imp->typeChoice->addItem("Group");
            _imp->typeChoice->addItem("Page");
            _imp->typeChoice->addItem("Button");
            QObject::connect(_imp->typeChoice, SIGNAL(currentIndexChanged(int)),this, SLOT(onTypeCurrentIndexChanged(int)));
            
            thirdRowLayout->addWidget(_imp->typeChoice);
        }
        _imp->animatesLabel = new Natron::Label(tr("Animates:"),thirdRowContainer);

        if (!knob) {
            thirdRowLayout->addWidget(_imp->animatesLabel);
        }
        _imp->animatesCheckbox = new QCheckBox(thirdRowContainer);
        _imp->animatesCheckbox->setToolTip(Natron::convertFromPlainText(tr("When checked this parameter will be able to animate with keyframes."), Qt::WhiteSpaceNormal));
        if (knob) {
            _imp->animatesCheckbox->setChecked(knob->isAnimationEnabled());
        }

        thirdRowLayout->addWidget(_imp->animatesCheckbox);
        _imp->evaluatesLabel = new Natron::Label(Natron::convertFromPlainText(tr("Render on change:"), Qt::WhiteSpaceNormal),thirdRowContainer);
        thirdRowLayout->addWidget(_imp->evaluatesLabel);
        _imp->evaluatesOnChange = new QCheckBox(thirdRowContainer);
        _imp->evaluatesOnChange->setToolTip(Natron::convertFromPlainText(tr("If checked, when the value of this parameter changes a new render will be triggered."), Qt::WhiteSpaceNormal));
        if (knob) {
            _imp->evaluatesOnChange->setChecked(knob->getEvaluateOnChange());
        }
        thirdRowLayout->addWidget(_imp->evaluatesOnChange);
        thirdRowLayout->addStretch();
        
        if (!knob) {
            _imp->mainLayout->addRow(_imp->typeLabel, thirdRowContainer);
        } else {
            _imp->mainLayout->addRow(_imp->animatesLabel, thirdRowContainer);
        }
    }
    {
        _imp->tooltipLabel = new Natron::Label(tr("Tooltip:"),this);
        _imp->tooltipArea = new QTextEdit(this);
        _imp->tooltipArea->setToolTip(Natron::convertFromPlainText(tr("The help tooltip that will appear when hovering the parameter with the mouse."), Qt::WhiteSpaceNormal));
        _imp->mainLayout->addRow(_imp->tooltipLabel,_imp->tooltipArea);
        if (knob) {
            _imp->tooltipArea->setPlainText(knob->getHintToolTip().c_str());
        }
    }
    {
        _imp->menuItemsLabel = new Natron::Label(tr("Menu items:"),this);
        _imp->menuItemsEdit = new QTextEdit(this);
        QString tt = Natron::convertFromPlainText(tr("The entries that will be available in the drop-down menu. \n"
                                                 "Each line defines a new menu entry. You can specify a specific help tooltip for each entry "
                                                 "by separating the entry text from the help with the following characters on the line: "
                                                 "<?> \n\n"
                                                 "E.g: Special function<?>Will use our very own special function."), Qt::WhiteSpaceNormal);
        _imp->menuItemsEdit->setToolTip(tt);
        _imp->mainLayout->addRow(_imp->menuItemsLabel,_imp->menuItemsEdit);
        
        KnobChoice* isChoice = dynamic_cast<KnobChoice*>(knob.get());
        if (isChoice) {
            std::vector<std::string> entries = isChoice->getEntries_mt_safe();
            std::vector<std::string> entriesHelp = isChoice->getEntriesHelp_mt_safe();
            QString data;
            for (U32 i = 0; i < entries.size(); ++i) {
                QString line(entries[i].c_str());
                if (i < entriesHelp.size() && !entriesHelp[i].empty()) {
                    line.append("<?>");
                    line.append(entriesHelp[i].c_str());
                }
                data.append(line);
                data.append('\n');
            }
            _imp->menuItemsEdit->setPlainText(data);
        }
    }
    {
        QWidget* optContainer = new QWidget(this);
        QHBoxLayout* optLayout = new QHBoxLayout(optContainer);
        optLayout->setContentsMargins(0, 0, 15, 0);
        
        _imp->multiLineLabel = new Natron::Label(tr("Multi-line:"),optContainer);
        _imp->multiLine = new QCheckBox(optContainer);
        _imp->multiLine->setToolTip(Natron::convertFromPlainText(tr("Should this text be multi-line or single-line ?"), Qt::WhiteSpaceNormal));
        optLayout->addWidget(_imp->multiLine);
        _imp->mainLayout->addRow(_imp->multiLineLabel, optContainer);
        
        KnobString* isStr = dynamic_cast<KnobString*>(knob.get());
        if (isStr) {
            if (isStr && isStr->isMultiLine()) {
                _imp->multiLine->setChecked(true);
            }
        }
    }
    {
        QWidget* optContainer = new QWidget(this);
        QHBoxLayout* optLayout = new QHBoxLayout(optContainer);
        optLayout->setContentsMargins(0, 0, 15, 0);
        
        _imp->richTextLabel = new Natron::Label(tr("Rich text:"),optContainer);
        _imp->richText = new QCheckBox(optContainer);
        QString tt = Natron::convertFromPlainText(tr("If checked, the text area will be able to use rich text encoding with a sub-set of html.\n "
                                                 "This property is only valid for multi-line input text only."), Qt::WhiteSpaceNormal);

        _imp->richText->setToolTip(tt);
        optLayout->addWidget(_imp->richText);
        _imp->mainLayout->addRow(_imp->richTextLabel, optContainer);
        
        KnobString* isStr = dynamic_cast<KnobString*>(knob.get());
        if (isStr) {
            if (isStr && isStr->isMultiLine() && isStr->usesRichText()) {
                _imp->richText->setChecked(true);
            }
        }
    }
    {
        QWidget* optContainer = new QWidget(this);
        QHBoxLayout* optLayout = new QHBoxLayout(optContainer);
        optLayout->setContentsMargins(0, 0, 15, 0);
        
        _imp->sequenceDialogLabel = new Natron::Label(tr("Use sequence dialog:"),optContainer);
        _imp->sequenceDialog = new QCheckBox(optContainer);
        _imp->sequenceDialog->setToolTip(Natron::convertFromPlainText(tr("If checked the file dialog for this parameter will be able to decode image sequences."), Qt::WhiteSpaceNormal));
        optLayout->addWidget(_imp->sequenceDialog);
        _imp->mainLayout->addRow(_imp->sequenceDialogLabel, optContainer);
        
        KnobFile* isFile = dynamic_cast<KnobFile*>(knob.get());
        KnobOutputFile* isOutFile = dynamic_cast<KnobOutputFile*>(knob.get());
        if (isFile) {
            if (isFile->isInputImageFile()) {
                _imp->sequenceDialog->setChecked(true);
            }
        } else if (isOutFile) {
            if (isOutFile->isOutputImageFile()) {
                _imp->sequenceDialog->setChecked(true);
            }
        }
    }
    {
        QWidget* optContainer = new QWidget(this);
        QHBoxLayout* optLayout = new QHBoxLayout(optContainer);
        optLayout->setContentsMargins(0, 0, 15, 0);
        
        _imp->multiPathLabel = new Natron::Label(Natron::convertFromPlainText(tr("Multiple paths:"), Qt::WhiteSpaceNormal),optContainer);
        _imp->multiPath = new QCheckBox(optContainer);
        _imp->multiPath->setToolTip(Natron::convertFromPlainText(tr("If checked the parameter will be a table where each entry points to a different path."), Qt::WhiteSpaceNormal));
        optLayout->addWidget(_imp->multiPath);
        _imp->mainLayout->addRow(_imp->multiPathLabel, optContainer);
        
        KnobPath* isStr = dynamic_cast<KnobPath*>(knob.get());
        if (isStr) {
            if (isStr && isStr->isMultiPath()) {
                _imp->multiPath->setChecked(true);
            }
        }
    }
    {
        QWidget* optContainer = new QWidget(this);
        QHBoxLayout* optLayout = new QHBoxLayout(optContainer);
        optLayout->setContentsMargins(0, 0, 15, 0);
        
        _imp->groupAsTabLabel = new Natron::Label(tr("Group as tab:"),optContainer);
        _imp->groupAsTab = new QCheckBox(optContainer);
        _imp->groupAsTab->setToolTip(Natron::convertFromPlainText(tr("If checked the group will be a tab instead."), Qt::WhiteSpaceNormal));
        optLayout->addWidget(_imp->groupAsTab);
        _imp->mainLayout->addRow(_imp->groupAsTabLabel, optContainer);
        
        KnobGroup* isGrp = dynamic_cast<KnobGroup*>(knob.get());
        if (isGrp) {
            if (isGrp && isGrp->isTab()) {
                _imp->groupAsTab->setChecked(true);
            }
        }

    }
    {
        QWidget* minMaxContainer = new QWidget(this);
        QWidget* dminMaxContainer = new QWidget(this);
        QHBoxLayout* minMaxLayout = new QHBoxLayout(minMaxContainer);
        QHBoxLayout* dminMaxLayout = new QHBoxLayout(dminMaxContainer);
        minMaxLayout->setContentsMargins(0, 0, 0, 0);
        dminMaxLayout->setContentsMargins(0, 0, 0, 0);
        _imp->minLabel = new Natron::Label(tr("Minimum:"),minMaxContainer);

        _imp->minBox = new SpinBox(minMaxContainer,SpinBox::eSpinBoxTypeDouble);
        _imp->minBox->setToolTip(Natron::convertFromPlainText(tr("Set the minimum value for the parameter. Even though the user might input "
                                                             "a value higher or lower than the specified min/max range, internally the "
                                                             "real value will be clamped to this interval."), Qt::WhiteSpaceNormal));
        minMaxLayout->addWidget(_imp->minBox);
        
        _imp->maxLabel = new Natron::Label(tr("Maximum:"),minMaxContainer);
        _imp->maxBox = new SpinBox(minMaxContainer,SpinBox::eSpinBoxTypeDouble);
        _imp->maxBox->setToolTip(Natron::convertFromPlainText(tr("Set the maximum value for the parameter. Even though the user might input "
                                                             "a value higher or lower than the specified min/max range, internally the "
                                                             "real value will be clamped to this interval."), Qt::WhiteSpaceNormal));
        minMaxLayout->addWidget(_imp->maxLabel);
        minMaxLayout->addWidget(_imp->maxBox);
        minMaxLayout->addStretch();
        
        _imp->dminLabel = new Natron::Label(tr("Display Minimum:"),dminMaxContainer);
        _imp->dminBox = new SpinBox(dminMaxContainer,SpinBox::eSpinBoxTypeDouble);
        _imp->dminBox->setToolTip(Natron::convertFromPlainText(tr("Set the display minimum value for the parameter. This is a hint that is typically "
                                                              "used to set the range of the slider."), Qt::WhiteSpaceNormal));
        dminMaxLayout->addWidget(_imp->dminBox);
        
        _imp->dmaxLabel = new Natron::Label(tr("Display Maximum:"),dminMaxContainer);
        _imp->dmaxBox = new SpinBox(dminMaxContainer,SpinBox::eSpinBoxTypeDouble);
        _imp->dmaxBox->setToolTip(Natron::convertFromPlainText(tr("Set the display maximum value for the parameter. This is a hint that is typically "
                                                              "used to set the range of the slider."), Qt::WhiteSpaceNormal));
        dminMaxLayout->addWidget(_imp->dmaxLabel);
        dminMaxLayout->addWidget(_imp->dmaxBox);
       
        dminMaxLayout->addStretch();
        
        KnobDouble* isDbl = dynamic_cast<KnobDouble*>(knob.get());
        KnobInt* isInt = dynamic_cast<KnobInt*>(knob.get());
        KnobColor* isColor = dynamic_cast<KnobColor*>(knob.get());
        

        if (isDbl) {
            double min = isDbl->getMinimum(0);
            double max = isDbl->getMaximum(0);
            double dmin = isDbl->getDisplayMinimum(0);
            double dmax = isDbl->getDisplayMaximum(0);
            _imp->minBox->setValue(min);
            _imp->maxBox->setValue(max);
            _imp->dminBox->setValue(dmin);
            _imp->dmaxBox->setValue(dmax);
        } else if (isInt) {
            int min = isInt->getMinimum(0);
            int max = isInt->getMaximum(0);
            int dmin = isInt->getDisplayMinimum(0);
            int dmax = isInt->getDisplayMaximum(0);
            _imp->minBox->setValue(min);
            _imp->maxBox->setValue(max);
            _imp->dminBox->setValue(dmin);
            _imp->dmaxBox->setValue(dmax);

        } else if (isColor) {
            double min = isColor->getMinimum(0);
            double max = isColor->getMaximum(0);
            double dmin = isColor->getDisplayMinimum(0);
            double dmax = isColor->getDisplayMaximum(0);
            _imp->minBox->setValue(min);
            _imp->maxBox->setValue(max);
            _imp->dminBox->setValue(dmin);
            _imp->dmaxBox->setValue(dmax);

        }
        
        _imp->mainLayout->addRow(_imp->minLabel, minMaxContainer);
        _imp->mainLayout->addRow(_imp->dminLabel, dminMaxContainer);
    }
    
    {
        QWidget* defValContainer = new QWidget(this);
        QHBoxLayout* defValLayout = new QHBoxLayout(defValContainer);
        defValLayout->setContentsMargins(0, 0, 0, 0);
        _imp->defaultValueLabel = new Natron::Label(tr("Default value:"),defValContainer);

        _imp->default0 = new SpinBox(defValContainer,SpinBox::eSpinBoxTypeDouble);
        _imp->default0->setValue(0);
        _imp->default0->setToolTip(Natron::convertFromPlainText(tr("Set the default value for the parameter (dimension 0)."), Qt::WhiteSpaceNormal));
        defValLayout->addWidget(_imp->default0);
        
        _imp->default1 = new SpinBox(defValContainer,SpinBox::eSpinBoxTypeDouble);
        _imp->default1->setValue(0);
        _imp->default1->setToolTip(Natron::convertFromPlainText(tr("Set the default value for the parameter (dimension 1)."), Qt::WhiteSpaceNormal));
        defValLayout->addWidget(_imp->default1);
        
        _imp->default2 = new SpinBox(defValContainer,SpinBox::eSpinBoxTypeDouble);
        _imp->default2->setValue(0);
        _imp->default2->setToolTip(Natron::convertFromPlainText(tr("Set the default value for the parameter (dimension 2)."), Qt::WhiteSpaceNormal));
        defValLayout->addWidget(_imp->default2);
        
        _imp->default3 = new SpinBox(defValContainer,SpinBox::eSpinBoxTypeDouble);
        _imp->default3->setValue(0);
        _imp->default3->setToolTip(Natron::convertFromPlainText(tr("Set the default value for the parameter (dimension 3)."), Qt::WhiteSpaceNormal));
        defValLayout->addWidget(_imp->default3);

        
        _imp->defaultStr = new LineEdit(defValContainer);
        _imp->defaultStr->setToolTip(Natron::convertFromPlainText(tr("Set the default value for the parameter."), Qt::WhiteSpaceNormal));
        defValLayout->addWidget(_imp->defaultStr);
        
        
        _imp->defaultBool = new QCheckBox(defValContainer);
        _imp->defaultBool->setToolTip(Natron::convertFromPlainText(tr("Set the default value for the parameter."), Qt::WhiteSpaceNormal));
        _imp->defaultBool->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE,NATRON_MEDIUM_BUTTON_SIZE);
        defValLayout->addWidget(_imp->defaultBool);

        defValLayout->addStretch();
        
        _imp->mainLayout->addRow(_imp->defaultValueLabel, defValContainer);
        
        
        Knob<double>* isDbl = dynamic_cast<Knob<double>*>(knob.get());
        Knob<int>* isInt = dynamic_cast<Knob<int>*>(knob.get());
        KnobBool* isBool = dynamic_cast<KnobBool*>(knob.get());
        Knob<std::string>* isStr = dynamic_cast<Knob<std::string>*>(knob.get());
        
        if (isDbl) {
            _imp->default0->setValue(isDbl->getDefaultValue(0));
            if (isDbl->getDimension() >= 2) {
                _imp->default1->setValue(isDbl->getDefaultValue(1));
            }
            if (isDbl->getDimension() >= 3) {
                _imp->default2->setValue(isDbl->getDefaultValue(2));
            }
            if (isDbl->getDimension() >= 4) {
                _imp->default3->setValue(isDbl->getDefaultValue(3));
            }
        } else if (isInt) {
            _imp->default0->setValue(isInt->getDefaultValue(0));
            if (isInt->getDimension() >= 2) {
                _imp->default1->setValue(isInt->getDefaultValue(1));
            }
            if (isInt->getDimension() >= 3) {
                _imp->default2->setValue(isInt->getDefaultValue(2));
            }

        } else if (isBool) {
            _imp->defaultBool->setChecked(isBool->getDefaultValue(0));
        } else if (isStr) {
            _imp->defaultStr->setText(isStr->getDefaultValue(0).c_str());
        }
        
    }
    
    
    const std::map<boost::weak_ptr<KnobI>,KnobGui*>& knobs = _imp->panel->getKnobs();
    for (std::map<boost::weak_ptr<KnobI>,KnobGui*>::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        boost::shared_ptr<KnobI> knob = it->first.lock();
        if (knob->isUserKnob()) {
            KnobGroup* isGrp = dynamic_cast<KnobGroup*>(knob.get());
            if (isGrp) {
                _imp->userGroups.push_back(isGrp);
            }
        }
    }
    
    
    
    if (!_imp->userGroups.empty()) {
        QWidget* optContainer = new QWidget(this);
        QHBoxLayout* optLayout = new QHBoxLayout(optContainer);
        optLayout->setContentsMargins(0, 0, 15, 0);
        
        _imp->parentGroupLabel = new Natron::Label(tr("Group:"),optContainer);
        _imp->parentGroup = new ComboBox(optContainer);
        
        _imp->parentGroup->setToolTip(Natron::convertFromPlainText(tr("The name of the group under which this parameter will appear."), Qt::WhiteSpaceNormal));
        optLayout->addWidget(_imp->parentGroup);
        
        _imp->mainLayout->addRow(_imp->parentGroupLabel, optContainer);
    }
    
    if (!knob) {
        QWidget* optContainer = new QWidget(this);
        QHBoxLayout* optLayout = new QHBoxLayout(optContainer);
        optLayout->setContentsMargins(0, 0, 15, 0);
        _imp->parentPageLabel = new Natron::Label(tr("Page:"),optContainer);
        _imp->parentPage = new ComboBox(optContainer);
        _imp->parentPage->addItem(NATRON_USER_MANAGED_KNOBS_PAGE);
        QObject::connect(_imp->parentPage,SIGNAL(currentIndexChanged(int)),this,SLOT(onPageCurrentIndexChanged(int)));
        const std::vector<boost::shared_ptr<KnobI> >& knobs = _imp->panel->getHolder()->getKnobs();
        for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it = knobs.begin() ; it != knobs.end(); ++it) {
            if ((*it)->isUserKnob()) {
                KnobPage* isPage = dynamic_cast<KnobPage*>(it->get());
                if (isPage && isPage->getName() != NATRON_USER_MANAGED_KNOBS_PAGE) {
                    _imp->userPages.push_back(isPage);
                }
            }
        }
        
        for (std::list<KnobPage*>::iterator it = _imp->userPages.begin(); it != _imp->userPages.end(); ++it) {
            _imp->parentPage->addItem((*it)->getName().c_str());
        }
        _imp->parentPage->setToolTip(Natron::convertFromPlainText(tr("The tab under which this parameter will appear."), Qt::WhiteSpaceNormal));
        optLayout->addWidget(_imp->parentPage);
        if (knob) {
            ////find in which page the knob should be
            KnobPage* isTopLevelParentAPage = knob->getTopLevelPage();
            assert(isTopLevelParentAPage);
            if (isTopLevelParentAPage->getName() != NATRON_USER_MANAGED_KNOBS_PAGE) {
                int index = 1; // 1 because of the "User" item
                bool found = false;
                for (std::list<KnobPage*>::iterator it = _imp->userPages.begin(); it != _imp->userPages.end(); ++it, ++index) {
                    if ((*it) == isTopLevelParentAPage) {
                        found = true;
                        break;
                    }
                }
                if (found) {
                    _imp->parentPage->setCurrentIndex(index);
                }
            }
            
            
        }
        
        _imp->mainLayout->addRow(_imp->parentPageLabel, optContainer);
        onPageCurrentIndexChanged(0);
    } else { // if(!knob)
        
        if (_imp->parentGroup) {
            KnobPage* topLvlPage = knob->getTopLevelPage();
            assert(topLvlPage);
            boost::shared_ptr<KnobI> parent = knob->getParentKnob();
            KnobGroup* isParentGrp = dynamic_cast<KnobGroup*>(parent.get());
            _imp->parentGroup->addItem("-");
            int idx = 1;
            for (std::list<KnobGroup*>::iterator it = _imp->userGroups.begin(); it != _imp->userGroups.end(); ++it, ++idx) {
                KnobPage* page = (*it)->getTopLevelPage();
                assert(page);
                
                ///add only grps whose parent page is the selected page
                if (page == topLvlPage) {
                    _imp->parentGroup->addItem((*it)->getName().c_str());
                    if (isParentGrp && isParentGrp == *it) {
                        _imp->parentGroup->setCurrentIndex(idx);
                    }
                }
                
            }
        }
    }
    
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::StandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel),Qt::Horizontal,this);
    QObject::connect(buttons,SIGNAL(rejected()), this, SLOT(reject()));
    QObject::connect(buttons,SIGNAL(accepted()), this, SLOT(onOkClicked()));
    _imp->vLayout->addWidget(buttons);
    
    int type;
    if (!knob) {
        type = _imp->typeChoice->activeIndex();
    } else {
        type = getChoiceIndexFromKnobType(knob.get());
        assert(type != -1);
    }
    onTypeCurrentIndexChanged(type);
    _imp->panel->setUserPageActiveIndex();
    
    if (knob) {
        _imp->originalKnobSerialization.reset(new KnobSerialization(knob));
    }
}

void
AddKnobDialog::onPageCurrentIndexChanged(int index)
{
    if (!_imp->parentGroup) {
        return;
    }
    _imp->parentGroup->clear();
    _imp->parentGroup->addItem("-");
    
    std::string selectedPage = _imp->parentPage->itemText(index).toStdString();
    KnobPage* parentPage = 0;
    
    if (selectedPage == NATRON_USER_MANAGED_KNOBS_PAGE) {
        parentPage = _imp->panel->getUserPageKnob().get();
    } else {
        for (std::list<KnobPage*>::iterator it = _imp->userPages.begin(); it != _imp->userPages.end(); ++it) {
            if ((*it)->getName() == selectedPage) {
                parentPage = *it;
                break;
            }
        }
    }
    
    for (std::list<KnobGroup*>::iterator it = _imp->userGroups.begin(); it != _imp->userGroups.end(); ++it) {
        KnobPage* page = (*it)->getTopLevelPage();
        assert(page);
        
        ///add only grps whose parent page is the selected page
        if (page == parentPage) {
            _imp->parentGroup->addItem((*it)->getName().c_str());
        }
        
    }
}

void
AddKnobDialog::onTypeCurrentIndexChanged(int index)
{
    if (_imp->isKnobAlias) {
        _imp->setVisibleAnimates(false);
        _imp->setVisibleEvaluate(false);
        _imp->setVisibleHide(false);
        _imp->setVisibleMenuItems(false);
        _imp->setVisibleMinMax(false);
        _imp->setVisibleStartNewLine(true);
        _imp->setVisibleMultiLine(false);
        _imp->setVisibleMultiPath(false);
        _imp->setVisibleRichText(false);
        _imp->setVisibleSequence(false);
        _imp->setVisibleGrpAsTab(false);
        _imp->setVisibleParent(false);
        _imp->setVisibleDefaultValues(false,AddKnobDialogPrivate::eDefaultValueTypeInt, 1);
        return;
    }
    _imp->setVisiblePage(index != 16);
    switch (index) {
        case 0: // int
        case 1: // int 2D
        case 2: // int 3D
        case 3: // fp
        case 4: // fp 2D
        case 5: // fp 3D
        case 6: // RGB
        case 7: // RGBA
            _imp->setVisibleAnimates(true);
            _imp->setVisibleEvaluate(true);
            _imp->setVisibleHide(true);
            _imp->setVisibleMenuItems(false);
            _imp->setVisibleMinMax(true);
            _imp->setVisibleStartNewLine(true);
            _imp->setVisibleMultiLine(false);
            _imp->setVisibleMultiPath(false);
            _imp->setVisibleRichText(false);
            _imp->setVisibleSequence(false);
            _imp->setVisibleGrpAsTab(false);
            _imp->setVisibleParent(true);
            if (index == 0 || index == 3) {
                _imp->setVisibleDefaultValues(true,
                                              index == 0 ? AddKnobDialogPrivate::eDefaultValueTypeInt : AddKnobDialogPrivate::eDefaultValueTypeDouble,
                                              1);
            } else if (index == 1 || index == 4) {
                _imp->setVisibleDefaultValues(true,
                                              index == 1 ? AddKnobDialogPrivate::eDefaultValueTypeInt : AddKnobDialogPrivate::eDefaultValueTypeDouble,
                                              2);
            } else if (index == 2 || index == 5 || index == 6) {
                _imp->setVisibleDefaultValues(true,
                                              index == 2 ? AddKnobDialogPrivate::eDefaultValueTypeInt : AddKnobDialogPrivate::eDefaultValueTypeDouble,
                                              3);
            } else if (index == 7) {
                _imp->setVisibleDefaultValues(true,AddKnobDialogPrivate::eDefaultValueTypeDouble,4);
            }
            break;
        case 8: // choice
            _imp->setVisibleAnimates(true);
            _imp->setVisibleEvaluate(true);
            _imp->setVisibleHide(true);
            _imp->setVisibleMenuItems(true);
            _imp->setVisibleMinMax(false);
            _imp->setVisibleStartNewLine(true);
            _imp->setVisibleMultiLine(false);
            _imp->setVisibleMultiPath(false);
            _imp->setVisibleRichText(false);
            _imp->setVisibleSequence(false);
            _imp->setVisibleGrpAsTab(false);
            _imp->setVisibleParent(true);
            _imp->setVisibleDefaultValues(true,AddKnobDialogPrivate::eDefaultValueTypeInt,1);
            break;
        case 9: // bool
            _imp->setVisibleAnimates(true);
            _imp->setVisibleEvaluate(true);
            _imp->setVisibleHide(true);
            _imp->setVisibleMenuItems(false);
            _imp->setVisibleMinMax(false);
            _imp->setVisibleStartNewLine(true);
            _imp->setVisibleMultiLine(false);
            _imp->setVisibleMultiPath(false);
            _imp->setVisibleRichText(false);
            _imp->setVisibleSequence(false);
            _imp->setVisibleGrpAsTab(false);
            _imp->setVisibleParent(true);
            _imp->setVisibleDefaultValues(true,AddKnobDialogPrivate::eDefaultValueTypeBool,1);
            break;
        case 10: // label
            _imp->setVisibleAnimates(false);
            _imp->setVisibleEvaluate(false);
            _imp->setVisibleHide(true);
            _imp->setVisibleMenuItems(false);
            _imp->setVisibleMinMax(false);
            _imp->setVisibleStartNewLine(true);
            _imp->setVisibleMultiLine(false);
            _imp->setVisibleMultiPath(false);
            _imp->setVisibleRichText(false);
            _imp->setVisibleSequence(false);
            _imp->setVisibleGrpAsTab(false);
            _imp->setVisibleParent(true);
            _imp->setVisibleDefaultValues(true,AddKnobDialogPrivate::eDefaultValueTypeString,1);
            break;
        case 11: // text input
            _imp->setVisibleAnimates(true);
            _imp->setVisibleEvaluate(true);
            _imp->setVisibleHide(true);
            _imp->setVisibleMenuItems(false);
            _imp->setVisibleMinMax(false);
            _imp->setVisibleStartNewLine(false);
            _imp->setVisibleMultiLine(true);
            _imp->setVisibleMultiPath(false);
            _imp->setVisibleRichText(true);
            _imp->setVisibleSequence(false);
            _imp->setVisibleGrpAsTab(false);
            _imp->setVisibleParent(true);
            _imp->setVisibleDefaultValues(true,AddKnobDialogPrivate::eDefaultValueTypeString,1);
            break;
        case 12: // input file
        case 13: // output file
            _imp->setVisibleAnimates(false);
            _imp->setVisibleEvaluate(true);
            _imp->setVisibleHide(true);
            _imp->setVisibleMenuItems(false);
            _imp->setVisibleMinMax(false);
            _imp->setVisibleStartNewLine(false);
            _imp->setVisibleMultiLine(false);
            _imp->setVisibleMultiPath(false);
            _imp->setVisibleRichText(false);
            _imp->setVisibleSequence(true);
            _imp->setVisibleGrpAsTab(false);
            _imp->setVisibleParent(true);
            _imp->setVisibleDefaultValues(true,AddKnobDialogPrivate::eDefaultValueTypeString,1);
            break;
        case 14: // path
            _imp->setVisibleAnimates(false);
            _imp->setVisibleEvaluate(true);
            _imp->setVisibleHide(true);
            _imp->setVisibleMenuItems(false);
            _imp->setVisibleMinMax(false);
            _imp->setVisibleStartNewLine(false);
            _imp->setVisibleMultiLine(false);
            _imp->setVisibleMultiPath(true);
            _imp->setVisibleRichText(false);
            _imp->setVisibleSequence(false);
            _imp->setVisibleGrpAsTab(false);
            _imp->setVisibleParent(true);
            _imp->setVisibleDefaultValues(true,AddKnobDialogPrivate::eDefaultValueTypeString,1);
            break;
        case 15: // grp
            _imp->setVisibleAnimates(false);
            _imp->setVisibleEvaluate(false);
            _imp->setVisibleHide(true);
            _imp->setVisibleMenuItems(false);
            _imp->setVisibleMinMax(false);
            _imp->setVisibleStartNewLine(false);
            _imp->setVisibleMultiLine(false);
            _imp->setVisibleMultiPath(false);
            _imp->setVisibleRichText(false);
            _imp->setVisibleSequence(false);
            _imp->setVisibleGrpAsTab(true);
            _imp->setVisibleParent(false);
            _imp->setVisibleDefaultValues(false,AddKnobDialogPrivate::eDefaultValueTypeInt,1);
            break;
        case 16: // page
            _imp->setVisibleAnimates(false);
            _imp->setVisibleEvaluate(false);
            _imp->setVisibleHide(false);
            _imp->setVisibleMenuItems(false);
            _imp->setVisibleMinMax(false);
            _imp->setVisibleStartNewLine(false);
            _imp->setVisibleMultiLine(false);
            _imp->setVisibleMultiPath(false);
            _imp->setVisibleRichText(false);
            _imp->setVisibleSequence(false);
            _imp->setVisibleGrpAsTab(false);
            _imp->setVisibleParent(false);
            _imp->setVisibleDefaultValues(false,AddKnobDialogPrivate::eDefaultValueTypeInt,1);
            break;
        case 17: // button
            _imp->setVisibleAnimates(false);
            _imp->setVisibleEvaluate(false);
            _imp->setVisibleHide(false);
            _imp->setVisibleMenuItems(false);
            _imp->setVisibleMinMax(false);
            _imp->setVisibleStartNewLine(true);
            _imp->setVisibleMultiLine(false);
            _imp->setVisibleMultiPath(false);
            _imp->setVisibleRichText(false);
            _imp->setVisibleSequence(false);
            _imp->setVisibleGrpAsTab(false);
            _imp->setVisibleParent(true);
            _imp->setVisibleDefaultValues(false,AddKnobDialogPrivate::eDefaultValueTypeInt,1);
            break;
        default:
            break;
    }
}

AddKnobDialog::~AddKnobDialog()
{
    
}

boost::shared_ptr<KnobI>
AddKnobDialog::getKnob() const
{
    return _imp->knob;
}

void
AddKnobDialogPrivate::createKnobFromSelection(int index,int optionalGroupIndex)
{
    assert(!knob);
    std::string label = labelLineEdit->text().toStdString();
    
    switch (index) {
        case 0:
        case 1:
        case 2: {
            //int
            int dim = index + 1;
            boost::shared_ptr<KnobInt> k = Natron::createKnob<KnobInt>(panel->getHolder(), label, dim, false);
            std::vector<int> mins(dim),dmins(dim);
            std::vector<int> maxs(dim),dmaxs(dim);
            
            for (int i = 0; i < dim; ++i) {
                mins[i] = std::floor(minBox->value() + 0.5);
                dmins[i] = std::floor(dminBox->value() + 0.5);
                maxs[i] = std::floor(maxBox->value() + 0.5);
                dmaxs[i] = std::floor(dmaxBox->value() + 0.5);
            }
            k->setMinimumsAndMaximums(mins, maxs);
            k->setDisplayMinimumsAndMaximums(dmins, dmaxs);
            std::vector<int> defValues;
            if (dim >= 1) {
                defValues.push_back(default0->value());
            }
            if (dim >= 2) {
                defValues.push_back(default1->value());
            }
            if (dim >= 3) {
                defValues.push_back(default2->value());
            }
            for (U32 i = 0; i < defValues.size(); ++i) {
                k->setDefaultValue(defValues[i],i);
            }
            knob = k;
        } break;
        case 3:
        case 4:
        case 5: {
            //double
            int dim = index - 2;
            boost::shared_ptr<KnobDouble> k = Natron::createKnob<KnobDouble>(panel->getHolder(), label, dim, false);
            std::vector<double> mins(dim),dmins(dim);
            std::vector<double> maxs(dim),dmaxs(dim);
            for (int i = 0; i < dim; ++i) {
                mins[i] = minBox->value();
                dmins[i] = dminBox->value();
                maxs[i] = maxBox->value();
                dmaxs[i] = dmaxBox->value();
            }
            k->setMinimumsAndMaximums(mins, maxs);
            k->setDisplayMinimumsAndMaximums(dmins, dmaxs);
            std::vector<double> defValues;
            if (dim >= 1) {
                defValues.push_back(default0->value());
            }
            if (dim >= 2) {
                defValues.push_back(default1->value());
            }
            if (dim >= 3) {
                defValues.push_back(default2->value());
            }
            for (U32 i = 0; i < defValues.size(); ++i) {
                k->setDefaultValue(defValues[i],i);
            }

            
            knob = k;
        } break;
        case 6:
        case 7: {
            // color
            int dim = index - 3;
            boost::shared_ptr<KnobColor> k = Natron::createKnob<KnobColor>(panel->getHolder(), label, dim, false);
            std::vector<double> mins(dim),dmins(dim);
            std::vector<double> maxs(dim),dmaxs(dim);
            for (int i = 0; i < dim; ++i) {
                mins[i] = minBox->value();
                dmins[i] = dminBox->value();
                maxs[i] = maxBox->value();
                dmaxs[i] = dmaxBox->value();
            }
            std::vector<double> defValues;
            if (dim >= 1) {
                defValues.push_back(default0->value());
            }
            if (dim >= 2) {
                defValues.push_back(default1->value());
            }
            if (dim >= 3) {
                defValues.push_back(default2->value());
            }
            if (dim >= 4) {
                defValues.push_back(default3->value());
            }
            for (U32 i = 0; i < defValues.size(); ++i) {
                k->setDefaultValue(defValues[i],i);
            }

            k->setMinimumsAndMaximums(mins, maxs);
            k->setDisplayMinimumsAndMaximums(dmins, dmaxs);
            knob = k;
        }  break;
        case 8: {

            boost::shared_ptr<KnobChoice> k = Natron::createKnob<KnobChoice>(panel->getHolder(), label, 1, false);
            QString entriesRaw = menuItemsEdit->toPlainText();
            QTextStream stream(&entriesRaw);
            std::vector<std::string> entries,helps;

            while (!stream.atEnd()) {
                QString line = stream.readLine();
                int foundHelp = line.indexOf("<?>");
                if (foundHelp != -1) {
                    QString entry = line.mid(0,foundHelp);
                    QString help = line.mid(foundHelp + 3,-1);
                    for (int i = 0; i < (int)entries.size() - (int)helps.size(); ++i) {
                        helps.push_back("");
                    }
                    entries.push_back(entry.toStdString());
                    helps.push_back(help.toStdString());
                } else {
                    entries.push_back(line.toStdString());
                    if (!helps.empty()) {
                        helps.push_back("");
                    }
                }
            }
            k->populateChoices(entries,helps);
            
            int defValue = default0->value();
            if (defValue < (int)entries.size() && defValue >= 0) {
                k->setDefaultValue(defValue);
            }
            
            knob = k;
        } break;
        case 9: {
            boost::shared_ptr<KnobBool> k = Natron::createKnob<KnobBool>(panel->getHolder(), label, 1, false);
            bool defValue = defaultBool->isChecked();
            k->setDefaultValue(defValue);
            knob = k;
        }   break;
        case 10:
        case 11: {
            boost::shared_ptr<KnobString> k = Natron::createKnob<KnobString>(panel->getHolder(), label, 1, false);
            if (multiLine->isChecked()) {
                k->setAsMultiLine();
                if (richText->isChecked()) {
                    k->setUsesRichText(true);
                }
            } else {
                if (index == 10) {
                    k->setAsLabel();
                }
            }
            std::string defValue = defaultStr->text().toStdString();
            k->setDefaultValue(defValue);
            knob = k;
        }   break;
        case 12: {
            boost::shared_ptr<KnobFile> k = Natron::createKnob<KnobFile>(panel->getHolder(), label, 1, false);
            if (sequenceDialog->isChecked()) {
                k->setAsInputImage();
            }
            std::string defValue = defaultStr->text().toStdString();
            k->setDefaultValue(defValue);
            knob = k;
        } break;
        case 13: {
            boost::shared_ptr<KnobOutputFile> k = Natron::createKnob<KnobOutputFile>(panel->getHolder(), label, 1, false);
            if (sequenceDialog->isChecked()) {
                k->setAsOutputImageFile();
            }
            std::string defValue = defaultStr->text().toStdString();
            k->setDefaultValue(defValue);
            knob = k;
        } break;
        case 14: {
            boost::shared_ptr<KnobPath> k = Natron::createKnob<KnobPath>(panel->getHolder(), label, 1, false);
            if (multiPath->isChecked()) {
                k->setMultiPath(true);
            }
            std::string defValue = defaultStr->text().toStdString();
            k->setDefaultValue(defValue);
            knob = k;
        } break;
        case 15: {
            boost::shared_ptr<KnobGroup> k = Natron::createKnob<KnobGroup>(panel->getHolder(), label, 1, false);
            if (groupAsTab->isChecked()) {
                k->setAsTab();
            }
            k->setDefaultValue(true); //< default to opened
            knob = k;
        } break;
        case 16: {
            boost::shared_ptr<KnobPage> k = Natron::createKnob<KnobPage>(panel->getHolder(), label, 1, false);
            knob = k;
        } break;
        case 17: {
            boost::shared_ptr<KnobButton> k = Natron::createKnob<KnobButton>(panel->getHolder(), label, 1, false);
            knob = k;
        } break;
        default:
            break;
    }
    
    
    assert(knob);
    knob->setAsUserKnob();
    if (knob->canAnimate()) {
        knob->setAnimationEnabled(animatesCheckbox->isChecked());
    }
    knob->setEvaluateOnChange(evaluatesOnChange->isChecked());
    
    
    
    knob->setSecretByDefault(hideBox->isChecked());
    knob->setName(nameLineEdit->text().toStdString());
    knob->setHintToolTip(tooltipArea->toPlainText().toStdString());
    bool addedInGrp = false;
    KnobGroup* selectedGrp = getSelectedGroup();
    if (selectedGrp) {
        if (optionalGroupIndex != -1) {
            selectedGrp->insertKnob(optionalGroupIndex, knob);
        } else {
            selectedGrp->addKnob(knob);
        }
        addedInGrp = true;
    }
    
    
    if (index != 16 && parentPage && !addedInGrp) {
        std::string selectedItem = parentPage->getCurrentIndexText().toStdString();
        if (selectedItem == NATRON_USER_MANAGED_KNOBS_PAGE) {
            boost::shared_ptr<KnobPage> userPage = panel->getUserPageKnob();
            userPage->addKnob(knob);
            panel->setUserPageActiveIndex();
        } else {
            for (std::list<KnobPage*>::iterator it = userPages.begin(); it != userPages.end(); ++it) {
                if ((*it)->getName() == selectedItem) {
                    (*it)->addKnob(knob);
                    break;
                }
            }

        }
    }
    
    
    KnobHolder* holder = panel->getHolder();
    assert(holder);
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(holder);
    assert(isEffect);
    if (isEffect) {
        isEffect->getNode()->declarePythonFields();
    }
}

KnobGroup*
AddKnobDialogPrivate::getSelectedGroup() const
{
    if (parentGroup) {
        std::string selectedItem = parentGroup->getCurrentIndexText().toStdString();
        if (selectedItem != "-") {
            for (std::list<KnobGroup*>::const_iterator it = userGroups.begin(); it != userGroups.end(); ++it) {
                if ((*it)->getName() == selectedItem) {
                    return *it;
                }
            }
        }
    }
    return 0;
}

void
AddKnobDialog::onOkClicked()
{
    QString name = _imp->nameLineEdit->text();
    bool badFormat = false;
    if (name.isEmpty()) {
        badFormat = true;
    }
    if (!badFormat && !name[0].isLetter()) {
        badFormat = true;
    }
    
    if (!badFormat) {
        //make sure everything is alphaNumeric without spaces
        for (int i = 0; i < name.size(); ++i) {
            if (name[i] == QChar('_')) {
                continue;
            }
            if (name[i] == QChar(' ') || !name[i].isLetterOrNumber()) {
                badFormat = true;
                break;
            }
        }
    }
    
    //check if another knob has the same script name
    std::string stdName = name.toStdString();
    const std::vector<boost::shared_ptr<KnobI> >& knobs = _imp->panel->getHolder()->getKnobs();
    for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it = knobs.begin() ; it != knobs.end(); ++it) {
        if ((*it)->getName() == stdName && (*it) != _imp->knob) {
            badFormat = true;
            break;
        }
    }
    
    if (badFormat) {
        Natron::errorDialog(tr("Error").toStdString(), tr("A parameter must have a unique script name composed only of characters from "
                                                          "[a - z / A- Z] and digits [0 - 9]. This name cannot contain spaces for scripting purposes.")
                            .toStdString());
        return;
        
    }
    
    Natron::EffectInstance* effect = 0;
    
    
    {
        KnobHolder* holder = _imp->panel->getHolder();
        assert(holder);
        
        NodeGroup* isHolderGroup = dynamic_cast<NodeGroup*>(holder);
        if (isHolderGroup) {
            //Check if the group has a node with the exact same script name as the param script name, in which case we error
            //otherwise the attribute on the python object would be overwritten
            NodeList nodes = isHolderGroup->getNodes();
            for (NodeList::iterator it = nodes.begin(); it!=nodes.end(); ++it) {
                if ((*it)->getScriptName() == stdName) {
                    Natron::errorDialog(tr("Error").toStdString(), tr("A parameter on a group cannot have the same script-name as a node within "
                                                                      "the group for scripting purposes.")
                                        .toStdString());
                    return;

                }
            }
        }
    }
    
    ///Remove the previous knob, and recreate it.
    
    ///Index of the type in the combo
    int index;
    
    ///Remember the old page in which to insert the knob
    KnobPage* oldParentPage = 0;
    
    ///If the knob was in a group, we need to place it at the same index
    int oldIndexInGroup = -1;
    
    std::string oldKnobScriptName;
    std::vector<std::pair<std::string,bool> > expressions;
    std::map<boost::shared_ptr<KnobI>,std::vector<std::pair<std::string,bool> > > listenersExpressions;
    

    
    if (!_imp->knob) {
        assert(_imp->typeChoice);
        index = _imp->typeChoice->activeIndex();
    } else {
        oldKnobScriptName = _imp->knob->getName();
        effect = dynamic_cast<Natron::EffectInstance*>(_imp->knob->getHolder());
        oldParentPage = _imp->knob->getTopLevelPage();
        index = getChoiceIndexFromKnobType(_imp->knob.get());
        boost::shared_ptr<KnobI> parent = _imp->knob->getParentKnob();
        KnobGroup* isParentGrp = dynamic_cast<KnobGroup*>(parent.get());
        if (isParentGrp && isParentGrp == _imp->getSelectedGroup()) {
            std::vector<boost::shared_ptr<KnobI> > children = isParentGrp->getChildren();
            for (U32 i = 0; i < children.size(); ++i) {
                if (children[i] == _imp->knob) {
                    oldIndexInGroup = i;
                    break;
                }
            }
        } else {
            std::vector<boost::shared_ptr<KnobI> > children;
            if (oldParentPage) {
                children = oldParentPage->getChildren();
            }
            for (U32 i = 0; i < children.size(); ++i) {
                if (children[i] == _imp->knob) {
                    oldIndexInGroup = i;
                    break;
                }
            }
        }
        expressions.resize(_imp->knob->getDimension());
        for (std::size_t i = 0 ; i < expressions.size(); ++i) {
            std::string expr = _imp->knob->getExpression(i);
            bool useRetVar = _imp->knob->isExpressionUsingRetVariable(i);
            expressions[i] = std::make_pair(expr,useRetVar);
        }
        
        //Since removing this knob will also remove all expressions from listeners, conserve them and try
        //to recover them afterwards
        std::list<boost::shared_ptr<KnobI> > listeners;
        _imp->knob->getListeners(listeners);
        for (std::list<boost::shared_ptr<KnobI> >::iterator it = listeners.begin(); it != listeners.end(); ++it) {
            std::vector<std::pair<std::string,bool> > exprs;
            for (int i = 0; i < (*it)->getDimension(); ++i) {
                std::pair<std::string,bool> e;
                e.first = (*it)->getExpression(i);
                e.second = (*it)->isExpressionUsingRetVariable(i);
                exprs.push_back(e);
            }
            listenersExpressions[*it] = exprs;
        }
        
        _imp->panel->getHolder()->removeDynamicKnob(_imp->knob.get());

        if (!_imp->isKnobAlias) {
            
            _imp->knob.reset();
        }
    } //if (!_imp->knob) {
    
    
    
    if (!_imp->isKnobAlias) {
        _imp->createKnobFromSelection(index, oldIndexInGroup);
        assert(_imp->knob);
        
        if (oldParentPage && !_imp->knob->getParentKnob()) {
            if (oldIndexInGroup == -1) {
                oldParentPage->addKnob(_imp->knob);
            } else {
                oldParentPage->insertKnob(oldIndexInGroup, _imp->knob);
            }
        }
        
        //If startsNewLine is false, set the flag on the previous knob
        bool startNewLine = _imp->startNewLineBox->isChecked();
        boost::shared_ptr<KnobI> parentKnob = _imp->knob->getParentKnob();
        if (parentKnob) {
            KnobGroup* parentIsGrp = dynamic_cast<KnobGroup*>(parentKnob.get());
            KnobPage* parentIsPage = dynamic_cast<KnobPage*>(parentKnob.get());
            assert(parentIsGrp || parentIsPage);
            std::vector<boost::shared_ptr<KnobI> > children;
            if (parentIsGrp) {
                children = parentIsGrp->getChildren();
            } else if (parentIsPage) {
                children = parentIsPage->getChildren();
            }
            for (U32 i = 0; i < children.size(); ++i) {
                if (children[i] == _imp->knob) {
                    if (i > 0) {
                        children[i - 1]->setAddNewLine(startNewLine);
                    }
                    break;
                }
            }
        }
        
        
        if (_imp->originalKnobSerialization) {
            _imp->knob->clone(_imp->originalKnobSerialization->getKnob().get());
        }
        
        //Recover expressions
        try {
            for (std::size_t i = 0 ; i < expressions.size(); ++i) {
                if (!expressions[i].first.empty()) {
                    _imp->knob->setExpression(i, expressions[i].first, expressions[i].second);
                }
            }
        } catch (...) {
            
        }
    } // if (!_imp->isKnobAlias) {
    else {
        //Alias knobs can only have these properties changed
        assert(effect);
        _imp->knob = _imp->isKnobAlias->createDuplicateOnNode(effect, true, stdName, _imp->labelLineEdit->text().toStdString(), _imp->tooltipArea->toPlainText().toStdString(), _imp->startNewLineBox->isChecked());
    }
    
    //Recover listeners expressions
    for (std::map<boost::shared_ptr<KnobI>,std::vector<std::pair<std::string,bool> > >::iterator it = listenersExpressions.begin();it != listenersExpressions.end(); ++it) {
        assert(it->first->getDimension() == (int)it->second.size());
        for (int i = 0; i < it->first->getDimension(); ++i) {
            try {
                std::string expr;
                if (oldKnobScriptName != _imp->knob->getName()) {
                    //Change in expressions the script-name
                    QString estr(it->second[i].first.c_str());
                    estr.replace(oldKnobScriptName.c_str(), _imp->knob->getName().c_str());
                    expr = estr.toStdString();
                } else {
                    expr = it->second[i].first;
                }
                it->first->setExpression(i, expr, it->second[i].second);
            } catch (...) {
                
            }
        }
    }
    
    
    accept();
}

void
AddKnobDialogPrivate::setVisibleMinMax(bool visible)
{
    minLabel->setVisible(visible);
    minBox->setVisible(visible);
    maxLabel->setVisible(visible);
    maxBox->setVisible(visible);
    dminLabel->setVisible(visible);
    dminBox->setVisible(visible);
    dmaxLabel->setVisible(visible);
    dmaxBox->setVisible(visible);
    if (typeChoice) {
        int type = typeChoice->activeIndex();
        
        if (type == 6 || type == 7) {
            // color range to 0-1
            minBox->setValue(INT_MIN);
            maxBox->setValue(INT_MAX);
            dminBox->setValue(0.);
            dmaxBox->setValue(1.);
        } else {
            minBox->setValue(INT_MIN);
            maxBox->setValue(INT_MAX);
            dminBox->setValue(0);
            dmaxBox->setValue(100);
        }
    }
}

void
AddKnobDialogPrivate::setVisibleMenuItems(bool visible)
{
    menuItemsLabel->setVisible(visible);
    menuItemsEdit->setVisible(visible);
}

void
AddKnobDialogPrivate::setVisibleAnimates(bool visible)
{
    animatesLabel->setVisible(visible);
    animatesCheckbox->setVisible(visible);
    if (!knob) {
        animatesCheckbox->setChecked(visible);
    }
}

void
AddKnobDialogPrivate::setVisibleEvaluate(bool visible)
{
    evaluatesLabel->setVisible(visible);
    evaluatesOnChange->setVisible(visible);
    if (!knob) {
        evaluatesOnChange->setChecked(visible);
    }
}

void
AddKnobDialogPrivate::setVisibleStartNewLine(bool visible)
{
    startNewLineLabel->setVisible(visible);
    startNewLineBox->setVisible(visible);
    if (!knob) {
        startNewLineBox->setChecked(true);
    }
}

void
AddKnobDialogPrivate::setVisibleHide(bool visible)
{
    hideLabel->setVisible(visible);
    hideBox->setVisible(visible);
    if (!knob) {
        hideBox->setChecked(false);
    }
}

void
AddKnobDialogPrivate::setVisibleMultiLine(bool visible)
{
    multiLineLabel->setVisible(visible);
    multiLine->setVisible(visible);
    if (!knob) {
        multiLine->setChecked(false);
    }
}

void
AddKnobDialogPrivate::setVisibleRichText(bool visible)
{
    richTextLabel->setVisible(visible);
    richText->setVisible(visible);
    if (!knob) {
        richText->setChecked(false);
    }
}

void
AddKnobDialogPrivate::setVisibleSequence(bool visible)
{
    sequenceDialogLabel->setVisible(visible);
    sequenceDialog->setVisible(visible);
    if (!knob) {
        sequenceDialog->setChecked(false);
    }
}

void
AddKnobDialogPrivate::setVisibleMultiPath(bool visible)
{
    multiPathLabel->setVisible(visible);
    multiPath->setVisible(visible);
    if (!knob) {
        multiPath->setChecked(false);
    }
}

void
AddKnobDialogPrivate::setVisibleGrpAsTab(bool visible)
{
    groupAsTabLabel->setVisible(visible);
    groupAsTab->setVisible(visible);
    if (!knob) {
        groupAsTab->setChecked(false);
    }
}

void
AddKnobDialogPrivate::setVisibleParent(bool visible)
{
    if (!userGroups.empty()) {
        assert(parentGroup);
        parentGroup->setVisible(visible);
        parentGroupLabel->setVisible(visible);
    }
}

void
AddKnobDialogPrivate::setVisiblePage(bool visible)
{
    if (parentPage) {
        parentPage->setVisible(visible);
        parentPageLabel->setVisible(visible);
    }
}

void
AddKnobDialogPrivate::setVisibleDefaultValues(bool visible,AddKnobDialogPrivate::DefaultValueType type,int dimensions)
{
    if (!visible) {
        defaultStr->setVisible(false);
        default0->setVisible(false);
        default1->setVisible(false);
        default2->setVisible(false);
        default3->setVisible(false);
        defaultValueLabel->setVisible(false);
        defaultBool->setVisible(false);
    } else {
        defaultValueLabel->setVisible(true);

        if (type == eDefaultValueTypeInt || type == eDefaultValueTypeDouble) {
            defaultStr->setVisible(false);
            defaultBool->setVisible(false);
            if (dimensions == 1) {
                default0->setVisible(true);
                default1->setVisible(false);
                default2->setVisible(false);
                default3->setVisible(false);
            } else if (dimensions == 2) {
                default0->setVisible(true);
                default1->setVisible(true);
                default2->setVisible(false);
                default3->setVisible(false);
            } else if (dimensions == 3) {
                default0->setVisible(true);
                default1->setVisible(true);
                default2->setVisible(true);
                default3->setVisible(false);
            } else if (dimensions == 4) {
                default0->setVisible(true);
                default1->setVisible(true);
                default2->setVisible(true);
                default3->setVisible(true);
            } else {
                assert(false);
            }
            if (type == eDefaultValueTypeDouble) {
                default0->setType(SpinBox::eSpinBoxTypeDouble);
                default1->setType(SpinBox::eSpinBoxTypeDouble);
                default2->setType(SpinBox::eSpinBoxTypeDouble);
                default3->setType(SpinBox::eSpinBoxTypeDouble);
            } else {
                default0->setType(SpinBox::eSpinBoxTypeInt);
                default1->setType(SpinBox::eSpinBoxTypeInt);
                default2->setType(SpinBox::eSpinBoxTypeInt);
                default3->setType(SpinBox::eSpinBoxTypeInt);
            }
        } else if (type == eDefaultValueTypeString) {
            defaultStr->setVisible(true);
            default0->setVisible(false);
            default1->setVisible(false);
            default2->setVisible(false);
            default3->setVisible(false);
            defaultBool->setVisible(false);
        } else {
            defaultStr->setVisible(false);
            default0->setVisible(false);
            default1->setVisible(false);
            default2->setVisible(false);
            default3->setVisible(false);
            defaultBool->setVisible(true);
        }
    }
}

