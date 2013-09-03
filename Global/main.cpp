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
#include "Global/Controler.h"
#include "Engine/Model.h"


using namespace std;

int main(int argc, char *argv[])
{	
#ifdef _WIN32
	//VLDEnable();
	//VLDReportLeaks();
#endif
    QApplication app(argc, argv);
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
	/*instanciating the core*/
    Model* coreEngine=new Model();
	/*instancating the controler, that will in turn create the GUI*/
    Controler* ctrl= Controler::instance();
	/*we create the GUI in the initControler function*/
    
    QString projectFile;
    QStringList args = QCoreApplication::arguments();
    for (int i = 0 ; i < args.size(); ++i) {
        if(args.at(i).contains(".rs")){
            projectFile = args.at(i);
        }
    }
    ctrl->initControler(coreEngine,splashScreen,projectFile);

    
    
    return app.exec();


}

