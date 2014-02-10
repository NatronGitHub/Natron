//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#include <iostream>
#include <QApplication>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QIcon>
#include <QtGui/QPixmap>
#include <QDebug>
#include <QFontDatabase>
#include <QMetaType>
#include <QAbstractSocket>

#include "Global/GlobalDefines.h"
#include "Global/AppManager.h"

#include "Engine/Knob.h"
#include "Engine/ChannelSet.h"
#include "Engine/Log.h"
#include "Engine/Settings.h"
#include "Engine/Node.h"

#include "Gui/KnobGui.h"
#include "Gui/CurveWidget.h"
#include "Gui/SplashScreen.h"

#if QT_VERSION < 0x050000
Q_DECLARE_METATYPE(QAbstractSocket::SocketState)
#endif

void registerMetaTypes(){
    qRegisterMetaType<Variant>();
    qRegisterMetaType<Natron::ChannelSet>();
    qRegisterMetaType<Format>();
    qRegisterMetaType<Natron::StandardButtons>();
    qRegisterMetaType<SequenceTime>("SequenceTime");
    qRegisterMetaType<Knob*>();
    qRegisterMetaType<Natron::Node*>();
    qRegisterMetaType<CurveWidget*>();
#if QT_VERSION < 0x050000
    qRegisterMetaType<QAbstractSocket::SocketState>("SocketState");
#endif
}

void printBackGroundWelcomeMessage(){
    std::cout << "================================================================================" << std::endl;
    std::cout << NATRON_APPLICATION_NAME << "    " << " version: " << NATRON_VERSION_STRING << std::endl;
    std::cout << ">>>Running in background mode (off-screen rendering only).<<<" << std::endl;
    std::cout << "Please note that the background mode is in early stage and accepts only project files "
    "that would produce a valid output from the graphical version of " NATRON_APPLICATION_NAME << std::endl;
    std::cout << "If the background mode doesn't output any result, please adjust your project via the application interface "
    "and then re-try using the background mode." << std::endl;
}

void printUsage(){
    std::cout << NATRON_APPLICATION_NAME << " usage: " << std::endl;
    std::cout << "./" NATRON_APPLICATION_NAME "    <project file path>" << std::endl;
    std::cout << "[--background] or [-b] enables background mode rendering. No graphical interface will be shown." << std::endl;
    std::cout << "[--writer <Writer node name>] When in background mode, the renderer will only try to render with the node"
    " name following the --writer argument. If no such node exists in the project file, the process will abort."
    "Note that if you don't pass the --writer argument, it will try to start rendering with all the writers in the project's file."<< std::endl;

}

void initOpenColorIOConfig() {
    ///set the env var OCIO so all OpenColorIO plug-ins will use by default the same config
    QByteArray ocioEnvVar = qgetenv("OCIO");
    QString ocioEnvVarStr(ocioEnvVar);
    
    //if the variable is empty then set it
    if (ocioEnvVarStr.isEmpty()) {
        
        ///we must copy all the Luts + config.ocio of the default OCIO profile to a "known" location
        //in order for OCIO to be able to open it.
        
        ///first off copy all the luts inside the luts/ directory. We assume that the luts directory is
        ///placed right next to the config.ocio file.
        QDir lutsToCopyDir(QString(NATRON_DEFAULT_OCIO_CONFIG_DIR_PATH) + QDir::separator() + "luts");
        QDir::temp().mkdir("luts");
        QDir lutsCopyDir(QDir::tempPath() + QDir::separator() + "luts");
        QStringList entries = lutsToCopyDir.entryList(QDir::Files);
        for (int i = 0; i < entries.size(); ++i) {
            QString filePath = lutsToCopyDir.absoluteFilePath(entries.at(i));
            QFile file(filePath);
            file.copy(lutsCopyDir.absolutePath() + QDir::separator() + entries.at(i));
        }
        
        
        ///now copy the config file
        QString configPath(QString(NATRON_DEFAULT_OCIO_CONFIG_DIR_PATH) + QDir::separator() + "config.ocio");
        QString configCopyPath = QDir::tempPath() + QDir::separator() + "config.ocio";
        
        QFile configFile(configPath);
        configFile.copy(configCopyPath);
        
        qputenv("OCIO",configCopyPath.toUtf8());

    }

}

/**
 * @brief Extracts from argv[0] the path of the application's binary
 **/
static QString extractBinaryPath(const QString arg0) {
    int lastSepPos = arg0.lastIndexOf(QDir::separator());
    if (lastSepPos == -1) {
        return "";
    } else {
        return arg0.left(lastSepPos);
    }
}

