//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com


/**
 ALL THE CODE IN THAT FILE IS DEPRECATED AND NEED TO BE REVIEWED THOROUGHLY, DO NOT PAY HEED TO THE CODE WHILE THIS WARNING
 IS STILL PRESENT
**/

#ifndef __PowiterOsX__readffmpeg__
#define __PowiterOsX__readffmpeg__

#include "Reader/Read.h"
#include "Core/referenceCountedObj.h"
#include "Core/DataBuffer.h"
#include <iostream>
#include <QtCore/QMutex>
#ifdef _WIN32
#include <io.h>
#endif

extern "C" {
#include <errno.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavutil/error.h>
}
// Set non-zero to enable tracing of file information inspected while opening a file in FFmpegFile::FFmpegFile().
// Make sure this is disabled before checking-in this file.
#define TRACE_FILE_OPEN 0

// Set non-zero to enable tracing of FFmpegFile::decode() general processing (plus error reports).
// Make sure this is disabled before checking-in this file.
#define TRACE_DECODE_PROCESS 0

// Set non-zero to enable tracing of the first few bytes of each data block in the bitstream for each frame decoded. This
// assumes a 4-byte big-endian byte count at the start of each data block, followed by that many bytes of data. There may be
// multiple data blocks per frame. Each data block represents an atomic unit of data input to the decoder (e.g. an H.264 NALU).
// It works nicely for H.264 streams in .mov files but there's no guarantee for any other format. This can be useful if you
// need to see what kind of NALUs are being decoded.
// Make sure this is disabled before checking-in this file.
#define TRACE_DECODE_BITSTREAM 0

#define CHECK(x) \
{\
int error = x;\
if (error<0) {\
setInternalError(error);\
return;\
}\
}\


inline bool IsCodecBlacklisted(const char* name)
{
    static const char* codecBlacklist[] =
    {
        "r10k",
        "ogg",
        "mpjpeg",
        "asf",
        "asf_stream",
        "h261",
        "h263",
        "rcv",
        "yuv4mpegpipe",
        "prores",
        "dnxhd",
        "swf",
        "dv",
        "ipod",
        "psp",
        "image2",
        "3g2",
        "3gp",
        "RoQ",
        
#ifdef _WIN32
        
        "dirac",
        "ffm",
        
#else
        
        "vc1",
        
#endif
        NULL
    };
    
    const char** iterator = codecBlacklist;
    
    while (*iterator != NULL) {
        if (strncmp(name, *iterator, strlen(*iterator)) == 0) {
            return true;
        }
        
        ++iterator;
    }
    return false;
}
class Row;
class FFMPEGFile : public ReferenceCountedObject{
    
    struct Stream
    {
        int _idx;                      // stream index
        AVStream* _avstream;           // video stream
        AVCodecContext* _codecContext; // video codec context
        AVCodec* _videoCodec;
        AVFrame* _avFrame;             // decoding frame
        SwsContext* _convertCtx;
        
        int _fpsNum;
        int _fpsDen;
        
        int64_t _startPTS;     // PTS of the first frame in the stream
        int64_t _frames;       // video duration in frames
        
        bool _ptsSeen;                      // True if a read AVPacket has ever contained a valid PTS during this stream's decode,
        // indicating that this stream does contain PTSs.
        int64_t AVPacket::*_timestampField; // Pointer to member of AVPacket from which timestamps are to be retrieved. Enables
        // fallback to using DTSs for a stream if PTSs turn out not to be available.
        
        int _width;
        int _height;
        double _aspect;
        
        int _decodeNextFrameIn; // The 0-based index of the next frame to be fed into decode. Negative before any
        // frames have been decoded or when we've just seeked but not yet found a relevant frame. Equal to
        // frames_ when all available frames have been fed into decode.
        
        int _decodeNextFrameOut; // The 0-based index of the next frame expected out of decode. Negative before
        // any frames have been decoded or when we've just seeked but not yet found a relevant frame. Equal to
        // frames_ when all available frames have been output from decode.
        
        int _accumDecodeLatency; // The number of frames that have been input without any frame being output so far in this stream
        // since the last seek. This is part of a guard mechanism to detect when decode appears to have
        // stalled and ensure that FFmpegFile::decode() does not loop indefinitely.
        
        Stream()
        : _idx(0)
        , _avstream(NULL)
        , _codecContext(NULL)
        , _videoCodec(NULL)
        , _avFrame(NULL)
        , _convertCtx(NULL)
        , _fpsNum(1)
        , _fpsDen(1)
        , _startPTS(0)
        , _frames(0)
        , _ptsSeen(false)
        , _timestampField(&AVPacket::pts)
        , _width(0)
        , _height(0)
        , _aspect(1.0)
        , _decodeNextFrameIn(-1)
        , _decodeNextFrameOut(-1)
        , _accumDecodeLatency(0)
        {}
        
