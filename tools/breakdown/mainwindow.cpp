#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QByteArray>
#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QDebug>
#include <QFileDialog>
#include <QDir>
#include <QTextStream>
#include <QTimer>
#include <QSettings>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("Breakdown");

    repoPath = QDir::homePath()+"/.config/breakdown";
    reportsPath = QDir::homePath()+"/.config/breakdown/reports";
    serverPath = "http://downloads.natron.fr/crash_reports";

    QDir dirs;
    if (!dirs.exists(QDir::homePath()+"/.config"))
        dirs.mkdir(QDir::homePath()+"/.config");
    if (!dirs.exists(repoPath))
        dirs.mkdir(repoPath);
    if (!dirs.exists(reportsPath))
        dirs.mkdir(reportsPath);

    manager = new QNetworkAccessManager(this);
    connect(manager,SIGNAL(finished(QNetworkReply*)),this,SLOT(replyFinished(QNetworkReply*)));
    connect(manager,SIGNAL(authenticationRequired(QNetworkReply*,QAuthenticator*)),this,SLOT(networkAuth(QNetworkReply*,QAuthenticator*)));

    QTimer::singleShot(1,this,SLOT(restoreSettings()));
}

MainWindow::~MainWindow()
{
    saveSettings();
    delete ui;
}

void MainWindow::saveSettings()
{
    QSettings settings;
    settings.beginGroup("default");
    settings.setValue("state",this->saveState());
    settings.setValue("size",this->size());
    settings.setValue("pos",this->pos());
    if (this->isMaximized())
        settings.setValue("max","true");
    else
        settings.setValue("max","false");
    settings.setValue("reportsHeader",this->ui->reports->header()->saveState());
    settings.setValue("crashHeader",this->ui->crashing_threads->header()->saveState());
    settings.setValue("threadsHeader",this->ui->threads->header()->saveState());
    settings.setValue("modulesHeader",this->ui->modules->header()->saveState());
    if (this->ui->username->text()!=settings.value("username").toString() && !this->ui->username->text().isEmpty())
        settings.setValue("username",this->ui->username->text());
    if (this->ui->password->text()!=settings.value("password").toString() && !this->ui->password->text().isEmpty())
        settings.setValue("password",this->ui->password->text());
    settings.endGroup();
    settings.sync();
}

void MainWindow::restoreSettings()
{
    QSettings settings;
    settings.beginGroup("default");
    if (settings.value("server").isValid())
        serverPath=settings.value("server").toString();
    if (settings.value("reports").isValid())
        reportsPath=settings.value("reports").toString();
    if (settings.value("repo").isValid())
        repoPath=settings.value("repo").toString();
    if (settings.value("state").isValid())
        this->restoreState(settings.value("state").toByteArray());
    if (settings.value("size").isValid())
        this->resize(settings.value("size",QSize(640,480)).toSize());
    if (settings.value("pos").isValid())
        this->move(settings.value("pos",QPoint(0,0)).toPoint());
    if (settings.value("max").toBool()==true)
        this->showMaximized();
    if (settings.value("reportsHeader").isValid())
        this->ui->reports->header()->restoreState(settings.value("reportsHeader").toByteArray());
    if (settings.value("crashHeader").isValid())
        this->ui->crashing_threads->header()->restoreState(settings.value("crashHeader").toByteArray());
    if (settings.value("threadsHeader").isValid())
        this->ui->threads->header()->restoreState(settings.value("threadsHeader").toByteArray());
    if (settings.value("modulesHeader").isValid())
        this->ui->modules->header()->restoreState(settings.value("modulesHeader").toByteArray());
    if (settings.value("username").isValid())
        this->ui->username->setText(settings.value("username").toString());
    if (settings.value("password").isValid())
        this->ui->password->setText(settings.value("password").toString());
    settings.endGroup();

    parseReports();
    manager->get(QNetworkRequest(QUrl(serverPath+"/repo.txt")));
}

void MainWindow::on_actionExit_triggered()
{
    qApp->quit();
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this,tr("About Breakdown"),"<h3>Breakpad/Socorro front-end for Natron</h3><p>Written by Ole-André Rodlie for the Natron project.</p>");
}

void MainWindow::on_actionAbout_Qt_triggered()
{
    QMessageBox::aboutQt(this,tr("About Qt"));
}

void MainWindow::on_actionOpen_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open JSON"),QDir::homePath() , tr("JSON file (*.json)"));
    if (!fileName.isEmpty())
        parseJson(fileName);
}

