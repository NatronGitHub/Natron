#include <QtCore/QCoreApplication>
#include <QtCore/QString>
#include "Core/workerthread.h"
#include "Core/hash.h"
#include "Core/outputnode.h"
#include "Core/inputnode.h"
#include "Core/row.h"
#include "Core/viewerNode.h"

using namespace std;
using Powiter_Enums::ROW_RANK;

WorkerThread::WorkerThread(Row* row,InputNode* input,OutputNode* output,ROW_RANK rank,bool cachedTask):QRunnable(),
cached(cachedTask),_rank(rank){
    this->input = input;
    this->output = output;
    this->row=row;
}
WorkerThread::~WorkerThread(){
    
}
    

void WorkerThread::run(){
	vector<char*> alreadyComputedNodes;
    meta_engine_recursive(alreadyComputedNodes,dynamic_cast<Node*>(input),output,row);
    
}

void WorkerThread::meta_engine_recursive(vector<char*> &alreadyComputedNodes,Node* node,OutputNode* output,Row *row){
//    for(int i =0;i<alreadyComputedNodes.size();i++){
//        if(!strcmp(node->getName().toStdString().c_str(),alreadyComputedNodes[i]))
//            return;
//    }
    
    if((node->get_output_channels() & node->getInfo()->channels())){
    
        if(!cached){
            if( !strcmp(node->class_name(),"Viewer")){
                static_cast<Viewer*>(node)->engine(row->y(),row->offset(),row->range(),
                                                   node->get_requested_channels() & node->getInfo()->channels(),row,_rank);
            }else{
                node->engine(row->y(),row->offset(),row->range(),node->get_requested_channels() & node->getInfo()->channels(),row);
            }
        }else{
            if(node == output){
                if( !strcmp(node->class_name(),"Viewer")){
                    static_cast<Viewer*>(node)->engine(row->y(),row->offset(),row->range(),
                                                       node->get_requested_channels() & node->getInfo()->channels(),row,_rank);
                }else{
                    node->engine(row->y(),row->offset(),row->range(),node->get_requested_channels() & node->getInfo()->channels(),row);
                }
            }
        }
        
    }else{
        cout << qPrintable(node->getName()) << " doesn't output any channel " << endl;
    }
//    char* name = (char*)malloc(node->getName().size());
//    name=strcpy(name, node->getName().toStdString().c_str());
//	alreadyComputedNodes.push_back(name);
    foreach(Node* child,node->getChildren()){
			meta_engine_recursive(alreadyComputedNodes,child,output,row);
    }    
}


