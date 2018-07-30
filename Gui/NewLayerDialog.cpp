/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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

#include "NewLayerDialog.h"

#include <cfloat>
#include <algorithm> // min, max
#include <stdexcept>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QGridLayout>
#include <QHBoxLayout>
#include <QStyle>
#include <QColorDialog>
#include <QToolTip>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QApplication>
#include <QScrollArea>
GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QKeyEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QtCore/QDebug>
#include <QFontComboBox>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Image.h"
#include "Engine/KnobTypes.h"
#include "Engine/Lut.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"

#include "Gui/Button.h"
#include "Gui/ClickableLabel.h"
#include "Gui/ComboBox.h"
#include "Gui/CurveGui.h"
#include "Gui/DialogButtonBox.h"
#include "Gui/DockablePanel.h"
#include "Gui/GroupBoxLabel.h"
#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiMacros.h"
#include "Gui/KnobUndoCommand.h"
#include "Gui/Label.h"
#include "Gui/ProjectGui.h"
#include "Gui/ScaleSliderQWidget.h"
#include "Gui/SpinBox.h"
#include "Gui/TabGroup.h"

#include <ofxNatron.h>


NATRON_NAMESPACE_ENTER

using std::make_pair;

struct NewLayerDialogPrivate
{
    QGridLayout* mainLayout;
    Label* layerLabel;
    LineEdit* layerEdit;
    Label* numCompsLabel;
    SpinBox* numCompsBox;
    Label* rLabel;
    LineEdit* rEdit;
    Label* gLabel;
    LineEdit* gEdit;
    Label* bLabel;
    LineEdit* bEdit;
    Label* aLabel;
    LineEdit* aEdit;
    Button* setRgbaButton;
    DialogButtonBox* buttons;

    NewLayerDialogPrivate()
        : mainLayout(0)
        , layerLabel(0)
        , layerEdit(0)
        , numCompsLabel(0)
        , numCompsBox(0)
        , rLabel(0)
        , rEdit(0)
        , gLabel(0)
        , gEdit(0)
        , bLabel(0)
        , bEdit(0)
        , aLabel(0)
        , aEdit(0)
        , setRgbaButton(0)
        , buttons(0)
    {
    }
};

