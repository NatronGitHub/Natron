#ifndef WORKERTHREAD_H
#define WORKERTHREAD_H

#include <QtCore/qrunnable.h>
//#include <QtCore/qobject.h>
#include <vector>
#include "Superviser/powiterFn.h"

using Powiter_Enums::ROW_RANK;

class OutputNode;
class InputNode;
class Node;
class Row;
class VideoEngine;
class WorkerThread : public QRunnable
{
    //Q_OBJECT
public:
	
 
    WorkerThread(Row* row, InputNode* input,OutputNode* output,ROW_RANK rank,bool cachedTask = false);
    virtual ~WorkerThread();
    void run();
    Row* getRow(){return row;}


// signals:
//     void finishedTask();

    
private:
	void meta_engine_recursive(std::vector<char*> &alreadyComputedNodes,Node* node,OutputNode* output,Row *row);
    OutputNode* output;
    InputNode* input;
	Row* row;
    bool cached;
    Powiter_Enums::ROW_RANK _rank;
};

#endif // WORKERTHREAD_H
