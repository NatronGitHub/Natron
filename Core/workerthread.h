//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#ifndef WORKERTHREAD_H
#define WORKERTHREAD_H

#include <QtCore/qrunnable.h>
//#include <QtCore/qobject.h>
#include <vector>
#include "Superviser/powiterFn.h"

/*The workthread is a task launched by the threadpool. It executes the graph
 *for one row and then stops.*/

class OutputNode;
class InputNode;
class Node;
class Row;
class VideoEngine;
class WorkerThread 
{
    
public:
	
    static void metaEngineRecursive(Node* node,OutputNode* output,Row *row);

};

#endif // WORKERTHREAD_H
