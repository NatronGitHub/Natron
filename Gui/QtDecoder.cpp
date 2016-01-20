/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "QtDecoder.h"

#ifdef NATRON_ENABLE_QT_IO_NODES

#include <stdexcept>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtGui/QImage>
#include <QtGui/QColor>
#include <QtGui/QImageReader>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/AppManager.h"
#include "Engine/Image.h"
#include "Engine/Lut.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/Knob.h"
#include "Engine/Project.h"
#include "Engine/Node.h"

NATRON_NAMESPACE_ENTER;
using std::cout; using std::endl;

QtReader::QtReader(boost::shared_ptr<Node> node)
    : EffectInstance(node)
      , _lut( Color::LutManager::sRGBLut() )
      , _img(0)
      , _fileKnob()
      , _firstFrame()
      , _before()
      , _lastFrame()
      , _after()
      , _missingFrameChoice()
      , _frameMode()
      , _startingFrame()
      , _timeOffset()
      , _settingFrameRange(false)
{
}

QtReader::~QtReader()
{
    delete _img;
}

std::string
QtReader::getPluginID() const
{
    return PLUGINID_NATRON_READQT;
}

std::string
QtReader::getPluginLabel() const
{
    return "ReadQt";
}

void
QtReader::getPluginGrouping(std::list<std::string>* grouping) const
{
    grouping->push_back(PLUGIN_GROUP_IMAGE);
}

std::string
QtReader::getPluginDescription() const
{
    return QObject::tr("A QImage (Qt) based image reader.").toStdString();
}

void
QtReader::initializeKnobs()
{
    Dialogs::warningDialog( getScriptName_mt_safe(), QObject::tr("This plugin exists only to help the developers team to test %1"
                                                  ". You cannot use it when rendering a project.").arg(NATRON_APPLICATION_NAME).toStdString() );


    _fileKnob = getNode()->createKnob<KnobFile>( QObject::tr("File").toStdString() );
    _fileKnob->setAsInputImage();

    _firstFrame =  getNode()->createKnob<KnobInt>( QObject::tr("First frame").toStdString() );
    _firstFrame->setAnimationEnabled(false);
    _firstFrame->setDefaultValue(0,0);


    _before = getNode()->createKnob<KnobChoice>( QObject::tr("Before").toStdString() );
    std::vector<std::string> beforeOptions;
    beforeOptions.push_back( QObject::tr("hold").toStdString() );
    beforeOptions.push_back( QObject::tr("loop").toStdString() );
    beforeOptions.push_back( QObject::tr("bounce").toStdString() );
    beforeOptions.push_back( QObject::tr("black").toStdString() );
    beforeOptions.push_back( QObject::tr("error").toStdString() );
    _before->populateChoices(beforeOptions);
    _before->setAnimationEnabled(false);
    _before->setDefaultValue(0,0);

    _lastFrame =  getNode()->createKnob<KnobInt>( QObject::tr("Last frame").toStdString() );
    _lastFrame->setAnimationEnabled(false);
    _lastFrame->setDefaultValue(0,0);


    _after = getNode()->createKnob<KnobChoice>( QObject::tr("After").toStdString() );
    std::vector<std::string> afterOptions;
    afterOptions.push_back( QObject::tr("hold").toStdString() );
    afterOptions.push_back( QObject::tr("loop").toStdString() );
    afterOptions.push_back( QObject::tr("bounce").toStdString() );
    afterOptions.push_back( QObject::tr("black").toStdString() );
    afterOptions.push_back( QObject::tr("error").toStdString() );
    _after->populateChoices(beforeOptions);
    _after->setAnimationEnabled(false);
    _after->setDefaultValue(0,0);

    _missingFrameChoice = getNode()->createKnob<KnobChoice>( QObject::tr("On missing frame").toStdString() );
    std::vector<std::string> missingFrameOptions;
    missingFrameOptions.push_back( QObject::tr("Load nearest").toStdString() );
    missingFrameOptions.push_back( QObject::tr("Error").toStdString() );
    missingFrameOptions.push_back( QObject::tr("Black image").toStdString() );
    _missingFrameChoice->populateChoices(missingFrameOptions);
    _missingFrameChoice->setDefaultValue(0,0);
    _missingFrameChoice->setAnimationEnabled(false);

    _frameMode = getNode()->createKnob<KnobChoice>( QObject::tr("Frame mode").toStdString() );
    _frameMode->setAnimationEnabled(false);
    std::vector<std::string> frameModeOptions;
    frameModeOptions.push_back( QObject::tr("Starting frame").toStdString() );
    frameModeOptions.push_back( QObject::tr("Time offset").toStdString() );
    _frameMode->populateChoices(frameModeOptions);
    _frameMode->setDefaultValue(0,0);

    _startingFrame = getNode()->createKnob<KnobInt>( QObject::tr("Starting frame").toStdString() );
    _startingFrame->setAnimationEnabled(false);
    _startingFrame->setDefaultValue(0,0);

    _timeOffset = getNode()->createKnob<KnobInt>( QObject::tr("Time offset").toStdString() );
    _timeOffset->setAnimationEnabled(false);
    _timeOffset->setDefaultValue(0,0);
    _timeOffset->setSecretByDefault(true);
} // initializeKnobs

