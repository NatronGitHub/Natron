#include <iostream>
#include <QtGui/QApplication>
#include <QtCore/QObject>
#include <QtGui/qsplashscreen.h>
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
    std::string filename(ROOT);
    filename.append(IMAGES_PATH"splashscreen.png");
    QPixmap pixmap(filename.c_str());
    pixmap=pixmap.scaled(640, 400);
    QSplashScreen* splashScreen = new QSplashScreen(pixmap,Qt::WindowStaysOnTopHint);
    splashScreen->show();
    
	/*instanciating the core*/
    Model* coreEngine=new Model();

	/*instancating the controler, that will in turn create the GUI*/
    Controler* ctrl=new Controler(coreEngine,&app);
	/*we create the GUI in the initControler function*/
    ctrl->initControler(coreEngine,splashScreen);
    return app.exec();

}