NewLayerDialog::NewLayerDialog(const ImagePlaneDesc& original,
                               QWidget* parent)
    : QDialog(parent)
    , _imp( new NewLayerDialogPrivate() )
{
    _imp->mainLayout = new QGridLayout(this);
    _imp->layerLabel = new Label(tr("Layer Name"), this);
    _imp->layerEdit = new LineEdit(this);

    _imp->numCompsLabel = new Label(tr("No. Channels"), this);
    _imp->numCompsBox = new SpinBox(this, SpinBox::eSpinBoxTypeInt);
    QObject::connect( _imp->numCompsBox, SIGNAL(valueChanged(double)), this, SLOT(onNumCompsChanged(double)) );
    _imp->numCompsBox->setMinimum(1);
    _imp->numCompsBox->setMaximum(4);
    _imp->numCompsBox->setValue(4);

    _imp->rLabel = new Label(tr("1st Channel"), this);
    _imp->rEdit = new LineEdit(this);
    _imp->gLabel = new Label(tr("2nd Channel"), this);
    _imp->gEdit = new LineEdit(this);
    _imp->bLabel = new Label(tr("3rd Channel"), this);
    _imp->bEdit = new LineEdit(this);
    _imp->aLabel = new Label(tr("4th Channel"), this);
    _imp->aEdit = new LineEdit(this);

    _imp->setRgbaButton = new Button(this);
    _imp->setRgbaButton->setText( tr("Set RGBA") );
    QObject::connect( _imp->setRgbaButton, SIGNAL(clicked(bool)), this, SLOT(onRGBAButtonClicked()) );

    _imp->buttons = new DialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    QObject::connect( _imp->buttons, SIGNAL(accepted()), this, SLOT(accept()) );
    QObject::connect( _imp->buttons, SIGNAL(rejected()), this, SLOT(reject()) );

    _imp->mainLayout->addWidget(_imp->layerLabel, 0, 0, 1, 1);
    _imp->mainLayout->addWidget(_imp->layerEdit, 0, 1, 1, 1);

    _imp->mainLayout->addWidget(_imp->numCompsLabel, 1, 0, 1, 1);
    _imp->mainLayout->addWidget(_imp->numCompsBox, 1, 1, 1, 1);

    _imp->mainLayout->addWidget(_imp->rLabel, 2, 0, 1, 1);
    _imp->mainLayout->addWidget(_imp->rEdit, 2, 1, 1, 1);

    _imp->mainLayout->addWidget(_imp->gLabel, 3, 0, 1, 1);
    _imp->mainLayout->addWidget(_imp->gEdit, 3, 1, 1, 1);


    _imp->mainLayout->addWidget(_imp->bLabel, 4, 0, 1, 1);
    _imp->mainLayout->addWidget(_imp->bEdit, 4, 1, 1, 1);


    _imp->mainLayout->addWidget(_imp->aLabel, 5, 0, 1, 1);
    _imp->mainLayout->addWidget(_imp->aEdit, 5, 1, 1, 1);

    _imp->mainLayout->addWidget(_imp->setRgbaButton, 6, 0, 1, 2);

    _imp->mainLayout->addWidget(_imp->buttons, 7, 0, 1, 2);

    if (original.getNumComponents() != 0) {
        _imp->layerEdit->setText( QString::fromUtf8( original.getPlaneLabel().c_str() ) );

        LineEdit* edits[4] = {_imp->rEdit, _imp->gEdit, _imp->bEdit, _imp->aEdit};
        Label* labels[4] = {_imp->rLabel, _imp->gLabel, _imp->bLabel, _imp->aLabel};
        const std::vector<std::string>& channels = original.getChannels();
        for (int i = 0; i < 4; ++i) {
            if ( i >= (int)channels.size() ) {
                edits[i]->setVisible(false);
                labels[i]->setVisible(false);
            } else {
                edits[i]->setText( QString::fromUtf8( channels[i].c_str() ) );
            }
        }
        _imp->numCompsBox->setValue( (double)channels.size() );
    }
}

NewLayerDialog::~NewLayerDialog()
{
}

void
NewLayerDialog::onNumCompsChanged(double value)
{
    if (value == 1) {
        _imp->rLabel->setVisible(false);
        _imp->rEdit->setVisible(false);
        _imp->gLabel->setVisible(false);
        _imp->gEdit->setVisible(false);
        _imp->bLabel->setVisible(false);
        _imp->bEdit->setVisible(false);
        _imp->aLabel->setVisible(true);
        _imp->aEdit->setVisible(true);
    } else if (value == 2) {
        _imp->rLabel->setVisible(true);
        _imp->rEdit->setVisible(true);
        _imp->gLabel->setVisible(true);
        _imp->gEdit->setVisible(true);
        _imp->bLabel->setVisible(false);
        _imp->bEdit->setVisible(false);
        _imp->aLabel->setVisible(false);
        _imp->aEdit->setVisible(false);
    } else if (value == 3) {
        _imp->rLabel->setVisible(true);
        _imp->rEdit->setVisible(true);
        _imp->gLabel->setVisible(true);
        _imp->gEdit->setVisible(true);
        _imp->bLabel->setVisible(true);
        _imp->bEdit->setVisible(true);
        _imp->aLabel->setVisible(false);
        _imp->aEdit->setVisible(false);
    } else if (value == 3) {
        _imp->rLabel->setVisible(true);
        _imp->rEdit->setVisible(true);
        _imp->gLabel->setVisible(true);
        _imp->gEdit->setVisible(true);
        _imp->bLabel->setVisible(true);
        _imp->bEdit->setVisible(true);
        _imp->aLabel->setVisible(true);
        _imp->aEdit->setVisible(true);
    }
}

