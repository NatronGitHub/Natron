//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "RotoSmear.h"

#include "Engine/Node.h"
#include "Engine/Image.h"
#include "Engine/KnobTypes.h"
#include "Engine/RotoContext.h"

using namespace Natron;

RotoSmear::RotoSmear(boost::shared_ptr<Natron::Node> node)
: EffectInstance(node)
{
    setSupportsRenderScaleMaybe(eSupportsYes);
}

RotoSmear::~RotoSmear()
{
    
}

void
RotoSmear::addAcceptedComponents(int /*inputNb*/,std::list<Natron::ImageComponents>* comps)
{
    comps->push_back(ImageComponents::getRGBAComponents());
    comps->push_back(ImageComponents::getRGBComponents());
    comps->push_back(ImageComponents::getXYComponents());
    comps->push_back(ImageComponents::getAlphaComponents());
}

void
RotoSmear::addSupportedBitDepth(std::list<Natron::ImageBitDepthEnum>* depths) const
{
    depths->push_back(Natron::eImageBitDepthFloat);
}

void
RotoSmear::getPreferredDepthAndComponents(int /*inputNb*/,std::list<Natron::ImageComponents>* comp,Natron::ImageBitDepthEnum* depth) const
{
    EffectInstance* input = getInput(0);
    if (input) {
        return input->getPreferredDepthAndComponents(-1, comp, depth);
    } else {
        comp->push_back(ImageComponents::getRGBAComponents());
        *depth = eImageBitDepthFloat;
    }
}


Natron::ImagePremultiplicationEnum
RotoSmear::getOutputPremultiplication() const
{
    EffectInstance* input = getInput(0);
    if (input) {
        return input->getOutputPremultiplication();
    } else {
        return eImagePremultiplicationPremultiplied;
    }
    
}

Natron::StatusEnum
RotoSmear::getRegionOfDefinition(U64 hash,SequenceTime time, const RenderScale & scale, int view, RectD* rod)
{
    (void)EffectInstance::getRegionOfDefinition(hash, time, scale, view, rod);
    
    RectD maskRod;
    boost::shared_ptr<Node> node = getNode();
    node->getPaintStrokeRoD(time, &maskRod);
    if (rod->isNull()) {
        *rod = maskRod;
    } else {
        rod->merge(maskRod);
    }
    return Natron::eStatusOK;
}

double
RotoSmear::getPreferredAspectRatio() const
{
    EffectInstance* input = getInput(0);
    if (input) {
        return input->getPreferredAspectRatio();
    } else {
        return 1.;
    }
    
}

bool
RotoSmear::isIdentity(SequenceTime time,
                const RenderScale & scale,
                const RectI & roi,
                int /*view*/,
                SequenceTime* inputTime,
                int* inputNb)
{
    RectD maskRod;
    boost::shared_ptr<Node> node = getNode();
    node->getPaintStrokeRoD(time, &maskRod);
    
    RectI maskPixelRod;
    maskRod.toPixelEnclosing(scale, getPreferredAspectRatio(), &maskPixelRod);
    if (!maskPixelRod.intersects(roi)) {
        *inputTime = time;
        *inputNb = 0;
        return true;
    }
    return false;
}

static ImagePtr renderSmearDot(const boost::shared_ptr<RotoContext>& context, boost::shared_ptr<RotoStrokeItem>& stroke, const Point& p, double pressure, double brushSize, const ImageComponents& comps, ImageBitDepthEnum depth, unsigned int mipmapLevel)
{
    RectD dotRod(p.x - brushSize / 2., p.y - brushSize / 2., p.x + brushSize / 2., p.y + brushSize / 2.);
    
    std::list<std::pair<Point,double> > points;
    points.push_back(std::make_pair(p, pressure));
    ImagePtr ret;
    (void)context->renderSingleStroke(stroke, dotRod, points, mipmapLevel, comps, depth, &ret);
    return ret;
}

