//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#ifdef _WIN32
//#include <vld.h> //< visual studio memory leaks finder header
#endif
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
#include "Gui/KnobGui.h"
#include "Writers/Writer.h"


void registerMetaTypes(){
    qRegisterMetaType<Variant>();
    qRegisterMetaType<Writer*>();
    qRegisterMetaType<Powiter::ChannelSet>();
    qRegisterMetaType<KnobGui*>();
    qRegisterMetaType<Format>();

}

int main(int argc, char *argv[])
{	
#ifdef _WIN32
	//VLDEnable();
	//VLDReportLeaks();
#endif
    QApplication app(argc, argv);
    app.setOrganizationName(POWITER_ORGANIZATION_NAME);
    app.setOrganizationDomain(POWITER_ORGANIZATION_DOMAIN);
    app.setApplicationName(POWITER_APPLICATION_NAME);

    registerMetaTypes();
    
    
    /*Display a splashscreen while we wait for the engine to load*/
    QString filename("");
    filename.append(POWITER_IMAGES_PATH"splashscreen.png");
    QPixmap pixmap(filename);
    pixmap=pixmap.scaled(640, 400);
    QLabel* splashScreen = new QLabel;
    splashScreen->setWindowFlags(Qt::SplashScreen);
    splashScreen->setPixmap(pixmap);
#ifndef POWITER_DEBUG
    splashScreen->show();
#endif
    QCoreApplication::processEvents();
    
    AppManager* manager = AppManager::instance();
    
    QString projectFile;
    QStringList args = QCoreApplication::arguments();
    for (int i = 0 ; i < args.size(); ++i) {
        if(args.at(i).contains("." POWITER_PROJECT_FILE_EXTENION)){
            projectFile = args.at(i);
        }
    }

    manager->newAppInstance(false,projectFile);
	    
    splashScreen->hide();
    delete splashScreen;
    
    
    return app.exec();


}