int main(int argc, char *argv[])
{	

    /*Parse args to find a valid project file name. This is not fool-proof:
     any file with a matching extension will pass through...
     @TODO : use some magic number to identify uniquely the project file type.*/
    QString projectFile;
    bool isBackGround = false;
    QStringList writers;
    bool expectWriterNameOnNextArg = false;
    bool expectPipeFileNameOnNextArg = false;
    QString mainProcessServerName;
    QStringList args;
    
    QString binaryPath;
    for(int i = 0; i < argc ;++i){
        if (i == 0) {
            extractBinaryPath(argv[i]);
        }
        args.push_back(QString(argv[i]));
    }
    for (int i = 0 ; i < args.size(); ++i) {
        if(args.at(i).contains("." NATRON_PROJECT_FILE_EXT)){
            if(expectWriterNameOnNextArg || expectPipeFileNameOnNextArg) {
                printUsage();
                return 1;
            }
            projectFile = args.at(i);
            continue;
        }else if(args.at(i) == "--background" || args.at(i) == "-b"){
            if(expectWriterNameOnNextArg || expectPipeFileNameOnNextArg){
                printUsage();
                return 1;
            }
            isBackGround = true;
            continue;
        }else if(args.at(i) == "--writer"){
            if(expectWriterNameOnNextArg  || expectPipeFileNameOnNextArg){
                printUsage();
                return 1;
            }
            expectWriterNameOnNextArg = true;
            continue;
        }else if(args.at(i) == "--IPCpipe"){
            if (expectWriterNameOnNextArg || expectPipeFileNameOnNextArg) {
                printUsage();
                return 1;
            }
            expectPipeFileNameOnNextArg = true;
            continue;
        }
        
        if(expectWriterNameOnNextArg){
            assert(!expectPipeFileNameOnNextArg);
            writers << args.at(i);
            expectWriterNameOnNextArg = false;
        }
        if (expectPipeFileNameOnNextArg) {
            assert(!expectWriterNameOnNextArg);
            mainProcessServerName = args.at(i);
            expectPipeFileNameOnNextArg = false;
        }
    }

    QCoreApplication* app = NULL;
    if(!isBackGround){
        QApplication* guiApp = new QApplication(argc, argv);
        guiApp->setFont(QFont(NATRON_FONT, NATRON_FONT_SIZE_12));
        app = guiApp;
    }else{
        app = new QCoreApplication(argc,argv);
        
    }
    
    app->setOrganizationName(NATRON_ORGANIZATION_NAME);
    app->setOrganizationDomain(NATRON_ORGANIZATION_DOMAIN);
    app->setApplicationName(NATRON_APPLICATION_NAME);

    registerMetaTypes();
    
    Natron::Log::instance();//< enable logging
    
    SplashScreen* splashScreen = 0;
    if(!isBackGround){
        /*Display a splashscreen while we wait for the engine to load*/
        QString filename(NATRON_IMAGES_PATH"splashscreen.png");
        splashScreen = new SplashScreen(filename);
        QCoreApplication::processEvents();
        
        QPixmap appIcPixmap;
        appPTR->getIcon(Natron::NATRON_PIXMAP_APP_ICON, &appIcPixmap);
        QIcon appIc(appIcPixmap);
        qApp->setWindowIcon(appIc);
        //load custom fonts
        QString fontResource = QString(":/Resources/Fonts/%1.ttf");
        
        
        QStringList fontFilenames;
        fontFilenames << fontResource.arg("DroidSans");
        fontFilenames << fontResource.arg("DroidSans-Bold");
        
        foreach(QString fontFilename, fontFilenames)
        {
            splashScreen->updateText("Loading font " + fontFilename);
            //qDebug() << "attempting to load" << fontFilename;
            int fontID = QFontDatabase::addApplicationFont(fontFilename);
            qDebug() << "fontID=" << fontID << "families=" << QFontDatabase::applicationFontFamilies(fontID);
        }
    }
    
    AppManager* manager = AppManager::instance(); //< load the AppManager singleton
    
    manager->load(splashScreen);
    
    if (isBackGround && !mainProcessServerName.isEmpty()) {
        manager->initProcessInputChannel(mainProcessServerName);
        printBackGroundWelcomeMessage();
    }
    
    AppInstance::AppType mainInstanceType = isBackGround ? AppInstance::APP_BACKGROUND_AUTO_RUN : AppInstance::APP_GUI;
    AppInstance* mainInstance = manager->newAppInstance(mainInstanceType,projectFile,writers);
    if(!mainInstance){
        printUsage();
        AppManager::quit();
        return 1;
    }else{
        if(isBackGround){
            delete mainInstance;
        }
    }
 
    if(!isBackGround){
        manager->hideSplashScreen();
    }else{
        //in background mode, exit...
        AppManager::quit();
        return 0;
    }
    
    int retCode =  app->exec();
    delete app;
    return retCode;

}

