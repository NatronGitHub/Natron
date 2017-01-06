/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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

#include "EditNodeViewerContextDialog.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>

#include "Engine/KnobTypes.h"
#include "Engine/Utils.h"

#include "Gui/AnimatedCheckBox.h"
#include "Gui/ComboBox.h"
#include "Gui/Label.h"
#include "Gui/LineEdit.h"
#include "Gui/SpinBox.h"
#include "Gui/GuiApplicationManager.h"


NATRON_NAMESPACE_ENTER;

struct EditNodeViewerContextDialogPrivate
{
    KnobIPtr knob;

    QVBoxLayout* vLayout;
    QWidget* mainContainer;
    QFormLayout* mainLayout;

    Label* viewerUILabelLabel;
    LineEdit* viewerUILabelEdit;

    Label* checkedIconLabel;
    LineEdit* checkedIconLineEdit;

    Label* uncheckedIconLabel;
    LineEdit* uncheckedIconLineEdit;

    /*Label* addToShortcutEditorLabel;
    AnimatedCheckBox* addToShortcutEditorCheckbox;*/

    Label* layoutTypeLabel;
    ComboBox* layoutTypeChoice;
    SpinBox* itemSpacingSpinbox;

    Label* hiddenLabel;
    AnimatedCheckBox* hiddenCheckbox;

    EditNodeViewerContextDialogPrivate(const KnobIPtr& knob)
    : knob(knob)
    , vLayout(0)
    , mainContainer(0)
    , mainLayout(0)
    , viewerUILabelLabel(0)
    , viewerUILabelEdit(0)
    , checkedIconLabel(0)
    , checkedIconLineEdit(0)
    , uncheckedIconLabel(0)
    , uncheckedIconLineEdit(0)
    //, addToShortcutEditorLabel(0)
    //, addToShortcutEditorCheckbox(0)
    , layoutTypeLabel(0)
    , layoutTypeChoice(0)
    , itemSpacingSpinbox(0)
    , hiddenLabel(0)
    , hiddenCheckbox(0)
    {

    }
};