void
QtReader::knobChanged(KnobI* k,
                      ValueChangedReasonEnum /*reason*/,
                      int /*view*/,
                      double /*time*/,
                      bool /*originatedFromMainThread*/)
{
    if ( k == _fileKnob.get() ) {
        SequenceTime first,last;
        getSequenceTimeDomain(first,last);
        timeDomainFromSequenceTimeDomain(first,last, true);
        _startingFrame->setValue(first,0);
    } else if ( ( k == _firstFrame.get() ) && !_settingFrameRange ) {
        int first = _firstFrame->getValue();
        _lastFrame->setMinimum(first);

        int offset = _timeOffset->getValue();
        _settingFrameRange = true;
        _startingFrame->setValue(first + offset,0);
        _settingFrameRange = false;
    } else if ( k == _lastFrame.get() ) {
        int last = _lastFrame->getValue();
        _firstFrame->setMaximum(last);
    } else if ( k == _frameMode.get() ) {
        int mode = _frameMode->getValue();
        switch (mode) {
        case 0:     //starting frame
            _startingFrame->setSecret(false);
            _timeOffset->setSecret(true);
            break;
        case 1:     //time offset
            _startingFrame->setSecret(true);
            _timeOffset->setSecret(false);
            break;
        default:
            //no such case
            assert(false);
            break;
        }
    } else if ( ( k == _startingFrame.get() ) && !_settingFrameRange ) {
        //also update the time offset
        int startingFrame = _startingFrame->getValue();
        int firstFrame = _firstFrame->getValue();


        ///prevent recursive calls of setValue(...)
        _settingFrameRange = true;
        _timeOffset->setValue(startingFrame - firstFrame,0);
        _settingFrameRange = false;
    } else if ( ( k == _timeOffset.get() ) && !_settingFrameRange ) {
        //also update the starting frame
        int offset = _timeOffset->getValue();
        int first = _firstFrame->getValue();


        ///prevent recursive calls of setValue(...)
        _settingFrameRange = true;
        _startingFrame->setValue(offset + first,0);
        _settingFrameRange = false;
    }
} // knobChanged

void
QtReader::getSequenceTimeDomain(SequenceTime & first,
                                SequenceTime & last)
{
    first = _fileKnob->firstFrame();
    last = _fileKnob->lastFrame();
}

