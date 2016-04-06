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

class LoginDialog : public QDialog
{
public:
    LoginDialog(const QString html);
public Q_SLOTS:
    void loginButtonClicked();
    void signupButtonClicked();
private:
    QPushButton* loginButton;
    QPushButton* signupButton;
    QVBoxLayout* mainLayout;
    QFrame* mainFrame;
    QVBoxLayout* mainFrameLayout;
    QHBoxLayout* buttonsLayout;
    QFrame* buttonsFrame;
    QLineEdit* loginUsername;
    QLineEdit* loginPassword;
    QHBoxLayout* loginEditLayout;
    QTextEdit* loginInfo;
};

#endif // LOGINDIALOG_H
