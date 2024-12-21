/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "MessageBox.h"

#include <stdexcept>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtGlobal> // for Q_OS_*
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStyle>
#include <QPushButton>
#include <QCheckBox>
#include <QTextEdit>
#include <QApplication>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/DialogButtonBox.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/Label.h"
#include "Gui/QtEnumConvert.h"

NATRON_NAMESPACE_ENTER

struct MessageBoxPrivate
{
    MessageBox::MessageBoxTypeEnum type;
    QGridLayout* grid;
    Label* iconLabel;
    Label* label;
    QTextEdit* infoEdit; //< used if the text is too long so the user can scroll
    QCheckBox* checkbox;
    DialogButtonBox* buttonBox;
    QAbstractButton* clickedButton;

    MessageBoxPrivate(MessageBox::MessageBoxTypeEnum type)
        : type(type)
        , grid(0)
        , iconLabel(0)
        , label(0)
        , infoEdit(0)
        , checkbox(0)
        , buttonBox(0)
        , clickedButton(0)
    {
    }

    int layoutMinimumWidth()
    {
        grid->activate();

        return grid->totalMinimumSize().width();
    }
};

MessageBox::MessageBox(const QString & title,
                       const QString & message,
                       MessageBoxTypeEnum type,
                       const StandardButtons& buttons,
                       StandardButtonEnum defaultButton,
                       QWidget* parent)
    : QDialog(parent, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint)
    , _imp( new MessageBoxPrivate(type) )
{
    init(title, message, buttons, defaultButton);
}

