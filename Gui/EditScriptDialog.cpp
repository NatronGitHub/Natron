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

#include "Gui/EditScriptDialog.h"

#include <cassert>
#include <climits>
#include <cfloat>
#include <stdexcept>

#include <boost/weak_ptr.hpp>


#include <QtCore/QString>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFormLayout>
#include <QFileDialog>
#include <QTextEdit>
#include <QStyle> // in QtGui on Qt4, in QtWidgets on Qt5
#include <QTimer>

GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QKeyEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QColorDialog>
#include <QGroupBox>
#include <QtGui/QVector4D>
#include <QStyleFactory>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QCompleter>

#include "Global/GlobalDefines.h"

#include "Engine/Curve.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobSerialization.h"
#include "Engine/KnobTypes.h"
#include "Engine/LibraryBinary.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/Variant.h"
#include "Engine/ViewerInstance.h"

#include "Gui/AnimationButton.h"
#include "Gui/ComboBox.h"
#include "Gui/CurveEditor.h"
#include "Gui/CurveGui.h"
#include "Gui/CustomParamInteract.h"
#include "Gui/DockablePanel.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/KnobGuiGroup.h"
#include "Gui/Label.h"
#include "Gui/LineEdit.h"
#include "Gui/Menu.h"
#include "Gui/Menu.h"
#include "Gui/NodeCreationDialog.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/ScriptTextEdit.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/SpinBox.h"
#include "Gui/TabWidget.h"
#include "Gui/TimeLineGui.h"
#include "Gui/Utils.h"
#include "Gui/ViewerTab.h"

NATRON_NAMESPACE_USING


struct NATRON_NAMESPACE::EditScriptDialogPrivate
{
    QVBoxLayout* mainLayout;
    
    Natron::Label* expressionLabel;
    InputScriptTextEdit* expressionEdit;
    
    QWidget* midButtonsContainer;
    QHBoxLayout* midButtonsLayout;
    
    Button* useRetButton;
    Button* helpButton;
    
    Natron::Label* resultLabel;
    OutputScriptTextEdit* resultEdit;
    
    QDialogButtonBox* buttons;
    
    EditScriptDialogPrivate()
    : mainLayout(0)
    , expressionLabel(0)
    , expressionEdit(0)
    , midButtonsContainer(0)
    , midButtonsLayout(0)
    , useRetButton(0)
    , helpButton(0)
    , resultLabel(0)
    , resultEdit(0)
    , buttons(0)
    {
        
    }
};

EditScriptDialog::EditScriptDialog(QWidget* parent)
: QDialog(parent)
, _imp(new EditScriptDialogPrivate())
{
    
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
}

