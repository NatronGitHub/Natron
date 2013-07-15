//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

/**
 ALL THE CODE IN THAT FILE IS DEPRECATED AND NEED TO BE REVIEWED THOROUGHLY, DO NOT PAY HEED TO THE CODE WHILE THIS WARNING
 IS STILL PRESENT
 **/

#include "Reader/readffmpeg.h"
#include "Reader/Reader.h"
#include "Gui/GLViewer.h"
#include "Core/channels.h"
#include "Gui/node_ui.h"
#include "Core/row.h"
using namespace std;
using namespace Powiter_Enums;



void FFMPEGFile::setError(const char* msg, const char* prefix )
{
    if (prefix) {
        _errorMsg = prefix;
        _errorMsg += msg;
#if TRACE_DECODE_PROCESS
        std::cout << "!!ERROR: " << prefix << msg << std::endl;
#endif
    }
    else {
        _errorMsg = msg;
#if TRACE_DECODE_PROCESS
        std::cout << "!!ERROR: " << msg << std::endl;
#endif
    }
    _invalidState = true;
}



void FFMPEGFile::setInternalError(const int error, const char* prefix )
{
    char errorBuf[1024];
    av_strerror(error, errorBuf, sizeof(errorBuf));
    setError(errorBuf, prefix);
}


int64_t FFMPEGFile::getStreamStartTime(Stream& stream)
{
#if TRACE_FILE_OPEN
    std::cout << "      Determining stream start PTS:" << std::endl;
#endif
    
    // Read from stream. If the value read isn't valid, get it from the first frame in the stream that provides such a
    // value.
    int64_t startPTS = stream._avstream->start_time;
#if TRACE_FILE_OPEN
    if (startPTS != int64_t(AV_NOPTS_VALUE))
        std::cout << "        Obtained from AVStream::start_time=";
#endif
    
    if (startPTS ==  int64_t(AV_NOPTS_VALUE)) {
#if TRACE_FILE_OPEN
        std::cout << "        Not specified by AVStream::start_time, searching frames..." << std::endl;
#endif
        
        // Seek 1st key-frame in video stream.
        avcodec_flush_buffers(stream._codecContext);
        
        if (av_seek_frame(_context, stream._idx, 0, 0) >= 0) {
            av_init_packet(&_avPacket);
            
            // Read frames until we get one for the video stream that contains a valid PTS.
            do {
                if (av_read_frame(_context, &_avPacket) < 0)  {
                    // Read error or EOF. Abort search for PTS.
#if TRACE_FILE_OPEN
                    std::cout << "          Read error, aborted search" << std::endl;
#endif
                    break;
                }
                if (_avPacket.stream_index == stream._idx) {
                    // Packet read for video stream. Get its PTS. Loop will continue if the PTS is AV_NOPTS_VALUE.
                    startPTS = _avPacket.pts;
                }
                
                av_free_packet(&_avPacket);
            } while (startPTS ==  int64_t(AV_NOPTS_VALUE));
        }
#if TRACE_FILE_OPEN
        else
            std::cout << "          Seek error, aborted search" << std::endl;
#endif
        
#if TRACE_FILE_OPEN
        if (startPTS != int64_t(AV_NOPTS_VALUE))
            std::cout << "        Found by searching frames=";
#endif
    }
    
    // If we still don't have a valid initial PTS, assume 0. (This really shouldn't happen for any real media file, as
    // it would make meaningful playback presentation timing and seeking impossible.)
    if (startPTS ==  int64_t(AV_NOPTS_VALUE)) {
#if TRACE_FILE_OPEN
        std::cout << "        Not found by searching frames, assuming ";
#endif
        startPTS = 0;
    }
    
#if TRACE_FILE_OPEN
    std::cout << startPTS << " ticks, " << double(startPTS) * double(stream._avstream->time_base.num) /
    double(stream._avstream->time_base.den) << " s" << std::endl;
#endif
    
    return startPTS;
}


