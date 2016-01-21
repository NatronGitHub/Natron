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

#include "AddKnobDialog.h"

#include <cfloat> // floor

#include <QCheckBox>
#include <QTextEdit>
#include <QtCore/QTextStream>
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


NATRON_NAMESPACE_ENTER;


struct AddKnobDialogPrivate
{
    boost::shared_ptr<KnobI> knob;
    boost::shared_ptr<KnobSerialization> originalKnobSerialization;
    boost::shared_ptr<KnobI> isKnobAlias;
    
    DockablePanel* panel;
    
    QVBoxLayout* vLayout;
    
    QWidget* mainContainer;
    QFormLayout* mainLayout;
    
    Label* typeLabel;
    ComboBox* typeChoice;
    Label* nameLabel;
    LineEdit* nameLineEdit;

    
    Label* labelLabel;
    LineEdit* labelLineEdit;
    
    Label* hideLabel;
    QCheckBox* hideBox;
    
    Label* startNewLineLabel;
    QCheckBox* startNewLineBox;
    
    Label* animatesLabel;
    QCheckBox* animatesCheckbox;
    
    Label* evaluatesLabel;
    QCheckBox* evaluatesOnChange;
    
    Label* tooltipLabel;
    QTextEdit* tooltipArea;
    
    Label* minLabel;
    SpinBox* minBox;
    
    Label* maxLabel;
    SpinBox* maxBox;
    
    Label* dminLabel;
    SpinBox* dminBox;
    
    Label* dmaxLabel;
    SpinBox* dmaxBox;
    
    enum DefaultValueType
    {
        eDefaultValueTypeInt,
        eDefaultValueTypeDouble,
        eDefaultValueTypeBool,
        eDefaultValueTypeString
    };
    
    
    Label* defaultValueLabel;
    SpinBox* default0;
    SpinBox* default1;
    SpinBox* default2;
    SpinBox* default3;
    LineEdit* defaultStr;
    QCheckBox* defaultBool;
    
    Label* menuItemsLabel;
    QTextEdit* menuItemsEdit;
    
    Label* multiLineLabel;
    QCheckBox* multiLine;
    
    Label* richTextLabel;
    QCheckBox* richText;
    
    Label* sequenceDialogLabel;
    QCheckBox* sequenceDialog;
    
    Label* multiPathLabel;
    QCheckBox* multiPath;
    
    Label* groupAsTabLabel;
    QCheckBox* groupAsTab;
    
    Label* parentGroupLabel;
    ComboBox* parentGroup;
    
    Label* parentPageLabel;
    ComboBox* parentPage;
    
    std::list<KnobGroup*> userGroups;
    std::list<boost::shared_ptr<KnobPage> > userPages; //< all user pages except the "User" one
    
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
    void setVisibleLabel(bool visible);
    
    void setVisibleToolTipEdit(bool visible);
    
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
    
    template <typename T>
    void setKnobMinMax(KnobI* knob);
    
    boost::shared_ptr<KnobPage> getSelectedPage() const;
};

enum ParamDataTypeEnum {
    eParamDataTypeInteger, // 0
    eParamDataTypeInteger2D, // 1
    eParamDataTypeInteger3D, // 2
    eParamDataTypeFloatingPoint, //3
    eParamDataTypeFloatingPoint2D, // 4
    eParamDataTypeFloatingPoint3D, // 5
    eParamDataTypeColorRGB, // 6
    eParamDataTypeColorRGBA, // 7
    eParamDataTypeChoice, // 8
    eParamDataTypeCheckbox, // 9
    eParamDataTypeLabel, // 10
    eParamDataTypeTextInput, // 11
    eParamDataTypeInputFile, // 12
    eParamDataTypeOutputFile, // 13
    eParamDataTypeDirectory, // 14
    eParamDataTypeGroup, // 15
    eParamDataTypePage, // 16
    eParamDataTypeButton, // 17
    eParamDataTypeSeparator, // 18
    eParamDataTypeCount // 19
};

static const char* dataTypeString(ParamDataTypeEnum t)
{
    switch (t) {
        case eParamDataTypeInteger:
            return "Integer";
        case eParamDataTypeInteger2D:
            return "Integer 2D";
        case eParamDataTypeInteger3D:
            return "Integer 3D";
        case eParamDataTypeFloatingPoint:
            return "Floating Point";
        case eParamDataTypeFloatingPoint2D:
            return "Floating Point 2D";
        case eParamDataTypeFloatingPoint3D:
            return "Floating Point 3D";
        case eParamDataTypeColorRGB:
            return "Color RGB";
        case eParamDataTypeColorRGBA:
            return "Color RGBA";
        case eParamDataTypeChoice:
            return "Choice (Pulldown)";
        case eParamDataTypeCheckbox:
            return "Checkbox";
        case eParamDataTypeLabel:
            return "Label";
        case eParamDataTypeTextInput:
            return "Text Input";
        case eParamDataTypeInputFile:
            return "Input File";
        case eParamDataTypeOutputFile:
            return "Output File";
        case eParamDataTypeDirectory:
            return "Directory";
        case eParamDataTypeGroup:
            return "Group";
        case eParamDataTypePage:
            return "Page";
        case eParamDataTypeButton:
            return "Button";
        case eParamDataTypeSeparator:
            return "Separator";
        default:
            return NULL;
    }
}

static int dataTypeDim(ParamDataTypeEnum t)
{
    switch (t) {
        case eParamDataTypeInteger:
            return 1;
        case eParamDataTypeInteger2D:
            return 2;
        case eParamDataTypeInteger3D:
            return 3;
        case eParamDataTypeFloatingPoint:
            return 1;
        case eParamDataTypeFloatingPoint2D:
            return 2;
        case eParamDataTypeFloatingPoint3D:
            return 3;
        case eParamDataTypeColorRGB:
            return 3;
        case eParamDataTypeColorRGBA:
            return 4;
        case eParamDataTypeChoice:
        case eParamDataTypeCheckbox:
        case eParamDataTypeLabel:
        case eParamDataTypeTextInput:
        case eParamDataTypeInputFile:
        case eParamDataTypeOutputFile:
        case eParamDataTypeDirectory:
        case eParamDataTypeGroup:
        case eParamDataTypePage:
        case eParamDataTypeButton:
        case eParamDataTypeSeparator:
        default:
            return 1;
    }
}

