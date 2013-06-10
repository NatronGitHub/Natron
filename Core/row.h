//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#ifndef ROW_H
#define ROW_H

#include <cstdlib>
#include <map>
#include "Core/channels.h"
#include "Superviser/powiterFn.h"
#include "Core/abstractCache.h"
#include <boost/noncopyable.hpp>
#define MAX_BUFFERS_PER_ROW 100

/*Utility class used by the Row when using the BACKED_BY_FILE
 mode. It retains a mapping of channels and an offset into a buffer.*/
class ChannelsToFile {
    std::map<Channel,std::pair<size_t,size_t> > _mapping; // stores a mapping between channels
    size_t _totalSize;
    // and a pair of <offset in file, size of chunk>
public:
    typedef std::map<Channel,std::pair<size_t,size_t> >::iterator MappingIterator;
    
    MappingIterator begin(){return _mapping.begin();}
    
    MappingIterator end(){return _mapping.end();}
    
    size_t totalSize() const {return _totalSize;}

    
    std::string printMapping();
    
    
    /*builds the mapping. totalSize must be exactly equal to
     mapping.size() * (r-x) * sizeof(float) , otherwise an assertion
     is thrown.*/
    ChannelsToFile(ChannelSet& mapping,int x, int r,size_t totalSize);
};

/*This class represent one line in the image.
 *One line is a set of buffers (channels) defined
 *in the range [x,r]*/
class MemoryFile;
class Node;
class Row : public MemoryMappedEntry
{
    
public:
    
    Row(Powiter_Enums::RowStorageMode mode = IN_MEMORY);
    Row(int x,int y,int right,ChannelSet channels,Powiter_Enums::RowStorageMode mode = IN_MEMORY);
    virtual ~Row();
	
    /*Must be called explicitly after the constructor and BEFORE any usage
     of the Row object. Specify a path for a file name if the row must be backed by
     a file.*/
    void allocate(const char* path = 0);
   

    /*Should be called when you're done using the row.
     For In memory rows, it will call the destructor, otherwise
     it will just do nothing.*/
    void release();
    
    /*only for rows backed by a file : restores the channels from
     the backing file, opening it into the application's address space.*/
    void restoreFromBackingFile();
    
	/*Returns a writable pointer to the channel c.
	 *WARNING : the pointer returned is pointing to 0.
	 *if x != 0 then the start of the row can be obtained
	 *as such : float* start = row->writable(c)+row->offset()*/
    float* writable(Channel c);

	/*Copy into the current row, the channels defined in the source
	 *in the range [o,r}*/
    void copy(const Row *source,ChannelSet channels,int o,int r);

	

	/*set to 0s the entirety of the chnnel c*/
    void erase(Channel c);
    void erase(ChannelSet set){
        foreachChannels(z, set){
            erase(z);
        }
    }

	/*Returns a read-only pointer to the channel z.
	 *WARNING : the pointer returned is pointing to 0.
	 *if x != 0 then the start of the row can be obtained
	 *as such : const float* start = (*row)[z]+row->offset()*/
    const float* operator[](Channel z) const;
    
    int y() const {return _y;}

    int right() const {return r;}

	/*changes the range of the row to be in [offset,right] ONLY
	 *if the range is wider. That means a series of call to range
	 *will only return the union of that series of ranges.
	 *This function is using realloc() so the data has been preserved
	 *in the previous range. If you carefully access the row after that
	 *call you can access the old data.*/
	void range(int offset,int right);

	/*activate the channel C for this row and allocates memory for it*/
    void turnOn(Channel c);

    int offset() const {return x;}

    /*Do not pay heed to these, they're used internally so the viewer
     know how to fill the texture.*/
    int zoomedY(){return _zoomedY;}
    void zoomedY(int z){_zoomedY = z;}
    
    virtual std::string printOut();
    
    static std::pair<U64,Row*> recoverFromString(QString str);
    
    const ChannelMask& channels() const {return _channels;}
   
    /*Returns a key computed from the parameters.*/
    static U64 computeHashKey(U64 nodeKey,std::string filename, int x , int r, int y);
    
private:
    /*internally used by allocate when backed by a file*/
    virtual void allocate(U64 size,const char* path = 0);
    
    std::string _path;
    MemoryFile* _backingFile;
    ChannelsToFile* _channelsToFileMapping;
    
    Powiter_Enums::RowStorageMode _storageMode;
    int _y; // the line index in the fullsize image
    int _zoomedY; // the line index in the zoomed version as it appears on the viewer
    ChannelMask _channels; // channels held by the row
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
    
    const ChannelMask& channels() const { return _row->channels();}
    
    void range(int offset,int right){_row->range(offset,right);}
    
    /*Returns a read-only pointer to the channel z.
	 *WARNING : the pointer returned is pointing to 0.
	 *if x != 0 then the start of the row can be obtained
	 *as such : const float* start = (*row)[z]+row->offset()*/
    const float* operator[](Channel z) const{return (*_row)[z];}
    
    int y() const {return _row->y();}
    
    int right() const {return _row->right();}
    
    /*Returns a writable pointer to the channel c.
	 *WARNING : the pointer returned is pointing to 0.
	 *if x != 0 then the start of the row can be obtained
	 *as such : float* start = row->writable(c)+row->offset()*/
    float* writable(Channel c){return _row->writable(c);}
    
	/*Copy into the current row, the channels defined in the source
	 *in the range [o,r}*/
    void copy(const Row *source,ChannelSet channels,int o,int r){_row->copy(source,channels,o,r);}
    
	
	/*set to 0s the entirety of the chnnel c*/
    void erase(Channel c){_row->erase(c);}
    void erase(ChannelSet set){_row->erase(set);}
        
    /*DO NOT CALL THESE EVER*/
    void setInternalRow(Row* ptr){_row = ptr;}
    Row* getInternalRow() const {return _row;}
        
    ~InputRow(){_row->release();}
};

#endif // ROW_H
