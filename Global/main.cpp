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
#include <QLabel>
#include <QtGui/QPixmap>

#include "Global/GlobalDefines.h"
#include "Global/AppManager.h"

#include "Engine/Knob.h"
#include "Engine/ChannelSet.h"
#include "Engine/Log.h"
#include "Engine/Settings.h"

#include "Gui/KnobGui.h"


void registerMetaTypes(){
    qRegisterMetaType<Variant>();
    qRegisterMetaType<Natron::ChannelSet>();
    qRegisterMetaType<KnobGui*>();
    qRegisterMetaType<KeyFrame*>();
    qRegisterMetaType<Format>();
    qRegisterMetaType<Natron::StandardButtons>();
    qRegisterMetaType<SequenceTime>("SequenceTime");

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
    std::cout << "[--background] enables background mode rendering. No graphical interface will be shown." << std::endl;
    std::cout << "[--writer <Writer node name>] When in background mode, the renderer will only try to render with the node"
    " name following the --writer argument. If no such node exists in the project file, the process will abort."
    "Note that if you don't pass the --writer argument, it will try to start rendering with all the writers in the project's file."<< std::endl;

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
    QStringList args;
    for(int i = 0; i < argc ;++i){
        args.push_back(QString(argv[i]));
    }
    for (int i = 0 ; i < args.size(); ++i) {
        if(args.at(i).contains("." NATRON_PROJECT_FILE_EXT)){
            if(expectWriterNameOnNextArg){
                printUsage();
                return 1;
            }
            projectFile = args.at(i);
        }else if(args.at(i) == "--background"){
            if(expectWriterNameOnNextArg){
                printUsage();
                return 1;
            }
            isBackGround = true;
        }else if(args.at(i) == "--writer"){
            if(expectWriterNameOnNextArg){
                printUsage();
                return 1;
            }
            expectWriterNameOnNextArg = true;
        }else if(expectWriterNameOnNextArg){
            writers << args.at(i);
            expectWriterNameOnNextArg = false;
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
    
    AppManager* manager = AppManager::instance(); //< load the AppManager singleton

    
    QLabel* splashScreen = 0;
    if(!isBackGround){
        /*Display a splashscreen while we wait for the engine to load*/
        QString filename("");
        filename.append(NATRON_IMAGES_PATH"splashscreen.png");
        QPixmap pixmap(filename);
        pixmap=pixmap.scaled(640, 400);
        splashScreen = new QLabel;
        splashScreen->setWindowFlags(Qt::SplashScreen);
        splashScreen->setPixmap(pixmap);
#ifndef NATRON_DEBUG
        splashScreen->show();
#endif
        //load custom fonts
        QString fontResource = QString(":/Resources/Fonts/%1.ttf");

        QStringList fontFilenames;
        fontFilenames << fontResource.arg("DroidSans");
        fontFilenames << fontResource.arg("DroidSans-Bold");

        foreach(QString fontFilename, fontFilenames)
        {
            qDebug() << "attempting to load" << fontFilename;
            int fontID = QFontDatabase::addApplicationFont(fontFilename);
            qDebug() << "fontID=" << fontID << "families=" << QFontDatabase::applicationFontFamilies(fontID);
        }
        QCoreApplication::processEvents();
    }else{
        printBackGroundWelcomeMessage();
    }
    if(!manager->newAppInstance(isBackGround,projectFile,writers)){
        printUsage();
        AppManager::quit();
        return 1;
    }
	  
    if(!isBackGround){
        splashScreen->hide();
        delete splashScreen;
    }else{
        //in background mode, exit...
        AppManager::quit();
        return 0;
    }
    
    int retCode =  app->exec();
    delete app;
    return retCode;

}