Natron::StatusEnum
RotoSmear::render(const RenderActionArgs& args)
{
    boost::shared_ptr<Node> node = getNode();
    boost::shared_ptr<RotoStrokeItem> stroke = node->getAttachedStrokeItem();
    boost::shared_ptr<RotoContext> context = stroke->getContext();
    assert(context);
    
    unsigned int mipmapLevel = Image::getLevelFromScale(args.originalScale.x);
    
    std::list<std::pair<Natron::Point,double> > points;
    node->getLastPaintStrokePoints(&points);
    
    RectD pointsBbox;
    node->getLastPaintStrokeRoD(&pointsBbox);
    
    if (points.size() <= 1) {
        return eStatusOK;
    }
    
    std::list<ImageComponents> bgComps;
    Natron::ImageBitDepthEnum bgDepth;
    getPreferredDepthAndComponents(0, &bgComps, &bgDepth);
    assert(!bgComps.empty());
    
    double par = getPreferredAspectRatio();
    RectI bgImgRoI;
    ImagePtr bgImg = getImage(0, args.time, args.mappedScale, args.view, 0, bgComps.front(), bgDepth, par, false, &bgImgRoI);
    
    if (!bgImg) {
        for (std::list<std::pair<Natron::ImageComponents,boost::shared_ptr<Natron::Image> > >::const_iterator plane = args.outputPlanes.begin();
             plane != args.outputPlanes.end(); ++plane) {
            plane->second->fill(args.roi, 0., 0., 0., 0.);
        }
        return eStatusOK;
    }
    
    Image::ReadAccess racc = bgImg->getReadRights();
    
    double brushSize = stroke->getBrushSizeKnob()->getValue();
    
    for (std::list<std::pair<Natron::ImageComponents,boost::shared_ptr<Natron::Image> > >::const_iterator plane = args.outputPlanes.begin();
         plane != args.outputPlanes.end(); ++plane) {
        
        int nComps = plane->first.getNumComponents();
        
        std::list<std::pair<Natron::Point,double> >::iterator it = points.begin();
        std::list<std::pair<Natron::Point,double> >::iterator next = it;
        ++next;
        
        for (; next != points.end(); ++it, ++next) {
            ImagePtr dotMask = renderSmearDot(context, stroke, next->first, next->second, brushSize, ImageComponents::getAlphaComponents(), plane->second->getBitDepth(), mipmapLevel);
            assert(dotMask);
            
            RectI nextDotBounds = dotMask->getBounds();
            
            RectI prevDotBounds(it->first.x - nextDotBounds.width() / 2., it->first.y - nextDotBounds.height() / 2.,
                                it->first.x + nextDotBounds.width() /2., it->first.y + nextDotBounds.height() / 2.);
            
            assert(prevDotBounds.width() == nextDotBounds.width() && prevDotBounds.height() == nextDotBounds.height());
            
            Image::WriteAccess wacc = plane->second->getWriteRights();
            Image::ReadAccess mracc = dotMask->getReadRights();
            
            int yPrev = prevDotBounds.y1;
            for (int y = nextDotBounds.y1; y < nextDotBounds.y2; ++y,++yPrev) {
                
                float* dstPixels = (float*)wacc.pixelAt(nextDotBounds.x1, y);
                const float* maskPixels = (const float*)mracc.pixelAt(nextDotBounds.x1, y);
                assert(dstPixels && maskPixels);
                
                int xPrev = prevDotBounds.x1;
                for (int x = nextDotBounds.x1; x < nextDotBounds.x2; ++x, ++xPrev, dstPixels += nComps, ++maskPixels) {
                    
                    const float* srcPixels = (const float*)racc.pixelAt(xPrev, yPrev);
                    for (int k = 0; k < nComps; ++k) {
                        dstPixels[k] = srcPixels ? srcPixels[k] * *maskPixels : 0.;
                    }
                }
            }
            
        }
        
    }
    return Natron::eStatusOK;
}