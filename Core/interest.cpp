//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

#include "interest.h"
#include "Core/node.h"
#include <QtConcurrent/QtConcurrentMap>
#include <boost/bind.hpp>
Interest::Interest(Node* node, int x, int y, int r, int t, ChannelMask channels):_isFinished(false),
_x(x),_y(y),_r(r),_t(t),_channels(channels){
    for (int i = y; i <= t; i++) {
        InputRow* row = new InputRow;
        _interest.insert(std::make_pair(y,row));
        _sequence.push_back(row);
    }//y x r chan
    _results = new QFutureWatcher<void>;
    QObject::connect(_results, SIGNAL(resultReadyAt(int)), this, SLOT(notifyFinishedAt(int)));
    QObject::connect(_results, SIGNAL(finished()), this, SLOT(setFinished()));
    QFuture<void> future = QtConcurrent::map(_sequence.begin(),_sequence.end(),boost::bind(&Interest::getInputRow,this,node,_1));
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
    delete _results;
}
void Interest::setFinished(){_isFinished = true; emit hasFinishedCompletly();}

void Interest::notifyFinishedAt(int y){
    emit hasFinishedAt(y);
}