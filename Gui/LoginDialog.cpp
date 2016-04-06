#include "LoginDialog.h"
#include <QMessageBox>
#include <QTimer>
#include <QDesktopServices>
#include <QUrl>

NATRON_NAMESPACE_ENTER;

LoginDialog::LoginDialog(const QString html, const QString title): QDialog(0,Qt::Dialog/* | Qt::WindowStaysOnTopHint*/)
{
    setWindowTitle(title);
    setMinimumWidth(530);
    setMinimumHeight(600);
    setMaximumWidth(530);
    setMaximumHeight(600);

    //setAttribute(Qt::WA_DeleteOnClose, false);

    keychain = new KeychainManager;

    mainLayout = new QVBoxLayout(this);

    mainFrame = new QFrame(this);
    mainFrame->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    mainFrame->setStyleSheet(QString::fromUtf8("background-color:white;background:url(:/Resources/Images/logindialog-background.png) no-repeat;"));

    mainFrameLayout = new QVBoxLayout(mainFrame);
    buttonsFrame = new QFrame(this);
    buttonsLayout = new QHBoxLayout(buttonsFrame);

    loginButton = new QPushButton(QObject::tr("Log In"),this);
    loginButton->setFocusPolicy(Qt::TabFocus);
    signupButton = new QPushButton(QObject::tr("Sign Up"),this);
    signupButton->setFocusPolicy(Qt::TabFocus);
    cancelButton = new QPushButton(QObject::tr("Close"),this);
    cancelButton->setFocusPolicy(Qt::TabFocus);
    loginSave = new QCheckBox(QObject::tr("Store credentials"),this);
    loginSave->setChecked(true);
    loginSave->setStyleSheet(QString::fromUtf8("color:white;"));

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
    loginPassword->setEchoMode(QLineEdit::Password);
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
    buttonsLayout->addWidget(cancelButton);
    buttonsLayout->addWidget(loginSave);

    mainLayout->addWidget(mainFrame);
    mainLayout->addWidget(buttonsFrame);

    QObject::connect(loginButton,SIGNAL(clicked()),this,SLOT(loginButtonClicked()));
    QObject::connect(cancelButton,SIGNAL(clicked()),this,SLOT(close()));
    QObject::connect(signupButton,SIGNAL(clicked()),this,SLOT(signupButtonClicked()));
    QTimer::singleShot(100,this,SLOT(restoreSettings()));
}

void LoginDialog::restoreSettings()
{
    QString savedUsername = keychain->readValue(QString::fromUtf8("username"));
    QString savedPassword = keychain->readValue(QString::fromUtf8("password"));

    if (!savedUsername.isEmpty()) {
        loginUsername->setText(savedUsername);
    }
    if (!savedPassword.isEmpty()) {
        loginPassword->setText(savedPassword);
    }

    /*if (!savedUsername.isEmpty() && !savedPassword.isEmpty()) {
        signupButton->hide();
    }*/
}

void LoginDialog::loginButtonClicked()
{
    if (!loginUsername->text().isEmpty() && !loginPassword->text().isEmpty()) {
        if (loginSave) {
            bool saveUsernameOk = false;
            bool savePasswordOk = false;
            saveUsernameOk = keychain->writeValue(QString::fromUtf8("username"),loginUsername->text());
            savePasswordOk = keychain->writeValue(QString::fromUtf8("password"),loginPassword->text());
            if (!saveUsernameOk || !savePasswordOk) {
                QMessageBox::warning(this,QObject::tr("Keychain failed"),QObject::tr("Unable to save credentials to keychain. This will not affect usage, but you will not be able to restore credentials during log in."));
            }
        }
    }
}

void LoginDialog::signupButtonClicked()
{
    // testing
    QDesktopServices::openUrl(QUrl(QString::fromUtf8(NATRON_SUPPORT_ONLINE)));
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_LoginDialog.cpp"