EditNodeViewerContextDialog::EditNodeViewerContextDialog(const KnobIPtr& knob, QWidget* parent)
: QDialog(parent)
, _imp(new EditNodeViewerContextDialogPrivate(knob))
{

    setWindowTitle(tr("Edit %1 viewer interface").arg(QString::fromUtf8(knob->getName().c_str())));

    _imp->vLayout = new QVBoxLayout(this);
    _imp->vLayout->setContentsMargins(0, 0, TO_DPIX(15), 0);

    _imp->mainContainer = new QWidget(this);
    _imp->mainLayout = new QFormLayout(_imp->mainContainer);
    _imp->mainLayout->setLabelAlignment(Qt::AlignVCenter | Qt::AlignRight);
    _imp->mainLayout->setFormAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    _imp->mainLayout->setSpacing(TO_DPIX(3));
    _imp->mainLayout->setContentsMargins(0, 0, TO_DPIX(15), 0);

    _imp->vLayout->addWidget(_imp->mainContainer);

    if (knob->isUserKnob()) {
        QWidget* rowContainer = new QWidget(this);
        QHBoxLayout* rowLayout = new QHBoxLayout(rowContainer);
        rowLayout->setContentsMargins(0, 0, 0, 0);

        _imp->viewerUILabelLabel = new Label(tr("Viewer Interface Label:"), this);
        _imp->viewerUILabelEdit = new LineEdit(rowContainer);
        QString labelTooltip = NATRON_NAMESPACE::convertFromPlainText(tr("The text label of the parameter that will appear in its viewer interface"), NATRON_NAMESPACE::WhiteSpaceNormal);
        _imp->viewerUILabelEdit->setToolTip(labelTooltip);
        _imp->viewerUILabelLabel->setToolTip(labelTooltip);

        if (knob) {
            _imp->viewerUILabelEdit->setText( QString::fromUtf8( knob->getInViewerContextLabel().c_str() ) );
        }
        rowLayout->addWidget(_imp->viewerUILabelEdit);
        rowLayout->addStretch();

        _imp->mainLayout->addRow(_imp->viewerUILabelLabel, rowContainer);
    }

    KnobButtonPtr isButtonKnob = toKnobButton(knob);

    if (knob->isUserKnob()) {
        QWidget* rowContainer = new QWidget(this);
        QHBoxLayout* rowLayout = new QHBoxLayout(rowContainer);
        rowLayout->setContentsMargins(0, 0, 0, 0);

        QString text;
        if (isButtonKnob) {
            text = tr("Button Checked Icon:");
        } else {
            text = tr("Icon label:");
        }
        QString tooltip;
        if (isButtonKnob) {
            tooltip = NATRON_NAMESPACE::convertFromPlainText(tr("The icon of the button when checked"), NATRON_NAMESPACE::WhiteSpaceNormal);
        } else {
            tooltip = NATRON_NAMESPACE::convertFromPlainText(tr("This icon will be used instead of the text label"), NATRON_NAMESPACE::WhiteSpaceNormal);
        }
        _imp->checkedIconLabel = new Label(text, this);
        _imp->checkedIconLineEdit = new LineEdit(rowContainer);
        _imp->checkedIconLineEdit->setToolTip(tooltip);
        _imp->checkedIconLabel->setToolTip(tooltip);

        _imp->checkedIconLineEdit->setText( QString::fromUtf8( knob->getInViewerContextIconFilePath(true).c_str() ) );

        rowLayout->addWidget(_imp->checkedIconLineEdit);
        rowLayout->addStretch();

        _imp->mainLayout->addRow(_imp->checkedIconLabel, rowContainer);
    }

    if (isButtonKnob && knob->isUserKnob()) {
        QWidget* rowContainer = new QWidget(this);
        QHBoxLayout* rowLayout = new QHBoxLayout(rowContainer);
        rowLayout->setContentsMargins(0, 0, 0, 0);

        QString text;
        text = tr("Button Unchecked Icon:");

        QString tooltip;
        tooltip = NATRON_NAMESPACE::convertFromPlainText(tr("The icon of the button when unchecked"), NATRON_NAMESPACE::WhiteSpaceNormal);

        _imp->uncheckedIconLabel = new Label(text, this);
        _imp->uncheckedIconLineEdit = new LineEdit(rowContainer);
        _imp->uncheckedIconLineEdit->setToolTip(tooltip);
        _imp->uncheckedIconLabel->setToolTip(tooltip);

        _imp->uncheckedIconLineEdit->setText( QString::fromUtf8( knob->getInViewerContextIconFilePath(false).c_str() ) );

        rowLayout->addWidget(_imp->uncheckedIconLineEdit);
        rowLayout->addStretch();

        _imp->mainLayout->addRow(_imp->uncheckedIconLabel, rowContainer);
    }

    {
        QWidget* rowContainer = new QWidget(this);
        QHBoxLayout* rowLayout = new QHBoxLayout(rowContainer);
        rowLayout->setContentsMargins(0, 0, 0, 0);

        QString text = tr("Layout Type:");
        QString tooltip = NATRON_NAMESPACE::convertFromPlainText(tr("The layout type for this parameter"), NATRON_NAMESPACE::WhiteSpaceNormal);
        _imp->layoutTypeLabel = new Label(text, this);
        _imp->layoutTypeChoice = new ComboBox(rowContainer);
        _imp->layoutTypeLabel->setToolTip(tooltip);
        _imp->layoutTypeChoice->setToolTip(tooltip);
        _imp->layoutTypeChoice->addItem(tr("Spacing (px)"), QIcon(), QKeySequence(), tr("The spacing in pixels to add after the parameter"));
        _imp->layoutTypeChoice->addItem(tr("Separator"), QIcon(), QKeySequence(), tr("A vertical line will be added after the parameter"));
        _imp->layoutTypeChoice->addItem(tr("Stretch After"), QIcon(), QKeySequence(), tr("The layout will be stretched between this parameter and the next"));
        _imp->layoutTypeChoice->addItem(tr("New Line"), QIcon(), QKeySequence(), tr("A new line will be added after this parameter"));

        ViewerContextLayoutTypeEnum type = knob->getInViewerContextLayoutType();
        _imp->layoutTypeChoice->setCurrentIndex_no_emit((int)type);

        QObject::connect(_imp->layoutTypeChoice, SIGNAL(currentIndexChanged(int)), this, SLOT(onLayoutTypeChoiceChanged(int)));
        rowLayout->addWidget(_imp->layoutTypeChoice);

        _imp->itemSpacingSpinbox = new SpinBox(rowContainer, SpinBox::eSpinBoxTypeInt);
        _imp->itemSpacingSpinbox->setMinimum(0);
        _imp->itemSpacingSpinbox->setValue(_imp->knob->getInViewerContextItemSpacing());
        _imp->itemSpacingSpinbox->setVisible(_imp->layoutTypeChoice->activeIndex() == 0);
        _imp->itemSpacingSpinbox->setToolTip(NATRON_NAMESPACE::convertFromPlainText(tr("The spacing in pixels to add after the parameter"), NATRON_NAMESPACE::WhiteSpaceNormal));
        rowLayout->addWidget(_imp->itemSpacingSpinbox);
        rowLayout->addStretch();

        _imp->mainLayout->addRow(_imp->layoutTypeLabel, rowContainer);
    }

    {
        QWidget* rowContainer = new QWidget(this);
        QHBoxLayout* rowLayout = new QHBoxLayout(rowContainer);
        rowLayout->setContentsMargins(0, 0, 0, 0);

        QString text = tr("Hidden:");
        QString tooltip = NATRON_NAMESPACE::convertFromPlainText(tr("When checked, the parameter will be hidden from the viewer interface"), NATRON_NAMESPACE::WhiteSpaceNormal);
        _imp->hiddenLabel = new Label(text, this);
        _imp->hiddenCheckbox = new AnimatedCheckBox(rowContainer);
        _imp->hiddenLabel->setToolTip(tooltip);
        _imp->hiddenCheckbox->setToolTip(tooltip);
        _imp->hiddenCheckbox->setChecked(knob->getInViewerContextSecret());
        rowLayout->addWidget(_imp->hiddenCheckbox);
        rowLayout->addStretch();

        _imp->mainLayout->addRow(_imp->hiddenLabel, rowContainer);
    }

   /* if (isButtonKnob) {
        QWidget* rowContainer = new QWidget(this);
        QHBoxLayout* rowLayout = new QHBoxLayout(rowContainer);
        rowLayout->setContentsMargins(0, 0, 0, 0);

        QString text = tr("Add to Shortcut Editor:");
        QString tooltip = GuiUtils::convertFromPlainText(tr("When checked, the parameter can be attributed a shortcut from the Shortcut Editor"), Qt::WhiteSpaceNormal);
        _imp->addToShortcutEditorLabel = new Label(text, this);
        _imp->addToShortcutEditorCheckbox = new AnimatedCheckBox(rowContainer);
        _imp->addToShortcutEditorLabel->setToolTip(tooltip);
        _imp->addToShortcutEditorCheckbox->setToolTip(tooltip);
        _imp->addToShortcutEditorCheckbox->setChecked(knob->getInViewerContextHasShortcut());
        rowLayout->addWidget(_imp->addToShortcutEditorCheckbox);
        rowLayout->addStretch();

        _imp->mainLayout->addRow(_imp->addToShortcutEditorLabel, rowContainer);
    }*/


    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::StandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel), Qt::Horizontal, this);
    QObject::connect( buttons, SIGNAL(rejected()), this, SLOT(reject()) );
    QObject::connect( buttons, SIGNAL(accepted()), this, SLOT(onOkClicked()) );
    _imp->vLayout->addWidget(buttons);
}