void MainWindow::clearUI()
{
    this->ui->cpu_count->clear();
    this->ui->cpu_info->clear();
    this->ui->crashing_threads->clear();
    this->ui->crash_adr->clear();
    this->ui->cpu_arch->clear();
    this->ui->crash_thread->clear();
    this->ui->crash_type->clear();
    this->ui->modules->clear();
    this->ui->os->clear();
    this->ui->os_version->clear();
    this->ui->threads->clear();
    this->ui->uuid->clear();
    this->ui->timestamp->clear();
    this->ui->comment->clear();
    this->ui->product->clear();
    this->ui->version->clear();
}

void MainWindow::parseJson(QString file)
{
    if (!file.isEmpty()) {
        QFile jsonFile(file);
        if (jsonFile.open(QIODevice::ReadOnly|QIODevice::Text)) {
            QString rawData = jsonFile.readAll();
            jsonFile.close();

            clearUI();
            ui->toolBox->setCurrentIndex(1);

            QJsonDocument doc(QJsonDocument::fromJson(rawData.toUtf8()));
            QJsonObject obj = doc.object();

            // crash info
            QJsonValue submitted_timestamp = obj.value(QString("submitted_timestamp"));
            QJsonValue uuid = obj.value(QString("uuid"));
            QJsonValue comments = obj.value(QString("Comments"));
            QJsonValue product = obj.value(QString("ProductName"));
            QJsonValue product_version = obj.value(QString("Version"));

            this->ui->timestamp->setText(submitted_timestamp.toString());
            this->ui->uuid->setText(uuid.toString());
            this->ui->comment->setText(comments.toString());
            this->ui->product->setText(product.toString());
            this->ui->version->setText(product_version.toString());

            // system info
            QJsonValue crash_info = obj.value(QString("crash_info"));
            QJsonObject crash_info_item = crash_info.toObject();
            QJsonValue system_info = obj.value(QString("system_info"));
            QJsonObject system_info_item = system_info.toObject();

            this->ui->crash_adr->setText(crash_info_item["address"].toString());
            this->ui->crash_thread->setText(QString::number(crash_info_item["crashing_thread"].toInt()));
            this->ui->crash_type->setText(crash_info_item["type"].toString());
            this->ui->cpu_arch->setText(system_info_item["cpu_arch"].toString());
            this->ui->cpu_count->setText(QString::number(system_info_item["cpu_count"].toInt()));
            this->ui->os->setText(system_info_item["os"].toString());
            this->ui->os_version->setText(system_info_item["os_ver"].toString());
            this->ui->cpu_info->setText(system_info_item["cpu_info"].toString());

            // crash thread
            QJsonValue crashing_thread = obj.value(QString("crashing_thread"));
            QJsonObject crashing_thread_item = crashing_thread.toObject();
            QJsonArray crashing_thread_item_frames = crashing_thread_item["frames"].toArray();
            foreach (const QJsonValue &value, crashing_thread_item_frames) {
                QJsonObject output = value.toObject();
                QTreeWidgetItem *crashItem = new QTreeWidgetItem;
                crashItem->setText(0,output["function"].toString());
                crashItem->setText(1,output["function_offset"].toString());
                crashItem->setText(2,output["module_offset"].toString());
                crashItem->setText(3,QString::number(output["frame"].toInt()));
                crashItem->setText(4,output["module"].toString());
                crashItem->setText(5,output["file"].toString());
                crashItem->setText(6,QString::number(output["line"].toInt()));
                crashItem->setText(7,output["offset"].toString());
                crashItem->setText(8,output["trust"].toString());
                this->ui->crashing_threads->addTopLevelItem(crashItem);
                // registers needed?
            }
            // modules
            QJsonArray modules = obj["modules"].toArray();
            foreach (const QJsonValue &value, modules) {
                QJsonObject output = value.toObject();
                QTreeWidgetItem *moduleItem = new QTreeWidgetItem;
                moduleItem->setText(0,output["base_addr"].toString());
                moduleItem->setText(1,output["code_id"].toString());
                moduleItem->setText(2,output["filename"].toString());
                moduleItem->setText(3,output["debug_file"].toString());
                QString loaded_symbols;
                if (output["loaded_symbols"].toBool())
                    loaded_symbols = "yes";
                else
                    loaded_symbols = "no";
                moduleItem->setText(4,loaded_symbols);
                moduleItem->setText(5,output["version"].toString());
                moduleItem->setText(6,output["end_addr"].toString());
                moduleItem->setText(7,output["debug_id"].toString());
                this->ui->modules->addTopLevelItem(moduleItem);
            }
            // threads
            QJsonArray threads_item_frames = obj["threads"].toArray();
            foreach (const QJsonValue &value, threads_item_frames) {
                QJsonObject output = value.toObject();
                foreach (const QJsonValue &value2, output["frames"].toArray()) {
                    QJsonObject output2 = value2.toObject();
                    QTreeWidgetItem *threadsItem = new QTreeWidgetItem;
                    threadsItem->setText(0,output2["module_offset"].toString());
                    QString missing_symbols;
                    if (output2["missing_symbols"].toBool())
                        missing_symbols = "yes";
                    else
                        missing_symbols = "no";
                    threadsItem->setText(1,missing_symbols);
                    threadsItem->setText(2,QString::number(output2["frame"].toInt()));
                    threadsItem->setText(3,output2["module"].toString());
                    threadsItem->setText(4,output2["file"].toString());
                    threadsItem->setText(5,QString::number(output2["line"].toInt()));
                    threadsItem->setText(6,output2["offset"].toString());
                    threadsItem->setText(7,output2["function"].toString());
                    threadsItem->setText(8,output2["function_offset"].toString());
                    threadsItem->setText(9,output2["trust"].toString());
                    this->ui->threads->addTopLevelItem(threadsItem);
                }
            }
        }
    }
}