void
EditScriptDialog::create(const QString& initialScript,bool makeUseRetButton)
{
    setTitle();
    
    QFont font(appFont,appFontSize);
    _imp->mainLayout = new QVBoxLayout(this);
    
    QStringList modules;
    getImportedModules(modules);
    std::list<std::pair<QString,QString> > variables;
    getDeclaredVariables(variables);
    QString labelHtml(tr("<br><b>Python</b> script: </br>"));
    if (!modules.empty()) {
        labelHtml.append(tr("<br>For convenience the following module(s) have been imported: </br> <br/>"));
        for (int i = 0; i < modules.size(); ++i) {
            QString toAppend = QString("<br><i><font color=orange>from %1 import *</font></i></br>").arg(modules[i]);
            labelHtml.append(toAppend);
        }
        labelHtml.append("<br/>");
    }
    if (!variables.empty()) {
        labelHtml.append(tr("<br>Also the following variables have been declared: </br>"
                         "<br/>"));
        for (std::list<std::pair<QString,QString> > ::iterator it = variables.begin(); it != variables.end(); ++it) {
            QString toAppend = QString("<br><b>%1</b>: %2</br>").arg(it->first).arg(it->second);
            labelHtml.append(toAppend);
        }
        labelHtml.append("<p>" + tr("Note that parameters can be referenced by drag&dropping them from their animation button.") + "</p>");
    }
    
    _imp->expressionLabel = new Natron::Label(labelHtml,this);
    //_imp->expressionLabel->setFont(font);
    _imp->mainLayout->addWidget(_imp->expressionLabel);
    
    _imp->expressionEdit = new InputScriptTextEdit(this);
    _imp->expressionEdit->setAcceptDrops(true);
    _imp->expressionEdit->setMouseTracking(true);
    QFontMetrics fm = _imp->expressionEdit->fontMetrics();
    _imp->expressionEdit->setTabStopWidth(4 * fm.width(' '));
    _imp->mainLayout->addWidget(_imp->expressionEdit);
    _imp->expressionEdit->setPlainText(initialScript);
    
    _imp->midButtonsContainer = new QWidget(this);
    _imp->midButtonsLayout = new QHBoxLayout(_imp->midButtonsContainer);

    
    if (makeUseRetButton) {
        
        bool retVariable = hasRetVariable();
        _imp->useRetButton = new Button(tr("Multi-line"),_imp->midButtonsContainer);
        _imp->useRetButton->setToolTip(Natron::convertFromPlainText(tr("When checked the Python expression will be interpreted "
                                                                   "as series of statement. The return value should be then assigned to the "
                                                                   "\"ret\" variable. When unchecked the expression must not contain "
                                                                   "any new line character and the result will be interpreted from the "
                                                                   "interpretation of the single line."), Qt::WhiteSpaceNormal));
        _imp->useRetButton->setCheckable(true);
        bool checked = !initialScript.isEmpty() && retVariable;
        _imp->useRetButton->setChecked(checked);
        _imp->useRetButton->setDown(checked);
        QObject::connect(_imp->useRetButton, SIGNAL(clicked(bool)), this, SLOT(onUseRetButtonClicked(bool)));
        _imp->midButtonsLayout->addWidget(_imp->useRetButton);

    }
    
    
    _imp->helpButton = new Button(tr("Help"),_imp->midButtonsContainer);
    QObject::connect(_imp->helpButton, SIGNAL(clicked(bool)), this, SLOT(onHelpRequested()));
    _imp->midButtonsLayout->addWidget(_imp->helpButton);
    _imp->midButtonsLayout->addStretch();
    
    _imp->mainLayout->addWidget(_imp->midButtonsContainer);
    
    _imp->resultLabel = new Natron::Label(tr("Result:"),this);
    //_imp->resultLabel->setFont(font);
    _imp->mainLayout->addWidget(_imp->resultLabel);
    
    _imp->resultEdit = new OutputScriptTextEdit(this);
    _imp->resultEdit->setFixedHeight(TO_DPIY(80));
    _imp->resultEdit->setReadOnly(true);
    _imp->mainLayout->addWidget(_imp->resultEdit);
    
    _imp->buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,Qt::Horizontal,this);
    _imp->mainLayout->addWidget(_imp->buttons);
    QObject::connect(_imp->buttons,SIGNAL(accepted()),this,SLOT(accept()));
    QObject::connect(_imp->buttons,SIGNAL(rejected()),this,SLOT(reject()));
    
    if (!initialScript.isEmpty()) {
        compileAndSetResult(initialScript);
    }
    QObject::connect(_imp->expressionEdit, SIGNAL(textChanged()), this, SLOT(onTextEditChanged()));
    _imp->expressionEdit->setFocus();
}


void
EditScriptDialog::compileAndSetResult(const QString& script)
{
    QString ret = compileExpression(script);
    _imp->resultEdit->setPlainText(ret);
}