int64_t FFMPEGFile::getStreamFrames(Stream& stream)
{
#if TRACE_FILE_OPEN
    std::cout << "      Determining stream frame count:" << std::endl;
#endif
    
    // Obtain from stream's number of frames if specified. Will be 0 if unknown.
    int64_t frames = stream._avstream->nb_frames;
#if TRACE_FILE_OPEN
    if (frames)
        std::cout << "        Obtained from AVStream::nb_frames=";
#endif
    
    // If number of frames still unknown, attempt to calculate from stream's duration, fps and timebase. This is
    // preferable to using AVFormatContext's duration as this value is exactly specified using rational numbers, so can
    // correctly represent durations that would be infinitely recurring when expressed in decimal form. By contrast,
    // durations in AVFormatContext are represented in units of AV_TIME_BASE (1000000), so may be imprecise, leading to
    // loss of an otherwise-available frame.
    if (!frames) {
#if TRACE_FILE_OPEN
        std::cout << "        Not specified by AVStream::nb_frames, calculating from duration & framerate..." << std::endl;
#endif
        frames = (int64_t(stream._avstream->duration) * stream._avstream->time_base.num  * stream._fpsNum) /
        (int64_t(stream._avstream->time_base.den) * stream._fpsDen);
#if TRACE_FILE_OPEN
        if (frames)
            std::cout << "        Calculated from duration & framerate=";
#endif
    }
    
    // If the number of frames is still unknown, attempt to measure it from the last frame PTS for the stream in the
    // file relative to first (which we know from earlier).
    if (!frames) {
#if TRACE_FILE_OPEN
        std::cout << "        Not specified by duration & framerate, searching frames for last PTS..." << std::endl;
#endif
        
        int64_t maxPts = stream._startPTS;
        
        // Seek last key-frame.
        avcodec_flush_buffers(stream._codecContext);
        av_seek_frame(_context, stream._idx, stream.frameToPts(1<<29), AVSEEK_FLAG_BACKWARD);
        
        // Read up to last frame, extending max PTS for every valid PTS value found for the video stream.
        av_init_packet(&_avPacket);
        
        while (av_read_frame(_context, &_avPacket) >= 0) {
            if (_avPacket.stream_index == stream._idx && _avPacket.pts != int64_t(AV_NOPTS_VALUE) && _avPacket.pts > maxPts)
                maxPts = _avPacket.pts;
            av_free_packet(&_avPacket);
        }
#if TRACE_FILE_OPEN
        std::cout << "          Start PTS=" << stream._startPTS << ", Max PTS found=" << maxPts << std::endl;
#endif
        
        // Compute frame range from min to max PTS. Need to add 1 as both min and max are at starts of frames, so stream
        // extends for 1 frame beyond this.
        frames = 1 + stream.ptsToFrame(maxPts);
#if TRACE_FILE_OPEN
        std::cout << "        Calculated from frame PTS range=";
#endif
    }
    
#if TRACE_FILE_OPEN
    std::cout << frames << std::endl;
#endif
    
    return frames;
}



