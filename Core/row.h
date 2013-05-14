#ifndef ROW_H
#define ROW_H

#include <cstdlib>
#include <map>
#include "Core/channels.h"

#define MAX_BUFFERS_PER_ROW 100

/*This class represent one line in the image.
 *One line is a set of buffers (channels) defined
 *in the range [x,r]*/
class Node;
class Row
{
public:
    
    Row();
    Row(int x,int y,int right,ChannelSet channels);
    ~Row();
	
    /*Must be called explicitly after the constructor and BEFORE any usage
     of the Row object. This allocates all the active channels of the row*/
    void allocate();
    
	/*Returns a writable pointer to the channel c.
	 *WARNING : the pointer returned is pointing to 0.
	 *if x != 0 then the start of the row can be obtained
	 *as such : float* start = row->writable(c)+row->offset()*/
    float* writable(Channel c);

	/*Copy into the current row, the channels defined in the source
	 *in the range [o,r}*/
    void copy(const Row *source,ChannelSet channels,int o,int r);

	/*Undefined yet*/
    void get(Node &input,int y,int x,int range,ChannelSet channels);

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
    void y(int nb){_y=nb;}
    int right(){return r;}

	/*changes the range of the row to be in [offset,right] ONLY
	 *if the range is wider. That means a series of call to range
	 *will only return the union of that series of ranges.
	 *This function is using realloc() so the data has been preserved
	 *in the previous range. If you carefully access the row after that
	 *call you can access the old data.*/
	void range(int offset,int right);

	/*activate the channel C for this row and allocates memory for it*/
    void turnOn(Channel c);

    int offset(){return x;}

    int zoomedY(){return _zoomedY;}
    void zoomedY(int z){_zoomedY = z;}
    
    
    ChannelMask channels(){return _channels;}
    
    bool cached(){return _cached;}
    void cached(bool c){_cached = c;}
    
private:
    int _y; // the line index in the fullsize image
    int _zoomedY; // the line index in the zoomed version on the viewer
    ChannelMask _channels; // channels held by the row
    int x; // starting point of the row
    int r; // end of the row
    float** buffers; // channels array
    bool _cached;// whether it is already present in the previous buffer
    
    
    
};
bool compareRows(const Row &a,const Row &b);

#endif // ROW_H
