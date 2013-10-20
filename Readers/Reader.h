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

#ifndef POWITER_READERS_READER_H_
#define POWITER_READERS_READER_H_
#include <QString>
#include <QStringList>
#include <QtCore/QMutex>
#include <QtCore/QFutureWatcher>

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

#include "Global/Macros.h"
#include "Engine/Node.h"
#include "Engine/LRUcache.h"
namespace Powiter{
    class FrameEntry;
}
class File_Knob;
class ViewerGL;
class Decoder;
class ViewerCache;

/** @class Reader is the node associated to all image format readers. The reader creates the appropriate Read
 *to decode a certain image format.
**/
class Reader : public Node
{
    Q_OBJECT
    
public:
        
    
    /** @class This class manages the LRU buffer of the reader, i.e:
     *a reader can have several frames stored in its buffer.
     *The size of the buffer is variable, see Reader::setMaxFrameBufferSize(...)
     *The thread-safety of the buffer is assured by the Reader
    **/
    class Buffer{
    public:
        
        typedef boost::shared_ptr<Decoder> value_type;
        typedef std::string key_type;
        
#ifdef USE_VARIADIC_TEMPLATES
        
#ifdef POWITER_CACHE_USE_BOOST
#ifdef POWITER_CACHE_USE_HASH
        typedef BoostLRUCacheContainer<key_type, value_type >, boost::bimaps::unordered_set_of> CacheContainer;
#else
        typedef BoostLRUCacheContainer<key_type, value_type , boost::bimaps::set_of> CacheContainer;
#endif
        typedef typename CacheContainer::container_type::left_iterator CacheIterator;
        typedef typename CacheContainer::container_type::left_const_iterator ConstCacheIterator;
        static value_type getValueFromIterator(CacheIterator it){return it->second;}
        
#else // cache use STL
        
#ifdef POWITER_CACHE_USE_HASH
        typedef StlLRUCache<key_type,value_type, std::unordered_map> CacheContainer;
#else
        typedef StlLRUCache<key_type,value_type, std::map> CacheContainer;
#endif
        typedef typename CacheContainer::key_to_value_type::iterator CacheIterator;
        typedef typename CacheContainer::key_to_value_type::const_iterator ConstCacheIterator;
        static value_type  getValueFromIterator(CacheIterator it){return it->second;}
#endif // POWITER_CACHE_USE_BOOST
        
#else // !USE_VARIADIC_TEMPLATES
        
#ifdef POWITER_CACHE_USE_BOOST
#ifdef POWITER_CACHE_USE_HASH
        typedef BoostLRUHashCache<key_type, value_type > CacheContainer;
#else
        typedef BoostLRUTreeCache<key_type, value_type > CacheContainer;
#endif
        typedef typename CacheContainer::container_type::left_iterator CacheIterator;
        typedef typename CacheContainer::container_type::left_const_iterator ConstCacheIterator;
        static value_type  getValueFromIterator(CacheIterator it){return it->second;}
        
#else // cache use STL and tree (std map)
        
        typedef StlLRUTreeCache<key_type, value_type> CacheContainer;
        typedef typename CacheContainer::key_to_value_type::iterator CacheIterator;
        typedef typename CacheContainer::key_to_value_type::const_iterator ConstCacheIterator;
        static value_type  getValueFromIterator(CacheIterator it){return it->second.first;}
#endif // POWITER_CACHE_USE_BOOST
#endif // USE_VARIADIC_TEMPLATES
        
        /**
         * @brief Construct an Empty buffer capable of containing 2 frames.
         */
        Buffer():_maximumBufferSize(5){}

        ~Buffer(){clear();}

        /**
         * @brief Clears out the buffer.
         */
        void clear(){
            _buffer.clear();
        }

        /**
         * @brief SetSize will fix the maximum size of the buffer in frames.
         * @param size Size of the buffer in frames.
         */
        void setMaximumSize(int size){ _maximumBufferSize = size; }
        