FFMPEGFile::FFMPEGFile(const char* filename)
: _context(NULL)
, _format(NULL)
, _invalidState(false)
, _locker()
{
    // FIXME_GC: shouldn't the plugin be passed the filename without the prefix?
    int offset = 0;
    if (std::string(filename).find("ffmpeg:") != std::string::npos)
        offset = 7;
    
#if TRACE_FILE_OPEN
    std::cout << "ffmpegReader=" << this << "::c'tor(): filename=" << filename + offset << std::endl;
#endif
    
    CHECK( avformat_open_input(&_context, filename + offset, _format, NULL) );
    CHECK( avformat_find_stream_info(_context, NULL) );
    
#if TRACE_FILE_OPEN
    std::cout << "  " << _context->nb_streams << " streams:" << std::endl;
#endif
    
    // fill the array with all available video streams
    bool unsuported_codec = false;
    
    // find all streams that the library is able to decode
    for (unsigned i = 0; i < _context->nb_streams; ++i) {
#if TRACE_FILE_OPEN
        std::cout << "    FFmpeg stream index " << i << ": ";
#endif
        AVStream* avstream = _context->streams[i];
        
        // be sure to have a valid stream
        if (!avstream || !avstream->codec) {
#if TRACE_FILE_OPEN
            std::cout << "No valid stream or codec, skipping..." << std::endl;
#endif
            continue;
        }
        
        // considering only video streams, skipping audio
        if (avstream->codec->codec_type != AVMEDIA_TYPE_VIDEO) {
#if TRACE_FILE_OPEN
            std::cout << "Not a video stream, skipping..." << std::endl;
#endif
            continue;
        }
        
        // find the codec
        AVCodec* videoCodec = avcodec_find_decoder(avstream->codec->codec_id);
        if (videoCodec == NULL) {
#if TRACE_FILE_OPEN
            std::cout << "Decoder not found, skipping..." << std::endl;
#endif
            continue;
        }
        
        // skip codec from the black list
        if (IsCodecBlacklisted(videoCodec->name)) {
#if TRACE_FILE_OPEN
            std::cout << "Decoder blacklisted, skipping..." << std::endl;
#endif
            unsuported_codec = true;
            continue;
        }
        
        // skip if the codec can't be open
        if (avcodec_open2(avstream->codec, videoCodec, NULL) < 0) {
#if TRACE_FILE_OPEN
            std::cout << "Decoder failed to open, skipping..." << std::endl;
#endif
            continue;
        }
        
#if TRACE_FILE_OPEN
        std::cout << "Video decoder opened ok, getting stream properties:" << std::endl;
#endif
        
        Stream* stream = new Stream();
        stream->_idx = i;
        stream->_avstream = avstream;
        stream->_codecContext = avstream->codec;
        stream->_videoCodec = videoCodec;
        stream->_avFrame = avcodec_alloc_frame();
        
#if TRACE_FILE_OPEN
        std::cout << "      Timebase=" << avstream->time_base.num << "/" << avstream->time_base.den << " s/tick" << std::endl;
        std::cout << "      Duration=" << avstream->duration << " ticks, " <<
        double(avstream->duration) * double(avstream->time_base.num) /
        double(avstream->time_base.den) << " s" << std::endl;
#endif
        
        // If FPS is specified, record it.
        // Otherwise assume 1 fps (default value).
        if ( avstream->r_frame_rate.num != 0 &&  avstream->r_frame_rate.den != 0 ) {
            stream->_fpsNum = avstream->r_frame_rate.num;
            stream->_fpsDen = avstream->r_frame_rate.den;
#if TRACE_FILE_OPEN
            std::cout << "      Framerate=" << stream->_fpsNum << "/" << stream->_fpsDen << ", " <<
            double(stream->_fpsNum) / double(stream->_fpsDen) << " fps" << std::endl;
#endif
        }
#if TRACE_FILE_OPEN
        else
            std::cout << "      Framerate unspecified, assuming 1 fps" << std::endl;
#endif
        
        stream->_width  = avstream->codec->width;
        stream->_height = avstream->codec->height;
#if TRACE_FILE_OPEN
        std::cout << "      Image size=" << stream->_width << "x" << stream->_height << std::endl;
#endif
        
        // set aspect ratio
        if (stream->_avstream->sample_aspect_ratio.num) {
            stream->_aspect = av_q2d(stream->_avstream->sample_aspect_ratio);
#if TRACE_FILE_OPEN
            std::cout << "      Aspect ratio (from stream)=" << stream->_aspect << std::endl;
#endif
        }
        else if (stream->_codecContext->sample_aspect_ratio.num) {
            stream->_aspect = av_q2d(stream->_codecContext->sample_aspect_ratio);
#if TRACE_FILE_OPEN
            std::cout << "      Aspect ratio (from codec)=" << stream->_aspect << std::endl;
#endif
        }
#if TRACE_FILE_OPEN
        else
            std::cout << "      Aspect ratio unspecified, assuming " << stream->_aspect << std::endl;
#endif
        
        // set stream start time and numbers of frames
        stream->_startPTS = getStreamStartTime(*stream);
        stream->_frames   = getStreamFrames(*stream);
        
        // save the stream
        _streams.push_back(stream);
    }
    
    if (_streams.empty())
        setError( unsuported_codec ? "unsupported codec..." : "unable to find video stream" );
}


FFMPEGFile::~FFMPEGFile(){
    // force to close all resources needed for all streams
    std::for_each(_streams.begin(), _streams.end(), Stream::destroy);
    
    if (_context)
        avformat_close_input(&_context);
    
}

const char* FFMPEGFile::error() const{
    return _errorMsg.c_str();
}

bool FFMPEGFile::invalid() const
{
    return _invalidState;
}

unsigned FFMPEGFile::streams() const
{
    return _streams.size();
}