void
QtReader::timeDomainFromSequenceTimeDomain(SequenceTime & first,
                                           SequenceTime & last
                                           ,
                                           bool mustSetFrameRange)
{
    ///the values held by GUI parameters
    int frameRangeFirst,frameRangeLast;
    int startingFrame;

    if (mustSetFrameRange) {
        frameRangeFirst = first;
        frameRangeLast = last;
        startingFrame = first;
        _settingFrameRange = true;
        _firstFrame->setMinimum(first);
        _firstFrame->setMaximum(last);
        _lastFrame->setMinimum(first);
        _lastFrame->setMaximum(last);

        _firstFrame->setValue(first,0);
        _lastFrame->setValue(last,0);
        _settingFrameRange = false;
    } else {
        ///these are the value held by the "First frame" and "Last frame" param
        frameRangeFirst = _firstFrame->getValue();
        frameRangeLast = _lastFrame->getValue();
        startingFrame = _startingFrame->getValue();
    }

    first = startingFrame;
    last =  startingFrame + frameRangeLast - frameRangeFirst;
}

void
QtReader::getFrameRange(double *first,
                        double *last)
{
    int f,l;
    getSequenceTimeDomain(f, l);
    timeDomainFromSequenceTimeDomain(f, l, false);
    *first = f;
    *last = l;
}

void
QtReader::supportedFileFormats_static(std::vector<std::string>* formats)
{
    const QList<QByteArray> & supported = QImageReader::supportedImageFormats();

    for (int i = 0; i < supported.size(); ++i) {
        formats->push_back( supported.at(i).data() );
    }
};
std::vector<std::string>
QtReader::supportedFileFormats() const
{
    std::vector<std::string> ret;
    QtReader::supportedFileFormats_static(&ret);

    return ret;
}

SequenceTime
QtReader::getSequenceTime(SequenceTime t)
{
    int timeOffset = _timeOffset->getValue();
    SequenceTime first = _firstFrame->getValue();
    SequenceTime last = _lastFrame->getValue();


    ///offset the time wrt the starting time
    SequenceTime sequenceTime =  t - timeOffset;

    ///get the offset from the starting time of the sequence in case we bounce or loop
    int timeOffsetFromStart = sequenceTime -  first;

    if (sequenceTime < first) {
        /////if we're before the first frame
        int beforeChoice = _before->getValue();
        switch (beforeChoice) {
        case 0:     //hold
            sequenceTime = first;
            break;
        case 1:     //loop
                    //call this function recursively with the appropriate offset in the time range
            timeOffsetFromStart %= (int)(last - first + 1);
            sequenceTime = last + timeOffsetFromStart;
            break;
        case 2:     //bounce
                    //call this function recursively with the appropriate offset in the time range
        {
            int sequenceIntervalsCount = (last == first) ? 0 : ( timeOffsetFromStart / (last - first) );
            ///if the sequenceIntervalsCount is odd then do exactly like loop, otherwise do the load the opposite frame
            if (sequenceIntervalsCount % 2 == 0) {
                timeOffsetFromStart %= (int)(last - first + 1);
                sequenceTime = first - timeOffsetFromStart;
            } else {
                timeOffsetFromStart %= (int)(last - first + 1);
                sequenceTime = last + timeOffsetFromStart;
            }
            break;
        }
        case 3:     //black
            throw std::invalid_argument("Out of frame range.");
            break;
        case 4:     //error
            setPersistentMessage( eMessageTypeError,  QObject::tr("Missing frame").toStdString() );
            throw std::invalid_argument("Out of frame range.");
            break;
        default:
            break;
        }
    } else if (sequenceTime > last) {
        /////if we're after the last frame
        int afterChoice = _after->getValue();

        switch (afterChoice) {
        case 0:     //hold
            sequenceTime = last;
            break;
        case 1:     //loop
                    //call this function recursively with the appropriate offset in the time range
            timeOffsetFromStart %= (int)(last - first + 1);
            sequenceTime = first + timeOffsetFromStart;
            break;
        case 2:     //bounce
                    //call this function recursively with the appropriate offset in the time range
        {
            int sequenceIntervalsCount = (last == first) ? 0 : ( timeOffsetFromStart / (last - first) );
            ///if the sequenceIntervalsCount is odd then do exactly like loop, otherwise do the load the opposite frame
            if (sequenceIntervalsCount % 2 == 0) {
                timeOffsetFromStart %= (int)(last - first + 1);
                sequenceTime = first + timeOffsetFromStart;
            } else {
                timeOffsetFromStart %= (int)(last - first + 1);
                sequenceTime = last - timeOffsetFromStart;
            }
            break;
        }
        case 3:     //black
            throw std::invalid_argument("Out of frame range.");
            break;
        case 4:     //error
            setPersistentMessage( eMessageTypeError, QObject::tr("Missing frame").toStdString() );
            throw std::invalid_argument("Out of frame range.");
            break;
        default:
            break;
        }
    }
    assert(sequenceTime >= first && sequenceTime <= last);

    return sequenceTime;
} // getSequenceTime

