//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

#include <iostream>
#include <QtWidgets/QApplication>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtWidgets/QLabel>
#include <QtGui/qpixmap.h>

//#include <vld.h> //< visual studio memory leaks finder header
#include "Superviser/controler.h"
#include "Core/model.h"
#include "Superviser/powiterFn.h"

using namespace std;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    /*Display a splashscreen while we wait for the engine to load*/
    QString filename("");
    filename.append(IMAGES_PATH"splashscreen.png");
    QPixmap pixmap(filename);
    pixmap=pixmap.scaled(640, 400);
    QLabel* splashScreen = new QLabel(0,Qt::WindowStaysOnTopHint | Qt::SplashScreen);
    splashScreen->setPixmap(pixmap);
    splashScreen->show();
    QCoreApplication::processEvents();
	/*instanciating the core*/
    Model* coreEngine=new Model;

	/*instancating the controler, that will in turn create the GUI*/
    Controler* ctrl=new Controler(coreEngine);
	/*we create the GUI in the initControler function*/
    ctrl->initControler(coreEngine,splashScreen);
    return app.exec();

}

