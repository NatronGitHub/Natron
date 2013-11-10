//  Powiter
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

#include "Global/Macros.h"
#include "Global/AppManager.h"

#include "Engine/Knob.h"
#include "Engine/ChannelSet.h"
#include "Engine/Log.h"
#include "Engine/Settings.h"

#include "Gui/KnobGui.h"

#include "Writers/Writer.h"


void registerMetaTypes(){
    qRegisterMetaType<Variant>();
    qRegisterMetaType<Powiter::ChannelSet>();
    qRegisterMetaType<KnobGui*>();
    qRegisterMetaType<Format>();

}

void printBackGroundWelcomeMessage(){
    std::cout << "================================================================================" << std::endl;
    std::cout << POWITER_APPLICATION_NAME << "    " << " version: " << POWITER_VERSION_STRING << std::endl;
    std::cout << ">>>Running in background mode (off-screen rendering only).<<<" << std::endl;
    std::cout << "Please note that the background mode is in early stage and accepts only project files "
    "that would produce a valid output from the graphical version of " POWITER_APPLICATION_NAME
    ". If the background mode doesn't output any result, please adjust your project via the application interface "
    "and then re-try using the background mode." << std::endl;
}

int main(int argc, char *argv[])
{	

    QApplication app(argc, argv);
    app.setOrganizationName(POWITER_ORGANIZATION_NAME);
    app.setOrganizationDomain(POWITER_ORGANIZATION_DOMAIN);
    app.setApplicationName(POWITER_APPLICATION_NAME);

    registerMetaTypes();
    
#ifdef POWITER_LOG
    Powiter::Log::instance();//< enable logging
#endif
    
    AppManager* manager = AppManager::instance(); //< load the AppManager singleton
    
    /*Parse args to find a valid project file name. This is not fool-proof:
     any file with a matching extension will pass through...
     @TODO : use some magic number to identify uniquely the project file type.*/
    QString projectFile;
    bool isBackGround = false;
    QStringList writers;
    bool expectWriterNameOnNextArg = false;
    QStringList args = QCoreApplication::arguments();
    for (int i = 0 ; i < args.size(); ++i) {
        if(args.at(i).contains("." POWITER_PROJECT_FILE_EXTENION)){
            if(expectWriterNameOnNextArg){
                manager->printUsage();
                delete manager;
                return 1;
            }
            projectFile = args.at(i);
        }else if(args.at(i) == "--background"){
            if(expectWriterNameOnNextArg){
                manager->printUsage();
                delete manager;
                return 1;
            }
            isBackGround = true;
        }else if(args.at(i) == "--writer"){
            if(expectWriterNameOnNextArg){
                manager->printUsage();
                delete manager;
                return 1;
            }
            expectWriterNameOnNextArg = true;
        }else if(expectWriterNameOnNextArg){
            writers << args.at(i);
            expectWriterNameOnNextArg = false;
        }
    }
    
    QLabel* splashScreen = 0;
    if(!isBackGround){
        /*Display a splashscreen while we wait for the engine to load*/
        QString filename("");
        filename.append(POWITER_IMAGES_PATH"splashscreen.png");
        QPixmap pixmap(filename);
        pixmap=pixmap.scaled(640, 400);
        splashScreen = new QLabel;
        splashScreen->setWindowFlags(Qt::SplashScreen);
        splashScreen->setPixmap(pixmap);
#ifndef POWITER_DEBUG
        splashScreen->show();
#endif
        QCoreApplication::processEvents();
    }else{
        printBackGroundWelcomeMessage();
    }
    
    if(isBackGround){
        std::cout << "Background rendering feature is not fully implemented. Exiting." << std::endl;
        delete manager;
        return 1;
    }
    
    if(!manager->newAppInstance(isBackGround,projectFile,writers)){
        delete manager;
        return 1;
    }
	  
    if(!isBackGround){
        splashScreen->hide();
        delete splashScreen;
    }else{
        //in background mode, exit...
        
        return 0;
    }
    
    return app.exec();

}

