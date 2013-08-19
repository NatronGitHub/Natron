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







#ifndef Q_MOC_RUN

#include "Core/ImageFetcher.h"

#include <QtConcurrentMap>
#include <QtGui/QImage>
#include <boost/bind.hpp>
#include "Core/Node.h"
#endif

using namespace std;
using namespace Powiter;
ImageFetcher::ImageFetcher(Node* node, int x, int y, int r, int t, ChannelSet channels):
_x(x),_y(y),_r(r),_t(t),_channels(channels),_node(node),_isFinished(false),_results(0){
    for (int i = y; i <= t; i++) {
        InputRow* row = new InputRow(i,x,r);
        _interest.insert(std::make_pair(i,row));
        _sequence.push_back(row);
    }
}

void ImageFetcher::claimInterest(bool blocking){
    _results = new QFutureWatcher<void>;
    QObject::connect(_results, SIGNAL(resultReadyAt(int)), this, SLOT(notifyFinishedAt(int)));
    QObject::connect(_results, SIGNAL(finished()), this, SLOT(setFinished()));
    if(!blocking){
        QFuture<void> future = QtConcurrent::map(_sequence.begin(),_sequence.end(),boost::bind(&ImageFetcher::getInputRow,this,_node,_1));
        _results->setFuture(future);
    }else{
        QtConcurrent::blockingMap(_sequence, boost::bind(&ImageFetcher::getInputRow,this,_node,_1));
    }
}

void ImageFetcher::getInputRow(Node* node,InputRow* row){
    node->get(*row);
}

const InputRow& ImageFetcher::at(int y) const {
    std::map<int,InputRow*>::const_iterator it = _interest.find(y);
    if(it != _interest.end()){
        return *(it->second);
    }else{
        
        QString toThrow("Interest::at : Couldn't return a valid row for y = ");
        toThrow.append(QString::number(y));
        throw toThrow.toStdString();
    }
}

ImageFetcher::~ImageFetcher(){
    for (U32 i = 0; i < _sequence.size(); i++) {
        delete _sequence[i]; // deleting will decrease ref counting and allow the cache to free these elements again
    }
    delete _results;
}
void ImageFetcher::setFinished(){_isFinished = true; emit hasFinishedCompletly();}

void ImageFetcher::notifyFinishedAt(int y){
    emit hasFinishedAt(y);
}
void ImageFetcher::debugImageFetcher(const std::string& filename){
    int w = _r - _x;
    int h = _t - _y + 1;
    QImage img(w,h,QImage::Format_ARGB32);
    for (int i = _y; i < h+_y; i++) {
        QRgb* dstPixels = (QRgb*)img.scanLine(i);
        map<int,InputRow*>::const_iterator it = _interest.find(i);
        if(it!=_interest.end()){
            const InputRow& srcPixels = (*it->second);
            const float* r = srcPixels[Channel_red];
            const float* g = srcPixels[Channel_green];
            const float* b = srcPixels[Channel_blue] ;
            const float* a = srcPixels[Channel_alpha] ;
            for (int j = _x; j < w+_x ; j++) {
                dstPixels[j] = qRgba(r ? (int)(r[i]*255.f) : 0,
                                     g ? (int)(g[i]*255.f) : 0,
                                     b ? (int)(b[i]*255.f) : 0,
                                     a ? (int)(a[i]*255.f) : 255);
            }
        }
    }
    img.save(filename.c_str());
}
