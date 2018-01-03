/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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

#include "AddKnobDialog.h"

#include <cfloat> // floor
#include <stdexcept>
#include <sstream> // stringstream

#include <QCheckBox>
#include <QTextEdit>
#include <QtCore/QTextStream>
#include <QFormLayout>

#include "Engine/EffectInstance.h"
#include "Engine/KnobFile.h" // KnobFile
#include "Serialization/KnobSerialization.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/Utils.h" // convertFromPlainText

#include "Gui/ActionShortcuts.h"
#include "Gui/ComboBox.h"
#include "Gui/DialogButtonBox.h"
#include "Gui/DockablePanel.h"
#include "Gui/Gui.h"
#include "Gui/GuiDefines.h"
#include "Gui/KnobGui.h"
#include "Gui/Label.h"
#include "Gui/LineEdit.h"
#include "Gui/SpinBox.h"


NATRON_NAMESPACE_ENTER


struct AddKnobDialogPrivate
{
    KnobIPtr knob;
    SERIALIZATION_NAMESPACE::KnobSerializationPtr originalKnobSerialization;
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
    Label* addToViewerUILabel;
    QCheckBox* addToViewerUICheckbox;
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
    Label* fileDialogSelectExistingLabel;
    QCheckBox* fileDialogSelectExisting;
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
    std::list<KnobGroupPtr> userGroups;
    std::list<KnobPagePtr > userPages; //< all user pages except the "User" one

    AddKnobDialogPrivate(DockablePanel* panel)
        : knob()
        , originalKnobSerialization()
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
        , addToViewerUILabel(0)
        , addToViewerUICheckbox(0)
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
        , fileDialogSelectExistingLabel(0)
        , fileDialogSelectExisting(0)
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

    void setVisibleViewerUi(bool visible);

    void setVisibleRichText(bool visible);

    void setVisibleFileStuff(bool visible);

    void setVisibleMultiPath(bool visible);

    void setVisibleGrpAsTab(bool visible);

    void setVisibleParent(bool visible);

    void setVisiblePage(bool visible);

    void setVisibleDefaultValues(bool visible, AddKnobDialogPrivate::DefaultValueType type, int dimensions);

    void createKnobFromSelection(AddKnobDialog::ParamDataTypeEnum type, int optionalGroupIndex = -1);

    KnobGroupPtr getSelectedGroup() const;

    template <typename T>
    void setKnobMinMax(const KnobIPtr& knob);

    KnobPagePtr getSelectedPage() const;
};

const char*
AddKnobDialog::dataTypeString(ParamDataTypeEnum t)
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

        return "File";
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
    } // switch
} // dataTypeString

int
AddKnobDialog::dataTypeDim(ParamDataTypeEnum t)
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
    case eParamDataTypeDirectory:
    case eParamDataTypeGroup:
    case eParamDataTypePage:
    case eParamDataTypeButton:
    case eParamDataTypeSeparator:
    default:

        return 1;
    }
}


AddKnobDialog::ParamDataTypeEnum
AddKnobDialog::getChoiceIndexFromKnobType(const KnobIPtr& knob)
{
    int dim = knob->getNDimensions();
    KnobIntPtr isInt = toKnobInt(knob);
    KnobDoublePtr isDbl = boost::dynamic_pointer_cast<KnobDouble>(knob);
    KnobColorPtr isColor = toKnobColor(knob);
    KnobChoicePtr isChoice = toKnobChoice(knob);
    KnobBoolPtr isBool = toKnobBool(knob);
    KnobStringPtr isStr = toKnobString(knob);
    KnobFilePtr isFile = toKnobFile(knob);
    KnobPathPtr isPath = toKnobPath(knob);
    KnobGroupPtr isGrp = toKnobGroup(knob);
    KnobPagePtr isPage = toKnobPage(knob);
    KnobButtonPtr isBtn = toKnobButton(knob);
    KnobSeparatorPtr isSep = toKnobSeparator(knob);

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
        if ( isStr->isLabel() ) {
            return eParamDataTypeLabel;
        } else {
            return eParamDataTypeTextInput;
        }
    } else if (isFile) {
        return eParamDataTypeInputFile;
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
} // getChoiceIndexFromKnobType

