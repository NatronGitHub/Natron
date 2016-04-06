#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QTextEdit>
#include <QCheckBox>
#include "KeychainManager.h"

#include "Global/Macros.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER;

class LoginDialog : public QDialog
{
    Q_OBJECT
    QPushButton* loginButton;
    QPushButton* signupButton;
    QPushButton* cancelButton;
    QVBoxLayout* mainLayout;
    QFrame* mainFrame;
    QVBoxLayout* mainFrameLayout;
    QHBoxLayout* buttonsLayout;
    QFrame* buttonsFrame;
    QLineEdit* loginUsername;
    QLineEdit* loginPassword;
    QHBoxLayout* loginEditLayout;
    QTextEdit* loginInfo;
    QCheckBox* loginSave;
    KeychainManager* keychain;
public:
    LoginDialog(const QString html, const QString title);
private Q_SLOTS:
    void restoreSettings();
    void loginButtonClicked();
    void signupButtonClicked();
};

NATRON_NAMESPACE_EXIT;

#endif // LOGINDIALOG_H
