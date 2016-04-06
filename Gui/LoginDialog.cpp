#include "LoginDialog.h"

LoginDialog::LoginDialog(const QString html): QDialog(0,Qt::Dialog | Qt::WindowStaysOnTopHint)
{
    setWindowTitle(QObject::tr("Natron Login"));
    setMinimumWidth(530);
    setMinimumHeight(600);
    setMaximumWidth(530);
    setMaximumHeight(600);

    //setAttribute(Qt::WA_DeleteOnClose, false);

    mainLayout = new QVBoxLayout(this);

    mainFrame = new QFrame(this);
    mainFrame->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    mainFrame->setStyleSheet(QString::fromUtf8("background-color:white;background:url(:/Resources/Images/logindialog-background.png) no-repeat;"));

    mainFrameLayout = new QVBoxLayout(mainFrame);
    buttonsFrame = new QFrame(this);
    buttonsLayout = new QHBoxLayout(buttonsFrame);

    loginButton = new QPushButton(QObject::tr("Login"),this);
    loginButton->setFocusPolicy(Qt::TabFocus);
    signupButton = new QPushButton(QObject::tr("Signup"),this);
    signupButton->setFocusPolicy(Qt::TabFocus);

    loginInfo = new QTextEdit(this);
    loginInfo->setStyleSheet(QString::fromUtf8("background: rgba(0, 0, 0, 0);"));
    loginInfo->setHtml(html);
    loginInfo->setMinimumHeight(120);
    loginInfo->setMaximumHeight(120);
    loginInfo->setFrameShape(QFrame::NoFrame);

    loginUsername = new QLineEdit(this);
    loginPassword = new QLineEdit(this);
    loginUsername->setPlaceholderText(QObject::tr("Your username ..."));
    loginPassword->setPlaceholderText(QObject::tr("Your password ..."));
    loginUsername->setStyleSheet(QString::fromUtf8("background: rgba(255, 255, 255, 100);font-weight:bold;"));
    loginPassword->setStyleSheet(QString::fromUtf8("background: rgba(255, 255, 255, 100);font-weight:bold;"));
    loginUsername->setContentsMargins(20,0,20,0);
    loginPassword->setContentsMargins(20,0,20,20);

    mainFrameLayout->addStretch();
    mainFrameLayout->addWidget(loginInfo);
    mainFrameLayout->addWidget(loginUsername);
    mainFrameLayout->addWidget(loginPassword);

    buttonsLayout->addWidget(loginButton);
    buttonsLayout->addWidget(signupButton);
    mainLayout->addWidget(mainFrame);
    mainLayout->addWidget(buttonsFrame);

}

void LoginDialog::loginButtonClicked()
{

}

void LoginDialog::signupButtonClicked()
{

}