AddKnobDialog::AddKnobDialog(DockablePanel* panel,
                             const KnobIPtr& knob,
                             const std::string& selectedPageName,
                             const std::string& selectedGroupName,
                             QWidget* parent)
    : QDialog(parent)
    , _imp( new AddKnobDialogPrivate(panel) )
{
    _imp->knob = knob;
    assert( !knob || knob->getKnobDeclarationType() == KnobI::eKnobDeclarationTypeUser );

    {
        EffectInstancePtr effect = toEffectInstance( panel->getHolder() );
        QString title = QString::fromUtf8("Add Parameter");
        if (!knob) {
            // Add...
            if (effect) {
                title += QString::fromUtf8(" to ");
                title += QString::fromUtf8( effect->getScriptName().c_str() );
            }
        } else {
            // Edit...
            title = QString::fromUtf8("Edit Parameter ");
            if (effect) {
                title += QString::fromUtf8( effect->getScriptName().c_str() );
                title += QChar::fromLatin1('.');
            }
            title += QString::fromUtf8( knob->getName().c_str() );
        }
        setWindowTitle(title);
    }

    //QFont font(NATRON_FONT,NATRON_FONT_SIZE_11);

    _imp->vLayout = new QVBoxLayout(this);
    _imp->vLayout->setContentsMargins(0, 0, TO_DPIX(15), 0);

    _imp->mainContainer = new QWidget(this);
    _imp->mainLayout = new QFormLayout(_imp->mainContainer);
    _imp->mainLayout->setLabelAlignment(Qt::AlignVCenter | Qt::AlignRight);
    _imp->mainLayout->setFormAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    _imp->mainLayout->setSpacing(TO_DPIX(3));
    _imp->mainLayout->setContentsMargins(0, 0, TO_DPIX(15), 0);

    _imp->vLayout->addWidget(_imp->mainContainer);


    {
        QWidget* firstRowContainer = new QWidget(this);
        QHBoxLayout* firstRowLayout = new QHBoxLayout(firstRowContainer);
        firstRowLayout->setContentsMargins(0, 0, 0, 0);

        _imp->nameLabel = new Label(tr("Script name:"), this);
        _imp->nameLineEdit = new LineEdit(firstRowContainer);
        _imp->nameLineEdit->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("The name of the parameter as it will be used in Python scripts"), NATRON_NAMESPACE::WhiteSpaceNormal) );

        if (knob) {
            _imp->nameLineEdit->setText( QString::fromUtf8( knob->getName().c_str() ) );
        }
        firstRowLayout->addWidget(_imp->nameLineEdit);
        firstRowLayout->addStretch();

        _imp->mainLayout->addRow(_imp->nameLabel, firstRowContainer);
    }

    {
        QWidget* secondRowContainer = new QWidget(this);
        QHBoxLayout* secondRowLayout = new QHBoxLayout(secondRowContainer);
        secondRowLayout->setContentsMargins(0, 0, 15, 0);
        _imp->labelLabel = new Label(tr("Label:"), secondRowContainer);
        _imp->labelLineEdit = new LineEdit(secondRowContainer);
        _imp->labelLineEdit->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("The label of the parameter as displayed on the graphical user interface"), NATRON_NAMESPACE::WhiteSpaceNormal) );
        if (knob) {
            _imp->labelLineEdit->setText( QString::fromUtf8( knob->getLabel().c_str() ) );
        }
        secondRowLayout->addWidget(_imp->labelLineEdit);
        _imp->hideLabel = new Label(tr("Hide:"), secondRowContainer);
        secondRowLayout->addWidget(_imp->hideLabel);
        _imp->hideBox = new QCheckBox(secondRowContainer);
        _imp->hideBox->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("If checked the parameter will not be visible on the user interface"), NATRON_NAMESPACE::WhiteSpaceNormal) );
        if (knob) {
            _imp->hideBox->setChecked( knob->getIsSecret() );
        }

        secondRowLayout->addWidget(_imp->hideBox);
        _imp->startNewLineLabel = new Label(tr("Start new line:"), secondRowContainer);
        secondRowLayout->addWidget(_imp->startNewLineLabel);
        _imp->startNewLineBox = new QCheckBox(secondRowContainer);
        _imp->startNewLineBox->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("If unchecked the parameter will be on the same line as the previous parameter"), NATRON_NAMESPACE::WhiteSpaceNormal) );
        if (knob) {
            // get the flag on the previous knob
            bool startNewLine = true;
            KnobIPtr parentKnob = _imp->knob->getParentKnob();
            if (parentKnob) {
                KnobGroupPtr parentIsGrp = toKnobGroup(parentKnob);
                KnobPagePtr parentIsPage = toKnobPage(parentKnob);
                assert(parentIsGrp || parentIsPage);
                KnobsVec children;
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
            _imp->typeLabel = new Label(tr("Type:"), thirdRowContainer);
            _imp->typeChoice = new ComboBox(thirdRowContainer);
            _imp->typeChoice->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("The data type of the parameter."), NATRON_NAMESPACE::WhiteSpaceNormal) );
            for (int i = 0; i < (int)eParamDataTypeCount; ++i) {
                assert(_imp->typeChoice->count() == i);
                _imp->typeChoice->addItem( tr( dataTypeString( (ParamDataTypeEnum)i ) ) );
            }
            QObject::connect( _imp->typeChoice, SIGNAL(currentIndexChanged(int)), this, SLOT(onTypeCurrentIndexChanged(int)) );

            thirdRowLayout->addWidget(_imp->typeChoice);
        }
        _imp->animatesLabel = new Label(tr("Animates:"), thirdRowContainer);

        if (!knob) {
            thirdRowLayout->addWidget(_imp->animatesLabel);
        }
        _imp->animatesCheckbox = new QCheckBox(thirdRowContainer);
        _imp->animatesCheckbox->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("When checked this parameter will be able to animate with keyframes."), NATRON_NAMESPACE::WhiteSpaceNormal) );
        if (knob) {
            _imp->animatesCheckbox->setChecked( knob->isAnimationEnabled() );
        }

        thirdRowLayout->addWidget(_imp->animatesCheckbox);
        _imp->evaluatesLabel = new Label(NATRON_NAMESPACE::convertFromPlainText(tr("Render on change:"), NATRON_NAMESPACE::WhiteSpaceNormal), thirdRowContainer);
        thirdRowLayout->addWidget(_imp->evaluatesLabel);
        _imp->evaluatesOnChange = new QCheckBox(thirdRowContainer);
        _imp->evaluatesOnChange->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("If checked, when the value of this parameter changes a new render will be triggered."), NATRON_NAMESPACE::WhiteSpaceNormal) );
        if (knob) {
            _imp->evaluatesOnChange->setChecked( knob->getEvaluateOnChange() );
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
        _imp->tooltipLabel = new Label(tr("ToolTip:"), this);
        _imp->tooltipArea = new QTextEdit(this);
        _imp->tooltipArea->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("The help tooltip that will appear when hovering the parameter with the mouse."), NATRON_NAMESPACE::WhiteSpaceNormal) );
        _imp->mainLayout->addRow(_imp->tooltipLabel, _imp->tooltipArea);
        if (knob) {
            _imp->tooltipArea->setPlainText( QString::fromUtf8( knob->getHintToolTip().c_str() ) );
        }
    }
    {
        _imp->menuItemsLabel = new Label(tr("Menu items:"), this);
        _imp->menuItemsEdit = new QTextEdit(this);
        QString tt = NATRON_NAMESPACE::convertFromPlainText(tr("The entries of the drop-down menu. \n"
                                                       "Each line defines a new menu entry. You can specify a specific help tooltip for each entry "
                                                       "by separating the entry text from the help with the following characters on the line: "
                                                       "<?> \n\n"
                                                       "e.g.: Special function<?>Will use our very own special function."), NATRON_NAMESPACE::WhiteSpaceNormal);
        _imp->menuItemsEdit->setToolTip(tt);
        _imp->mainLayout->addRow(_imp->menuItemsLabel, _imp->menuItemsEdit);

        KnobChoicePtr isChoice = toKnobChoice(knob);
        if (isChoice) {
            std::vector<ChoiceOption> entries = isChoice->getEntries();

            QString data;
            for (U32 i = 0; i < entries.size(); ++i) {
                QString line( QString::fromUtf8( entries[i].id.c_str() ) );
                if ( !entries[i].tooltip.empty() ) {
                    line.append( QString::fromUtf8("<?>") );
                    line.append( QString::fromUtf8( entries[i].tooltip.c_str() ) );
                }
                data.append(line);
                data.append( QLatin1Char('\n') );
            }
            _imp->menuItemsEdit->setPlainText(data);
        }
    }
    {
        QWidget* rowContainer = new QWidget(this);
        QHBoxLayout* rowLayout = new QHBoxLayout(rowContainer);
        rowLayout->setContentsMargins(0, 0, 15, 0);
        _imp->addToViewerUILabel = new Label(tr("Add to Viewer UI:"), rowContainer);
        _imp->addToViewerUICheckbox = new QCheckBox(rowContainer);
        QString tt = NATRON_NAMESPACE::convertFromPlainText(tr("When checked, the parameter will also appear in the Viewer interface for this node"), NATRON_NAMESPACE::WhiteSpaceNormal);
        _imp->addToViewerUICheckbox->setToolTip(tt);
        _imp->addToViewerUILabel->setToolTip(tt);
        if (knob) {
            _imp->addToViewerUICheckbox->setChecked(knob->getHolder()->getInViewerContextKnobIndex(knob) != -1);
        }
        rowLayout->addWidget(_imp->addToViewerUICheckbox);
        rowLayout->addStretch();
        _imp->mainLayout->addRow(_imp->addToViewerUILabel, rowContainer);
    }
    {
        QWidget* optContainer = new QWidget(this);
        QHBoxLayout* optLayout = new QHBoxLayout(optContainer);
        optLayout->setContentsMargins(0, 0, 15, 0);

        _imp->multiLineLabel = new Label(tr("Multi-line:"), optContainer);
        _imp->multiLine = new QCheckBox(optContainer);
        _imp->multiLine->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Should this text be multi-line or single-line?"), NATRON_NAMESPACE::WhiteSpaceNormal) );
        optLayout->addWidget(_imp->multiLine);
        _imp->mainLayout->addRow(_imp->multiLineLabel, optContainer);

        KnobStringPtr isStr = toKnobString(knob);
        if (isStr) {
            if ( isStr && isStr->isMultiLine() ) {
                _imp->multiLine->setChecked(true);
            }
        }
    }
    {
        QWidget* optContainer = new QWidget(this);
        QHBoxLayout* optLayout = new QHBoxLayout(optContainer);
        optLayout->setContentsMargins(0, 0, 15, 0);

        _imp->richTextLabel = new Label(tr("Rich text:"), optContainer);
        _imp->richText = new QCheckBox(optContainer);
        QString tt = NATRON_NAMESPACE::convertFromPlainText(tr("If checked, the text area will be able to use rich text encoding with a sub-set of html.\n "
                                                       "This property is only valid for multi-line input text only."), NATRON_NAMESPACE::WhiteSpaceNormal);

        _imp->richText->setToolTip(tt);
        optLayout->addWidget(_imp->richText);
        _imp->mainLayout->addRow(_imp->richTextLabel, optContainer);

        KnobStringPtr isStr = toKnobString(knob);
        if (isStr) {
            if ( isStr && isStr->isMultiLine() && isStr->usesRichText() ) {
                _imp->richText->setChecked(true);
            }
        }
    }
    {
        QWidget* optContainer = new QWidget(this);
        QHBoxLayout* optLayout = new QHBoxLayout(optContainer);
        optLayout->setContentsMargins(0, 0, 15, 0);

        _imp->sequenceDialogLabel = new Label(tr("Use sequence dialog:"), optContainer);
        _imp->sequenceDialog = new QCheckBox(optContainer);
        _imp->sequenceDialog->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("If checked the file dialog for this parameter will be able to decode image sequences."), NATRON_NAMESPACE::WhiteSpaceNormal) );
        optLayout->addWidget(_imp->sequenceDialog);



        _imp->fileDialogSelectExistingLabel = new Label(tr("Select only existing files:"), optContainer);
        optLayout->addWidget(_imp->fileDialogSelectExistingLabel);
        _imp->fileDialogSelectExisting = new QCheckBox(optContainer);
        _imp->fileDialogSelectExisting->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("If checked the file dialog will only open file(s) that already exists"), NATRON_NAMESPACE::WhiteSpaceNormal) );
        optLayout->addWidget(_imp->fileDialogSelectExisting);

        _imp->mainLayout->addRow(_imp->sequenceDialogLabel, optContainer);

        KnobFilePtr isFile = toKnobFile(knob);
        if (isFile) {
            if ( isFile->getDialogType() == KnobFile::eKnobFileDialogTypeSaveFileSequences ||
                isFile->getDialogType() == KnobFile::eKnobFileDialogTypeOpenFileSequences) {
                _imp->sequenceDialog->setChecked(true);
            }
            if ( isFile->getDialogType() == KnobFile::eKnobFileDialogTypeOpenFile ||
                isFile->getDialogType() == KnobFile::eKnobFileDialogTypeOpenFileSequences) {
                _imp->fileDialogSelectExisting->setChecked(true);
            }
        }
    }
    {
        QWidget* optContainer = new QWidget(this);
        QHBoxLayout* optLayout = new QHBoxLayout(optContainer);
        optLayout->setContentsMargins(0, 0, 15, 0);

        _imp->multiPathLabel = new Label(NATRON_NAMESPACE::convertFromPlainText(tr("Multiple paths:"), NATRON_NAMESPACE::WhiteSpaceNormal), optContainer);
        _imp->multiPath = new QCheckBox(optContainer);
        _imp->multiPath->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("If checked the parameter will be a table where each entry points to a different path."), NATRON_NAMESPACE::WhiteSpaceNormal) );
        optLayout->addWidget(_imp->multiPath);
        _imp->mainLayout->addRow(_imp->multiPathLabel, optContainer);

        KnobPathPtr isStr = toKnobPath(knob);
        if (isStr) {
            if ( isStr && isStr->isMultiPath() ) {
                _imp->multiPath->setChecked(true);
            }
        }
    }
    {
        QWidget* optContainer = new QWidget(this);
        QHBoxLayout* optLayout = new QHBoxLayout(optContainer);
        optLayout->setContentsMargins(0, 0, 15, 0);

        _imp->groupAsTabLabel = new Label(tr("Group as tab:"), optContainer);
        _imp->groupAsTab = new QCheckBox(optContainer);
        _imp->groupAsTab->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("If checked the group will be a tab instead."), NATRON_NAMESPACE::WhiteSpaceNormal) );
        optLayout->addWidget(_imp->groupAsTab);
        _imp->mainLayout->addRow(_imp->groupAsTabLabel, optContainer);

        KnobGroupPtr isGrp = toKnobGroup(knob);
        if (isGrp) {
            if ( isGrp && isGrp->isTab() ) {
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
        _imp->minLabel = new Label(tr("Minimum:"), minMaxContainer);

        _imp->minBox = new SpinBox(minMaxContainer, SpinBox::eSpinBoxTypeDouble);
        _imp->minBox->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Set the minimum value for the parameter. Even though the user might input "
                                                                    "a value higher or lower than the specified min/max range, internally the "
                                                                    "real value will be clamped to this interval."), NATRON_NAMESPACE::WhiteSpaceNormal) );
        minMaxLayout->addWidget(_imp->minBox);

        _imp->maxLabel = new Label(tr("Maximum:"), minMaxContainer);
        _imp->maxBox = new SpinBox(minMaxContainer, SpinBox::eSpinBoxTypeDouble);
        _imp->maxBox->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Set the maximum value for the parameter. Even though the user might input "
                                                                    "a value higher or lower than the specified min/max range, internally the "
                                                                    "real value will be clamped to this interval."), NATRON_NAMESPACE::WhiteSpaceNormal) );
        minMaxLayout->addWidget(_imp->maxLabel);
        minMaxLayout->addWidget(_imp->maxBox);
        minMaxLayout->addStretch();

        _imp->dminLabel = new Label(tr("Display Minimum:"), dminMaxContainer);
        _imp->dminBox = new SpinBox(dminMaxContainer, SpinBox::eSpinBoxTypeDouble);
        _imp->dminBox->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Set the display minimum value for the parameter. This is a hint that is typically "
                                                                     "used to set the range of the slider."), NATRON_NAMESPACE::WhiteSpaceNormal) );
        dminMaxLayout->addWidget(_imp->dminBox);

        _imp->dmaxLabel = new Label(tr("Display Maximum:"), dminMaxContainer);
        _imp->dmaxBox = new SpinBox(dminMaxContainer, SpinBox::eSpinBoxTypeDouble);
        _imp->dmaxBox->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Set the display maximum value for the parameter. This is a hint that is typically "
                                                                     "used to set the range of the slider."), NATRON_NAMESPACE::WhiteSpaceNormal) );
        dminMaxLayout->addWidget(_imp->dmaxLabel);
        dminMaxLayout->addWidget(_imp->dmaxBox);

        dminMaxLayout->addStretch();

        KnobDoublePtr isDbl = boost::dynamic_pointer_cast<KnobDouble>(knob);
        KnobIntPtr isInt = toKnobInt(knob);
        KnobColorPtr isColor = toKnobColor(knob);


        if (isDbl) {
            double min = isDbl->getMinimum();
            double max = isDbl->getMaximum();
            double dmin = isDbl->getDisplayMinimum();
            double dmax = isDbl->getDisplayMaximum();
            _imp->minBox->setValue(min);
            _imp->maxBox->setValue(max);
            _imp->dminBox->setValue(dmin);
            _imp->dmaxBox->setValue(dmax);
        } else if (isInt) {
            int min = isInt->getMinimum();
            int max = isInt->getMaximum();
            int dmin = isInt->getDisplayMinimum();
            int dmax = isInt->getDisplayMaximum();
            _imp->minBox->setValue(min);
            _imp->maxBox->setValue(max);
            _imp->dminBox->setValue(dmin);
            _imp->dmaxBox->setValue(dmax);
        } else if (isColor) {
            double min = isColor->getMinimum();
            double max = isColor->getMaximum();
            double dmin = isColor->getDisplayMinimum();
            double dmax = isColor->getDisplayMaximum();
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
        _imp->defaultValueLabel = new Label(tr("Default value:"), defValContainer);

        _imp->default0 = new SpinBox(defValContainer, SpinBox::eSpinBoxTypeDouble);
        _imp->default0->setValue(0);
        _imp->default0->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Set the default value for the parameter (dimension 0)."), NATRON_NAMESPACE::WhiteSpaceNormal) );
        defValLayout->addWidget(_imp->default0);

        _imp->default1 = new SpinBox(defValContainer, SpinBox::eSpinBoxTypeDouble);
        _imp->default1->setValue(0);
        _imp->default1->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Set the default value for the parameter (dimension 1)."), NATRON_NAMESPACE::WhiteSpaceNormal) );
        defValLayout->addWidget(_imp->default1);

        _imp->default2 = new SpinBox(defValContainer, SpinBox::eSpinBoxTypeDouble);
        _imp->default2->setValue(0);
        _imp->default2->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Set the default value for the parameter (dimension 2)."), NATRON_NAMESPACE::WhiteSpaceNormal) );
        defValLayout->addWidget(_imp->default2);

        _imp->default3 = new SpinBox(defValContainer, SpinBox::eSpinBoxTypeDouble);
        _imp->default3->setValue(0);
        _imp->default3->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Set the default value for the parameter (dimension 3)."), NATRON_NAMESPACE::WhiteSpaceNormal) );
        defValLayout->addWidget(_imp->default3);


        _imp->defaultStr = new LineEdit(defValContainer);
        _imp->defaultStr->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Set the default value for the parameter."), NATRON_NAMESPACE::WhiteSpaceNormal) );
        defValLayout->addWidget(_imp->defaultStr);


        _imp->defaultBool = new QCheckBox(defValContainer);
        _imp->defaultBool->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Set the default value for the parameter."), NATRON_NAMESPACE::WhiteSpaceNormal) );
        _imp->defaultBool->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
        defValLayout->addWidget(_imp->defaultBool);

        defValLayout->addStretch();

        _imp->mainLayout->addRow(_imp->defaultValueLabel, defValContainer);


        KnobDoubleBasePtr isDbl = toKnobDoubleBase(knob);
        KnobIntBasePtr isInt = toKnobIntBase(knob);
        KnobBoolPtr isBool = toKnobBool(knob);
        KnobStringBasePtr isStr = toKnobStringBase(knob);
        KnobChoicePtr isChoice = toKnobChoice(knob);

        if (isChoice) {
            _imp->defaultStr->setText( QString::fromUtf8( isChoice->getEntry( isChoice->getDefaultValue(DimIdx(0)) ).id.c_str() ) );

        } else if (isDbl) {
            _imp->default0->setValue( isDbl->getDefaultValue(DimIdx(0)) );
            if (isDbl->getNDimensions() >= 2) {
                _imp->default1->setValue( isDbl->getDefaultValue(DimIdx(1)) );
            }
            if (isDbl->getNDimensions() >= 3) {
                _imp->default2->setValue( isDbl->getDefaultValue(DimIdx(2)) );
            }
            if (isDbl->getNDimensions() >= 4) {
                _imp->default3->setValue( isDbl->getDefaultValue(DimIdx(3)) );
            }
        } else if (isInt) {
            _imp->default0->setValue( isInt->getDefaultValue(DimIdx(0)) );
            if (isInt->getNDimensions() >= 2) {
                _imp->default1->setValue( isInt->getDefaultValue(DimIdx(1)) );
            }
            if (isInt->getNDimensions() >= 3) {
                _imp->default2->setValue( isInt->getDefaultValue(DimIdx(2)) );
            }
        } else if (isBool) {
            _imp->defaultBool->setChecked( isBool->getDefaultValue(DimIdx(0)) );
        } else if (isStr) {
            _imp->defaultStr->setText( QString::fromUtf8( isStr->getDefaultValue(DimIdx(0)).c_str() ) );
        }
    }


    const KnobsGuiMapping& knobs = _imp->panel->getKnobsMapping();
    for (KnobsGuiMapping::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        KnobIPtr knob = it->first.lock();
        if (!knob) {
            continue;
        }
        if ( knob->getKnobDeclarationType() == KnobI::eKnobDeclarationTypeUser) {
            KnobGroupPtr isGrp = toKnobGroup(knob);
            if (isGrp) {
                _imp->userGroups.push_back(isGrp);
            }
        }
    }


    if ( !_imp->userGroups.empty() ) {
        QWidget* optContainer = new QWidget(this);
        QHBoxLayout* optLayout = new QHBoxLayout(optContainer);
        optLayout->setContentsMargins(0, 0, 15, 0);

        _imp->parentGroupLabel = new Label(tr("Group:"), optContainer);
        _imp->parentGroup = new ComboBox(optContainer);

        _imp->parentGroup->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("The name of the group under which this parameter will appear."), NATRON_NAMESPACE::WhiteSpaceNormal) );
        optLayout->addWidget(_imp->parentGroup);

        _imp->mainLayout->addRow(_imp->parentGroupLabel, optContainer);
    }

    QWidget* optContainer = new QWidget(this);
    QHBoxLayout* optLayout = new QHBoxLayout(optContainer);
    optLayout->setContentsMargins(0, 0, 15, 0);
    _imp->parentPageLabel = new Label(tr("Page:"), optContainer);
    _imp->parentPage = new ComboBox(optContainer);

    QObject::connect( _imp->parentPage, SIGNAL(currentIndexChanged(int)), this, SLOT(onPageCurrentIndexChanged(int)) );
    const KnobsVec& internalKnobs = _imp->panel->getHolder()->getKnobs();
    for (KnobsVec::const_iterator it = internalKnobs.begin(); it != internalKnobs.end(); ++it) {
        if ( (*it)->getKnobDeclarationType() == KnobI::eKnobDeclarationTypeUser ) {
            KnobPagePtr isPage = toKnobPage(*it);
            if (isPage) {
                _imp->userPages.push_back(isPage);
            }
        }
    }

    for (std::list<KnobPagePtr >::iterator it = _imp->userPages.begin(); it != _imp->userPages.end(); ++it) {
        _imp->parentPage->addItem( QString::fromUtf8( (*it)->getName().c_str() ) );
    }
    _imp->parentPage->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("The tab under which this parameter will appear."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    optLayout->addWidget(_imp->parentPage);

    int pageIndexLoaded = -1;
    if (knob) {
        ////find in which page the knob should be

        KnobPagePtr isTopLevelParentAPage = knob->getTopLevelPage();
        if (isTopLevelParentAPage) {
            int index = 0; // 1 because of the "User" item
            bool found = false;
            for (std::list<KnobPagePtr >::iterator it = _imp->userPages.begin(); it != _imp->userPages.end(); ++it, ++index) {
                if (*it == isTopLevelParentAPage) {
                    found = true;
                    break;
                }
            }
            if (found) {
                _imp->parentPage->setCurrentIndex(index, false);
                pageIndexLoaded = index;
            }
        }
    } else {
        ///If the selected page name in the manage user params dialog is valid, set the page accordingly
        if ( _imp->parentPage && !selectedPageName.empty() ) {
            int index = 0;
            for (std::list<KnobPagePtr >::iterator it = _imp->userPages.begin(); it != _imp->userPages.end(); ++it, ++index) {
                if ( (*it)->getName() == selectedPageName ) {
                    _imp->parentPage->setCurrentIndex(index, false);
                    pageIndexLoaded = index;
                    break;
                }
            }
        }
    }

    _imp->mainLayout->addRow(_imp->parentPageLabel, optContainer);

    onPageCurrentIndexChanged(pageIndexLoaded == -1 ? 0 : pageIndexLoaded);

    if (_imp->parentGroup && knob) {
        KnobPagePtr topLvlPage = knob->getTopLevelPage();
        if (topLvlPage) {
            KnobIPtr parent = knob->getParentKnob();
            KnobGroupPtr isParentGrp = toKnobGroup(parent);
            if (isParentGrp) {
                for (std::list<KnobGroupPtr>::iterator it = _imp->userGroups.begin(); it != _imp->userGroups.end(); ++it) {
                    KnobPagePtr page = (*it)->getTopLevelPage();
                    assert(page);

                    ///add only grps whose parent page is the selected page
                    if ( (isParentGrp == *it) && (page == topLvlPage) ) {
                        for (int i = 0; i < _imp->parentGroup->count(); ++i) {
                            if ( _imp->parentGroup->itemText(i) == QString::fromUtf8( isParentGrp->getName().c_str() ) ) {
                                _imp->parentGroup->setCurrentIndex(i);
                                break;
                            }
                        }
                        break;
                    }
                }
            }
        }
    } else {
        ///If the selected group name in the manage user params dialog is valid, set the group accordingly
        if (_imp->parentGroup) {
            for (int i = 0; i < _imp->parentGroup->count(); ++i) {
                if ( _imp->parentGroup->itemText(i) == QString::fromUtf8( selectedGroupName.c_str() ) ) {
                    _imp->parentGroup->setCurrentIndex(i);
                    break;
                }
            }
        }
    }


    DialogButtonBox* buttons = new DialogButtonBox(QDialogButtonBox::StandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel), Qt::Horizontal, this);
    QObject::connect( buttons, SIGNAL(rejected()), this, SLOT(reject()) );
    QObject::connect( buttons, SIGNAL(accepted()), this, SLOT(onOkClicked()) );
    _imp->vLayout->addWidget(buttons);

    ParamDataTypeEnum t;
    if (!knob) {
        t = (ParamDataTypeEnum)_imp->typeChoice->activeIndex();
    } else {
        t = getChoiceIndexFromKnobType(knob);
        assert(t != eParamDataTypeCount);
    }
    onTypeCurrentIndexChanged( (int)t );

    if (knob) {
        _imp->originalKnobSerialization.reset( new SERIALIZATION_NAMESPACE::KnobSerialization );
        knob->toSerialization(_imp->originalKnobSerialization.get());
    }
}

