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



#ifndef Natron_shaders_h
#define Natron_shaders_h


extern const char* fragRGB;

extern const char* vertRGB;

/*There's a black texture used for when the user disconnect the viewer
 It's not just a shader,because we still need coordinates feedback.
 */
extern const char* blackFrag;

extern const char *histogramComputation_frag;

extern const char *histogramComputationVertex_vert;

extern const char *histogramRendering_frag;

extern const char *histogramRenderingVertex_vert;

extern const char* minimal_vert;

extern const char *histogramMaximum_frag;

#endif
