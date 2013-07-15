//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#ifdef _WIN32
#include <vld.h> //< visual studio memory leaks finder header
#endif
#include <iostream>
#include <QtWidgets/QApplication>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtWidgets/QLabel>
#include <QtGui/QPixmap>
#include "Superviser/powiterFn.h"
#include "Superviser/controler.h"
#include "Core/model.h"



using namespace std;

int main(int argc, char *argv[])
{	
#ifdef _WIN32
	VLDEnable();
	VLDReportLeaks();
#endif
    QApplication app(argc, argv);
    /*Display a splashscreen while we wait for the engine to load*/
    QString filename("");
    filename.append(IMAGES_PATH"splashscreen.png");
    QPixmap pixmap(filename);
    pixmap=pixmap.scaled(640, 400);
    QLabel* splashScreen = new QLabel;
    splashScreen->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::SplashScreen);
    splashScreen->setPixmap(pixmap);
    splashScreen->show();
    QCoreApplication::processEvents();
	/*instanciating the core*/
    Model* coreEngine=new Model();
	/*instancating the controler, that will in turn create the GUI*/
    Controler* ctrl= Controler::instance();
	/*we create the GUI in the initControler function*/
    ctrl->initControler(coreEngine,splashScreen);
    
    
    
    
    return app.exec();


}