void
QtReader::getFilenameAtSequenceTime(SequenceTime time,
                                    std::string &filename)
{
    int missingChoice = _missingFrameChoice->getValue();

    filename = _fileKnob->getFileName(time);

    switch (missingChoice) {
    case 0:     // Load nearest
                ///the nearest frame search went out of range and couldn't find a frame.
        if ( filename.empty() ) {
            filename = _fileKnob->getFileName(time);
            if ( filename.empty() ) {
                setPersistentMessage( eMessageTypeError, QObject::tr("Nearest frame search went out of range").toStdString() );
            }
        }
        break;
    case 1:     // Error
                /// For images sequences, if the offset is not 0, that means no frame were found at the  originally given
                /// time, we can safely say this is  a missing frame.
        if ( filename.empty() ) {
            setPersistentMessage( eMessageTypeError, QObject::tr("Missing frame").toStdString() );
        }
    case 2:     // Black image
                /// For images sequences, if the offset is not 0, that means no frame were found at the  originally given
                /// time, we can safely say this is  a missing frame.

        break;
    }
}

StatusEnum
QtReader::getRegionOfDefinition(U64 /*hash*/,double time,
                                const RenderScale & /*scale*/,
                                int /*view*/,
                                RectD* rod )
{
    QMutexLocker l(&_lock);
    double sequenceTime;

    try {
        sequenceTime =  getSequenceTime(time);
    } catch (const std::exception & e) {
        return eStatusFailed;
    }

    std::string filename;

    getFilenameAtSequenceTime(sequenceTime, filename);

    if ( filename.empty() ) {
        return eStatusFailed;
    }

    if (filename != _filename) {
        _filename = filename;
        delete _img;
        _img = new QImage( _filename.c_str() );
        if (_img->format() == QImage::Format_Invalid) {
            setPersistentMessage(eMessageTypeError, QObject::tr("Failed to load the image ").toStdString() + filename);

            return eStatusFailed;
        }
    }

    rod->x1 = 0;
    rod->x2 = _img->width();
    rod->y1 = 0;
    rod->y2 = _img->height();

    return eStatusOK;
}

