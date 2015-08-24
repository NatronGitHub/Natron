//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "MessageBox.h"

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QStyle>
#include <QPushButton>
#include <QCheckBox>
#include <QTextEdit>
#include <QApplication>
#include <QDesktopWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/QtEnumConvert.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/Label.h"

namespace Natron {
struct MessageBoxPrivate
{
    MessageBox::MessageBoxTypeEnum type;
    
    QHBoxLayout* mainLayout;
    QVBoxLayout* vLayout;
    
    Natron::Label* infoLabel;
    
    QWidget* vContainer;
    Natron::Label* questionLabel;
    QTextEdit* infoEdit; //< used if the text is too long so the user can scroll
    QCheckBox* checkbox;
    
    QDialogButtonBox* buttons;
    QAbstractButton* clickedButton;
    
    MessageBoxPrivate(MessageBox::MessageBoxTypeEnum type)
    : type(type)
    , mainLayout(0)
    , vLayout(0)
    , infoLabel(0)
    , vContainer(0)
    , questionLabel(0)
    , infoEdit(0)
    , checkbox(0)
    , buttons(0)
    , clickedButton(0)
    {
        
    }
    
    int layoutMinimumWidth()
    {
        mainLayout->activate();
        return mainLayout->totalMinimumSize().width();
    }
};

MessageBox::MessageBox(const QString & title,
                       const QString & message,
                       MessageBoxTypeEnum type,
                       const Natron::StandardButtons& buttons,
                       Natron::StandardButtonEnum defaultButton,
                       QWidget* parent)
: QDialog(parent,Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint)
, _imp(new MessageBoxPrivate(type))
{
    init(title, message, buttons, defaultButton);
}

void
MessageBox::init(const QString & title,
                 const QString & message,
                 const Natron::StandardButtons& buttons,
                 Natron::StandardButtonEnum defaultButton)
{
    _imp->mainLayout = new QHBoxLayout(this);
    
    _imp->infoLabel = new Natron::Label(this);
    _imp->infoLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    
    QStyle::StandardPixmap pixType;
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
    _imp->infoLabel->setPixmap(icon.pixmap(iconSize, iconSize));
    
    _imp->vContainer = new QWidget(this);
    _imp->vLayout = new QVBoxLayout(_imp->vContainer);
    
    if (message.size() < 1000) {
        _imp->questionLabel = new Natron::Label(message,_imp->vContainer);
        _imp->questionLabel->setTextInteractionFlags(Qt::TextInteractionFlags(style()->styleHint(QStyle::SH_MessageBox_TextInteractionFlags, 0, this)));
        _imp->questionLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        _imp->questionLabel->setOpenExternalLinks(true);
        _imp->questionLabel->setContentsMargins(16, 0, 0, 0);
        //QFont f(appFont,appFontSize);
        //_imp->questionLabel->setFont(f);
        _imp->vLayout->addWidget(_imp->questionLabel);
    } else {
        _imp->infoEdit = new QTextEdit(message,_imp->vContainer);
        _imp->infoEdit->setReadOnly(true);
        _imp->infoEdit->setContentsMargins(16, 0, 0, 0);
        _imp->infoEdit->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        //QFont f(appFont,appFontSize);
        //_imp->infoEdit->setFont(f);
        _imp->vLayout->addWidget(_imp->infoEdit);
    }
    
    
    _imp->buttons = new QDialogButtonBox(_imp->vContainer);
    _imp->buttons->setStandardButtons(QDialogButtonBox::StandardButtons(int(QtEnumConvert::toQtStandarButtons(buttons))));
    QPushButton* defaultB = _imp->buttons->button(QDialogButtonBox::StandardButton(
                                                                                   (QDialogButtonBox::StandardButton)QtEnumConvert::toQtStandardButton(defaultButton)));
    if (_imp->buttons->buttons().contains(defaultB)) {
        defaultB->setDefault(true);
        defaultB->setFocus();
    }
    _imp->buttons->setCenterButtons(style()->styleHint(QStyle::SH_MessageBox_CenterButtons, 0, this));
    QObject::connect(_imp->buttons, SIGNAL(clicked(QAbstractButton*)),
                     this, SLOT(onButtonClicked(QAbstractButton*)));
    _imp->vLayout->addWidget(_imp->buttons);
    
    _imp->mainLayout->addWidget(_imp->infoLabel);
    _imp->mainLayout->addWidget(_imp->vContainer);
    
    setWindowTitle(title);
    

}


MessageBox::~MessageBox()
{
    
}

void MessageBox::updateSize()
{
    if (_imp->questionLabel) {
        _imp->questionLabel->setWordWrap(true);
    } else {
        _imp->infoEdit->setWordWrapMode(QTextOption::WordWrap);
    }
    
    setFixedSize(400,150);
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
//    _imp->questionLabel->setWordWrap(false); // makes the label return min size
//    int width = _imp->layoutMinimumWidth();
//    _imp->questionLabel->setWordWrap(true);
//
//    if (width > softLimit) {
//        width = qMax(softLimit, _imp->layoutMinimumWidth());
//        
////        if (width > hardLimit) {
////            _imp->questionLabel->d_func()->ensureTextControl();
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
//    _imp->mainLayout->activate();
//    int height = (_imp->mainLayout->hasHeightForWidth())
//    ? _imp->mainLayout->totalHeightForWidth(width)
//    : _imp->mainLayout->totalMinimumSize().height();
//    
//
//    setFixedSize(width, height);
//    QCoreApplication::removePostedEvents(this, QEvent::LayoutRequest);
}

Natron::StandardButtonEnum
MessageBox::getReply() const
{
    return _imp->clickedButton ?
    QtEnumConvert::fromQtStandardButton((QMessageBox::StandardButton)_imp->buttons->standardButton(_imp->clickedButton)) :
    Natron::eStandardButtonEscape;
}

void
MessageBox::setCheckBox(QCheckBox* checkbox)
{
    _imp->checkbox = checkbox;
    _imp->vLayout->insertWidget(1, checkbox);
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
    switch (e->type()) {
        case QEvent::LayoutRequest:
            updateSize();
            break;
        default:
            break;
    }
    return result;
}

}