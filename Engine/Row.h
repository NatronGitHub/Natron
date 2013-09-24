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

#ifndef POWITER_ENGINE_ROW_H_
#define POWITER_ENGINE_ROW_H_

#include <string>

#include "Engine/ChannelSet.h"
#include "Engine/AbstractCache.h" // for InMemoryEntry

#define POWITER_MAX_BUFFERS_PER_ROW 10


/*This class represent one line in the image.
 *One line is a set of buffers (channels) defined
 *in the range [x,r]*/
class MemoryFile;
class Node;


/*The row is the  class that defines 1 line in an image. 
 It stores as many buffers as there're channels enabled for this row.
 Note that it inherits InMemoryEntry so it fits into the in-RAM NodeCache,
 but it doesn't use the allocate() and deallocate() version of the base-class.*/
// the user is responsible for locking the entry, using lock() and unlock()
class Row : public CacheEntryHelper
{
public:
    Row();
    Row(int x,int y,int right,ChannelSet channels);
    virtual ~Row() OVERRIDE;

    virtual bool isMemoryMappedEntry () const OVERRIDE {return false;};

    /*Must be called explicitly after the constructor and BEFORE any usage
     of the Row object.Returns true on success, false otherwise.*/
    bool allocateRow();
   

    /*Should be called when you're done using the row. Generally
     you never call this, it is called automatically for you
     when you manipulate the InputRow object.*/
    // on input, the row must be locked, and on output it is unlocked or deleted
    void release();
    
  
	/*Returns a writable pointer to the channel c.
	 *WARNING : the pointer returned is pointing to 0.
	 *if x != 0 then the start of the row can be obtained
	 *as such : float* start = row->writable(c)+row->offset()*/
    // the user is responsible for locking the entry before getting the pointer
    // and after manipulating its content, using lock() and unlock()
    float* writable(Powiter::Channel c);

	/*Copy into the current row, the channels defined in the source
	 *in the range [o,r}*/
    void copy(const Row *source,ChannelSet channels,int o,int r);

	

	/*set to 0s the entirety of the chnnel c*/
    void erase(Powiter::Channel c);
    void erase(ChannelSet set);
    
	/*Returns a read-only pointer to the channel z.
	 *WARNING : the pointer returned is pointing to 0.
	 *if x != 0 then the start of the row can be obtained
	 *as such : const float* start = (*row)[z]+row->offset()*/
    // the user is responsible for locking the entry before getting the pointer
    // and after manipulating its content, using lock() and unlock()
    const float* operator[](Powiter::Channel z) const;
    
    // the user is responsible for locking the entry, using lock() and unlock()
    int y() const { assert(!_lock.tryLock()); return _y; }

    // the user is responsible for locking the entry, using lock() and unlock()
    int right() const { assert(!_lock.tryLock()); return r;}

    // the user is responsible for locking the entry, using lock() and unlock()
    int offset() const { assert(!_lock.tryLock()); return x;}

	/*activates the channel C for this row and allocates memory for it.
     All (r-x) range for this channel will be set to 0.*/
    void turnOn(Powiter::Channel c);


    /*Do not pay heed to these, they're used internally so the viewer
     know how to fill the display texture.*/
    // the user is responsible for locking the entry, using lock() and unlock()
    int zoomedY() { assert(!_lock.tryLock()); return _zoomedY; }
    void set_zoomedY(int z) { assert(!_lock.tryLock()); _zoomedY = z; }

    
    /*Called by the NodeCache so the destructor of Row doesn't
     try to double-free the buffers.*/
    void notifyCacheForDeletion() { assert(!_lock.tryLock()); _cacheWillDelete = true; }
        
    // the user is responsible for locking the entry, using lock() and unlock()
    const ChannelSet& channels() const { assert(!_lock.tryLock()); return _channels; }
   
    /*Returns a key computed from the parameters.*/
    static U64 computeHashKey(U64 nodeKey, const std::string& filename, int x , int r, int y);
    
private:
 	/*changes the range of the row to be in [offset,right] ONLY
	 *if the range is wider. That means a series of call to range
	 *will only return the union of that series of ranges.
	 *This function is using realloc() so the data has been preserved
	 *in the previous range. If you carefully access the row after that
	 *call you can access the old data.*/
	void setRange(int offset,int right);
private:
    bool _cacheWillDelete;
    int _y; // the line index in the fullsize image
    int _zoomedY; // the line index in the zoomed version as it appears on the viewer
    ChannelSet _channels; // channels held by the row
    int x; // starting point of the row
    int r; // end of the row
    float** buffers; // channels array
};
bool compareRows(const Row &a,const Row &b);

#endif // POWITER_ENGINE_ROW_H_