void MainWindow::parseRepo()
{
    QFile repo(repoPath+"/repo.txt");
    if (!repo.exists()) {
        manager->get(QNetworkRequest(QUrl(serverPath+"/repo.txt")));
    }
    else {
        if (repo.open(QIODevice::ReadOnly|QIODevice::Text)) {
            QTextStream text(&repo);
            while(!text.atEnd()) {
                QString line = text.readLine();
                QStringList fields = line.split(" ");
                QString jsonFile = fields.takeLast();
                if (!jsonFile.isEmpty()) {
                    manager->get(QNetworkRequest(QUrl(serverPath+"/"+jsonFile)));
                }
            }
        }
        else {
            qDebug() << "Failed to open repo file!";
        }
    }
}

void MainWindow::parseReports()
{
    ui->reports->clear();
    ui->toolBox->setCurrentIndex(0);
    QStringList nameFilter("*.json");
    QDir directory(reportsPath);
    QStringList list = directory.entryList(nameFilter);
    for (int i = 0; i < list.size(); ++i) {
        QString report = list.at(i);
        if (!report.isEmpty())
            parseReport(report);
    }
}

void MainWindow::parseReport(QString file)
{
    if (!file.isEmpty()) {
        QFile jsonFile(reportsPath+"/"+file);
        if (jsonFile.open(QIODevice::ReadOnly|QIODevice::Text)) {
            QString rawData = jsonFile.readAll();
            jsonFile.close();
            QJsonDocument doc(QJsonDocument::fromJson(rawData.toUtf8()));
            QJsonObject obj = doc.object();

            QJsonValue submitted_timestamp = obj.value(QString("submitted_timestamp"));
            QJsonValue uuid = obj.value(QString("uuid"));
            QJsonValue comments = obj.value(QString("Comments"));
            QJsonValue product = obj.value(QString("ProductName"));
            QJsonValue product_version = obj.value(QString("Version"));

            QJsonValue system_info = obj.value(QString("system_info"));
            QJsonObject system_info_item = system_info.toObject();

            QTreeWidgetItem *newReport = new QTreeWidgetItem;
            newReport->setText(0,submitted_timestamp.toString());
            newReport->setText(1,uuid.toString());
            newReport->setText(2,product.toString());
            newReport->setText(3,product_version.toString());
            newReport->setText(4,system_info_item["os"].toString());
            newReport->setText(5,system_info_item["cpu_arch"].toString());
            newReport->setText(6,comments.toString());
            ui->reports->addTopLevelItem(newReport);
        }
    }
}

void MainWindow::replyFinished(QNetworkReply *reply)
{
    if (reply->error()) {
        qDebug() << reply->errorString();
    }
    else {
        QString url = reply->url().toString();
        QStringList list = url.split("/");
        QString fileName = list.takeLast();
        if (!fileName.isEmpty()) {
            QString filePath;
            if (fileName=="repo.txt")
                filePath = repoPath;
            else
                filePath = reportsPath;
            QFile *file = new QFile(filePath+"/"+fileName);
            if (!file->exists()) {
                if(file->open(QFile::WriteOnly)) {
                    file->write(reply->readAll());
                    file->flush();
                    file->close();
                    if (fileName=="repo.txt")
                        parseRepo();
                }
                else {
                    qDebug() << "failed to save file:" << fileName;
                }
            }
            delete file;
            if (fileName!="repo.txt")
                parseReports();
        }
    }
    reply->deleteLater();
}

void MainWindow::on_actionSync_triggered()
{
    parseRepo();
}

void MainWindow::on_reports_itemDoubleClicked(QTreeWidgetItem *item)
{
    parseJson(reportsPath+"/"+item->text(1)+".json");
}

void MainWindow::networkAuth(QNetworkReply */*reply*/, QAuthenticator *auth)
{
    auth->setUser(this->ui->username->text());
    auth->setPassword(this->ui->password->text());
}