// decode a single frame into the buffer thread safe
bool FFMPEGFile::decode(unsigned char* buffer, unsigned frame, unsigned streamIdx )
{
    QMutexLocker guard(&_locker);
    
    if (streamIdx >= _streams.size())
        return false;
    
    // get the stream
    Stream* stream = _streams[streamIdx];
    
    // Translate from the 1-based frames expected by Nuke to 0-based frame offsets for use in the rest of this code.
    int desiredFrame = frame - 1;
    
    // Early-out if out-of-range frame requested.
    if (desiredFrame < 0 || desiredFrame >= stream->_frames)
        return false;
    
#if TRACE_DECODE_PROCESS
    std::cout << "ffmpegReader=" << this << "::decode(): desiredFrame=" << desiredFrame << ", videoStream=" << streamIdx << ", streamIdx=" << stream->_idx << std::endl;
#endif
    
    // Number of read retries remaining when decode stall is detected before we give up (in the case of post-seek stalls,
    // such retries are applied only after we've searched all the way back to the start of the file and failed to find a
    // successful start point for playback)..
    //
    // We have a rather annoying case with a small subset of media files in which decode latency (between input and output
    // frames) will exceed the maximum above which we detect decode stall at certain frames on the first pass through the
    // file but those same frames will decode succesfully on a second attempt. The root cause of this is not understood but
    // it appears to be some oddity of FFmpeg. While I don't really like it, retrying decode enables us to successfully
    // decode those files rather than having to fail the read.
    int retriesRemaining = 1;
    
    // Whether we have just performed a seek and are still awaiting the first decoded frame after that seek. This controls
    // how we respond when a decode stall is detected.
    //
    // One cause of such stalls is when a file contains incorrect information indicating that a frame is a key-frame when it
    // is not; a seek may land at such a frame but the decoder will not be able to start decoding until a real key-frame is
    // reached, which may be a long way in the future. Once a frame has been decoded, we will expect it to be the first frame
    // input to decode but it will actually be the next real key-frame found, leading to subsequent frames appearing as
    // earlier frame numbers and the movie ending earlier than it should. To handle such cases, when a stall is detected
    // immediately after a seek, we seek to the frame before the previous seek's landing frame, allowing us to search back
    // through the movie for a valid key frame from which decode commences correctly; if this search reaches the beginning of
    // the movie, we give up and fail the read, thus ensuring that this method will exit at some point.
    //
    // Stalls once seeking is complete and frames are being decoded are handled differently; these result in immediate read
    // failure.
    bool awaitingFirstDecodeAfterSeek = false;
    
    // If the frame we want is not the next one to be decoded, seek to the keyframe before/at our desired frame. Set the last
    // seeked frame to indicate that we need to synchronise frame indices once we've read the first frame of the video stream,
    // since we don't yet know which frame number the seek will land at. Also invalidate current indices, reset accumulated
    // decode latency and record that we're awaiting the first decoded frame after a seek.
    int lastSeekedFrame = -1; // 0-based index of the last frame to which we seeked when seek in progress / negative when no
    // seek in progress,
    
    if (desiredFrame != stream->_decodeNextFrameOut) {
#if TRACE_DECODE_PROCESS
        std::cout << "  Next frame expected out=" << stream->_decodeNextFrameOut << ", Seeking to desired frame" << std::endl;
#endif
        
        lastSeekedFrame = desiredFrame;
        stream->_decodeNextFrameIn  = -1;
        stream->_decodeNextFrameOut = -1;
        stream->_accumDecodeLatency = 0;
        awaitingFirstDecodeAfterSeek = true;
        
        avcodec_flush_buffers(stream->_codecContext);
        int error = av_seek_frame(_context, stream->_idx, stream->frameToPts(desiredFrame), AVSEEK_FLAG_BACKWARD);
        if (error < 0) {
            // Seek error. Abort attempt to read and decode frames.
            setInternalError(error, "FFmpeg Reader failed to seek frame: ");
            return false;
        }
    }
#if TRACE_DECODE_PROCESS
    else
        std::cout << "  Next frame expected out=" << stream->_decodeNextFrameOut << ", No seek required" << std::endl;
#endif
    
    av_init_packet(&_avPacket);
    
    // Loop until the desired frame has been decoded. May also break from within loop on failure conditions where the
    // desired frame will never be decoded.
    bool hasPicture = false;
    do {
        bool decodeAttempted = false;
        int frameDecoded = 0;
        
        // If the next frame to decode is within range of frames (or negative implying invalid; we've just seeked), read
        // a new frame from the source file and feed it to the decoder if it's for the video stream.
        if (stream->_decodeNextFrameIn < stream->_frames) {
#if TRACE_DECODE_PROCESS
            std::cout << "  Next frame expected in=";
            if (stream->_decodeNextFrameIn >= 0)
                std::cout << stream->_decodeNextFrameIn;
            else
                std::cout << "unknown";
#endif
            
            int error = av_read_frame(_context, &_avPacket);
            if (error < 0) {
                // Read error. Abort attempt to read and decode frames.
#if TRACE_DECODE_PROCESS
                std::cout << ", Read failed" << std::endl;
#endif
                setInternalError(error, "FFmpeg Reader failed to read frame: ");
                break;
            }
#if TRACE_DECODE_PROCESS
            std::cout << ", Read OK, Packet data:" << std::endl;
            std::cout << "    PTS=" << _avPacket.pts <<
            ", DTS=" << _avPacket.dts <<
            ", Duration=" << _avPacket.duration <<
            ", KeyFrame=" << ((_avPacket.flags & AV_PKT_FLAG_KEY) ? 1 : 0) <<
            ", Corrupt=" << ((_avPacket.flags & AV_PKT_FLAG_CORRUPT) ? 1 : 0) <<
            ", StreamIdx=" << _avPacket.stream_index <<
            ", PktSize=" << _avPacket.size;
#endif
            
            // If the packet read belongs to the video stream, synchronise frame indices from it if required and feed it
            // into the decoder.
            if (_avPacket.stream_index == stream->_idx) {
#if TRACE_DECODE_PROCESS
                std::cout << ", Relevant stream" << std::endl;
#endif
                
                // If the packet read has a valid PTS, record that we have seen a PTS for this stream.
                if (_avPacket.pts != int64_t(AV_NOPTS_VALUE))
                    stream->_ptsSeen = true;
                
                // If a seek is in progress, we need to synchronise frame indices if we can...
                if (lastSeekedFrame >= 0) {
#if TRACE_DECODE_PROCESS
                    std::cout << "    In seek (" << lastSeekedFrame << ")";
#endif
                    
                    // Determine which frame the seek landed at, using whichever kind of timestamp is currently selected for this
                    // stream. If there's no timestamp available at that frame, we can't synchronise frame indices to know which
                    // frame we're first going to decode, so we need to seek back to an earlier frame in hope of obtaining a
                    // timestamp. Likewise, if the landing frame is after the seek target frame (this can happen, presumably a bug
                    // in FFmpeg seeking), we need to seek back to an earlier frame so that we can start decoding at or before the
                    // desired frame.
                    int landingFrame;
                    if (_avPacket.*stream->_timestampField == int64_t(AV_NOPTS_VALUE) ||
                        (landingFrame = stream->ptsToFrame(_avPacket.*stream->_timestampField)) > lastSeekedFrame) {
#if TRACE_DECODE_PROCESS
                        std::cout << ", landing frame not found";
                        if (_avPacket.*stream->_timestampField == int64_t(AV_NOPTS_VALUE))
                            std::cout << " (no timestamp)";
                        else
                            std::cout << " (landed after target at " << landingFrame << ")";
#endif
                        
                        // Wind back 1 frame from last seeked frame. If that takes us to before frame 0, we're never going to be
                        // able to synchronise using the current timestamp source...
                        if (--lastSeekedFrame < 0) {
#if TRACE_DECODE_PROCESS
                            std::cout << ", can't seek before start";
#endif
                            
                            // If we're currently using PTSs to determine the landing frame and we've never seen a valid PTS for any
                            // frame from this stream, switch to using DTSs and retry the read from the initial desired frame.
                            if (stream->_timestampField == &AVPacket::pts && !stream->_ptsSeen) {
                                stream->_timestampField = &AVPacket::dts;
                                lastSeekedFrame = desiredFrame;
#if TRACE_DECODE_PROCESS
                                std::cout << ", PTSs absent, switching to use DTSs";
#endif
                            }
                            // Otherwise, failure to find a landing point isn't caused by an absence of PTSs from the file or isn't
                            // recovered by using DTSs instead. Something is wrong with the file. Abort attempt to read and decode frames.
                            else {
#if TRACE_DECODE_PROCESS
                                if (stream->_timestampField == &AVPacket::dts)
                                    std::cout << ", search using DTSs failed";
                                else
                                    std::cout << ", PTSs present";
                                std::cout << ",  giving up" << std::endl;
#endif
                                setError("FFmpeg Reader failed to find timing reference frame, possible file corruption");
                                break;
                            }
                        }
                        
                        // Seek to the new frame. By leaving the seek in progress, we will seek backwards frame by frame until we
                        // either successfully synchronise frame indices or give up having reached the beginning of the stream.
#if TRACE_DECODE_PROCESS
                        std::cout << ", seeking to " << lastSeekedFrame << std::endl;
#endif
                        avcodec_flush_buffers(stream->_codecContext);
                        error = av_seek_frame(_context, stream->_idx, stream->frameToPts(lastSeekedFrame), AVSEEK_FLAG_BACKWARD);
                        if (error < 0) {
                            // Seek error. Abort attempt to read and decode frames.
                            setInternalError(error, "FFmpeg Reader failed to seek frame: ");
                            break;
                        }
                    }
                    // Otherwise, we have a valid landing frame, so set that as the next frame into and out of decode and set
                    // no seek in progress.
                    else {
#if TRACE_DECODE_PROCESS
                        std::cout << ", landed at " << landingFrame << std::endl;
#endif
                        stream->_decodeNextFrameOut = stream->_decodeNextFrameIn = landingFrame;
                        lastSeekedFrame = -1;
                    }
                }
                
                // If there's no seek in progress, feed this frame into the decoder.
                if (lastSeekedFrame < 0) {
#if TRACE_DECODE_BITSTREAM
                    std::cout << "  Decoding input frame " << stream->_decodeNextFrameIn << " bitstream:" << std::endl;
                    uint8_t *data = _avPacket.data;
                    uint32_t remain = _avPacket.size;
                    while (remain > 0) {
                        if (remain < 4) {
                            std::cout << "    Insufficient remaining bytes (" << remain << ") for block size at BlockOffset=" << (data - _avPacket.data) << std::endl;
                            remain = 0;
                        }
                        else {
                            uint32_t blockSize = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
                            data += 4;
                            remain -= 4;
                            std::cout << "    BlockOffset=" << (data - _avPacket.data) << ", Size=" << blockSize;
                            if (remain < blockSize) {
                                std::cout << ", Insufficient remaining bytes (" << remain << ")" << std::endl;
                                remain = 0;
                            }
                            else
                            {
                                std::cout << ", Bytes:";
                                int count = (blockSize > 16 ? 16 : blockSize);
                                for (int offset = 0; offset < count; offset++)
                                {
                                    static const char hexTable[] = "0123456789ABCDEF";
                                    uint8_t byte = data[offset];
                                    std::cout << ' ' << hexTable[byte >> 4] << hexTable[byte & 0xF];
                                }
                                std::cout << std::endl;
                                data += blockSize;
                                remain -= blockSize;
                            }
                        }
                    }
#elif TRACE_DECODE_PROCESS
                    std::cout << "  Decoding input frame " << stream->_decodeNextFrameIn << std::endl;
#endif
                    
                    // Advance next frame to input.
                    ++stream->_decodeNextFrameIn;
                    
                    // Decode the frame just read. frameDecoded indicates whether a decoded frame was output.
                    decodeAttempted = true;
                    error = avcodec_decode_video2(stream->_codecContext, stream->_avFrame, &frameDecoded, &_avPacket);
                    if (error < 0) {
                        // Decode error. Abort attempt to read and decode frames.
                        setInternalError(error, "FFmpeg Reader failed to decode frame: ");
                        break;
                    }
                }
            }
#if TRACE_DECODE_PROCESS
            else
                std::cout << ", Irrelevant stream" << std::endl;
#endif
        }
        
        // If the next frame to decode is out of frame range, there's nothing more to read and the decoder will be fed
        // null input frames in order to obtain any remaining output.
        else {
#if TRACE_DECODE_PROCESS
            std::cout << "  No more frames to read, pumping remaining decoder output" << std::endl;
#endif
            
            // Obtain remaining frames from the decoder. pkt_ contains NULL packet data pointer and size at this point,
            // required to pump out remaining frames with no more input. frameDecoded indicates whether a decoded frame
            // was output.
            decodeAttempted = true;
            int error = avcodec_decode_video2(stream->_codecContext, stream->_avFrame, &frameDecoded, &_avPacket);
            if (error < 0) {
                // Decode error. Abort attempt to read and decode frames.
                setInternalError(error, "FFmpeg Reader failed to decode frame: ");
                break;
            }
        }
        
        // If a frame was decoded, ...
        if (frameDecoded) {
#if TRACE_DECODE_PROCESS
            std::cout << "    Frame decoded=" << stream->_decodeNextFrameOut;
#endif
            
            // Now that we have had a frame decoded, we know that seek landed at a valid place to start decode. Any decode
            // stalls detected after this point will result in immediate decode failure.
            awaitingFirstDecodeAfterSeek = false;
            
            // If the frame just output from decode is the desired one, get the decoded picture from it and set that we
            // have a picture.
            if (stream->_decodeNextFrameOut == desiredFrame) {
#if TRACE_DECODE_PROCESS
                std::cout << ", is desired frame" << std::endl;
#endif
                AVPicture output;
                avpicture_fill(&output, buffer, PIX_FMT_RGB24, stream->_width, stream->_height);
                
                sws_scale(stream->getConvertCtx(), stream->_avFrame->data, stream->_avFrame->linesize, 0,  stream->_height, output.data, output.linesize);
                
                
                hasPicture = true;
            }
#if TRACE_DECODE_PROCESS
            else
                std::cout << ", is not desired frame (" << desiredFrame << ")" << std::endl;
#endif
            
            // Advance next output frame expected from decode.
            ++stream->_decodeNextFrameOut;
        }
        // If no frame was decoded but decode was attempted, determine whether this constitutes a decode stall and handle if so.
        else if (decodeAttempted) {
            // Failure to get an output frame for an input frame increases the accumulated decode latency for this stream.
            ++stream->_accumDecodeLatency;
            
#if TRACE_DECODE_PROCESS
            std::cout << "    No frame decoded, accumulated decode latency=" << stream->_accumDecodeLatency << ", max allowed latency=" << stream->getCodecDelay() << std::endl;
#endif
            
            // If the accumulated decode latency exceeds the maximum delay permitted for this codec at this time (the delay can
            // change dynamically if the codec discovers B-frames mid-stream), we've detected a decode stall.
            if (stream->_accumDecodeLatency > stream->getCodecDelay()) {
                int seekTargetFrame; // Target frame for any seek we might perform to attempt decode stall recovery.
                
                // Handle a post-seek decode stall.
                if (awaitingFirstDecodeAfterSeek) {
                    // If there's anywhere in the file to seek back to before the last seek's landing frame (which can be found in
                    // stream->_decodeNextFrameOut, since we know we've not decoded any frames since landing), then set up a seek to
                    // the frame before that landing point to try to find a valid decode start frame earlier in the file.
                    if (stream->_decodeNextFrameOut > 0) {
                        seekTargetFrame = stream->_decodeNextFrameOut - 1;
#if TRACE_DECODE_PROCESS
                        std::cout << "    Post-seek stall detected, trying earlier decode start, seeking frame " << seekTargetFrame << std::endl;
#endif
                    }
                    // Otherwise, there's nowhere to seek back. If we have any retries remaining, use one to attempt the read again,
                    // starting from the desired frame.
                    else if (retriesRemaining > 0) {
                        --retriesRemaining;
                        seekTargetFrame = desiredFrame;
#if TRACE_DECODE_PROCESS
                        std::cout << "    Post-seek stall detected, at start of file, retrying from desired frame " << seekTargetFrame << std::endl;
#endif
                    }
                    // Otherwise, all we can do is to fail the read so that this method exits safely.
                    else {
#if TRACE_DECODE_PROCESS
                        std::cout << "    Post-seek STALL DETECTED, at start of file, no more retries, failed read" << std::endl;
#endif
                        setError("FFmpeg Reader failed to find decode reference frame, possible file corruption");
                        break;
                    }
                }
                // Handle a mid-decode stall. All we can do is to fail the read so that this method exits safely.
                else {
                    // If we have any retries remaining, use one to attempt the read again, starting from the desired frame.
                    if (retriesRemaining > 0) {
                        --retriesRemaining;
                        seekTargetFrame = desiredFrame;
#if TRACE_DECODE_PROCESS
                        std::cout << "    Mid-decode stall detected, retrying from desired frame " << seekTargetFrame << std::endl;
#endif
                    }
                    // Otherwise, all we can do is to fail the read so that this method exits safely.
                    else {
#if TRACE_DECODE_PROCESS
                        std::cout << "    Mid-decode STALL DETECTED, no more retries, failed read" << std::endl;
#endif
                        setError("FFmpeg Reader detected decoding stall, possible file corruption");
                        break;
                    }
                }
                
                // If we reach here, seek to the target frame chosen above in an attempt to recover from the decode stall.
                lastSeekedFrame = seekTargetFrame;
                stream->_decodeNextFrameIn  = -1;
                stream->_decodeNextFrameOut = -1;
                stream->_accumDecodeLatency = 0;
                awaitingFirstDecodeAfterSeek = true;
                
                avcodec_flush_buffers(stream->_codecContext);
                int error = av_seek_frame(_context, stream->_idx, stream->frameToPts(seekTargetFrame), AVSEEK_FLAG_BACKWARD);
                if (error < 0) {
                    // Seek error. Abort attempt to read and decode frames.
                    setInternalError(error, "FFmpeg Reader failed to seek frame: ");
                    break;
                }
            }
        }
        
        av_free_packet(&_avPacket);
    } while (!hasPicture);
    
#if TRACE_DECODE_PROCESS
    std::cout << "<-validPicture=" << hasPicture << " for frame " << desiredFrame << std::endl;
#endif
    
    // If read failed, reset the next frame expected out so that we seek and restart decode on next read attempt. Also free
    // the AVPacket, since it won't have been freed at the end of the above loop (we reach here by a break from the main
    // loop when hasPicture is false).
    if (!hasPicture) {
        if (_avPacket.size > 0)
            av_free_packet(&_avPacket);
        stream->_decodeNextFrameOut = -1;
    }
    
    return hasPicture;
}

