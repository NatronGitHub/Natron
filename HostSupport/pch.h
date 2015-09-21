// -*-c++-*-
#ifndef _HostSupport_pch_h_
#define _HostSupport_pch_h_

#if defined(__cplusplus)

#include "ofxCore.h"
#include "ofxImageEffect.h"
#ifdef OFX_SUPPORTS_DIALOG
#include "ofxDialog.h"
#endif
#ifdef OFX_EXTENSIONS_VEGAS
#include "ofxSonyVegas.h"
#endif
#ifdef OFX_EXTENSIONS_TUTTLE
#include "tuttle/ofxReadWrite.h"
#endif
#ifdef OFX_EXTENSIONS_NUKE
#include "nuke/fnOfxExtensions.h"
#endif
#ifdef OFX_EXTENSIONS_NATRON
#include "ofxNatron.h"
#endif
#ifdef OFX_SUPPORTS_OPENGLRENDER
#include "ofxOpenGLRender.h"
#endif
#include "ofxOld.h" // old plugins may rely on deprecated properties being present

#endif // __cplusplus

#endif // _HostSupport_pch_h_
