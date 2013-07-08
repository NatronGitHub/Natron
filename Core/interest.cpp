//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

#include "interest.h"
#include "Core/node.h"
#include <QtConcurrent/QtConcurrentMap>
#include <boost/bind.hpp>
Interest::Interest(Node* node, int x, int y, int r, int t, ChannelMask channels):
_x(x),_y(y),_r(r),_t(t),_channels(channels),_node(node),_isFinished(false),_results(0){
    for (int i = y; i <= t; i++) {
        InputRow* row = new InputRow;
        _interest.insert(std::make_pair(y,row));
        _sequence.push_back(row);
    }
}

void Interest::claimInterest(){
    _results = new QFutureWatcher<void>;
    QObject::connect(_results, SIGNAL(resultReadyAt(int)), this, SLOT(notifyFinishedAt(int)));
    QObject::connect(_results, SIGNAL(finished()), this, SLOT(setFinished()));
    QFuture<void> future = QtConcurrent::map(_sequence.begin(),_sequence.end(),boost::bind(&Interest::getInputRow,this,_node,_1));
    _results->setFuture(future);
}

void Interest::getInputRow(Node* node,InputRow* row){
    node->get(_y,_x,_r,_channels,*row,true);
}

const InputRow& Interest::at(int y) const {
    std::map<int,InputRow*>::const_iterator it = _interest.find(y);
    if(it != _interest.end()){
        return *(it->second);
    }else{
        throw "Interest::at : Couldn't return a valid row for this coordinate";
    }
}

Interest::~Interest(){
    for (U32 i = 0; i < _sequence.size(); i++) {
        _sequence[i]->getInternalRow()->returnToNormalPriority();
        delete _sequence[i];
    }
    delete _results;
}
void Interest::setFinished(){_isFinished = true; emit hasFinishedCompletly();}

void Interest::notifyFinishedAt(int y){
    emit hasFinishedAt(y);
}