static ParamDataTypeEnum getChoiceIndexFromKnobType(KnobI* knob)
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
    KnobSeparator* isSep = dynamic_cast<KnobSeparator*>(knob);
    if (isInt) {
        if (dim == 1) {
            return eParamDataTypeInteger;
        } else if (dim == 2) {
            return eParamDataTypeInteger2D;
        } else if (dim == 3) {
            return eParamDataTypeInteger3D;
        }
    } else if (isDbl) {
        if (dim == 1) {
            return eParamDataTypeFloatingPoint;
        } else if (dim == 2) {
            return eParamDataTypeFloatingPoint2D;
        } else if (dim == 3) {
            return eParamDataTypeFloatingPoint3D;
        }
    } else if (isColor) {
        if (dim == 3) {
            return eParamDataTypeColorRGB;
        } else if (dim == 4) {
            return eParamDataTypeColorRGBA;
        }
    } else if (isChoice) {
        return eParamDataTypeChoice;
    } else if (isBool) {
        return eParamDataTypeCheckbox;
    } else if (isStr) {
        if (isStr->isLabel()) {
            return eParamDataTypeLabel;
        } else  {
            return eParamDataTypeTextInput;
        }
    } else if (isFile) {
        return eParamDataTypeInputFile;
    } else if (isOutputFile) {
        return eParamDataTypeOutputFile;
    } else if (isPath) {
        return eParamDataTypeDirectory;
    } else if (isGrp) {
        return eParamDataTypeGroup;
    } else if (isPage) {
        return eParamDataTypePage;
    } else if (isBtn) {
        return eParamDataTypeButton;
    } else if (isSep) {
        return eParamDataTypeSeparator;
    }
    return eParamDataTypeCount;
}

AddKnobDialog::AddKnobDialog(DockablePanel* panel,
                             const boost::shared_ptr<KnobI>& knob,
                             const std::string& selectedPageName,
                             const std::string& selectedGroupName,
                             QWidget* parent)
