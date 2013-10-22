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

#ifndef POWITER_ENGINE_IMAGEFETCHER_H_
#define POWITER_ENGINE_IMAGEFETCHER_H_

#include <map>
#include <string>
#include <QtCore/QFutureWatcher>
#include "Engine/Row.h"

#include "Engine/ChannelSet.h"

class Node;
/*This class is useful for spatial operators that need several rows, up to the whole image, of an input
 node. This object is kind of a generalisation of the node->get() function.
 Note that this object launches new threads to actually get the rows.*/
class ImageFetcher : public QObject{
    Q_OBJECT
public:
    /*Construct an image fetcher object that will fetch all the row in the range (b,t)
     for the given channels for the node specified in parameters. No fetching
     actually occurs in the constructor, you have to call claimInterest()
     to start computations. This lazy computation fashion allows you to 
     connect to the signal hasFinishedAt(int) to maybe update your piece
     of code for a specific row, or to signal the GUI of some modifications.
     
     !!WARNING: When the image fetcher object is deleted, all the buffers owned
     by the rows might be invalid.
     It's important to maintain the image fetcher living thoughout the
     entire duration you need to use the Rows contained in that object.*/
    ImageFetcher(Node* node,SequenceTime time, int l, int b, int r, int t, Powiter::ChannelSet channels);
    
    
    /*Launches new thread to fetch all the rows necessary from the node to fill the range (y,t).
     It is useful to note that the Interest object ,throughout its lifetime,will lock the rows
     into the cache and prevent them from being deleted(this statement is valid only if the node
     caches data , otherwise they will never be cached).
     When blocking is true this function returns when all threads have finished.
     
     !!WARNING :When blocking is false, results will not be available right away when returning from
     the function. You can connect to the signal hasFinishedCompletly to be sure
     that the rows are avaiable. You can also call isFinished().
     You can also connect to the signal hasFinishedAt(int) that will tell you
     when a specific row has been fetch from the input. This may be used
     to start following computation a bit ahead, but this is not really
     useful since the purpose of the image fetcher object is to retrieve a block
     of rows that are inter-dependent; if they were independent you could use
     Node::get in the first place.
     */
    // on output, all rows referenced in ImageFetcher are locked, and must be unlocked using Row::unlock()
    void claimInterest(bool blocking = false);
    
    bool isFinished() const {return _isFinished;}
    
    /*Returns the row  y computed by the node.Returns NULL
     if it couldn't find it.
     !!WARNING: calling at() while isFinished still returns false
     will block until the results are available for this row*/
    // on output, the row is locked, and must be unlocked using Row::unlock()
    boost::shared_ptr<const Powiter::Row> at(int y) const;

    // erase the row at y
    // the buffers owned by that row might be invalid after this call
    void erase(int y);

    /*write the rows to an image on disk using QImage.*/
    void debugImageFetcher(const std::string& filename);
    
    /*Deletes all the Rows in the image fetcher, and tell the cache they are
     no longer protected (i.e : they return to normal priority and can be evicted).*/
    ~ImageFetcher();
    
    public slots:
    
    void notifyFinishedAt(int);
    
    void onCompletion();
    
signals:
    /*Thrown when a row has been computed by the image fetcher. The parameter is the y
     coordinate of the row.*/
    void hasFinishedAt(int);
    void hasFinishedCompletly();
    
private:
    // on output, the row is locked, and must be unlocked using Row::unlock()
    boost::shared_ptr<const Powiter::Row> getInputRow(Node* node,SequenceTime time,int y, int x,int r,const Powiter::ChannelSet& channels);

private:
    int _x,_y,_r,_t;
    SequenceTime _time;
    Powiter::ChannelSet _channels;
    Node* _node;
    bool _isFinished;
    std::map<int,boost::shared_ptr<const Powiter::Row> > _interest;
    QVector<int> _sequence;
    QFutureWatcher<boost::shared_ptr<const Powiter::Row> >* _results;
};

#endif /* defined(POWITER_ENGINE_IMAGEFETCHER_H_) */
