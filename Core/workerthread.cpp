#include <QtCore/QCoreApplication>
#include <QtCore/QString>
#include "Core/workerthread.h"
#include "Core/hash.h"
#include "Core/outputnode.h"
#include "Core/inputnode.h"
#include "Core/row.h"
#include "Core/viewerNode.h"

using namespace std;

void WorkerThread::metaEngineRecursive(Node* node,OutputNode* output,Row *row){

    foreach(Node* parent,node->getParents()){
        metaEngineRecursive(parent,output,row);
    }
    if((node->getOutputChannels() & node->getInfo()->channels())){
        
        if(!row->cached()){
            node->engine(row->y(),row->offset(),row->right(),node->getRequestedChannels() & node->getInfo()->channels(),row);
        }else{
            if(node == output){
                node->engine(row->y(),row->offset(),row->right(),
                             node->getRequestedChannels() & node->getInfo()->channels(),row);
            }
        }
    }else{
        cout << qPrintable(node->getName()) << " doesn't output any channel " << endl;
    }
    
    
}



