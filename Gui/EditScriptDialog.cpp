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

#include "EditScriptDialog.h"

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
#include <QtCore/QTimer>

GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QKeyEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QColorDialog>
#include <QGroupBox>
#include <QtGui/QVector4D>
#include <QStyleFactory>
#include <QCompleter>

#include "Global/GlobalDefines.h"

#include "Engine/Curve.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"
#include "Engine/LibraryBinary.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/Utils.h" // convertFromPlainText
#include "Engine/Variant.h"
#include "Engine/ViewerInstance.h"

#include "Gui/Button.h"
#include "Gui/ComboBox.h"
#include "Gui/CurveGui.h"
#include "Gui/CustomParamInteract.h"
#include "Gui/DialogButtonBox.h"
#include "Gui/DockablePanel.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/KnobGuiGroup.h"
#include "Gui/KnobGui.h"
#include "Gui/KnobGuiContainerHelper.h"
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
#include "Gui/ViewerTab.h"

NATRON_NAMESPACE_ENTER


struct EditScriptDialogPrivate
{
    EditScriptDialog* _publicInterface;
    Gui* gui;
    KnobGuiWPtr knobExpressionReceiver;
    DimSpec knobDimension;
    ViewSetSpec knobView;
    QVBoxLayout* mainLayout;
    Label* expressionLabel;
    InputScriptTextEdit* expressionEdit;
    QWidget* midButtonsContainer;
    QHBoxLayout* midButtonsLayout;
    Label* languageLabel;
    ComboBox* expressionType;
    Button* useRetButton;
    Label* resultLabel;
    OutputScriptTextEdit* resultEdit;
    DialogButtonBox* buttons;

    EditScriptDialogPrivate(EditScriptDialog* publicInterface,
                            Gui* gui,
                            const KnobGuiPtr& knobExpressionReceiver,
                            DimSpec knobDimension,
                            ViewSetSpec knobView)
    : _publicInterface(publicInterface)
    , gui(gui)
    , knobExpressionReceiver(knobExpressionReceiver)
    , knobDimension(knobDimension)
    , knobView(knobView)
    , mainLayout(0)
    , expressionLabel(0)
    , expressionEdit(0)
    , midButtonsContainer(0)
    , midButtonsLayout(0)
    , languageLabel(0)
    , expressionType(0)
    , useRetButton(0)
    , resultLabel(0)
    , resultEdit(0)
    , buttons(0)
    {
    }

    ExpressionLanguageEnum getSelectedLanguage() const
    {
        return expressionType->activeIndex() == 0 ? eExpressionLanguageExprTk : eExpressionLanguagePython;
    }

    void refreshHeaderLabel(ExpressionLanguageEnum language);

    void refreshVisibility(ExpressionLanguageEnum language);

    void refreshUIForLanguage();
};

EditScriptDialog::EditScriptDialog(Gui* gui,
                                   const KnobGuiPtr& knobExpressionReceiver,
                                   DimSpec knobDimension,
                                   ViewSetSpec knobView,
                                   QWidget* parent)
    : QDialog(parent)
    , _imp( new EditScriptDialogPrivate(this, gui, knobExpressionReceiver, knobDimension, knobView) )
{
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
}

void
EditScriptDialogPrivate::refreshHeaderLabel(ExpressionLanguageEnum language)
{



    QString labelHtml;
    switch (language) {
        case eExpressionLanguagePython: {
            labelHtml +=  EditScriptDialog::tr("%1 script:").arg( QString::fromUtf8("<b>Python</b>") ) + QString::fromUtf8("<br />");
        }   break;
        case eExpressionLanguageExprTk: {
            labelHtml +=  EditScriptDialog::tr("%1 script:").arg( QString::fromUtf8("<b>ExprTk</b>") ) + QString::fromUtf8("<br />");
        }   break;
    }

    labelHtml.append( QString::fromUtf8("<p>") +
                      EditScriptDialog::tr("Read the <a href=\"%1\">documentation</a> for more information on how to write expressions.").arg( QString::fromUtf8("http://natron.readthedocs.io/en/master/guide/compositing-exprs.html") ) +
                      QString::fromUtf8("</p>") );
    QKeySequence s(Qt::CTRL);
    labelHtml.append( QString::fromUtf8("<p>") +
                      EditScriptDialog::tr("Note that parameters can be referenced by dragging and dropping while holding %1 on their widget").arg( s.toString(QKeySequence::NativeText) ) +
                      QString::fromUtf8("</p>") );


    expressionLabel->setText(labelHtml);
}