        ~Stream()
        {
            
            if (_avFrame)
                av_free(_avFrame);
            
            if (_codecContext)
                avcodec_close(_codecContext);
            
            if (_convertCtx)
                sws_freeContext(_convertCtx);
        }
        
        static void destroy(Stream* s)
        {
            delete(s);
        }
        
        int64_t frameToPts(int frame) const
        {
            return _startPTS + (int64_t(frame) * _fpsDen *  _avstream->time_base.den) /
            (int64_t(_fpsNum) * _avstream->time_base.num);
        }
        
        int ptsToFrame(int64_t pts) const
        {
            return (int64_t(pts - _startPTS) * _avstream->time_base.num *  _fpsNum) /
            (int64_t(_avstream->time_base.den) * _fpsDen);
        }
        
        SwsContext* getConvertCtx()
        {
            if (!_convertCtx)
                _convertCtx = sws_getContext(_width, _height, _codecContext->pix_fmt, _width, _height, PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);
            
            return _convertCtx;
        }
        
        // Return the number of input frames needed by this stream's codec before it can produce output. We expect to have to
        // wait this many frames to receive output; any more and a decode stall is detected.
        int getCodecDelay() const
        {
            return ((_videoCodec->capabilities & CODEC_CAP_DELAY) ? _codecContext->delay : 0) + _codecContext->has_b_frames;
        }
    };
    // AV structure
    AVFormatContext* _context;
    AVInputFormat*   _format;
    
    // store all video streams available in the file
    std::vector<Stream*> _streams;
    
    // reader error state
    std::string _errorMsg;  // internal decoding error string
    bool _invalidState;     // true if the reader is in an invalid state
    
    AVPacket _avPacket;
    
    // internal lock for multithread access
    QMutex _locker;
    
    // set reader error
    void setError(const char* msg, const char* prefix = 0);
    
    // set FFmpeg library error
    void setInternalError(const int error, const char* prefix = 0);
    
    // get stream start time
    int64_t getStreamStartTime(Stream& stream);
    
    // Get the video stream duration in frames...
    int64_t getStreamFrames(Stream& stream);
    
public:
    typedef RefCPtr<FFMPEGFile> Ptr;
    FFMPEGFile(const char* filename);
    virtual ~FFMPEGFile();

    // get the internal error string
    const char* error() const;
    
    // return true if the reader can't decode the frame
    bool invalid() const;
    
    // return the numbers of streams supported by the reader
    unsigned streams() const;
    
    // decode a single frame into the buffer thread safe
    bool decode(unsigned char* buffer, unsigned frame, unsigned streamIdx = 0);
    
    // get stream information
    bool info( int& width,int& height,double& aspect,int& frames,unsigned streamIdx = 0);
    
    QMutex* getLock(){return &_locker;}
};

class FFMPEGFileManager {
    typedef std::map<std::string, FFMPEGFile::Ptr> ReaderMap;
    ReaderMap _readers;
    QMutex lock_;
    
    // A lock manager function for FFmpeg, enabling it to use mutexes managed by this reader. Pass to av_lockmgr_register().
    static int FFMPEGLockManager(void** mutex, enum AVLockOp op);
    
public:
   

    
    FFMPEGFileManager();
    FFMPEGFile::Ptr get(const char* filename);
    void release(const char* filename);
    
    
    
};

static FFMPEGFileManager _readerManager;

class ReadFFMPEG : public Read
{
    
    
public:
    static Read* BuildRead(Reader* reader){return new ReadFFMPEG(reader);}
    
    ReadFFMPEG(Reader* op);
    virtual ~ReadFFMPEG();
    
    /*Should return the list of file types supported by the decoder: "png","jpg", etc..*/
    virtual std::vector<std::string> fileTypesDecoded(){
        std::vector<std::string> out;
        out.push_back("jpg");
        out.push_back("png");
        // TODO : complete the list
        return out;
    };
    
    /*Should return the name of the reader : "ffmpeg", "OpenEXR" ...*/
    virtual std::string decoderName(){return "FFmpeg";}
    virtual void engine(int y,int offset,int range,ChannelMask channels,Row* out);
    virtual bool supports_stereo(){return false;}
    virtual void readHeader(const QString filename,bool openBothViews);
    virtual void readAllData(bool openBothViews);
    virtual bool supportsScanLine(){return false;}
    virtual void make_preview();
    virtual void initializeColorSpace();

    bool fillBuffer();
    
private:
    FFMPEGFile::Ptr _reader;
    DataBuffer::Ptr _data;    // decoding buffer
    size_t _memNeeded;                // memory needed for decoding a single frame
    int  _numFrames;                  // number of frames in the selected stream
    QString _file;
	QMutex _lock;
    
};

#endif /* defined(__PowiterOsX__readffmpeg__) */