void
MessageBox::init(const QString & title,
                 const QString & message,
                 const StandardButtons& buttons,
                 StandardButtonEnum defaultButton)
{
    setMinimumSize(400, 150);
    if (message.size() < 1000) {
        _imp->label = new Label(message);
        _imp->label->setTextInteractionFlags( Qt::TextInteractionFlags( style()->styleHint(QStyle::SH_MessageBox_TextInteractionFlags, 0, this) ) );
        _imp->label->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        _imp->label->setOpenExternalLinks(true);
        _imp->label->setWordWrap(true);
#if defined(Q_WS_MAC)
        _imp->label->setContentsMargins(16, 0, 0, 0);
#elif !defined(Q_WS_QWS)
        _imp->label->setContentsMargins(2, 0, 0, 0);
        _imp->label->setIndent(9);
#endif
        //QFont f(appFont,appFontSize);
        //_imp->label->setFont(f);
    } else {
        setSizeGripEnabled(true);
        _imp->infoEdit = new QTextEdit;
        _imp->infoEdit->setFocusPolicy(Qt::NoFocus);
        _imp->infoEdit->setReadOnly(true);
        _imp->infoEdit->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        _imp->infoEdit->setWordWrapMode(QTextOption::WordWrap);
        if ( Qt::mightBeRichText(message) ) {
            _imp->infoEdit->setHtml(message);
        } else {
            _imp->infoEdit->setPlainText(message);
        }
        //QFont f(appFont,appFontSize);
        //_imp->infoEdit->setFont(f);
    }

    _imp->iconLabel = new Label;
    _imp->iconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QStyle::StandardPixmap pixType = QStyle::SP_MessageBoxCritical;
    switch (_imp->type) {
        case eMessageBoxTypeError:
            pixType = QStyle::SP_MessageBoxCritical;
            break;
        case eMessageBoxTypeWarning:
            pixType = QStyle::SP_MessageBoxWarning;
            break;
        case eMessageBoxTypeInformation:
            pixType = QStyle::SP_MessageBoxInformation;
            break;
        case eMessageBoxTypeQuestion:
            pixType = QStyle::SP_MessageBoxQuestion;
            break;
    }

    QIcon icon = style()->standardIcon(pixType, 0, this);
    int iconSize = style()->pixelMetric(QStyle::PM_MessageBoxIconSize, 0, this);
    _imp->iconLabel->setPixmap( icon.pixmap(iconSize, iconSize) );

    _imp->buttonBox = new DialogButtonBox(QDialogButtonBox::StandardButtons( int( QtEnumConvert::toQtStandarButtons(buttons) ) ),
                                          Qt::Horizontal);
    QPushButton* defaultB = _imp->buttonBox->button( QDialogButtonBox::StandardButton(
                                                                                      (QDialogButtonBox::StandardButton)QtEnumConvert::toQtStandardButton(defaultButton) ) );
    if ( _imp->buttonBox->buttons().contains(defaultB) ) {
        defaultB->setDefault(true);
        defaultB->setFocus();
    }
    _imp->buttonBox->setCenterButtons( style()->styleHint(QStyle::SH_MessageBox_CenterButtons, 0, this) );
    QObject::connect( _imp->buttonBox, SIGNAL(clicked(QAbstractButton*)),
                     this, SLOT(onButtonClicked(QAbstractButton*)) );


    QGridLayout *grid = new QGridLayout;
#ifndef Q_WS_MAC
#ifdef Q_WS_S60
    const int preferredIconColumn = (QApplication::layoutDirection() == Qt::LeftToRight) ? 1 : 0;
    const int preferredTextColumn = (QApplication::layoutDirection() == Qt::LeftToRight) ? 0 : 1;
#else
    const int preferredIconColumn = 0;
    const int preferredTextColumn = 1;
#endif
    grid->addWidget(_imp->iconLabel, 0, preferredIconColumn, 2, 1, Qt::AlignTop);
    if (_imp->label) {
        grid->addWidget(_imp->label, 0, preferredTextColumn, 1, 1);
    } else if (_imp->infoEdit) {
        grid->addWidget(_imp->infoEdit, 0, preferredTextColumn, 1, 1);
    }
    // -- leave space for information label --
    grid->addWidget(_imp->buttonBox, 2, 0, 1, 2);
#else
    grid->setMargin(0);
    grid->setVerticalSpacing(8);
    grid->setHorizontalSpacing(0);
    setContentsMargins(24, 15, 24, 20);
    grid->addWidget(_imp->iconLabel, 0, 0, 2, 1, Qt::AlignTop | Qt::AlignLeft);
    if (_imp->label) {
        grid->addWidget(_imp->label, 0, 1, 1, 1);
    } else if (_imp->infoEdit) {
        grid->addWidget(_imp->infoEdit, 0, 1, 1, 1);
    }
    // -- leave space for information label --
    grid->setRowStretch(1, 100);
    grid->setRowMinimumHeight(2, 6);
    grid->addWidget(_imp->buttonBox, 3, 1, 1, 1);
#endif

    grid->setSizeConstraint(QLayout::SetNoConstraint);
    setLayout(grid);

    setWindowTitle(title);
    setModal(true);
#ifdef Q_WS_MAC
    QFont f = font();
    f.setBold(true);
    if (_imp->label) {
        _imp->label->setFont(f);
    } else if (_imp->infoEdit) {
        _imp->infoEdit->setFont(f);
    }
#endif
    //retranslateStrings();

    _imp->grid = grid;
} // MessageBox::init

MessageBox::~MessageBox()
{
}

StandardButtonEnum
MessageBox::getReply() const
{
    return _imp->clickedButton ?
           QtEnumConvert::fromQtStandardButton( (QMessageBox::StandardButton)_imp->buttonBox->standardButton(_imp->clickedButton) ) :
           eStandardButtonEscape;
}

void
MessageBox::setCheckBox(QCheckBox* checkbox)
{
    _imp->checkbox = checkbox;
#ifndef Q_WS_MAC
#ifdef Q_WS_S60
    const int preferredTextColumn = (QApplication::layoutDirection() == Qt::LeftToRight) ? 0 : 1;
#else
    const int preferredTextColumn = 1;
#endif
#else
    const int preferredTextColumn = 1;
#endif
    _imp->grid->addWidget(checkbox, 1, preferredTextColumn, 1, 1);
}

bool
MessageBox::isCheckBoxChecked() const
{
    if (_imp->checkbox) {
        return _imp->checkbox->isChecked();
    } else {
        return false;
    }
}

void
MessageBox::onButtonClicked(QAbstractButton* button)
{
    _imp->clickedButton = button;
    accept();
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_MessageBox.cpp"
