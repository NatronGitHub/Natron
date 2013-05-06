#ifndef ROW_H
#define ROW_H

#include <cstdlib>
#include <map>
#include "Core/channels.h"

#define MAX_BUFFERS_PER_ROW 100

class Node;
class Row
{
public:
    
    Row();
    Row(int x,int y,int range,ChannelSet channels);
    ~Row();
	
    float* writable(Channel c);
    void copy(const Row *source,ChannelSet channels,int o,int r);
    void get(Node &input,int y,int x,int range,ChannelSet channels);
    void erase(Channel c);
    void erase(ChannelSet set){
        foreachChannels(z, set){
            erase(z);
        }
    }
    const float* operator[](Channel z) const;
    int y() const {return _y;}
    void y(int nb){_y=nb;}
    int range(){return r;}
	void changeSize(int offset,int range);
    void turn_on(Channel c);
    int offset(){return x;}
    int zoomedY(){return _zoomedY;}
    void zoomedY(int z){_zoomedY = z;}
    
    
    ChannelMask channels(){return _channels;}
    
private:
    int _y;
    int _zoomedY;
    ChannelMask _channels;
    int x;
    int r;
    float** buffers;
    
    void init();
    
    
};
bool compareRows(const Row &a,const Row &b);

#endif // ROW_H
