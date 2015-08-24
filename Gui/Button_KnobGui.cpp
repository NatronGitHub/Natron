//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Button_KnobGui.h"

#include <cfloat>
#include <algorithm> // min, max

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
#include <QDebug>
#include <QFontComboBox>
#include <QDialogButtonBox>
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
#include "Gui/DockablePanel.h"
#include "Gui/GroupBoxLabel.h"
#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiMacros.h"
#include "Gui/KnobUndoCommand.h"
#include "Gui/Label.h"
#include "Gui/NewLayerDialog.h"
#include "Gui/ProjectGui.h"
#include "Gui/ScaleSliderQWidget.h"
#include "Gui/SpinBox.h"
#include "Gui/TabGroup.h"
#include "Gui/Utils.h"

#include "ofxNatron.h"

using namespace Natron;
using std::make_pair;

//=============================BUTTON_KNOB_GUI===================================

Button_KnobGui::Button_KnobGui(boost::shared_ptr<KnobI> knob,
                               DockablePanel *container)
    : KnobGui(knob, container)
      , _button(0)
{
    _knob = boost::dynamic_pointer_cast<Button_Knob>(knob);
}

void
Button_KnobGui::createWidget(QHBoxLayout* layout)
{
    boost::shared_ptr<Button_Knob> knob = _knob.lock();
    QString label( knob->getDescription().c_str() );
    const std::string & iconFilePath = knob->getIconFilePath();
    
    QString filePath(iconFilePath.c_str());
    if (!iconFilePath.empty() && !QFile::exists(filePath)) {
        ///Search all natron paths for a file
        
        QStringList paths = appPTR->getAllNonOFXPluginsPaths();
        for (int i = 0; i < paths.size(); ++i) {
            QString tmp = paths[i] + QChar('/') + filePath;
            if (QFile::exists(tmp)) {
                filePath = tmp;
                break;
            }
        }
    }
    
    QPixmap pix;

    if (pix.load(filePath)) {
        _button = new Button( QIcon(pix),"",layout->parentWidget() );
        _button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    } else {
        _button = new Button( label,layout->parentWidget() );
    }
    QObject::connect( _button, SIGNAL(clicked()), this, SLOT(emitValueChanged()));
    if ( hasToolTip() ) {
        _button->setToolTip( toolTip() );
    }
    layout->addWidget(_button);
}

Button_KnobGui::~Button_KnobGui()
{
}

void Button_KnobGui::removeSpecificGui()
{
    delete _button;
}

void
Button_KnobGui::emitValueChanged()
{
   _knob.lock()->evaluateValueChange(0, Natron::eValueChangedReasonUserEdited);
}

void
Button_KnobGui::_hide()
{
    _button->hide();
}

void
Button_KnobGui::_show()
{
    _button->show();
}

void
Button_KnobGui::setEnabled()
{
    boost::shared_ptr<Button_Knob> knob = _knob.lock();
    bool b = knob->isEnabled(0)  && !knob->isSlave(0) && knob->getExpression(0).empty();

    _button->setEnabled(b);
}

void
Button_KnobGui::setReadOnly(bool readOnly,
                            int /*dimension*/)
{
    _button->setEnabled(!readOnly);
}

boost::shared_ptr<KnobI> Button_KnobGui::getKnob() const
{
    return _knob.lock();
}

