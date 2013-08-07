//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

 

 




#include "Core/imagefetcher.h"

#include <QtConcurrentMap>
#include <boost/bind.hpp>

#include "Core/node.h"

InputFetcher::InputFetcher(Node* node, int x, int y, int r, int t, ChannelSet channels):
_x(x),_y(y),_r(r),_t(t),_channels(channels),_node(node),_isFinished(false),_results(0){
    for (int i = y; i <= t; i++) {
        InputRow* row = new InputRow;
        _interest.insert(std::make_pair(y,row));
        _sequence.push_back(row);
    }
}

void InputFetcher::claimInterest(){
    _results = new QFutureWatcher<void>;
    QObject::connect(_results, SIGNAL(resultReadyAt(int)), this, SLOT(notifyFinishedAt(int)));
    QObject::connect(_results, SIGNAL(finished()), this, SLOT(setFinished()));
    QFuture<void> future = QtConcurrent::map(_sequence.begin(),_sequence.end(),boost::bind(&InputFetcher::getInputRow,this,_node,_1));
    _results->setFuture(future);
}

void InputFetcher::getInputRow(Node* node,InputRow* row){
    node->get(_y,_x,_r,_channels,*row);
}

const InputRow& InputFetcher::at(int y) const {
    std::map<int,InputRow*>::const_iterator it = _interest.find(y);
    if(it != _interest.end()){
        return *(it->second);
    }else{
        throw "Interest::at : Couldn't return a valid row for this coordinate";
    }
}

InputFetcher::~InputFetcher(){
    for (U32 i = 0; i < _sequence.size(); i++) {
        delete _sequence[i]; // deleting will decrease ref counting and allow the cache to free these elements again
    }
    delete _results;
}
void InputFetcher::setFinished(){_isFinished = true; emit hasFinishedCompletly();}

void InputFetcher::notifyFinishedAt(int y){
    emit hasFinishedAt(y);
}
