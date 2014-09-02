//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */


#include "QtEncoder.h"

#include <string>
#include <vector>
#include <stdexcept>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtGui/QImage>
#include <QtGui/QImageWriter>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/AppManager.h"
#include "Engine/AppInstance.h"
#include "Engine/Lut.h"
#include "Engine/Format.h"
#include "Engine/Image.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/TimeLine.h"
#include "Engine/Node.h"

using namespace Natron;

QtWriter::QtWriter(boost::shared_ptr<Natron::Node> node)
    : Natron::OutputEffectInstance(node)
      , _lut( Natron::Color::LutManager::sRGBLut() )
{
}

QtWriter::~QtWriter()
{
}

std::string
QtWriter::getPluginID() const
{
    return "WriteQt";
}

std::string
QtWriter::getPluginLabel() const
{
    return "WriteQt";
}

void
QtWriter::getPluginGrouping(std::list<std::string>* grouping) const
{
    grouping->push_back(PLUGIN_GROUP_IMAGE);
}

std::string
QtWriter::getDescription() const
{
    return QObject::tr("The QtWriter node can render on disk the output of a node graph using the QImage (Qt) library.").toStdString();
}

/*Should return the list of file types supported by the encoder: "png","jpg", etc..*/
void
QtWriter::supportedFileFormats_static(std::vector<std::string>* formats)
{
    // Qt Image reader should be the last solution (it cannot read 16-bits ppm or png)
    const QList<QByteArray> & supported = QImageWriter::supportedImageFormats();

    // Qt 4 supports: BMP, JPG, JPEG, PNG, PBM, PGM, PPM, TIFF, XBM, XPM
    // Qt 5 doesn't support TIFF
    for (int i = 0; i < supported.count(); ++i) {
        formats->push_back( std::string( supported.at(i).toLower().data() ) );
    }
}

std::vector<std::string>
QtWriter::supportedFileFormats() const
{
    std::vector<std::string> ret;

    supportedFileFormats_static(&ret);

    return ret;
}

void
QtWriter::getFrameRange(SequenceTime *first,
                        SequenceTime *last)
{
    int index = _frameRangeChoosal->getValue();

    if (index == 0) {
        EffectInstance* inp = getInput(0);
        if (inp) {
            inp->getFrameRange_public(inp->getRenderHash(),first, last);
        } else {
            *first = 0;
            *last = 0;
        }
    } else if (index == 1) {
        *first = getApp()->getTimeLine()->leftBound();
        *last = getApp()->getTimeLine()->rightBound();
    } else {
        *first = _firstFrameKnob->getValue();
        *last = _lastFrameKnob->getValue();
    }
}

void
QtWriter::initializeKnobs()
{
    Natron::warningDialog( getName(), QObject::tr("This plugin exists only to help the developpers team to test %1"
                                                  ". You cannot use it to render a project.").arg(NATRON_APPLICATION_NAME).toStdString() );


    _premultKnob = getNode()->createKnob<Bool_Knob>( QObject::tr("Premultiply by alpha").toStdString() );
    _premultKnob->setAnimationEnabled(false);
    _premultKnob->setDefaultValue(false,0);

    _fileKnob = getNode()->createKnob<OutputFile_Knob>("File");
    _fileKnob->setAsOutputImageFile();

    _frameRangeChoosal = getNode()->createKnob<Choice_Knob>( QObject::tr("Frame range").toStdString() );
    _frameRangeChoosal->setAnimationEnabled(false);
    std::vector<std::string> frameRangeChoosalEntries;
    frameRangeChoosalEntries.push_back( QObject::tr("Union of input ranges").toStdString() );
    frameRangeChoosalEntries.push_back( QObject::tr("Timeline bounds").toStdString() );
    frameRangeChoosalEntries.push_back( QObject::tr("Manual").toStdString() );
    _frameRangeChoosal->populateChoices(frameRangeChoosalEntries);
    _frameRangeChoosal->setDefaultValue(1,0);

    _firstFrameKnob = getNode()->createKnob<Int_Knob>( QObject::tr("First frame").toStdString() );
    _firstFrameKnob->setAnimationEnabled(false);
    _firstFrameKnob->setSecret(true);

    _lastFrameKnob = getNode()->createKnob<Int_Knob>( QObject::tr("Last frame").toStdString() );
    _lastFrameKnob->setAnimationEnabled(false);
    _lastFrameKnob->setSecret(true);

    _renderKnob = getNode()->createKnob<Button_Knob>( QObject::tr("Render").toStdString() );
    _renderKnob->setAsRenderButton();
}

