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
	
    static void metaEngineRecursive(std::vector<char*> &alreadyComputedNodes,
                                    Node* node,OutputNode* output,Row *row);

};

#endif // WORKERTHREAD_H
