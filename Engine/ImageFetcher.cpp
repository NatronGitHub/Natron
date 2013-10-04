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

#include "ImageFetcher.h"

#include <stdexcept>
#if QT_VERSION < 0x050000
#include <QtConcurrentMap>
#else
#include <QtConcurrent>
#endif
#include <QtGui/QImage>
#include <boost/bind.hpp>

#include "Engine/Node.h"

using namespace std;
using namespace Powiter;

ImageFetcher::ImageFetcher(Node* node, int x, int y, int r, int t, ChannelSet channels)
: _x(x)
, _y(y)
, _r(r)
, _t(t)
, _channels(channels)
, _node(node)
, _isFinished(false)
, _interest()
, _sequence()
, _results(0)
{
    for (int i = y; i < t; ++i) {
        _sequence.push_back(i);
    }
}

// on output, all rows referenced in ImageFetcher are locked, and must be unlocked using Row::unlock()
void ImageFetcher::claimInterest(bool blocking) {
    _results = new QFutureWatcher<Row*>;
    QObject::connect(_results, SIGNAL(resultReadyAt(int)), this, SLOT(notifyFinishedAt(int)));
    QObject::connect(_results, SIGNAL(finished()), this, SLOT(onCompletion()));
    if (!blocking) {
        QFuture<Row*> future = QtConcurrent::mapped(_sequence.begin(),_sequence.end(),
                                                    boost::bind(&ImageFetcher::getInputRow,this,_node,_1,_x,_r));
        _results->setFuture(future);
    } else {
        QVector<Row*> ret = QtConcurrent::blockingMapped(_sequence, boost::bind(&ImageFetcher::getInputRow,this,_node,_1,_x,_r));
        for (int i = 0; i < ret.size(); i++) {
            _interest.insert(make_pair(ret.at(i)->y(),ret.at(i)));
        }
    }
}

// on output, the row is locked, and must be unlocked using Row::unlock()
Row* ImageFetcher::getInputRow(Node* node,int y,int x,int r) {
    Row* row = node->get(y,x,r);
    if (!row) {
        cout << "Interest failure for row " << y << endl;
        return NULL;
    }
    return row;
}

// on output, the row is locked, and must be unlocked using Row::unlock()
Row* ImageFetcher::at(int y) const {
    std::map<int,Row*>::const_iterator it = _interest.find(y);
    if (it != _interest.end()) {
        return it->second;
    } else {
        return NULL;
    }
}

void ImageFetcher::erase(int y) {
    std::map<int,Row*>::iterator it = _interest.find(y);
    if (it != _interest.end()) {
        it->second->release();
        // row is unlocked by release()
        _interest.erase(y);
    }
}

ImageFetcher::~ImageFetcher() {
    for (std::map<int,Row*>::const_iterator it = _interest.begin(); it!=_interest.end(); ++it) {
        it->second->release();
        // row is unlocked by release()
    }
    delete _results;
}
void ImageFetcher::onCompletion() {
    _isFinished = true;
    for (int i = 0; i < _sequence.size(); i++) {
        Row* row = _results->resultAt(i);
        _interest.insert(make_pair(row->y(), row));
    }
    emit hasFinishedCompletly();
}

void ImageFetcher::notifyFinishedAt(int y){
    emit hasFinishedAt(y);
}
void ImageFetcher::debugImageFetcher(const std::string& filename){
    int w = _r - _x;
    int h = _t - _y + 1;
    QImage img(w,h,QImage::Format_ARGB32);
    for (int i = _y; i < h+_y; ++i) {
        QRgb* dstPixels = (QRgb*)img.scanLine(i);
        map<int,Row*>::const_iterator it = _interest.find(i);
        if(it!=_interest.end()){
            Row* srcPixels = it->second;
            const float* r = (*srcPixels)[Channel_red];
            const float* g = (*srcPixels)[Channel_green];
            const float* b = (*srcPixels)[Channel_blue] ;
            const float* a = (*srcPixels)[Channel_alpha] ;
            for (int j = _x; j < w+_x ; ++j) {
                dstPixels[j] = qRgba(r ? (int)(r[i]*255.f) : 0,
                                     g ? (int)(g[i]*255.f) : 0,
                                     b ? (int)(b[i]*255.f) : 0,
                                     a ? (int)(a[i]*255.f) : 255);
            }
        }
    }
    img.save(filename.c_str());
}