void
QtWriter::knobChanged(KnobI* k,
                      Natron::ValueChangedReason /*reason*/,
                      int /*view*/,
                      SequenceTime /*time*/)
{
    if ( k == _frameRangeChoosal.get() ) {
        int index = _frameRangeChoosal->getValue();
        if (index != 2) {
            _firstFrameKnob->setSecret(true);
            _lastFrameKnob->setSecret(true);
        } else {
            int first = getApp()->getTimeLine()->firstFrame();
            int last = getApp()->getTimeLine()->lastFrame();
            _firstFrameKnob->setValue(first,0);
            _firstFrameKnob->setDisplayMinimum(first);
            _firstFrameKnob->setDisplayMaximum(last);
            _firstFrameKnob->setSecret(false);

            _lastFrameKnob->setValue(last,0);
            _lastFrameKnob->setDisplayMinimum(first);
            _lastFrameKnob->setDisplayMaximum(last);
            _lastFrameKnob->setSecret(false);

            createKnobDynamically();
        }
    }
}

static std::string
filenameFromPattern(const std::string & pattern,
                    int frameIndex)
{
    std::string ret = pattern;
    int lastDot = pattern.find_last_of('.');

    if (lastDot == (int)std::string::npos) {
        ///the filename has not extension, return an empty str
        return "";
    }

    std::stringstream fStr;
    fStr << frameIndex;
    std::string frameIndexStr = fStr.str();
    int lastPos = pattern.find_last_of('#');

    if (lastPos == (int)std::string::npos) {
        ///the filename has no #, just put the digits between etxension and path
        ret.insert(lastDot, frameIndexStr);

        return pattern;
    }

    int nSharpChar = 0;
    int i = lastDot;
    --i; //< char before '.'
    while (i >= 0 && pattern[i] == '#') {
        --i;
        ++nSharpChar;
    }

    int prepending0s = nSharpChar > (int)frameIndexStr.size() ? nSharpChar - frameIndexStr.size() : 0;

    //remove all ocurrences of the # char
    ret.erase( std::remove(ret.begin(), ret.end(), '#'),ret.end() );

    //insert prepending zeroes
    std::string zeroesStr;
    for (int j = 0; j < prepending0s; ++j) {
        zeroesStr.push_back('0');
    }
    frameIndexStr.insert(0,zeroesStr);

    //refresh the last '.' position
    lastDot = ret.find_last_of('.');

    ret.insert(lastDot, frameIndexStr);

    return ret;
}

Natron::Status
QtWriter::render(SequenceTime time,
                 const RenderScale & scale,
                 const RectI & roi,
                 int view,
                 bool /*isSequentialRender*/,
                 bool /*isRenderResponseToUserInteraction*/,
                 boost::shared_ptr<Natron::Image> output)
{
    boost::shared_ptr<Natron::Image> src = getImage(0, time, scale, view, NULL, output->getComponents(), output->getBitDepth(), false);

    if ( hasOutputConnected() ) {
        output->pasteFrom( *src, src->getBounds() );
    }

    ////initializes to black
    unsigned char* buf = (unsigned char*)calloc(roi.area() * 4,1);
    QImage::Format type;
    bool premult = _premultKnob->getValue();
    if (premult) {
        type = QImage::Format_ARGB32_Premultiplied;
    } else {
        type = QImage::Format_ARGB32;
    }

    _lut->to_byte_packed(buf, (const float*)src->pixelAt(0, 0), roi, src->getBounds(), roi,
                         Natron::Color::PACKING_RGBA, Natron::Color::PACKING_BGRA, true, premult);

    QImage img(buf,roi.width(),roi.height(),type);
    std::string filename = _fileKnob->getValue();
    filename = filenameFromPattern( filename,std::floor(time + 0.5) );

    img.save( filename.c_str() );
    free(buf);

    return StatOK;
}

void
QtWriter::addAcceptedComponents(int /*inputNb*/,
                                std::list<Natron::ImageComponents>* comps)
{
    ///QtWriter only supports RGBA for now.
    comps->push_back(Natron::ImageComponentRGBA);
}

void
QtWriter::addSupportedBitDepth(std::list<Natron::ImageBitDepth>* depths) const
{
    depths->push_back(IMAGE_FLOAT);
}