        /**
         * @brief Inserts a new frame into the buffer.
         * @param desc  The frame will be identified by this descriptor.
         */
        void insert(const key_type& key,value_type value){
            if((int)_buffer.size() > _maximumBufferSize){
                _buffer.evict();
            }
            _buffer.insert(key,value);
        }
        
        /**
         * @brief  find
         * @param filename The name of the frame.
         * @return Returns an iterator pointing to a valid frame in the buffer if it could find it. Otherwise points to end()
         */
        value_type get(const key_type& key) {
            return _buffer(key);
        }

    private:
        CacheContainer _buffer; /// decoding buffer
        int _maximumBufferSize; /// maximum size of the buffer
    };
    
    Reader(Model* model);

    virtual ~Reader();
    
    /**
     * @brief hasFrames
     * @return True if the files list contains at list a file.
     */
	bool hasFrames() const;
    
    
    /**
     * @brief Reads the head of the file associated to the time.
     * @param filename The file of the frame to decode.
     */
    boost::shared_ptr<Decoder> decodeHeader(const QString& filename);
    
    /**
     * @brief firstFrame
     * @return Returns the index of the first frame in the sequence held by this Reader.
     */
    int firstFrame();

    /**
     * @brief lastFrame
     * @return Returns the index of the last frame in the sequence held by this Reader.
     */
	int lastFrame();
    
    /**
     * @brief nearestFrame
     * @return Returns the index of frame clamped to the Range [ firstFrame() - lastFrame( ].
     * @param f The index of the frame to clamp.
     */
    int nearestFrame(int f);
    
    /**
     * @brief getRandomFrameName
     * @param f The index of the frame.
     * @return The file name associated to the frame index. Returns an empty string if it couldn't find it.
     */
    const QString getRandomFrameName(int /*frame*/) const;

    virtual bool canMakePreviewImage() const {return true;}

    /**
     * @brief className
     * @return A string containing "Reader".
     */
    virtual std::string className() const OVERRIDE;

    /**
     * @brief description
     * @return A string containing "InputNode".
     */
    virtual std::string description() const OVERRIDE;

    virtual Powiter::Status getRegionOfDefinition(SequenceTime time,Box2D* rod);
	
    virtual void getFrameRange(SequenceTime *first,SequenceTime *last){
        *first = _frameRange.first;
        *last  = _frameRange.second;
    }
    
    /**
     * @brief Calls Read::engine(int,int,int,ChannelSet,Row*)
     */
	virtual void render(SequenceTime time,Powiter::Row* out) OVERRIDE;
    
    /**
     * @brief cacheData
     * @return true, indicating that the reader caches data.
     */
    virtual bool cacheData() const OVERRIDE { return true; }
    
   
    
    /** @brief Set the number of frames that a single reader can store.
    * Must be minimum 2. The appropriate way to call this
    * function is in the constructor of your Read*.
    * You shouldn't need to call this function. The
    * purpose of this function is to let the reader
    * have more buffering space for certain reader
    * that have a variable throughput (e.g: frameserver).
    **/
    void setMaxFrameBufferSize(int size){
        if(size < 2) return;
        _buffer.setMaximumSize(size);
    }
    
    
    virtual int maximumInputs() const OVERRIDE {return 0;}

    virtual int minimumInputs() const OVERRIDE {return 0;}
    
    virtual bool isInputNode() const OVERRIDE {return true;}
        
public slots:
    
    
    void onFrameRangeChanged(int first,int last);

protected:

	virtual void initKnobs() OVERRIDE;
    
private:
    
    
    boost::shared_ptr<Decoder> decoderForFileType(const QString& fileName);
    std::pair<int,int> _frameRange;
    Buffer _buffer;
    File_Knob* _fileKnob;
    QMutex _lock;
    boost::scoped_ptr<QFutureWatcher<void> > _previewWatcher;
};




#endif // POWITER_READERS_READER_H_
