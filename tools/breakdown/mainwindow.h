#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QTreeWidgetItem>
#include <QAuthenticator>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_actionExit_triggered();
    void on_actionAbout_triggered();
    void on_actionAbout_Qt_triggered();
    void on_actionOpen_triggered();
    void parseJson(QString file);
    void parseRepo();
    void parseReports();
    void parseReport(QString file);
    void clearUI();
    void replyFinished(QNetworkReply *reply);
    void on_actionSync_triggered();
    void networkAuth(QNetworkReply *reply,QAuthenticator *auth);
    void on_reports_itemDoubleClicked(QTreeWidgetItem *item);
    void restoreSettings();
    void saveSettings();
private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *manager;
    QString reportsPath;
    QString repoPath;
    QString serverPath;
};

#endif // MAINWINDOW_H