EditNodeViewerContextDialog::~EditNodeViewerContextDialog()
{
    
}

void
EditNodeViewerContextDialog::onLayoutTypeChoiceChanged(int index)
{
    _imp->itemSpacingSpinbox->setVisible(index == 0);
}

void
EditNodeViewerContextDialog::onOkClicked()
{
    int layoutType_i = _imp->layoutTypeChoice->activeIndex();
    ViewerContextLayoutTypeEnum type = (ViewerContextLayoutTypeEnum)layoutType_i;
    switch (type) {
        case eViewerContextLayoutTypeAddNewLine:
            _imp->knob->setInViewerContextLayoutType(eViewerContextLayoutTypeAddNewLine);
            break;
        case eViewerContextLayoutTypeSeparator:
            _imp->knob->setInViewerContextLayoutType(eViewerContextLayoutTypeSeparator);
            break;
        case eViewerContextLayoutTypeStretchAfter:
            _imp->knob->setInViewerContextLayoutType(eViewerContextLayoutTypeStretchAfter);
            break;
        case eViewerContextLayoutTypeSpacing:
            _imp->knob->setInViewerContextItemSpacing(_imp->itemSpacingSpinbox->value());
            _imp->knob->setInViewerContextLayoutType(eViewerContextLayoutTypeSpacing);
            break;

    }
    if (_imp->viewerUILabelEdit) {
        _imp->knob->setInViewerContextLabel(_imp->viewerUILabelEdit->text());
    }
    //_imp->knob->setInViewerContextCanHaveShortcut(_imp->addToShortcutEditorCheckbox->isChecked());

    _imp->knob->setInViewerContextSecret(_imp->hiddenCheckbox->isChecked());
    if (_imp->checkedIconLineEdit) {
        _imp->knob->setInViewerContextIconFilePath(_imp->checkedIconLineEdit->text().toStdString(),true);
    }
    if (_imp->uncheckedIconLineEdit) {
        _imp->knob->setInViewerContextIconFilePath(_imp->uncheckedIconLineEdit->text().toStdString(),false);
    }

    accept();
}

NATRON_NAMESPACE_EXIT;
NATRON_NAMESPACE_USING;
#include "moc_EditNodeViewerContextDialog.cpp"