void
EditScriptDialogPrivate::refreshUIForLanguage()
{
    ExpressionLanguageEnum language = getSelectedLanguage();
    refreshHeaderLabel(language);
    refreshVisibility(language);

    knobExpressionReceiver.lock()->getContainer()->getGui()->setEditExpressionDialogLanguage(language);
    knobExpressionReceiver.lock()->getContainer()->getGui()->setEditExpressionDialogLanguageValid(true);
}

void
EditScriptDialogPrivate::refreshVisibility(ExpressionLanguageEnum language)
{
    useRetButton->setVisible(language == eExpressionLanguagePython);
}

void
EditScriptDialog::create(ExpressionLanguageEnum language,
                         const QString& initialScript)
{
    setTitle();

    _imp->mainLayout = new QVBoxLayout(this);



    _imp->expressionLabel = new Label(QString(), this);
    _imp->expressionLabel->setOpenExternalLinks(true);
    _imp->mainLayout->addWidget(_imp->expressionLabel);

    _imp->expressionEdit = new InputScriptTextEdit(_imp->gui, _imp->knobExpressionReceiver.lock(), _imp->knobDimension, _imp->knobView, this);
    _imp->expressionEdit->setAcceptDrops(true);
    _imp->expressionEdit->setMouseTracking(true);
    _imp->mainLayout->addWidget(_imp->expressionEdit);
    _imp->expressionEdit->setPlainText(initialScript);

    _imp->midButtonsContainer = new QWidget(this);
    _imp->midButtonsLayout = new QHBoxLayout(_imp->midButtonsContainer);

    QString langTooltip = NATRON_NAMESPACE::convertFromPlainText(tr("Select the language used by this expression. ExprTk-based expressions are very simple and extremely fast expressions but a bit more constrained than "
                                                                    "Python-based expressions which allow all the flexibility of the Python A.P.I to the expense of being a lot more expensive to evaluate."), NATRON_NAMESPACE::WhiteSpaceNormal);

    _imp->languageLabel = new Label(tr("Language:"), _imp->midButtonsContainer);
    _imp->languageLabel->setToolTip(langTooltip);
    _imp->midButtonsLayout->addWidget(_imp->languageLabel);

    _imp->expressionType = new ComboBox(_imp->midButtonsContainer);
    _imp->expressionType->addItem(tr("ExprTk"));
    _imp->expressionType->addItem(tr("Python"));
    connect(_imp->expressionType, SIGNAL(currentIndexChanged(int)), this, SLOT(onLanguageCurrentIndexChanged()));
    _imp->expressionType->setCurrentIndex(language == eExpressionLanguagePython ? 1 : 0, false);
    _imp->expressionType->setToolTip(langTooltip);
    _imp->midButtonsLayout->addWidget(_imp->expressionType);

    {
        bool retVariable = hasRetVariable();
        _imp->useRetButton = new Button(tr("Multi-line"), _imp->midButtonsContainer);
        _imp->useRetButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("When checked the Python expression will be interpreted "
                                                                          "as series of statement. The return value should be then assigned to the "
                                                                          "\"ret\" variable. When unchecked the expression must not contain "
                                                                          "any new line character and the result will be interpreted from the "
                                                                          "interpretation of the single line."), NATRON_NAMESPACE::WhiteSpaceNormal) );
        _imp->useRetButton->setCheckable(true);
        bool checked = !initialScript.isEmpty() && retVariable;
        _imp->useRetButton->setChecked(checked);
        _imp->useRetButton->setDown(checked);
        QObject::connect( _imp->useRetButton, SIGNAL(clicked(bool)), this, SLOT(onUseRetButtonClicked(bool)) );
        _imp->midButtonsLayout->addWidget(_imp->useRetButton);
    }
    _imp->midButtonsLayout->addStretch();
    _imp->mainLayout->addWidget(_imp->midButtonsContainer);

    _imp->resultLabel = new Label(tr("Result:"), this);
    _imp->mainLayout->addWidget(_imp->resultLabel);

    _imp->resultEdit = new OutputScriptTextEdit(this);
    _imp->resultEdit->setFixedHeight( TO_DPIY(80) );
    _imp->resultEdit->setFocusPolicy(Qt::NoFocus);
    _imp->resultEdit->setReadOnly(true);
    _imp->mainLayout->addWidget(_imp->resultEdit);

    _imp->buttons = new DialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    _imp->mainLayout->addWidget(_imp->buttons);
    QObject::connect( _imp->buttons, SIGNAL(accepted()), this, SLOT(onAcceptedPressed()) );
    QObject::connect( _imp->buttons, SIGNAL(rejected()), this, SLOT(onRejectedPressed()) );

    if ( !initialScript.isEmpty() ) {
        compileAndSetResult(initialScript);
    }
    QObject::connect( _imp->expressionEdit, SIGNAL(textChanged()), this, SLOT(onTextEditChanged()) );
    _imp->expressionEdit->setFocus();

    QString fontFamily = QString::fromUtf8( appPTR->getCurrentSettings()->getSEFontFamily().c_str() );
    int fontSize = appPTR->getCurrentSettings()->getSEFontSize();
    QFont font(fontFamily, fontSize);
    if ( font.exactMatch() ) {
        _imp->expressionEdit->setFont(font);
        _imp->resultEdit->setFont(font);
    }
    QFontMetrics fm = _imp->expressionEdit->fontMetrics();
    _imp->expressionEdit->setTabStopWidth( 4 * fm.width( QLatin1Char(' ') ) );

    _imp->refreshUIForLanguage();
} // EditScriptDialog::create