ImagePlaneDesc
NewLayerDialog::getComponents() const
{
    QString layer = _imp->layerEdit->text();
    int nComps = (int)_imp->numCompsBox->value();
    QString r = _imp->rEdit->text();
    QString g = _imp->gEdit->text();
    QString b = _imp->bEdit->text();
    QString a = _imp->aEdit->text();
    std::string layerFixed = NATRON_PYTHON_NAMESPACE::makeNameScriptFriendlyWithDots( layer.toStdString() );
    std::string rFixed = NATRON_PYTHON_NAMESPACE::makeNameScriptFriendlyWithDots( r.toStdString() );
    std::string gFixed = NATRON_PYTHON_NAMESPACE::makeNameScriptFriendlyWithDots( g.toStdString() );
    std::string bFixed = NATRON_PYTHON_NAMESPACE::makeNameScriptFriendlyWithDots( b.toStdString() );
    std::string aFixed = NATRON_PYTHON_NAMESPACE::makeNameScriptFriendlyWithDots( a.toStdString() );

    if ( layerFixed.empty() ) {
        return ImagePlaneDesc::getNoneComponents();
    }

    if (nComps == 1) {
        if ( a.isEmpty() ) {
            return ImagePlaneDesc::getNoneComponents();
        }
        std::vector<std::string> comps;
        std::string compsGlobal;
        comps.push_back(aFixed);
        compsGlobal.append(aFixed);

        return ImagePlaneDesc(layerFixed, layerFixed, compsGlobal, comps);
    } else if (nComps == 2) {
        if ( rFixed.empty() || gFixed.empty() ) {
            return ImagePlaneDesc::getNoneComponents();
        }
        std::vector<std::string> comps;
        std::string compsGlobal;
        comps.push_back(rFixed);
        compsGlobal.append(rFixed);
        comps.push_back(gFixed);
        compsGlobal.append(gFixed);

        return ImagePlaneDesc(layerFixed, layerFixed, compsGlobal, comps);
    } else if (nComps == 3) {
        if ( rFixed.empty() || gFixed.empty() || bFixed.empty() ) {
            return ImagePlaneDesc::getNoneComponents();
        }
        std::vector<std::string> comps;
        std::string compsGlobal;
        comps.push_back(rFixed);
        compsGlobal.append(rFixed);
        comps.push_back(gFixed);
        compsGlobal.append(gFixed);
        comps.push_back(bFixed);
        compsGlobal.append(bFixed);

        return ImagePlaneDesc(layerFixed, layerFixed, compsGlobal, comps);
    } else if (nComps == 4) {
        if ( rFixed.empty() || gFixed.empty() || bFixed.empty() || aFixed.empty() ) {
            return ImagePlaneDesc::getNoneComponents();
        }
        std::vector<std::string> comps;
        std::string compsGlobal;
        comps.push_back(rFixed);
        compsGlobal.append(rFixed);
        comps.push_back(gFixed);
        compsGlobal.append(gFixed);
        comps.push_back(bFixed);
        compsGlobal.append(bFixed);
        comps.push_back(aFixed);
        compsGlobal.append(aFixed);

        return ImagePlaneDesc(layerFixed, layerFixed, compsGlobal, comps);
    }

    return ImagePlaneDesc::getNoneComponents();
} // NewLayerDialog::getComponents

void
NewLayerDialog::onRGBAButtonClicked()
{
    _imp->rEdit->setText( QString::fromUtf8("R") );
    _imp->gEdit->setText( QString::fromUtf8("G") );
    _imp->bEdit->setText( QString::fromUtf8("B") );
    _imp->aEdit->setText( QString::fromUtf8("A") );

    _imp->rLabel->setVisible(true);
    _imp->rEdit->setVisible(true);
    _imp->gLabel->setVisible(true);
    _imp->gEdit->setVisible(true);
    _imp->bLabel->setVisible(true);
    _imp->bEdit->setVisible(true);
    _imp->aLabel->setVisible(true);
    _imp->aEdit->setVisible(true);
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_NewLayerDialog.cpp"
