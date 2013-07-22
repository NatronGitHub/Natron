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

 

 



#ifndef ROW_H
#define ROW_H

#include <cstdlib>
#include <map>
#include "Core/channels.h"
#include "Core/abstractCache.h"
#include <boost/noncopyable.hpp>
#define MAX_BUFFERS_PER_ROW 100




/*This class represent one line in the image.
 *One line is a set of buffers (channels) defined
 *in the range [x,r]*/
class MemoryFile;
class Node;


/*The row is the  class that defines 1 line in an image. 
 It stores as many buffers as there're channels enabled for this row.
 Note that it inherits InMemoryEntry so it fits into the in-RAM NodeCache,
 but it doesn't use the allocate() and deallocate() version of the base-class.*/
class Row : public InMemoryEntry
{
    
public:
    
    Row();
    Row(int x,int y,int right,ChannelSet channels);
    virtual ~Row();
	
    /*Must be called explicitly after the constructor and BEFORE any usage
     of the Row object.Returns true on success, false otherwise.*/
    bool allocateRow(const char* path = 0);
   

    /*Should be called when you're done using the row. Generally
     you never call this, it is called automatically for you
     when you manipulate the InputRow object.*/
    void release();
    
  
	/*Returns a writable pointer to the channel c.
	 *WARNING : the pointer returned is pointing to 0.
	 *if x != 0 then the start of the row can be obtained
	 *as such : float* start = row->writable(c)+row->offset()*/
    float* writable(Powiter::Channel c);

	/*Copy into the current row, the channels defined in the source
	 *in the range [o,r}*/
    void copy(const Row *source,ChannelSet channels,int o,int r);

	

	/*set to 0s the entirety of the chnnel c*/
    void erase(Powiter::Channel c);
    void erase(ChannelSet set){
        foreachChannels(z, set){
            erase(z);
        }
    }

	/*Returns a read-only pointer to the channel z.
	 *WARNING : the pointer returned is pointing to 0.
	 *if x != 0 then the start of the row can be obtained
	 *as such : const float* start = (*row)[z]+row->offset()*/
    const float* operator[](Powiter::Channel z) const;
    
    int y() const {return _y;}

    int right() const {return r;}

	/*changes the range of the row to be in [offset,right] ONLY
	 *if the range is wider. That means a series of call to range
	 *will only return the union of that series of ranges.
	 *This function is using realloc() so the data has been preserved
	 *in the previous range. If you carefully access the row after that
	 *call you can access the old data.*/
	void range(int offset,int right);

	/*activates the channel C for this row and allocates memory for it.
     All (r-x) range for this channel will be set to 0.*/
    void turnOn(Powiter::Channel c);

    int offset() const {return x;}

    /*Do not pay heed to these, they're used internally so the viewer
     know how to fill the display texture.*/
    int zoomedY(){return _zoomedY;}
    void zoomedY(int z){_zoomedY = z;}
    
    /*Called by the NodeCache so the destructor of Row doesn't
     try to double-free the buffers.*/
    void notifyCacheForDeletion(){_cacheWillDelete = true;}
        
    const ChannelSet& channels() const {return _channels;}
   
    /*Returns a key computed from the parameters.*/
    static U64 computeHashKey(U64 nodeKey,std::string filename, int x , int r, int y);
    
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


/*A class encapsulating a row memory allocation scheme.
 It is useful so you don't have to call release when you're done
 with the row. This class is used by Node::get and should be used
 in the engine function when accessing the results from nodes upstream.*/
class InputRow : public boost::noncopyable {
    Row* _row;
public:
    
    InputRow():_row(0){}
    
    const ChannelSet& channels() const { return _row->channels();}
    
    void range(int offset,int right){_row->range(offset,right);}
    
    /*Returns a read-only pointer to the channel z.
	 *WARNING : the pointer returned is pointing to 0.
	 *if x != 0 then the start of the row can be obtained
	 *as such : const float* start = (*row)[z]+row->offset()*/
    const float* operator[](Powiter::Channel z) const{return (*_row)[z];}
    
    int y() const {return _row->y();}
    
    int right() const {return _row->right();}
    
    int offset() const {return _row->offset();}
    
    /*Returns a writable pointer to the channel c.
	 *WARNING : the pointer returned is pointing to 0.
	 *if x != 0 then the start of the row can be obtained
	 *as such : float* start = row->writable(c)+row->offset()*/
    float* writable(Powiter::Channel c){return _row->writable(c);}
    
	/*Copy into the current row, the channels defined in the source
	 *in the range [o,r}*/
    void copy(const Row *source,ChannelSet channels,int o,int r){_row->copy(source,channels,o,r);}
    
	/*activate the channel C for this row and allocates memory for it*/
    void turnOn(Powiter::Channel c){_row->turnOn(c);}
    
	/*set to 0s the entirety of the chnnel c*/
    void erase(Powiter::Channel c){_row->erase(c);}
    void erase(ChannelSet set){_row->erase(set);}
        
    /*DO NOT CALL THESE EVER*/
    void setInternalRow(Row* ptr){_row = ptr;}
    Row* getInternalRow() const {return _row;}
        
    ~InputRow(){
        if(_row)
            _row->release();
    }
};

#endif // ROW_H
