/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
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
#include <QtCore/QtGlobal> // for Q_OS_*
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStyle>
#include <QPushButton>
#include <QCheckBox>
#include <QTextEdit>
#include <QApplication>
#include <QDesktopWidget>
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
    if (message.size() < 1000) {
        _imp->label = new Label(message);
        _imp->label->setTextInteractionFlags( Qt::TextInteractionFlags( style()->styleHint(QStyle::SH_MessageBox_TextInteractionFlags, 0, this) ) );
        _imp->label->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        _imp->label->setOpenExternalLinks(true);
#if defined(Q_WS_MAC)
        _imp->label->setContentsMargins(16, 0, 0, 0);
#elif !defined(Q_WS_QWS)
        _imp->label->setContentsMargins(2, 0, 0, 0);
        _imp->label->setIndent(9);
#endif
        //QFont f(appFont,appFontSize);
        //_imp->label->setFont(f);
    } else {
        _imp->infoEdit = new QTextEdit;
        _imp->infoEdit->setFocusPolicy(Qt::NoFocus);
        _imp->infoEdit->setReadOnly(true);
        _imp->infoEdit->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        _imp->infoEdit->append(message);
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
            pixType = QStyle::SP_MessageBoxWarning;
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
        grid->addWidget(_imp->label, 0, preferredTextColumn, 1, 1);
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
    updateSize();
} // MessageBox::init

MessageBox::~MessageBox()
{
}

void
MessageBox::updateSize()
{
    if (_imp->label) {
        _imp->label->setWordWrap(true);
    } else if (_imp->infoEdit) {
        _imp->infoEdit->setWordWrapMode(QTextOption::WordWrap);
    }

    setFixedSize(400, 150);
//    if (!isVisible())
//        return;
//
//    QSize screenSize = QApplication::desktop()->availableGeometry(QCursor::pos()).size();
//#if defined(Q_WS_QWS) || defined(Q_WS_WINCE) || defined(Q_OS_SYMBIAN)
//    // the width of the screen, less the window border.
//    int hardLimit = screenSize.width() - (q->frameGeometry().width() - q->geometry().width());
//#else
//    int hardLimit = qMin(screenSize.width() - 480, 1000); // can never get bigger than this
//    // on small screens allows the messagebox be the same size as the screen
//    if (screenSize.width() <= 1024)
//        hardLimit = screenSize.width();
//#endif
//#ifdef Q_WS_MAC
//    int softLimit = qMin(screenSize.width()/2, 420);
//#elif defined(Q_WS_QWS)
//    int softLimit = qMin(hardLimit, 500);
//#else
//    // note: ideally on windows, hard and soft limits but it breaks compat
//#ifndef Q_WS_WINCE
//    int softLimit = qMin(screenSize.width()/2, 500);
//#else
//    int softLimit = qMin(screenSize.width() * 3 / 4, 500);
//#endif //Q_WS_WINCE
//#endif
//
//
//    _imp->label->setWordWrap(false); // makes the label return min size
//    int width = _imp->layoutMinimumWidth();
//    _imp->label->setWordWrap(true);
//
//    if (width > softLimit) {
//        width = qMax(softLimit, _imp->layoutMinimumWidth());
//
////        if (width > hardLimit) {
////            _imp->label->d_func()->ensureTextControl();
////            if (QTextControl *control = label->d_func()->control) {
////                QTextOption opt = control->document()->defaultTextOption();
////                opt.setWrapMode(QTextOption::WrapAnywhere);
////                control->document()->setDefaultTextOption(opt);
////            }
////            width = hardLimit;
////        }
//    }
//
//    QFontMetrics fm(QApplication::font("QWorkspaceTitleBar"));
//    int windowTitleWidth = qMin(fm.width(windowTitle()) + 50, hardLimit);
//    if (windowTitleWidth > width)
//        width = windowTitleWidth;
//
//    _imp->grid->activate();
//    int height = (_imp->grid->hasHeightForWidth())
//    ? _imp->grid->totalHeightForWidth(width)
//    : _imp->grid->totalMinimumSize().height();
//
//
//    setFixedSize(width, height);
//    QCoreApplication::removePostedEvents(this, QEvent::LayoutRequest);
} // MessageBox::updateSize

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

bool
MessageBox::event(QEvent* e)
{
    bool result = QDialog::event(e);

    switch ( e->type() ) {
    case QEvent::LayoutRequest:
        updateSize();
        break;
    default:
        break;
    }

    return result;
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_MessageBox.cpp"
