//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

#ifndef CONTROLER_H
#define CONTROLER_H
#include <QtCore/QObject>
#include "Gui/node_ui.h"
#include "Superviser/powiterFn.h"
#include "Core/singleton.h"
using namespace Powiter_Enums;

#define ctrlPTR Controler::instance()

class Model;
class Gui;
class QLabel;
class Controler : public QObject,public Singleton<Controler>
{

public:
    Controler();
    ~Controler();

    void setProgressBarProgression(int value);
    void createNode(qreal x, qreal y, QString name);
    QStringList& getNodeNameList();
    Gui* getGui(){return _gui;}
	Model* getModel(){return _model;}
    void initControler(Model* model,QLabel* loadingScreen);
    void exit();
private:
	 
    
    Model* _model;
    Gui* _gui;

};


#endif // CONTROLER_H