: QDialog(parent)
, _imp(new AddKnobDialogPrivate(panel))
{
    
    _imp->knob = knob;
    assert(!knob || knob->isUserKnob());

    {
        EffectInstance* effect = dynamic_cast<EffectInstance*>(panel->getHolder());
        QString title = "Add Parameter";
        if (!knob) {
            // Add...
            if (effect) {
                title += " to ";
                title += effect->getScriptName().c_str();
            }
        } else {
            // Edit...
            title = "Edit Parameter ";
            if (effect) {
                title += effect->getScriptName().c_str();
                title += '.';
            }
            title += knob->getName().c_str();
        }
        setWindowTitle(title);
    }

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
            KnobI::ListenerDimsMap listeners;
            knob->getListeners(listeners);
            if (!listeners.empty()) {
                listener = listeners.begin()->first.lock();
                if (listener) {
                    isAlias = listener->getAliasMaster();
                }
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
        
        _imp->nameLabel = new Label(tr("Script name:"),this);
        _imp->nameLineEdit = new LineEdit(firstRowContainer);
        _imp->nameLineEdit->setToolTip(GuiUtils::convertFromPlainText(tr("The name of the parameter as it will be used in Python scripts"), Qt::WhiteSpaceNormal));
        
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
        _imp->labelLabel = new Label(tr("Label:"),secondRowContainer);
        _imp->labelLineEdit = new LineEdit(secondRowContainer);
        _imp->labelLineEdit->setToolTip(GuiUtils::convertFromPlainText(tr("The label of the parameter as displayed on the graphical user interface"), Qt::WhiteSpaceNormal));
        if (knob) {
            _imp->labelLineEdit->setText(knob->getLabel().c_str());
        }
        secondRowLayout->addWidget(_imp->labelLineEdit);
        _imp->hideLabel = new Label(tr("Hide:"),secondRowContainer);
        secondRowLayout->addWidget(_imp->hideLabel);
        _imp->hideBox = new QCheckBox(secondRowContainer);
        _imp->hideBox->setToolTip(GuiUtils::convertFromPlainText(tr("If checked the parameter will not be visible on the user interface"), Qt::WhiteSpaceNormal));
        if (knob) {
            _imp->hideBox->setChecked(knob->getIsSecret());
        }

        secondRowLayout->addWidget(_imp->hideBox);
        _imp->startNewLineLabel = new Label(tr("Start new line:"),secondRowContainer);
        secondRowLayout->addWidget(_imp->startNewLineLabel);
        _imp->startNewLineBox = new QCheckBox(secondRowContainer);
        _imp->startNewLineBox->setToolTip(GuiUtils::convertFromPlainText(tr("If unchecked the parameter will be on the same line as the previous parameter"), Qt::WhiteSpaceNormal));
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
            _imp->typeLabel = new Label(tr("Type:"),thirdRowContainer);
            _imp->typeChoice = new ComboBox(thirdRowContainer);
            _imp->typeChoice->setToolTip(GuiUtils::convertFromPlainText(tr("The data type of the parameter."), Qt::WhiteSpaceNormal));
            for (int i = 0; i < (int)eParamDataTypeCount; ++i) {
                assert(_imp->typeChoice->count() == i);
                _imp->typeChoice->addItem(tr(dataTypeString((ParamDataTypeEnum)i)));
            }
            QObject::connect(_imp->typeChoice, SIGNAL(currentIndexChanged(int)),this, SLOT(onTypeCurrentIndexChanged(int)));
            
            thirdRowLayout->addWidget(_imp->typeChoice);
        }
        _imp->animatesLabel = new Label(tr("Animates:"),thirdRowContainer);

        if (!knob) {
            thirdRowLayout->addWidget(_imp->animatesLabel);
        }
        _imp->animatesCheckbox = new QCheckBox(thirdRowContainer);
        _imp->animatesCheckbox->setToolTip(GuiUtils::convertFromPlainText(tr("When checked this parameter will be able to animate with keyframes."), Qt::WhiteSpaceNormal));
        if (knob) {
            _imp->animatesCheckbox->setChecked(knob->isAnimationEnabled());
        }

        thirdRowLayout->addWidget(_imp->animatesCheckbox);
        _imp->evaluatesLabel = new Label(GuiUtils::convertFromPlainText(tr("Render on change:"), Qt::WhiteSpaceNormal),thirdRowContainer);
        thirdRowLayout->addWidget(_imp->evaluatesLabel);
        _imp->evaluatesOnChange = new QCheckBox(thirdRowContainer);
        _imp->evaluatesOnChange->setToolTip(GuiUtils::convertFromPlainText(tr("If checked, when the value of this parameter changes a new render will be triggered."), Qt::WhiteSpaceNormal));
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
        _imp->tooltipLabel = new Label(tr("Tooltip:"),this);
        _imp->tooltipArea = new QTextEdit(this);
        _imp->tooltipArea->setToolTip(GuiUtils::convertFromPlainText(tr("The help tooltip that will appear when hovering the parameter with the mouse."), Qt::WhiteSpaceNormal));
        _imp->mainLayout->addRow(_imp->tooltipLabel,_imp->tooltipArea);
        if (knob) {
            _imp->tooltipArea->setPlainText(knob->getHintToolTip().c_str());
        }
    }
    {
        _imp->menuItemsLabel = new Label(tr("Menu items:"),this);
        _imp->menuItemsEdit = new QTextEdit(this);
        QString tt = GuiUtils::convertFromPlainText(tr("The entries that will be available in the drop-down menu. \n"
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
        
        _imp->multiLineLabel = new Label(tr("Multi-line:"),optContainer);
        _imp->multiLine = new QCheckBox(optContainer);
        _imp->multiLine->setToolTip(GuiUtils::convertFromPlainText(tr("Should this text be multi-line or single-line ?"), Qt::WhiteSpaceNormal));
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
        
        _imp->richTextLabel = new Label(tr("Rich text:"),optContainer);
        _imp->richText = new QCheckBox(optContainer);
        QString tt = GuiUtils::convertFromPlainText(tr("If checked, the text area will be able to use rich text encoding with a sub-set of html.\n "
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
        
        _imp->sequenceDialogLabel = new Label(tr("Use sequence dialog:"),optContainer);
        _imp->sequenceDialog = new QCheckBox(optContainer);
        _imp->sequenceDialog->setToolTip(GuiUtils::convertFromPlainText(tr("If checked the file dialog for this parameter will be able to decode image sequences."), Qt::WhiteSpaceNormal));
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
        
        _imp->multiPathLabel = new Label(GuiUtils::convertFromPlainText(tr("Multiple paths:"), Qt::WhiteSpaceNormal),optContainer);
        _imp->multiPath = new QCheckBox(optContainer);
        _imp->multiPath->setToolTip(GuiUtils::convertFromPlainText(tr("If checked the parameter will be a table where each entry points to a different path."), Qt::WhiteSpaceNormal));
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
        
        _imp->groupAsTabLabel = new Label(tr("Group as tab:"),optContainer);
        _imp->groupAsTab = new QCheckBox(optContainer);
        _imp->groupAsTab->setToolTip(GuiUtils::convertFromPlainText(tr("If checked the group will be a tab instead."), Qt::WhiteSpaceNormal));
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
        _imp->minLabel = new Label(tr("Minimum:"),minMaxContainer);

        _imp->minBox = new SpinBox(minMaxContainer,SpinBox::eSpinBoxTypeDouble);
        _imp->minBox->setToolTip(GuiUtils::convertFromPlainText(tr("Set the minimum value for the parameter. Even though the user might input "
                                                             "a value higher or lower than the specified min/max range, internally the "
                                                             "real value will be clamped to this interval."), Qt::WhiteSpaceNormal));
        minMaxLayout->addWidget(_imp->minBox);
        
        _imp->maxLabel = new Label(tr("Maximum:"),minMaxContainer);
        _imp->maxBox = new SpinBox(minMaxContainer,SpinBox::eSpinBoxTypeDouble);
        _imp->maxBox->setToolTip(GuiUtils::convertFromPlainText(tr("Set the maximum value for the parameter. Even though the user might input "
                                                             "a value higher or lower than the specified min/max range, internally the "
                                                             "real value will be clamped to this interval."), Qt::WhiteSpaceNormal));
        minMaxLayout->addWidget(_imp->maxLabel);
        minMaxLayout->addWidget(_imp->maxBox);
        minMaxLayout->addStretch();
        
        _imp->dminLabel = new Label(tr("Display Minimum:"),dminMaxContainer);
        _imp->dminBox = new SpinBox(dminMaxContainer,SpinBox::eSpinBoxTypeDouble);
        _imp->dminBox->setToolTip(GuiUtils::convertFromPlainText(tr("Set the display minimum value for the parameter. This is a hint that is typically "
                                                              "used to set the range of the slider."), Qt::WhiteSpaceNormal));
        dminMaxLayout->addWidget(_imp->dminBox);
        
        _imp->dmaxLabel = new Label(tr("Display Maximum:"),dminMaxContainer);
        _imp->dmaxBox = new SpinBox(dminMaxContainer,SpinBox::eSpinBoxTypeDouble);
        _imp->dmaxBox->setToolTip(GuiUtils::convertFromPlainText(tr("Set the display maximum value for the parameter. This is a hint that is typically "
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
        _imp->defaultValueLabel = new Label(tr("Default value:"),defValContainer);

        _imp->default0 = new SpinBox(defValContainer,SpinBox::eSpinBoxTypeDouble);
        _imp->default0->setValue(0);
        _imp->default0->setToolTip(GuiUtils::convertFromPlainText(tr("Set the default value for the parameter (dimension 0)."), Qt::WhiteSpaceNormal));
        defValLayout->addWidget(_imp->default0);
        
        _imp->default1 = new SpinBox(defValContainer,SpinBox::eSpinBoxTypeDouble);
        _imp->default1->setValue(0);
        _imp->default1->setToolTip(GuiUtils::convertFromPlainText(tr("Set the default value for the parameter (dimension 1)."), Qt::WhiteSpaceNormal));
        defValLayout->addWidget(_imp->default1);
        
        _imp->default2 = new SpinBox(defValContainer,SpinBox::eSpinBoxTypeDouble);
        _imp->default2->setValue(0);
        _imp->default2->setToolTip(GuiUtils::convertFromPlainText(tr("Set the default value for the parameter (dimension 2)."), Qt::WhiteSpaceNormal));
        defValLayout->addWidget(_imp->default2);
        
        _imp->default3 = new SpinBox(defValContainer,SpinBox::eSpinBoxTypeDouble);
        _imp->default3->setValue(0);
        _imp->default3->setToolTip(GuiUtils::convertFromPlainText(tr("Set the default value for the parameter (dimension 3)."), Qt::WhiteSpaceNormal));
        defValLayout->addWidget(_imp->default3);

        
        _imp->defaultStr = new LineEdit(defValContainer);
        _imp->defaultStr->setToolTip(GuiUtils::convertFromPlainText(tr("Set the default value for the parameter."), Qt::WhiteSpaceNormal));
        defValLayout->addWidget(_imp->defaultStr);
        
        
        _imp->defaultBool = new QCheckBox(defValContainer);
        _imp->defaultBool->setToolTip(GuiUtils::convertFromPlainText(tr("Set the default value for the parameter."), Qt::WhiteSpaceNormal));
        _imp->defaultBool->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE,NATRON_MEDIUM_BUTTON_SIZE);
        defValLayout->addWidget(_imp->defaultBool);

        defValLayout->addStretch();
        
        _imp->mainLayout->addRow(_imp->defaultValueLabel, defValContainer);
        
        
        Knob<double>* isDbl = dynamic_cast<Knob<double>*>(knob.get());
        Knob<int>* isInt = dynamic_cast<Knob<int>*>(knob.get());
        KnobBool* isBool = dynamic_cast<KnobBool*>(knob.get());
        Knob<std::string>* isStr = dynamic_cast<Knob<std::string>*>(knob.get());
        KnobChoice* isChoice = dynamic_cast<KnobChoice*>(knob.get());
        
        if (isChoice) {
            _imp->defaultStr->setText(isChoice->getEntry(isChoice->getDefaultValue(0)).c_str());
        } else if (isDbl) {
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
        if (!knob) {
            continue;
        }
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
        
        _imp->parentGroupLabel = new Label(tr("Group:"),optContainer);
        _imp->parentGroup = new ComboBox(optContainer);
        
        _imp->parentGroup->setToolTip(GuiUtils::convertFromPlainText(tr("The name of the group under which this parameter will appear."), Qt::WhiteSpaceNormal));
        optLayout->addWidget(_imp->parentGroup);
        
        _imp->mainLayout->addRow(_imp->parentGroupLabel, optContainer);
    }
    
    QWidget* optContainer = new QWidget(this);
    QHBoxLayout* optLayout = new QHBoxLayout(optContainer);
    optLayout->setContentsMargins(0, 0, 15, 0);
    _imp->parentPageLabel = new Label(tr("Page:"),optContainer);
    _imp->parentPage = new ComboBox(optContainer);
    
    QObject::connect(_imp->parentPage,SIGNAL(currentIndexChanged(int)),this,SLOT(onPageCurrentIndexChanged(int)));
    const std::vector<boost::shared_ptr<KnobI> >& internalKnobs = _imp->panel->getHolder()->getKnobs();
    for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it = internalKnobs.begin() ; it != internalKnobs.end(); ++it) {
        if ((*it)->isUserKnob()) {
            boost::shared_ptr<KnobPage> isPage = boost::dynamic_pointer_cast<KnobPage>(*it);
            if (isPage) {
                _imp->userPages.push_back(isPage);
            }
        }
    }
    if (_imp->userPages.empty()) {
        _imp->parentPage->addItem(NATRON_USER_MANAGED_KNOBS_PAGE);
    }
    
    for (std::list<boost::shared_ptr<KnobPage> >::iterator it = _imp->userPages.begin(); it != _imp->userPages.end(); ++it) {
        _imp->parentPage->addItem((*it)->getName().c_str());
    }
    _imp->parentPage->setToolTip(GuiUtils::convertFromPlainText(tr("The tab under which this parameter will appear."), Qt::WhiteSpaceNormal));
    optLayout->addWidget(_imp->parentPage);
    
    int pageIndexLoaded = -1;
    if (knob) {
        ////find in which page the knob should be
        boost::shared_ptr<KnobPage> isTopLevelParentAPage = knob->getTopLevelPage();
        if (isTopLevelParentAPage && isTopLevelParentAPage->getName() != NATRON_USER_MANAGED_KNOBS_PAGE) {
            int index = 0; // 1 because of the "User" item
            bool found = false;
            for (std::list<boost::shared_ptr<KnobPage> >::iterator it = _imp->userPages.begin(); it != _imp->userPages.end(); ++it, ++index) {
                if (*it == isTopLevelParentAPage) {
                    found = true;
                    break;
                }
            }
            if (found) {
                _imp->parentPage->setCurrentIndex_no_emit(index);
                pageIndexLoaded = index;
            }
        }
    } else {
        ///If the selected page name in the manage user params dialog is valid, set the page accordingly
        if (_imp->parentPage && !selectedPageName.empty()) {
            int index = 0;
            for (std::list<boost::shared_ptr<KnobPage> >::iterator it = _imp->userPages.begin(); it != _imp->userPages.end(); ++it, ++index) {
                if ((*it)->getName() == selectedPageName) {
                    _imp->parentPage->setCurrentIndex_no_emit(index);
                    pageIndexLoaded = index;
                    break;
                }
            }
        }
    }
    
    _imp->mainLayout->addRow(_imp->parentPageLabel, optContainer);

    onPageCurrentIndexChanged(pageIndexLoaded == -1 ? 0 : pageIndexLoaded);
    
    if (_imp->parentGroup && knob) {
        boost::shared_ptr<KnobPage> topLvlPage = knob->getTopLevelPage();
        assert(topLvlPage);
        boost::shared_ptr<KnobI> parent = knob->getParentKnob();
        KnobGroup* isParentGrp = dynamic_cast<KnobGroup*>(parent.get());
        if (isParentGrp) {
            for (std::list<KnobGroup*>::iterator it = _imp->userGroups.begin(); it != _imp->userGroups.end(); ++it) {
                boost::shared_ptr<KnobPage> page = (*it)->getTopLevelPage();
                assert(page);
                
                ///add only grps whose parent page is the selected page
                if (isParentGrp == *it && page == topLvlPage) {
                    for (int i = 0; i < _imp->parentGroup->count(); ++i) {
                        if (_imp->parentGroup->itemText(i) == QString(isParentGrp->getName().c_str())) {
                            _imp->parentGroup->setCurrentIndex(i);
                            break;
                        }
                    }
                    break;
                }
                
            }
        }
    } else {
        ///If the selected group name in the manage user params dialog is valid, set the group accordingly
        if (_imp->parentGroup) {
            for (int i = 0; i < _imp->parentGroup->count(); ++i) {
                if (_imp->parentGroup->itemText(i) == QString(selectedGroupName.c_str())) {
                    _imp->parentGroup->setCurrentIndex(i);
                    break;
                }
            }
        }
    }
    
    
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::StandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel),Qt::Horizontal,this);
    QObject::connect(buttons,SIGNAL(rejected()), this, SLOT(reject()));
    QObject::connect(buttons,SIGNAL(accepted()), this, SLOT(onOkClicked()));
    _imp->vLayout->addWidget(buttons);
    
    ParamDataTypeEnum t;
    if (!knob) {
        t = (ParamDataTypeEnum)_imp->typeChoice->activeIndex();
    } else {
        t = getChoiceIndexFromKnobType(knob.get());
        assert(t != eParamDataTypeCount);
    }
    onTypeCurrentIndexChanged((int)t);
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
    boost::shared_ptr<KnobPage> parentPage ;
    
    if (selectedPage == NATRON_USER_MANAGED_KNOBS_PAGE) {
        parentPage = _imp->panel->getUserPageKnob();
    } else {
        for (std::list<boost::shared_ptr<KnobPage> >::iterator it = _imp->userPages.begin(); it != _imp->userPages.end(); ++it) {
            if ((*it)->getName() == selectedPage) {
                parentPage = *it;
                break;
            }
        }
    }
    
    for (std::list<KnobGroup*>::iterator it = _imp->userGroups.begin(); it != _imp->userGroups.end(); ++it) {
        boost::shared_ptr<KnobPage> page = (*it)->getTopLevelPage();
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
     enum ParamDataTypeEnum t = (ParamDataTypeEnum)index;
    _imp->setVisiblePage(t != eParamDataTypePage);
    _imp->setVisibleLabel(t != eParamDataTypeLabel);
    int d = dataTypeDim(t);
    switch (t) {
        case eParamDataTypeInteger: // int
        case eParamDataTypeInteger2D: // int 2D
        case eParamDataTypeInteger3D: // int 3D
            _imp->setVisibleToolTipEdit(true);
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
            _imp->setVisibleDefaultValues(true, AddKnobDialogPrivate::eDefaultValueTypeInt, d);
            break;

        case eParamDataTypeFloatingPoint: // fp
        case eParamDataTypeFloatingPoint2D: // fp 2D
        case eParamDataTypeFloatingPoint3D: // fp 3D
        case eParamDataTypeColorRGB: // RGB
        case eParamDataTypeColorRGBA: // RGBA
            _imp->setVisibleToolTipEdit(true);
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
            _imp->setVisibleDefaultValues(true, AddKnobDialogPrivate::eDefaultValueTypeDouble, d);
            break;

        case eParamDataTypeChoice: // choice
            _imp->setVisibleToolTipEdit(true);
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
            _imp->setVisibleDefaultValues(true, AddKnobDialogPrivate::eDefaultValueTypeString, d);
            break;
        case eParamDataTypeCheckbox: // bool
            _imp->setVisibleToolTipEdit(true);
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
            _imp->setVisibleDefaultValues(true, AddKnobDialogPrivate::eDefaultValueTypeBool, d);
            break;
        case eParamDataTypeLabel: // label
            _imp->setVisibleToolTipEdit(true);
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
            _imp->setVisibleDefaultValues(true, AddKnobDialogPrivate::eDefaultValueTypeString, d);
            break;
        case eParamDataTypeTextInput: // text input
            _imp->setVisibleToolTipEdit(true);
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
            _imp->setVisibleDefaultValues(true, AddKnobDialogPrivate::eDefaultValueTypeString, d);
            break;
        case eParamDataTypeInputFile: // input file
        case eParamDataTypeOutputFile: // output file
            _imp->setVisibleToolTipEdit(true);
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
            _imp->setVisibleDefaultValues(true, AddKnobDialogPrivate::eDefaultValueTypeString, d);
            break;
        case eParamDataTypeDirectory: // path
            _imp->setVisibleToolTipEdit(true);
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
            _imp->setVisibleDefaultValues(true, AddKnobDialogPrivate::eDefaultValueTypeString, d);
            break;
        case eParamDataTypeGroup: // grp
            _imp->setVisibleToolTipEdit(true);
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
            _imp->setVisibleDefaultValues(false, AddKnobDialogPrivate::eDefaultValueTypeInt, d);
            break;
        case eParamDataTypePage: // page
            _imp->setVisibleToolTipEdit(false);
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
            _imp->setVisibleDefaultValues(false, AddKnobDialogPrivate::eDefaultValueTypeInt, d);
            break;
        case eParamDataTypeButton: // button
            _imp->setVisibleToolTipEdit(true);
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
            _imp->setVisibleDefaultValues(false, AddKnobDialogPrivate::eDefaultValueTypeInt, d);
            break;
        case eParamDataTypeSeparator: // separator
            _imp->setVisibleToolTipEdit(true);
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
            _imp->setVisibleParent(true);
            _imp->setVisibleDefaultValues(false, AddKnobDialogPrivate::eDefaultValueTypeInt, d);
            break;
        default:
            break;
    }
    
    if (_imp->isKnobAlias) {
        _imp->setVisibleToolTipEdit(true);
        _imp->setVisibleAnimates(false);
        _imp->setVisibleEvaluate(false);
        _imp->setVisibleHide(false);
        _imp->setVisibleMenuItems(false);
        //_imp->setVisibleMinMax(false);
        _imp->setVisibleStartNewLine(true);
        _imp->setVisibleMultiLine(false);
        _imp->setVisibleMultiPath(false);
        _imp->setVisibleRichText(false);
        _imp->setVisibleSequence(false);
        _imp->setVisibleGrpAsTab(false);
        _imp->setVisibleParent(true);
       // _imp->setVisibleDefaultValues(false, AddKnobDialogPrivate::eDefaultValueTypeInt, d);
        _imp->setVisiblePage(true);
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

template <typename T>
void
AddKnobDialogPrivate::setKnobMinMax(KnobI* knob)
{
    int dim = knob->getDimension();
    
    Knob<T>* k = dynamic_cast<Knob<T>*>(knob);
    assert(k);
    
    std::vector<T> mins(dim),dmins(dim);
    std::vector<T> maxs(dim),dmaxs(dim);
    for (std::size_t i = 0; i < (std::size_t)dim; ++i) {
        mins[i] = minBox->value();
        dmins[i] = dminBox->value();
        maxs[i] = maxBox->value();
        dmaxs[i] = dmaxBox->value();
    }
    k->setMinimumsAndMaximums(mins, maxs);
    k->setDisplayMinimumsAndMaximums(dmins, dmaxs);
    std::vector<T> defValues;
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

}

void
AddKnobDialogPrivate::createKnobFromSelection(int index, int optionalGroupIndex)
{
    ParamDataTypeEnum t = (ParamDataTypeEnum)index;
    assert(!knob);
    std::string label = labelLineEdit->text().toStdString();
    int dim = dataTypeDim(t);
    switch (t) {
        case eParamDataTypeInteger:
        case eParamDataTypeInteger2D:
        case eParamDataTypeInteger3D: {
            //int
            boost::shared_ptr<KnobInt> k = AppManager::createKnob<KnobInt>(panel->getHolder(), label, dim, false);
            setKnobMinMax<int>(k.get());
            knob = k;
            break;
        }
        case eParamDataTypeFloatingPoint:
        case eParamDataTypeFloatingPoint2D:
        case eParamDataTypeFloatingPoint3D: {
            //double
            int dim = index - 2;
            boost::shared_ptr<KnobDouble> k = AppManager::createKnob<KnobDouble>(panel->getHolder(), label, dim, false);
            setKnobMinMax<double>(k.get());
            knob = k;
            break;
        }
        case eParamDataTypeColorRGB:
        case eParamDataTypeColorRGBA: {
            // color
            int dim = index - 3;
            boost::shared_ptr<KnobColor> k = AppManager::createKnob<KnobColor>(panel->getHolder(), label, dim, false);
            setKnobMinMax<double>(k.get());
            knob = k;
            break;
        }
        case eParamDataTypeChoice: {
            boost::shared_ptr<KnobChoice> k = AppManager::createKnob<KnobChoice>(panel->getHolder(), label, 1, false);
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
            
            std::string defValue = defaultStr->text().toStdString();
            int defIndex = -1;
            for (std::size_t i = 0; i < entries.size(); ++i) {
                if (entries[i] == defValue) {
                    defIndex = i;
                    break;
                }
            }
            if (defIndex == -1) {
                std::stringstream ss;
                ss << '"';
                ss << defValue;
                ss << '"';
                ss << " does not exist in the defined menu items";
                throw std::invalid_argument(ss.str());
            }
            if (defIndex < (int)entries.size() && defIndex >= 0) {
                k->setDefaultValue(defIndex);
            }
            
            knob = k;
            break;
        }
        case eParamDataTypeCheckbox: {
            boost::shared_ptr<KnobBool> k = AppManager::createKnob<KnobBool>(panel->getHolder(), label, 1, false);
            bool defValue = defaultBool->isChecked();
            k->setDefaultValue(defValue);
            knob = k;
            break;
        }
        case eParamDataTypeLabel:
        case eParamDataTypeTextInput: {
            boost::shared_ptr<KnobString> k = AppManager::createKnob<KnobString>(panel->getHolder(), label, 1, false);
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
            break;
        }
        case eParamDataTypeInputFile: {
            boost::shared_ptr<KnobFile> k = AppManager::createKnob<KnobFile>(panel->getHolder(), label, 1, false);
            if (sequenceDialog->isChecked()) {
                k->setAsInputImage();
            }
            std::string defValue = defaultStr->text().toStdString();
            k->setDefaultValue(defValue);
            knob = k;
            break;
        }
        case eParamDataTypeOutputFile: {
            boost::shared_ptr<KnobOutputFile> k = AppManager::createKnob<KnobOutputFile>(panel->getHolder(), label, 1, false);
            if (sequenceDialog->isChecked()) {
                k->setAsOutputImageFile();
            }
            std::string defValue = defaultStr->text().toStdString();
            k->setDefaultValue(defValue);
            knob = k;
            break;
        }
        case eParamDataTypeDirectory: {
            boost::shared_ptr<KnobPath> k = AppManager::createKnob<KnobPath>(panel->getHolder(), label, 1, false);
            if (multiPath->isChecked()) {
                k->setMultiPath(true);
            }
            std::string defValue = defaultStr->text().toStdString();
            k->setDefaultValue(defValue);
            knob = k;
        } break;
        case eParamDataTypeGroup: {
            boost::shared_ptr<KnobGroup> k = AppManager::createKnob<KnobGroup>(panel->getHolder(), label, 1, false);
            if (groupAsTab->isChecked()) {
                k->setAsTab();
            }
            k->setDefaultValue(true); //< default to opened
            knob = k;
        } break;
        case eParamDataTypePage: {
            boost::shared_ptr<KnobPage> k = AppManager::createKnob<KnobPage>(panel->getHolder(), label, 1, false);
            knob = k;
        } break;
        case eParamDataTypeButton: {
            boost::shared_ptr<KnobButton> k = AppManager::createKnob<KnobButton>(panel->getHolder(), label, 1, false);
            knob = k;
        } break;
        case eParamDataTypeSeparator: {
            boost::shared_ptr<KnobSeparator> k = AppManager::createKnob<KnobSeparator>(panel->getHolder(), label, 1, false);
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
    knob->setName(nameLineEdit->text().toStdString(), true);
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
        boost::shared_ptr<KnobPage> page = getSelectedPage();
        if (!page) {
            page = panel->getOrCreateUserPageKnob();
        }
        if (page) {
            if (optionalGroupIndex != -1) {
                page->insertKnob(optionalGroupIndex, knob);
            } else {
                page->addKnob(knob);
            }
            if (page->getName() == NATRON_USER_MANAGED_KNOBS_PAGE) {
                panel->setUserPageActiveIndex();
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
    if (parentGroup && parentGroup->isVisible()) {
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

boost::shared_ptr<KnobPage>
AddKnobDialogPrivate::getSelectedPage() const
{
    if (parentPage && parentPage->isVisible()) {
        std::string selectedItem = parentPage->getCurrentIndexText().toStdString();
        if (selectedItem == NATRON_USER_MANAGED_KNOBS_PAGE) {
            return panel->getUserPageKnob();
        }
        for (std::list<boost::shared_ptr<KnobPage> >::const_iterator it = userPages.begin(); it != userPages.end(); ++it) {
            if ((*it)->getName() == selectedItem) {
                return *it;
                break;
            }
        }
        
    }
    return boost::shared_ptr<KnobPage>();
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
        Dialogs::errorDialog(tr("Error").toStdString(), tr("A parameter must have a unique script name composed only of characters from "
                                                          "[a - z / A- Z] and digits [0 - 9]. This name cannot contain spaces for scripting purposes.")
                            .toStdString());
        return;
        
    }
    
    EffectInstance* effect = 0;
    
    
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
                    Dialogs::errorDialog(tr("Error").toStdString(), tr("A parameter on a group cannot have the same script-name as a node within "
                                                                      "the group for scripting purposes.")
                                        .toStdString());
                    return;

                }
            }
        }
    }
    
    ///Remove the previous knob, and recreate it.
    
    ///Index of the type in the combo
    ParamDataTypeEnum t;
    
    ///Remember the old page in which to insert the knob
    boost::shared_ptr<KnobPage> oldParentPage ;
    
    ///If the knob was in a group, we need to place it at the same index
    int oldIndexInParent = -1;
    
    std::string oldKnobScriptName;
    std::vector<std::pair<std::string,bool> > expressions;
    std::map<boost::shared_ptr<KnobI>,std::vector<std::pair<std::string,bool> > > listenersExpressions;
    
    boost::shared_ptr<KnobPage> oldKnobIsPage ;
    bool wasNewLineActivated = true;
    if (!_imp->knob) {
        assert(_imp->typeChoice);
        t = (ParamDataTypeEnum)_imp->typeChoice->activeIndex();
    } else {
        oldKnobIsPage = boost::dynamic_pointer_cast<KnobPage>(_imp->knob);
        oldKnobScriptName = _imp->knob->getName();
        effect = dynamic_cast<EffectInstance*>(_imp->knob->getHolder());
        oldParentPage = _imp->knob->getTopLevelPage();
        wasNewLineActivated = _imp->knob->isNewLineActivated();
        t = getChoiceIndexFromKnobType(_imp->knob.get());
        boost::shared_ptr<KnobI> parent = _imp->knob->getParentKnob();
        KnobGroup* isParentGrp = dynamic_cast<KnobGroup*>(parent.get());
        if (isParentGrp && isParentGrp == _imp->getSelectedGroup()) {
            std::vector<boost::shared_ptr<KnobI> > children = isParentGrp->getChildren();
            for (U32 i = 0; i < children.size(); ++i) {
                if (children[i] == _imp->knob) {
                    oldIndexInParent = i;
                    break;
                }
            }
        } else {
            std::vector<boost::shared_ptr<KnobI> > children;
            if (oldParentPage && oldParentPage == _imp->getSelectedPage()) {
                children = oldParentPage->getChildren();
            }
            for (U32 i = 0; i < children.size(); ++i) {
                if (children[i] == _imp->knob) {
                    oldIndexInParent = i;
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
        KnobI::ListenerDimsMap listeners;
        _imp->knob->getListeners(listeners);
        for (KnobI::ListenerDimsMap::iterator it = listeners.begin(); it != listeners.end(); ++it) {
            boost::shared_ptr<KnobI> listener = it->first.lock();
            if (!listener) {
                continue;
            }
            std::vector<std::pair<std::string,bool> > exprs;
            for (std::size_t i = 0; i < it->second.size(); ++i) {
                std::pair<std::string,bool> e;
                e.first = listener->getExpression(i);
                e.second = listener->isExpressionUsingRetVariable(i);
                exprs.push_back(e);
            }
            listenersExpressions[listener] = exprs;
        }
        
        if (!oldKnobIsPage) {
            _imp->panel->getHolder()->removeDynamicKnob(_imp->knob.get());
            
            if (!_imp->isKnobAlias) {
                
                _imp->knob.reset();
            }
        }
    } //if (!_imp->knob) {
    
    
    if (oldKnobIsPage) {
        try {
            oldKnobIsPage->setName(_imp->nameLineEdit->text().toStdString());
            oldKnobIsPage->setLabel(_imp->labelLineEdit->text().toStdString());
        } catch (const std::exception& e) {
            Dialogs::errorDialog(tr("Error while creating parameter").toStdString(), e.what());
            return;
        }
    } else if (!_imp->isKnobAlias) {
        try {
            _imp->createKnobFromSelection((int)t, oldIndexInParent);
        }   catch (const std::exception& e) {
            Dialogs::errorDialog(tr("Error while creating parameter").toStdString(), e.what());
            return;
        }
        assert(_imp->knob);
        
        
        if (_imp->originalKnobSerialization) {
            _imp->knob->clone(_imp->originalKnobSerialization->getKnob().get());
        }
        
        KnobString* isLabelKnob = dynamic_cast<KnobString*>(_imp->knob.get());
        if (isLabelKnob && isLabelKnob->isLabel()) {
            ///Label knob only has a default value, but the "clone" function call above will keep the previous value,
            ///so we have to force a reset to the default value.
            isLabelKnob->resetToDefaultValue(0);
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
        boost::shared_ptr<KnobPage> page = _imp->getSelectedPage();
        if (!page) {
            page = _imp->panel->getOrCreateUserPageKnob();
            _imp->panel->setUserPageActiveIndex();
        }
        KnobGroup* group = _imp->getSelectedGroup();
        boost::shared_ptr<KnobGroup> shrdGrp;
        if (group) {
            shrdGrp = boost::dynamic_pointer_cast<KnobGroup>(group->shared_from_this());
        }
        
        try {
            _imp->knob = _imp->isKnobAlias->createDuplicateOnNode(effect,
                                                                  page,
                                                                  shrdGrp,
                                                                  oldIndexInParent,
                                                                  true,
                                                                  stdName,
                                                                  _imp->labelLineEdit->text().toStdString(),
                                                                  _imp->tooltipArea->toPlainText().toStdString(),
                                                                  false);
        } catch (const std::exception& e) {
            Dialogs::errorDialog(tr("Error while creating parameter").toStdString(), e.what());
            return;
        }
        
        KnobColor* isColor = dynamic_cast<KnobColor*>(_imp->knob.get());
        KnobDouble* isDbl = dynamic_cast<KnobDouble*>(_imp->knob.get());
        KnobInt* isInt = dynamic_cast<KnobInt*>(_imp->knob.get());
        Knob<std::string>* isStr = dynamic_cast<Knob<std::string>*>(_imp->knob.get());
        KnobGroup* isGrp = dynamic_cast<KnobGroup*>(_imp->knob.get());
        KnobBool* isBool = dynamic_cast<KnobBool*>(_imp->knob.get());
        KnobChoice* isChoice = dynamic_cast<KnobChoice*>(_imp->knob.get());
        if (isColor || isDbl) {
            _imp->setKnobMinMax<double>(_imp->knob.get());
        } else if (isInt) {
            _imp->setKnobMinMax<int>(_imp->knob.get());
        } else if (isStr) {
            isStr->setDefaultValue(_imp->defaultStr->text().toStdString());
        } else if (isGrp) {
            isGrp->setDefaultValue(true);
        } else if (isBool) {
            isBool->setDefaultValue(_imp->defaultBool->isChecked());
        } else if (isChoice) {
            std::string defValue = _imp->defaultStr->text().toStdString();
            int defIndex = -1;
            std::vector<std::string> entries = isChoice->getEntries_mt_safe();
            for (std::size_t i = 0; i < entries.size(); ++i) {
                if (entries[i] == defValue) {
                    defIndex = i;
                    break;
                }
            }
            if (defIndex == -1) {
                std::stringstream ss;
                ss << QObject::tr("The default value").toStdString();
                ss << " \"";
                ss << defValue;
                ss << "\" ";
                ss << QObject::tr("does not exist in the defined menu items").toStdString();
                Dialogs::errorDialog(tr("Error while creating parameter").toStdString(), ss.str());
                return;
            }
            if (defIndex < (int)entries.size() && defIndex >= 0) {
                isChoice->setDefaultValue(defIndex);
            }

           
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
    //also refresh the new line flag for this knob if it was set
    if (_imp->knob && !wasNewLineActivated) {
        _imp->knob->setAddNewLine(false);
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
AddKnobDialogPrivate::setVisibleLabel(bool visible)
{
    labelLineEdit->setVisible(visible);
    labelLabel->setVisible(visible);
}

void
AddKnobDialogPrivate::setVisibleToolTipEdit(bool visible)
{
    tooltipArea->setVisible(visible);
    tooltipLabel->setVisible(visible);
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
        ParamDataTypeEnum t = (ParamDataTypeEnum)typeChoice->activeIndex();
        
        if (t == eParamDataTypeColorRGB || t == eParamDataTypeColorRGBA) {
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
AddKnobDialogPrivate::setVisibleDefaultValues(bool visible,
                                              AddKnobDialogPrivate::DefaultValueType type,
                                              int dimensions)
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

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_AddKnobDialog.cpp"