void
AddKnobDialog::onPageCurrentIndexChanged(int index)
{
    if (!_imp->parentGroup) {
        return;
    }
    _imp->parentGroup->clear();
    _imp->parentGroup->addItem( QString::fromUtf8("-") );

    std::string selectedPage = _imp->parentPage->itemText(index).toStdString();
    KnobPagePtr parentPage;


    for (std::list<boost::shared_ptr<KnobPage> >::iterator it = _imp->userPages.begin(); it != _imp->userPages.end(); ++it) {
        if ( (*it)->getName() == selectedPage ) {
            parentPage = *it;
            break;
        }
    }


    for (std::list<KnobGroupPtr>::iterator it = _imp->userGroups.begin(); it != _imp->userGroups.end(); ++it) {
        KnobPagePtr page = (*it)->getTopLevelPage();
        assert(page);

        ///add only grps whose parent page is the selected page
        if (page == parentPage) {
            _imp->parentGroup->addItem( QString::fromUtf8( (*it)->getName().c_str() ) );
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
    case eParamDataTypeInteger:     // int
    case eParamDataTypeInteger2D:     // int 2D
    case eParamDataTypeInteger3D:     // int 3D
        _imp->setVisibleToolTipEdit(true);
        _imp->setVisibleAnimates(true);
        _imp->setVisibleEvaluate(true);
        _imp->setVisibleHide(true);
        _imp->setVisibleMenuItems(false);
        _imp->setVisibleMinMax(true);
        _imp->setVisibleStartNewLine(true);
        _imp->setVisibleMultiLine(false);
        _imp->setVisibleViewerUi(true);
        _imp->setVisibleMultiPath(false);
        _imp->setVisibleRichText(false);
        _imp->setVisibleFileStuff(false);
        _imp->setVisibleGrpAsTab(false);
        _imp->setVisibleParent(true);
        _imp->setVisibleDefaultValues(true, AddKnobDialogPrivate::eDefaultValueTypeInt, d);
        break;

    case eParamDataTypeFloatingPoint:     // fp
    case eParamDataTypeFloatingPoint2D:     // fp 2D
    case eParamDataTypeFloatingPoint3D:     // fp 3D
    case eParamDataTypeColorRGB:     // RGB
    case eParamDataTypeColorRGBA:     // RGBA
        _imp->setVisibleToolTipEdit(true);
        _imp->setVisibleAnimates(true);
        _imp->setVisibleEvaluate(true);
        _imp->setVisibleHide(true);
        _imp->setVisibleMenuItems(false);
        _imp->setVisibleMinMax(true);
        _imp->setVisibleStartNewLine(true);
        _imp->setVisibleMultiLine(false);
        _imp->setVisibleViewerUi(true);
        _imp->setVisibleMultiPath(false);
        _imp->setVisibleRichText(false);
        _imp->setVisibleFileStuff(false);
        _imp->setVisibleGrpAsTab(false);
        _imp->setVisibleParent(true);
        _imp->setVisibleDefaultValues(true, AddKnobDialogPrivate::eDefaultValueTypeDouble, d);
        break;

    case eParamDataTypeChoice:     // choice
        _imp->setVisibleToolTipEdit(true);
        _imp->setVisibleAnimates(true);
        _imp->setVisibleEvaluate(true);
        _imp->setVisibleHide(true);
        _imp->setVisibleMenuItems(true);
        _imp->setVisibleMinMax(false);
        _imp->setVisibleStartNewLine(true);
        _imp->setVisibleMultiLine(false);
        _imp->setVisibleViewerUi(true);
        _imp->setVisibleMultiPath(false);
        _imp->setVisibleRichText(false);
        _imp->setVisibleFileStuff(false);
        _imp->setVisibleGrpAsTab(false);
        _imp->setVisibleParent(true);
        _imp->setVisibleDefaultValues(true, AddKnobDialogPrivate::eDefaultValueTypeString, d);
        break;
    case eParamDataTypeCheckbox:     // bool
        _imp->setVisibleToolTipEdit(true);
        _imp->setVisibleAnimates(true);
        _imp->setVisibleEvaluate(true);
        _imp->setVisibleHide(true);
        _imp->setVisibleMenuItems(false);
        _imp->setVisibleMinMax(false);
        _imp->setVisibleStartNewLine(true);
        _imp->setVisibleMultiLine(false);
        _imp->setVisibleViewerUi(true);
        _imp->setVisibleMultiPath(false);
        _imp->setVisibleRichText(false);
        _imp->setVisibleFileStuff(false);
        _imp->setVisibleGrpAsTab(false);
        _imp->setVisibleParent(true);
        _imp->setVisibleDefaultValues(true, AddKnobDialogPrivate::eDefaultValueTypeBool, d);
        break;
    case eParamDataTypeLabel:     // label
        _imp->setVisibleToolTipEdit(true);
        _imp->setVisibleAnimates(false);
        _imp->setVisibleEvaluate(false);
        _imp->setVisibleHide(true);
        _imp->setVisibleMenuItems(false);
        _imp->setVisibleMinMax(false);
        _imp->setVisibleStartNewLine(true);
        _imp->setVisibleMultiLine(false);
        _imp->setVisibleViewerUi(true);
        _imp->setVisibleMultiPath(false);
        _imp->setVisibleRichText(false);
        _imp->setVisibleFileStuff(false);
        _imp->setVisibleGrpAsTab(false);
        _imp->setVisibleParent(true);
        _imp->setVisibleDefaultValues(true, AddKnobDialogPrivate::eDefaultValueTypeString, d);
        break;
    case eParamDataTypeTextInput:     // text input
        _imp->setVisibleToolTipEdit(true);
        _imp->setVisibleAnimates(true);
        _imp->setVisibleEvaluate(true);
        _imp->setVisibleHide(true);
        _imp->setVisibleMenuItems(false);
        _imp->setVisibleMinMax(false);
        _imp->setVisibleStartNewLine(false);
        _imp->setVisibleMultiLine(true);
        _imp->setVisibleViewerUi(true);
        _imp->setVisibleMultiPath(false);
        _imp->setVisibleRichText(true);
        _imp->setVisibleFileStuff(false);
        _imp->setVisibleGrpAsTab(false);
        _imp->setVisibleParent(true);
        _imp->setVisibleDefaultValues(true, AddKnobDialogPrivate::eDefaultValueTypeString, d);
        break;
    case eParamDataTypeInputFile:     // input file
        _imp->setVisibleToolTipEdit(true);
        _imp->setVisibleAnimates(false);
        _imp->setVisibleEvaluate(true);
        _imp->setVisibleHide(true);
        _imp->setVisibleMenuItems(false);
        _imp->setVisibleMinMax(false);
        _imp->setVisibleStartNewLine(false);
        _imp->setVisibleMultiLine(false);
        _imp->setVisibleViewerUi(true);
        _imp->setVisibleMultiPath(false);
        _imp->setVisibleRichText(false);
        _imp->setVisibleFileStuff(true);
        _imp->setVisibleGrpAsTab(false);
        _imp->setVisibleParent(true);
        _imp->setVisibleDefaultValues(true, AddKnobDialogPrivate::eDefaultValueTypeString, d);
        break;
    case eParamDataTypeDirectory:     // path
        _imp->setVisibleToolTipEdit(true);
        _imp->setVisibleAnimates(false);
        _imp->setVisibleEvaluate(true);
        _imp->setVisibleHide(true);
        _imp->setVisibleMenuItems(false);
        _imp->setVisibleMinMax(false);
        _imp->setVisibleStartNewLine(false);
        _imp->setVisibleMultiLine(false);
        _imp->setVisibleViewerUi(true);
        _imp->setVisibleMultiPath(true);
        _imp->setVisibleRichText(false);
        _imp->setVisibleFileStuff(false);
        _imp->setVisibleGrpAsTab(false);
        _imp->setVisibleParent(true);
        _imp->setVisibleDefaultValues(true, AddKnobDialogPrivate::eDefaultValueTypeString, d);
        break;
    case eParamDataTypeGroup:     // grp
        _imp->setVisibleToolTipEdit(true);
        _imp->setVisibleAnimates(false);
        _imp->setVisibleEvaluate(false);
        _imp->setVisibleHide(true);
        _imp->setVisibleMenuItems(false);
        _imp->setVisibleMinMax(false);
        _imp->setVisibleStartNewLine(false);
        _imp->setVisibleMultiLine(false);
        _imp->setVisibleViewerUi(false);
        _imp->setVisibleMultiPath(false);
        _imp->setVisibleRichText(false);
        _imp->setVisibleFileStuff(false);
        _imp->setVisibleGrpAsTab(true);
        _imp->setVisibleParent(false);
        _imp->setVisibleDefaultValues(false, AddKnobDialogPrivate::eDefaultValueTypeInt, d);
        break;
    case eParamDataTypePage:     // page
        _imp->setVisibleToolTipEdit(false);
        _imp->setVisibleAnimates(false);
        _imp->setVisibleEvaluate(false);
        _imp->setVisibleHide(true);
        _imp->setVisibleMenuItems(false);
        _imp->setVisibleMinMax(false);
        _imp->setVisibleStartNewLine(false);
        _imp->setVisibleMultiLine(false);
        _imp->setVisibleViewerUi(false);
        _imp->setVisibleMultiPath(false);
        _imp->setVisibleRichText(false);
        _imp->setVisibleFileStuff(false);
        _imp->setVisibleGrpAsTab(false);
        _imp->setVisibleParent(false);
        _imp->setVisibleDefaultValues(false, AddKnobDialogPrivate::eDefaultValueTypeInt, d);
        break;
    case eParamDataTypeButton:     // button
        _imp->setVisibleToolTipEdit(true);
        _imp->setVisibleAnimates(false);
        _imp->setVisibleEvaluate(false);
        _imp->setVisibleHide(false);
        _imp->setVisibleMenuItems(false);
        _imp->setVisibleMinMax(false);
        _imp->setVisibleStartNewLine(true);
        _imp->setVisibleMultiLine(false);
        _imp->setVisibleViewerUi(true);
        _imp->setVisibleMultiPath(false);
        _imp->setVisibleRichText(false);
        _imp->setVisibleFileStuff(false);
        _imp->setVisibleGrpAsTab(false);
        _imp->setVisibleParent(true);
        _imp->setVisibleDefaultValues(false, AddKnobDialogPrivate::eDefaultValueTypeInt, d);
        break;
    case eParamDataTypeSeparator:     // separator
        _imp->setVisibleToolTipEdit(true);
        _imp->setVisibleAnimates(false);
        _imp->setVisibleEvaluate(false);
        _imp->setVisibleHide(false);
        _imp->setVisibleMenuItems(false);
        _imp->setVisibleMinMax(false);
        _imp->setVisibleStartNewLine(false);
        _imp->setVisibleMultiLine(false);
        _imp->setVisibleViewerUi(false);
        _imp->setVisibleMultiPath(false);
        _imp->setVisibleRichText(false);
        _imp->setVisibleFileStuff(false);
        _imp->setVisibleGrpAsTab(false);
        _imp->setVisibleParent(true);
        _imp->setVisibleDefaultValues(false, AddKnobDialogPrivate::eDefaultValueTypeInt, d);
        break;
    default:
        break;
    } // switch

} // AddKnobDialog::onTypeCurrentIndexChanged

AddKnobDialog::~AddKnobDialog()
{
}

KnobIPtr
AddKnobDialog::getKnob() const
{
    return _imp->knob;
}

template <typename T>
void
AddKnobDialogPrivate::setKnobMinMax(const KnobIPtr& knob)
{
    int dim = knob->getNDimensions();

    boost::shared_ptr<Knob<T> > k = boost::dynamic_pointer_cast<Knob<T> >(knob);
    assert(k);
    if (!k) {
        return;
    }
    std::vector<T> mins(dim), dmins(dim);
    std::vector<T> maxs(dim), dmaxs(dim);
    for (std::size_t i = 0; i < (std::size_t)dim; ++i) {
        mins[i] = minBox->value();
        dmins[i] = dminBox->value();
        maxs[i] = maxBox->value();
        dmaxs[i] = dmaxBox->value();
    }
    k->setRangeAcrossDimensions(mins, maxs);
    k->setDisplayRangeAcrossDimensions(dmins, dmaxs);
    std::vector<T> defValues;
    if (dim >= 1) {
        defValues.push_back( default0->value() );
    }
    if (dim >= 2) {
        defValues.push_back( default1->value() );
    }
    if (dim >= 3) {
        defValues.push_back( default2->value() );
    }
    if (dim >= 4) {
        defValues.push_back( default3->value() );
    }
    for (std::size_t i = 0; i < defValues.size(); ++i) {
        k->setDefaultValue(defValues[i], DimIdx(i));
    }
}

void
AddKnobDialogPrivate::createKnobFromSelection(AddKnobDialog::ParamDataTypeEnum t,
                                              int optionalGroupIndex)
{

    assert(!knob);
    std::string name = nameLineEdit->text().toStdString();
    std::string label = labelLineEdit->text().toStdString();
    int dim = AddKnobDialog::dataTypeDim(t);
    switch (t) {
    case AddKnobDialog::eParamDataTypeInteger:
    case AddKnobDialog::eParamDataTypeInteger2D:
    case AddKnobDialog::eParamDataTypeInteger3D: {
        //int
        KnobIntPtr k = panel->getHolder()->createKnob<KnobInt>(name, dim);
        setKnobMinMax<int>(k);
        knob = k;
        break;
    }
    case AddKnobDialog::eParamDataTypeFloatingPoint:
    case AddKnobDialog::eParamDataTypeFloatingPoint2D:
    case AddKnobDialog::eParamDataTypeFloatingPoint3D: {
        //double
        int dim = (int)t - 2;
        KnobDoublePtr k = panel->getHolder()->createKnob<KnobDouble>(name, dim);
        setKnobMinMax<double>(k);
        knob = k;
        break;
    }
    case AddKnobDialog::eParamDataTypeColorRGB:
    case AddKnobDialog::eParamDataTypeColorRGBA: {
        // color
        int dim = (int)t - 3;
        KnobColorPtr k = panel->getHolder()->createKnob<KnobColor>(name, dim);
        setKnobMinMax<double>(k);
        knob = k;
        break;
    }
    case AddKnobDialog::eParamDataTypeChoice: {
        KnobChoicePtr k = panel->getHolder()->createKnob<KnobChoice>(name);
        QString entriesRaw = menuItemsEdit->toPlainText();
        QTextStream stream(&entriesRaw);
        std::vector<ChoiceOption> entries;

        while ( !stream.atEnd() ) {
            QString line = stream.readLine();
            int foundHelp = line.indexOf( QString::fromUtf8("<?>") );
            if (foundHelp != -1) {
                QString entry = line.mid(0, foundHelp);
                QString help = line.mid(foundHelp + 3, -1);

                entries.push_back(ChoiceOption( entry.toStdString(), "", help.toStdString()) );
            } else {
                entries.push_back( ChoiceOption(line.toStdString(), "", "") );
            }
        }
        k->populateChoices(entries);

        std::string defValue = defaultStr->text().toStdString();
        int defIndex = -1;
        for (std::size_t i = 0; i < entries.size(); ++i) {
            if (entries[i].id == defValue) {
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
            throw std::invalid_argument( ss.str() );
        }
        if ( ( defIndex < (int)entries.size() ) && ( defIndex >= 0) ) {
            k->setDefaultValue(defIndex);
        }

        knob = k;
        break;
    }
    case AddKnobDialog::eParamDataTypeCheckbox: {
        KnobBoolPtr k = panel->getHolder()->createKnob<KnobBool>(name);
        bool defValue = defaultBool->isChecked();
        k->setDefaultValue(defValue);
        knob = k;
        break;
    }
    case AddKnobDialog::eParamDataTypeLabel:
    case AddKnobDialog::eParamDataTypeTextInput: {
        KnobStringPtr k = panel->getHolder()->createKnob<KnobString>(name);
        if ( multiLine->isChecked() ) {
            k->setAsMultiLine();
            if ( richText->isChecked() ) {
                k->setUsesRichText(true);
            }
        } else {
            if (t == AddKnobDialog::eParamDataTypeLabel) {
                k->setAsLabel();
            }
        }
        std::string defValue = defaultStr->text().toStdString();
        k->setDefaultValue(defValue);
        knob = k;
        break;
    }
    case AddKnobDialog::eParamDataTypeInputFile: {
        KnobFilePtr k = panel->getHolder()->createKnob<KnobFile>(name);

        KnobFile::KnobFileDialogTypeEnum dialogType;
        if ( fileDialogSelectExisting->isChecked()) {
            if ( sequenceDialog->isChecked() ) {
                dialogType = KnobFile::eKnobFileDialogTypeOpenFileSequences;
            } else {
                dialogType = KnobFile::eKnobFileDialogTypeOpenFile;
            }
        } else {
            if ( sequenceDialog->isChecked() ) {
                dialogType = KnobFile::eKnobFileDialogTypeSaveFileSequences;
            } else {
                dialogType = KnobFile::eKnobFileDialogTypeSaveFile;
            }
        }
        k->setDialogType(dialogType);

        std::string defValue = defaultStr->text().toStdString();
        k->setDefaultValue(defValue);
        knob = k;
        break;
    }
    case AddKnobDialog::eParamDataTypeDirectory: {
        KnobPathPtr k = panel->getHolder()->createKnob<KnobPath>(name);
        if ( multiPath->isChecked() ) {
            k->setMultiPath(true);
        }
        std::string defValue = defaultStr->text().toStdString();
        k->setDefaultValue(defValue);
        knob = k;
        break;
    }
    case AddKnobDialog::eParamDataTypeGroup: {
        KnobGroupPtr k = panel->getHolder()->createKnob<KnobGroup>(name);
        if ( groupAsTab->isChecked() ) {
            k->setAsTab();
        }
        k->setDefaultValue(true);     //< default to opened
        knob = k;
        break;
    }
    case AddKnobDialog::eParamDataTypePage: {
        KnobPagePtr k = panel->getHolder()->createKnob<KnobPage>(name);
        knob = k;
        break;
    }
    case AddKnobDialog::eParamDataTypeButton: {
        KnobButtonPtr k = panel->getHolder()->createKnob<KnobButton>(name);
        knob = k;
        break;
    }
    case AddKnobDialog::eParamDataTypeSeparator: {
        KnobSeparatorPtr k = panel->getHolder()->createKnob<KnobSeparator>(name);
        knob = k;
        break;
    }
    default:
        break;
    } // switch
    knob->setLabel(label);
    assert(knob);
    knob->setKnobDeclarationType(KnobI::eKnobDeclarationTypeUser);
    if ( knob->canAnimate() ) {
        knob->setAnimationEnabled( animatesCheckbox->isChecked() );
    }
    knob->setEvaluateOnChange( evaluatesOnChange->isChecked() );

    knob->setHintToolTip( tooltipArea->toPlainText().toStdString() );
    bool addedInGrp = false;
    KnobGroupPtr selectedGrp = getSelectedGroup();
    if (selectedGrp) {
        if (optionalGroupIndex != -1) {
            selectedGrp->insertKnob(optionalGroupIndex, knob);
        } else {
            selectedGrp->addKnob(knob);
        }
        addedInGrp = true;
    }


    if ( (t != AddKnobDialog::eParamDataTypePage) && parentPage && !addedInGrp ) {

        // Ensure the knob is in a page
        KnobPagePtr page = getSelectedPage();
        assert(page);
        if (page) {
            if (optionalGroupIndex != -1) {
                page->insertKnob(optionalGroupIndex, knob);
            } else {
                page->addKnob(knob);
            }
            panel->setPageActiveIndex(page);

        }
    }



    KnobHolderPtr holder = panel->getHolder();
    assert(holder);
    EffectInstancePtr isEffect = toEffectInstance(holder);
    assert(isEffect);
    if (isEffect) {
        isEffect->getNode()->declarePythonKnobs();
    }
} // AddKnobDialogPrivate::createKnobFromSelection

KnobGroupPtr
AddKnobDialogPrivate::getSelectedGroup() const
{
    if ( parentGroup && parentGroup->isVisible() ) {
        std::string selectedItem = parentGroup->getCurrentIndexText().toStdString();
        if (selectedItem != "-") {
            for (std::list<KnobGroupPtr>::const_iterator it = userGroups.begin(); it != userGroups.end(); ++it) {
                if ( (*it)->getName() == selectedItem ) {
                    return *it;
                }
            }
        }
    }

    return KnobGroupPtr();
}

KnobPagePtr
AddKnobDialogPrivate::getSelectedPage() const
{
    if ( parentPage && parentPage->isVisible() ) {
        std::string selectedItem = parentPage->getCurrentIndexText().toStdString();
        for (std::list<KnobPagePtr>::const_iterator it = userPages.begin(); it != userPages.end(); ++it) {
            if ( (*it)->getName() == selectedItem ) {
                return *it;
                break;
            }
        }
    }

    return KnobPagePtr();
}


struct ListenerExpr
{
    std::string expression;
    bool hasRetVar;
    ExpressionLanguageEnum lang;
};

void
AddKnobDialog::onOkClicked()
{
    QString name = _imp->nameLineEdit->text();
    bool badFormat = false;

    if ( name.isEmpty() ) {
        badFormat = true;
    }
    if ( !badFormat && !name[0].isLetter() ) {
        badFormat = true;
    }

    if (!badFormat) {
        //make sure everything is alphaNumeric without spaces
        for (int i = 0; i < name.size(); ++i) {
            if ( name[i] == QLatin1Char('_') ) {
                continue;
            }
            if ( ( name[i] == QLatin1Char(' ') ) || !name[i].isLetterOrNumber() ) {
                badFormat = true;
                break;
            }
        }
    }

    //check if another knob has the same script name
    std::string stdName = name.toStdString();
    const KnobsVec& knobs = _imp->panel->getHolder()->getKnobs();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        if ( ( (*it)->getName() == stdName ) && ( (*it) != _imp->knob ) ) {
            badFormat = true;
            break;
        }
    }

    if (badFormat) {
        Dialogs::errorDialog( tr("Error").toStdString(), tr("A parameter must have a unique script name composed only of characters from "
                                                            "[a - z / A- Z] and digits [0 - 9]. This name cannot contain spaces for scripting purposes.")
                              .toStdString() );

        return;
    }

    EffectInstancePtr effect;


    {
        KnobHolderPtr holder = _imp->panel->getHolder();
        assert(holder);

        NodeGroupPtr isHolderGroup = toNodeGroup( toEffectInstance(holder) );
        if (isHolderGroup) {
            //Check if the group has a node with the exact same script name as the param script name, in which case we error
            //otherwise the attribute on the python object would be overwritten
            NodesList nodes = isHolderGroup->getNodes();
            for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
                if ( (*it)->getScriptName() == stdName ) {
                    Dialogs::errorDialog( tr("Error").toStdString(), tr("A parameter on a group cannot have the same script-name as a node within "
                                                                        "the group for scripting purposes.")
                                          .toStdString() );

                    return;
                }
            }
        }
    }

    ///Remove the previous knob, and recreate it.

    ///Index of the type in the combo
    ParamDataTypeEnum t;

    ///Remember the old page in which to insert the knob
    KnobPagePtr oldParentPage;

    ///If the knob was in a group, we need to place it at the same index
    int oldIndexInParent = -1;
    int oldViewerIndex = -1;
    std::string oldKnobScriptName;


    std::map<KnobIPtr, std::vector<ListenerExpr> > listenersExpressions;
    KnobPagePtr oldKnobIsPage;
    bool wasNewLineActivated = true;
    if (!_imp->knob) {
        assert(_imp->typeChoice);
        t = (ParamDataTypeEnum)_imp->typeChoice->activeIndex();
    } else {



        oldKnobIsPage = toKnobPage(_imp->knob);
        oldKnobScriptName = _imp->knob->getName();
        effect = toEffectInstance( _imp->knob->getHolder() );
        oldViewerIndex = effect->getInViewerContextKnobIndex(_imp->knob);
        if (oldViewerIndex != -1) {
            effect->removeKnobViewerUI(_imp->knob);
        }
        oldParentPage = _imp->knob->getTopLevelPage();
        wasNewLineActivated = _imp->knob->isNewLineActivated();
        t = getChoiceIndexFromKnobType(_imp->knob);
        KnobIPtr parent = _imp->knob->getParentKnob();
        KnobGroupPtr isParentGrp = toKnobGroup(parent);
        if ( isParentGrp && ( isParentGrp == _imp->getSelectedGroup() ) ) {
            KnobsVec children = isParentGrp->getChildren();
            for (U32 i = 0; i < children.size(); ++i) {
                if (children[i] == _imp->knob) {
                    oldIndexInParent = i;
                    break;
                }
            }
        } else {
            KnobsVec children;
            if ( oldParentPage && ( oldParentPage == _imp->getSelectedPage() ) ) {
                children = oldParentPage->getChildren();
            }
            for (U32 i = 0; i < children.size(); ++i) {
                if (children[i] == _imp->knob) {
                    oldIndexInParent = i;
                    break;
                }
            }
        }

        //Since removing this knob will also remove all expressions from listeners, conserve them and try
        //to recover them afterwards
        KnobDimViewKeySet listeners;
        _imp->knob->getListeners(listeners, KnobI::eListenersTypeExpression);
        for (KnobDimViewKeySet::iterator it = listeners.begin(); it != listeners.end(); ++it) {
            KnobIPtr listener = it->knob.lock();
            if (!listener) {
                continue;
            }
            int nDims = listener->getNDimensions();
            std::vector<ListenerExpr> exprs(nDims);
            for (int d = 0; d < nDims; ++d) {
                ListenerExpr e;
                e.expression = listener->getExpression(DimIdx(d), ViewIdx(0));
                e.hasRetVar = listener->isExpressionUsingRetVariable(ViewIdx(0), DimIdx(d));
                e.lang = listener->getExpressionLanguage(ViewIdx(0), DimIdx(d));
                exprs[d] = e;
            }
            listenersExpressions[listener] = exprs;
        }

        if (!oldKnobIsPage) {
            _imp->panel->getHolder()->deleteKnob(_imp->knob, true);
            _imp->knob.reset();

        }
    } //if (!_imp->knob) {


    if (oldKnobIsPage) {
        try {
            oldKnobIsPage->setName( _imp->nameLineEdit->text().toStdString() );
            oldKnobIsPage->setLabel( _imp->labelLineEdit->text().toStdString() );
        } catch (const std::exception& e) {
            Dialogs::errorDialog( tr("Error while creating parameter").toStdString(), e.what() );

            return;
        }
    } else {
        try {
            _imp->createKnobFromSelection(t, oldIndexInParent );
        }   catch (const std::exception& e) {
            Dialogs::errorDialog( tr("Error while creating parameter").toStdString(), e.what() );

            return;
        }

        assert(_imp->knob);


        if (_imp->originalKnobSerialization) {
            _imp->knob->fromSerialization(*_imp->originalKnobSerialization);
        }

        KnobStringPtr isLabelKnob = toKnobString(_imp->knob);
        if ( isLabelKnob && isLabelKnob->isLabel() ) {
            ///Label knob only has a default value, but the "fromSerialization" function call above will keep the previous value,
            ///so we have to force a reset to the default value.
            isLabelKnob->resetToDefaultValue();
        }


    }

    //If startsNewLine is false, set the flag on the previous knob
    bool startNewLine = _imp->startNewLineBox->isChecked();
    KnobIPtr parentKnob = _imp->knob->getParentKnob();
    if (parentKnob) {
        KnobGroupPtr parentIsGrp = toKnobGroup(parentKnob);
        KnobPagePtr parentIsPage = toKnobPage(parentKnob);
        assert(parentIsGrp || parentIsPage);
        KnobsVec children;
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

    if (_imp->knob && _imp->hideBox->isVisible()) {
        _imp->knob->setSecret( _imp->hideBox->isChecked() );
    }


    //also refresh the new line flag for this knob if it was set
    if (_imp->knob && !wasNewLineActivated) {
        _imp->knob->setAddNewLine(false);
    }

    //Check if we need to add the knob to the viewer ui
    if (_imp->addToViewerUICheckbox->isChecked()) {
        _imp->panel->getHolder()->insertKnobToViewerUI(_imp->knob, oldViewerIndex);
    }

    //Recover listeners expressions
    for (std::map<KnobIPtr, std::vector<ListenerExpr>  >::iterator it = listenersExpressions.begin(); it != listenersExpressions.end(); ++it) {
        assert( it->first->getNDimensions() == (int)it->second.size() );
        for (int i = 0; i < it->first->getNDimensions(); ++i) {
            try {
                std::string expr;
                if ( oldKnobScriptName != _imp->knob->getName() ) {
                    //Change in expressions the script-name
                    QString estr = QString::fromUtf8( it->second[i].expression.c_str() );
                    estr.replace( QString::fromUtf8( oldKnobScriptName.c_str() ), QString::fromUtf8( _imp->knob->getName().c_str() ) );
                    expr = estr.toStdString();
                } else {
                    expr = it->second[i].expression;
                }
                it->first->setExpression(DimIdx(i), ViewIdx(0), expr, it->second[i].lang, it->second[i].hasRetVar, false);
            } catch (...) {
            }
        }
    }


    accept();
} // AddKnobDialog::onOkClicked

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
        AddKnobDialog::ParamDataTypeEnum t = (AddKnobDialog::ParamDataTypeEnum)typeChoice->activeIndex();

        if ( (t == AddKnobDialog::eParamDataTypeColorRGB) || (t == AddKnobDialog::eParamDataTypeColorRGBA) ) {
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
AddKnobDialog::setVisibleType(bool visible)
{
    _imp->typeChoice->setVisible(visible);
    _imp->typeLabel->setVisible(visible);
}

void
AddKnobDialog::setType(ParamDataTypeEnum type)
{
    _imp->typeChoice->setCurrentIndex((int)type);
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
AddKnobDialogPrivate::setVisibleViewerUi(bool visible)
{
    addToViewerUILabel->setVisible(visible);
    addToViewerUICheckbox->setVisible(visible);
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
AddKnobDialogPrivate::setVisibleFileStuff(bool visible)
{
    fileDialogSelectExisting->setVisible(visible);
    fileDialogSelectExistingLabel->setVisible(visible);
    sequenceDialogLabel->setVisible(visible);
    sequenceDialog->setVisible(visible);
    if (!knob) {
        sequenceDialog->setChecked(false);
        fileDialogSelectExisting->setChecked(false);
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
    if ( !userGroups.empty() ) {
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

        if ( (type == eDefaultValueTypeInt) || (type == eDefaultValueTypeDouble) ) {
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
} // AddKnobDialogPrivate::setVisibleDefaultValues

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_AddKnobDialog.cpp"