bool FFMPEGFile::info( int& width,int& height,double& aspect,int& frames,unsigned streamIdx){
    QMutexLocker guard(&_locker);
    
    if (streamIdx >= _streams.size())
        return false;
    
    // get the stream
    Stream* stream = _streams[streamIdx];
    
    width  = stream->_width;
    height = stream->_height;
    aspect = stream->_aspect;
    frames = stream->_frames;
    
    return true;
}

int FFMPEGFileManager::FFMPEGLockManager(void** mutex, enum AVLockOp op)
{
    switch (op) {
        case AV_LOCK_CREATE: // Create a mutex.
            try {
                *mutex = static_cast< void* >(new QMutex());
                return 0;
            }
            catch(...) {
                // Return error if mutex creation failed.
                return 1;
            }
            
        case AV_LOCK_OBTAIN: // Lock the specified mutex.
            try {
                static_cast< QMutex* >(*mutex)->lock();
                return 0;
            }
            catch(...) {
                // Return error if mutex lock failed.
                return 1;
            }
            
        case AV_LOCK_RELEASE: // Unlock the specified mutex.
            // Mutex unlock can't fail.
            static_cast< QMutex* >(*mutex)->unlock();
            return 0;
            
        case AV_LOCK_DESTROY: // Destroy the specified mutex.
            // Mutex destruction can't fail.
            delete static_cast< QMutex* >(*mutex);
            *mutex = 0;
            return 0;
            
        default: // Unknown operation.
            
            return 1;
    }
}