QString
EditScriptDialog::getHelpPart1()
{
    return tr("<br>Each node in the scope already has a variable declared with its name, e.g if you have a node named "
              "<b>Transform1</b> in your project, then you can type <i>Transform1</i> to reference that node.</br>"
              "<br>Note that the scope includes all nodes within the same group as thisNode and the parent group node itself, "
              "if the node belongs to a group. If the node itself is a group, then it can also have expressions depending "
              "on parameters of its children.</br>"
              "<br/>"
              "<br>Each node has all its parameters declared as fields and you can reference a specific parameter by typing it's <b>script name</b>, e.g:</br>"
              "<br>Transform1.rotate</br>"
              "<br>The script-name of a parameter is the name in bold that is shown in the tooltip when hovering a parameter with the mouse, this is what "
              "identifies a parameter internally.</br>");
}

QString
EditScriptDialog::getHelpThisNodeVariable()
{
    return tr("<br>The current node which expression is being edited can be referenced by the variable <i>thisNode</i> for convenience.</br>");
}

QString
EditScriptDialog::getHelpThisGroupVariable()
{
     return tr("<br>The parent group containing the thisNode can be referenced by the variable <i>thisGroup</i> for convenience, if and "
               "only if thisNode belongs to a group.</br>");
}

QString
EditScriptDialog::getHelpThisParamVariable()
{
    return tr("<br>The <i>thisParam</i> variable has been defined for convenience when editing an expression. It refers to the current parameter.</br>");
}

QString
EditScriptDialog::getHelpDimensionVariable()
{
    return tr("<br>In the same way the <i>dimension</i> variable has been defined and references the current dimension of the parameter which expression is being set"
              ".</br>"
              "<br>The <i>dimension</i> is a 0-based index identifying a specific field of a parameter. For instance if we're editing the expression of the y "
              "field of the translate parameter of Transform1, the <i>dimension</i> would be 1. </br>");

}

QString
EditScriptDialog::getHelpPart2()
{
    return tr("<br>To access values of a parameter several functions are made accessible: </br>"
              "<br/>"
              "<br>The <b>get()</b> function will return a Tuple containing all the values for each dimension of the parameter. For instance "
              "let's say we have a node Transform1 in our comp, we could then reference the x value of the <i>center</i> parameter this way:</br>"
              "<br/>"
              "<br>Transform1.center.get().x</br>"
              "<br/>"
              "<br>The <b>get(</b><i>frame</i><b>)</b> works exactly like the <b>get()</b> function excepts that it takes an extra "
              "<i>frame</i> parameter corresponding to the time at which we want to fetch the value. For parameters with an animation "
              "it would then return their value at the corresponding timeline position. That value would then be either interpolated "
              "with the current interpolation filter, or the exact keyframe at that time if one exists.</br>");
}

void
EditScriptDialog::onHelpRequested()
{
    QString help = getCustomHelp();
    Natron::informationDialog(tr("Help").toStdString(), help.toStdString(),true);
}


QString
EditScriptDialog::getExpression(bool* hasRetVariable) const
{
    if (hasRetVariable) {
        *hasRetVariable = _imp->useRetButton ? _imp->useRetButton->isChecked() : false;
    }
    return _imp->expressionEdit->toPlainText();
}

bool
EditScriptDialog::isUseRetButtonChecked() const
{
    return _imp->useRetButton->isChecked();
}

void
EditScriptDialog::onTextEditChanged()
{
    compileAndSetResult(_imp->expressionEdit->toPlainText());
}

void
EditScriptDialog::onUseRetButtonClicked(bool useRet)
{
    compileAndSetResult(_imp->expressionEdit->toPlainText());
    _imp->useRetButton->setDown(useRet);
}

EditScriptDialog::~EditScriptDialog()
{
    
}

void
EditScriptDialog::keyPressEvent(QKeyEvent* e)
{
    if ( (e->key() == Qt::Key_Return) || (e->key() == Qt::Key_Enter) ) {
        accept();
    } else if (e->key() == Qt::Key_Escape) {
        reject();
    } else {
        QDialog::keyPressEvent(e);
    }
    
}

#include "moc_EditScriptDialog.cpp"