void
EditScriptDialog::compileAndSetResult(const QString& script)
{
    QString ret = compileExpression(script, getSelectedLanguage());

    _imp->resultEdit->setPlainText(ret);
}


QString
EditScriptDialog::getScript() const
{
    return _imp->expressionEdit->toPlainText();

}

ExpressionLanguageEnum
EditScriptDialog::getSelectedLanguage() const
{
    return _imp->getSelectedLanguage();
}


bool
EditScriptDialog::isUseRetButtonChecked() const
{
    return _imp->useRetButton && _imp->useRetButton->isVisible() ? _imp->useRetButton->isChecked() : false;
}

void
EditScriptDialog::onTextEditChanged()
{
    compileAndSetResult( _imp->expressionEdit->toPlainText() );
}

void
EditScriptDialog::onUseRetButtonClicked(bool useRet)
{
    compileAndSetResult( _imp->expressionEdit->toPlainText() );
    _imp->useRetButton->setDown(useRet);
}

EditScriptDialog::~EditScriptDialog()
{
    KnobGuiPtr k = _imp->knobExpressionReceiver.lock();
    if (k) {
        k->getContainer()->getGui()->setEditExpressionDialogLanguageValid(false);
    }

}

void
EditScriptDialog::keyPressEvent(QKeyEvent* e)
{
    if ( (e->key() == Qt::Key_Return) || (e->key() == Qt::Key_Enter) ) {
        Q_EMIT dialogFinished(true);
    } else if (e->key() == Qt::Key_Escape) {
        Q_EMIT dialogFinished(false);
    } else {
        QDialog::keyPressEvent(e);
    }
}


void
EditScriptDialog::onLanguageCurrentIndexChanged()
{
    _imp->refreshUIForLanguage();
    compileAndSetResult( _imp->expressionEdit->toPlainText() );
}


NATRON_NAMESPACE_EXIT


NATRON_NAMESPACE_USING
#include "moc_EditScriptDialog.cpp"
