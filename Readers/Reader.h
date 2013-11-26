//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#ifndef NATRON_READERS_READER_H_
#define NATRON_READERS_READER_H_
#include <QString>
#include <QStringList>
#include <QtCore/QMutex>
#include <QtCore/QFutureWatcher>

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

#include "Global/Macros.h"
#include "Engine/EffectInstance.h"
#include "Engine/LRUHashTable.h"
#include "Readers/Decoder.h"
namespace Natron{
    class FrameEntry;
}
class File_Knob;
class ComboBox_Knob;
class ViewerGL;
class ViewerCache;

/** @class Reader is the node associated to all image format readers. The reader creates the appropriate Read
 *to decode a certain image format.
**/
class Reader : public Natron::EffectInstance
{
    
public:
        
    
    /** @class This class manages the LRU buffer of the reader, i.e:
     *a reader can have several frames stored in its buffer.
     *The size of the buffer is variable, see Reader::setMaxFrameBufferSize(...)
     *The thread-safety of the buffer is assured by the Reader
    **/
    class Buffer{
    public:
        
        typedef boost::shared_ptr<Decoder> value_type;
        typedef std::string hash_type;
        
#ifdef USE_VARIADIC_TEMPLATES

#ifdef NATRON_CACHE_USE_BOOST
#ifdef NATRON_CACHE_USE_HASH
            typedef BoostLRUHashTable<hash_type, value_type>, boost::bimaps::unordered_set_of> CacheContainer;
#else
            typedef BoostLRUHashTable<hash_type, value_type > , boost::bimaps::set_of> CacheContainer;
#endif
            typedef CacheContainer::container_type::left_iterator CacheIterator;
            typedef CacheContainer::container_type::left_const_iterator ConstCacheIterator;
            static std::list<value_type>&  getValueFromIterator(CacheIterator it){return it->second;}

#else // cache use STL

#ifdef NATRON_CACHE_USE_HASH
            typedef StlLRUHashTable<hash_type,value_type >, std::unordered_map> CacheContainer;
#else
            typedef StlLRUHashTable<hash_type,value_type >, std::map> CacheContainer;
#endif
            typedef CacheContainer::key_to_value_type::iterator CacheIterator;
            typedef CacheContainer::key_to_value_type::const_iterator ConstCacheIterator;
            static std::list<value_type>&  getValueFromIterator(CacheIterator it){return it->second;}
#endif // NATRON_CACHE_USE_BOOST

#else // !USE_VARIADIC_TEMPLATES

#ifdef NATRON_CACHE_USE_BOOST
#ifdef NATRON_CACHE_USE_HASH
            typedef BoostLRUHashTable<hash_type,value_type> CacheContainer;
#else
            typedef BoostLRUHashTable<hash_type, value_type > CacheContainer;
#endif
            typedef CacheContainer::container_type::left_iterator CacheIterator;
            typedef CacheContainer::container_type::left_const_iterator ConstCacheIterator;
            static std::list<value_type>&  getValueFromIterator(CacheIterator it){return it->second;}

#else // cache use STL and tree (std map)

            typedef StlLRUHashTable<hash_type, value_type > CacheContainer;
            typedef CacheContainer::key_to_value_type::iterator CacheIterator;
            typedef CacheContainer::key_to_value_type::const_iterator ConstCacheIterator;
            static std::list<value_type>&   getValueFromIterator(CacheIterator it){return it->second.first;}
#endif // NATRON_CACHE_USE_BOOST

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
        void insert(const hash_type& key,value_type value){
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
        value_type get(const hash_type& key) {
            
            CacheIterator ret =  _buffer(key);
            if(ret != _buffer.end()){
                return *(getValueFromIterator(ret).begin());
            }else{
                return value_type();
            }
        }

    private:
        CacheContainer _buffer; /// decoding buffer
        int _maximumBufferSize; /// maximum size of the buffer
    };
    
    static Natron::EffectInstance* BuildEffect(Natron::Node* n){
        return new Reader(n);
    }
    
    Reader(Natron::Node* node);

    virtual ~Reader();
  
    /**
     * @brief Reads the head of the file associated to the time.
     * @param filename The file of the frame to decode.
     */
    boost::shared_ptr<Decoder> decodeHeader(const QString& filename);


    virtual bool makePreviewByDefault() const OVERRIDE {return true;}

    virtual std::string pluginID() const OVERRIDE;

    virtual std::string pluginLabel() const OVERRIDE;

    virtual std::string description() const OVERRIDE;

    virtual Natron::Status getRegionOfDefinition(SequenceTime time,RectI* rod) OVERRIDE;
	
    virtual void getFrameRange(SequenceTime *first,SequenceTime *last) OVERRIDE;

    
    
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
    
    virtual bool isGenerator() const OVERRIDE {return true;}

    virtual bool isInputOptional(int inputNb) const OVERRIDE;

    virtual Natron::Status preProcessFrame(SequenceTime time) OVERRIDE;

    virtual Natron::Status render(SequenceTime time,RenderScale scale,
                                   const RectI& roi,int view,boost::shared_ptr<Natron::Image> output) OVERRIDE;

    virtual void initializeKnobs() OVERRIDE;

    virtual Natron::EffectInstance::RenderSafety renderThreadSafety() const OVERRIDE {return Natron::EffectInstance::FULLY_SAFE;}

    virtual void onKnobValueChanged(Knob* k,Knob::ValueChangedReason reason) OVERRIDE;



private:
    
    
    boost::shared_ptr<Decoder> decoderForFileType(const QString& fileName);
    Buffer _buffer;
    File_Knob* _fileKnob;
    ComboBox_Knob* _missingFrameChoice;
    QMutex _lock;
    boost::scoped_ptr<QFutureWatcher<void> > _previewWatcher;
};




#endif // NATRON_READERS_READER_H_
