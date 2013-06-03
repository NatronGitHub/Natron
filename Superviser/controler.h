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
#include <boost/noncopyable.hpp>
using namespace Powiter_Enums;

class Model;
class Gui;
class QLabel;
class Controler : public QObject,public boost::noncopyable
{

public:
    Controler();
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

