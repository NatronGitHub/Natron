#ifndef CONTROLER_H
#define CONTROLER_H

#include <QtCore/QObject>
#include "Gui/node_ui.h"
#include "Superviser/powiterFn.h"
using namespace Powiter_Enums;

class Model;
class Gui;
class QLabel;
class Controler : public QObject
{

public:
    Controler(Model* coreEngine,QObject* parent=0);
    ~Controler();

    void setProgressBarProgression(int value);
    void addNewNode(qreal x, qreal y, QString name);
    QStringList& getNodeNameList();
    Gui* getGui(){return graphicalInterface;}
	Model* getModel(){return coreEngine;}
    void initControler(Model* coreEngine,QLabel* loadingScreen);
    void exit();
private:

    
    Model* coreEngine;
    Gui* graphicalInterface;

};


#endif // CONTROLER_H