StatusEnum
QtReader::render(const RenderActionArgs& args)
{
    assert(args.outputPlanes.size() == 1);
    
    const std::pair<ImageComponents,ImagePtr>& output = args.outputPlanes.front();
    
    int missingFrameChoice = _missingFrameChoice->getValue();

    if (!_img) {
        if ( !_img && (missingFrameChoice == 2) ) { // black image
            return eStatusOK;
        }
        assert(missingFrameChoice != 0); // nearest value - should never happen
        return eStatusFailed; // error
    }

    Image::WriteAccess acc = output.second->getWriteRights();
    
    assert(_img);
    switch ( _img->format() ) {
    case QImage::Format_RGB32:     // The image is stored using a 32-bit RGB format (0xffRRGGBB).
    case QImage::Format_ARGB32:     // The image is stored using a 32-bit ARGB format (0xAARRGGBB).
        //might have to invert y coordinates here
        _lut->from_byte_packed( (float*)acc.pixelAt(0, 0), _img->bits(), args.roi, output.second->getBounds(), output.second->getBounds(),
                                Color::ePixelPackingBGRA,Color::ePixelPackingRGBA,true,false );
        break;
    case QImage::Format_ARGB32_Premultiplied:     // The image is stored using a premultiplied 32-bit ARGB format (0xAARRGGBB).
        //might have to invert y coordinates here
        _lut->from_byte_packed( (float*)acc.pixelAt(0, 0), _img->bits(), args.roi, output.second->getBounds(), output.second->getBounds(),
                                Color::ePixelPackingBGRA,Color::ePixelPackingRGBA,true,true );
        break;
    case QImage::Format_Mono:     // The image is stored using 1-bit per pixel. Bytes are packed with the most significant bit (MSB) first.
    case QImage::Format_MonoLSB:     // The image is stored using 1-bit per pixel. Bytes are packed with the less significant bit (LSB) first.
    case QImage::Format_Indexed8:     // The image is stored using 8-bit indexes into a colormap.
    case QImage::Format_RGB16:     // The image is stored using a 16-bit RGB format (5-6-5).
    case QImage::Format_RGB666:     // The image is stored using a 24-bit RGB format (6-6-6). The unused most significant bits is always zero.
    case QImage::Format_RGB555:     // The image is stored using a 16-bit RGB format (5-5-5). The unused most significant bit is always zero.
    case QImage::Format_RGB888:     // The image is stored using a 24-bit RGB format (8-8-8).
    case QImage::Format_RGB444:     // The image is stored using a 16-bit RGB format (4-4-4). The unused bits are always zero.
    {
        QImage img = _img->convertToFormat(QImage::Format_ARGB32);
        _lut->from_byte_packed( (float*)acc.pixelAt(0, 0), img.bits(), args.roi, output.second->getBounds(), output.second->getBounds(),
                                Color::ePixelPackingBGRA, Color::ePixelPackingRGBA, true, false );
        break;
    }
    case QImage::Format_ARGB8565_Premultiplied:     // The image is stored using a premultiplied 24-bit ARGB format (8-5-6-5).
    case QImage::Format_ARGB6666_Premultiplied:     // The image is stored using a premultiplied 24-bit ARGB format (6-6-6-6).
    case QImage::Format_ARGB8555_Premultiplied:     // The image is stored using a premultiplied 24-bit ARGB format (8-5-5-5).
    case QImage::Format_ARGB4444_Premultiplied:     // The image is stored using a premultiplied 16-bit ARGB format (4-4-4-4).
    {
        QImage img = _img->convertToFormat(QImage::Format_ARGB32_Premultiplied);
        _lut->from_byte_packed( (float*)acc.pixelAt(0, 0), img.bits(), args.roi, output.second->getBounds(), output.second->getBounds(),
                                Color::ePixelPackingBGRA, Color::ePixelPackingRGBA, true, true );
        break;
    }
    case QImage::Format_Invalid:
    default:
        output.second->fill(args.roi,0.f,1.f);
        setPersistentMessage( eMessageTypeError, QObject::tr("Invalid image format.").toStdString() );

        return eStatusFailed;
    }

    return eStatusOK;
} // render

void
QtReader::addAcceptedComponents(int /*inputNb*/,
                                std::list<ImageComponents>* comps)
{
    ///QtReader only supports RGBA for now.
    comps->push_back(ImageComponents::getRGBAComponents());
}

void
QtReader::addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const
{
    depths->push_back(eImageBitDepthFloat);
}

NATRON_NAMESPACE_EXIT;

#endif // NATRON_ENABLE_QT_IO_NODES