FFMPEGFileManager::FFMPEGFileManager()
{
    av_log_set_level(AV_LOG_WARNING);
    av_register_all();
    
    // Register a lock manager callback with FFmpeg, providing it the ability to use mutex locking around
    // otherwise non-thread-safe calls.
    av_lockmgr_register(FFMPEGLockManager);
}

FFMPEGFile::Ptr FFMPEGFileManager::get(const char* filename)
{
    QMutexLocker guard(&lock_);
    FFMPEGFile::Ptr retVal;
    
    ReaderMap::iterator it = _readers.find(filename);
    if (it == _readers.end()) {
        retVal.allocate(filename);
        _readers.insert(std::make_pair(std::string(filename), retVal));
    }
    else {
        retVal = it->second;
    }
    return retVal;
    
}

// release a specific reader
void FFMPEGFileManager::release(const char* filename)
{
    
    QMutexLocker guard(&lock_);
    ReaderMap::iterator it = _readers.find(filename);
    
    if (it != _readers.end()) {
        if (it->second.count() == 1) {
            _readers.erase(it);
        }
    }
    
}

ReadFFMPEG::ReadFFMPEG(Reader* op):Read(op),_lock(){}

void ReadFFMPEG::initializeColorSpace(){
    _lut = NULL;
}

ReadFFMPEG::~ReadFFMPEG(){
    _reader.clear();
    _data.clear();
    // release the reader if is not used anymore
	QByteArray ba = _file.toLatin1();
    _readerManager.release(ba.constData());
    _dataBufferManager.release(ba.constData());
    
}
void ReadFFMPEG::readHeader(const QString filename,bool){
    this->_file = filename;
    const QByteArray ba = filename.toLatin1();
    _reader =_readerManager.get(ba.data());
    _data = _dataBufferManager.get(ba.data());
    if (_reader->invalid()) {
        cout << _reader->error() << endl;
        return ;
    }
    
    
    int width, height, frames;
    double aspect;
    
    if (_reader->info(width, height, aspect, frames)) {
        //  op->getInfo()->set_channels(Mask_RGBA);
        // op->getInfo().set_info(width, height, 3, aspect);
        Format imageFormat(0,0,width,height,"",aspect);
        Box2D bbox(0,0,width,height);
        //  op->getInfo()->set_full_size_format(format);
        // op->getInfo()->set(0, 0, width-1, height-1);
        // ui_context->regionOfInterest(dynamic_cast<IntegerBox&>(op->getInfo()));
        _numFrames = frames;
        _memNeeded = width * height * 3;
        setReaderInfo(imageFormat, bbox, filename, Mask_RGBA, -1, true);
    }
}
void ReadFFMPEG::readAllData(bool){
    _data->resize(_memNeeded);
    if(!fillBuffer())
        cout << "FFMPEG READER: open failed to fill the buffer" << endl;
}


bool ReadFFMPEG::fillBuffer(){
    if(!_data->valid())
        return false;
//    if(!_reader->decode(_data->buffer(), current_frame)){
//        cout << _reader->error() << endl;
//        return false;
//    }
    return true;
}

void ReadFFMPEG::engine(int y,int offset,int range,ChannelMask channels,Row* out){
    int w = op->getInfo()->getDisplayWindow().w();
    int h = op->getInfo()->getDisplayWindow().h();
	
	
	unsigned char* FROM = _data->buffer();
	FROM += (h - y - 1) * w * 3;
	FROM += offset * 3;

	foreachChannels(z, channels){
        float* to = out->writable(z) + offset;        
        if(to!=NULL){
            from_byte(z, to, FROM+z-1, FROM+4-1,(range-offset),3);
        }
    }


}
	


 

void ReadFFMPEG::make_preview(){
    Format frmt = op->getInfo()->getDisplayWindow();
    QImage *preview = new QImage(_data->buffer(), frmt.w(),frmt.h(),QImage::Format_RGB888);
    op->setPreview(preview);
}

