/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#include "Image.h"

#include <cassert>
#include <stdexcept>

#if !defined(SBK_RUN) && !defined(Q_MOC_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/math/special_functions/fpclassify.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

#include <QtCore/QDebug>

#include "Engine/OSGLContext.h"
#include "Engine/GLShader.h"


// disable some warnings due to unused parameters
GCC_DIAG_OFF(unused-parameter)
# if ( ( __GNUC__ * 100) + __GNUC_MINOR__) >= 406
GCC_DIAG_OFF(unused-but-set-variable) // only on gcc >= 4.6
#endif

// NATRON_COPY_CHANNELS_UNPREMULT:
// Repremult R G and B if output is premult and alpha was modified.
// We do not consider it a good thing, since the user explicitly deselected the channels, and expects
// to get the values from input instead.
//#define NATRON_COPY_CHANNELS_UNPREMULT

NATRON_NAMESPACE_ENTER

template <typename PIX, int maxValue, int srcNComps, int dstNComps, bool doR, bool doG, bool doB, bool doA, bool premult, bool originalPremult, bool ignorePremult>
void
Image::copyUnProcessedChannelsForPremult(const std::bitset<4> processChannels,
                                         const RectI& roi,
                                         const ImagePtr& originalImage)
{
    Q_UNUSED(processChannels); // silence warnings in release version
    assert( ( (doR == !processChannels[0]) || !(dstNComps >= 2) ) &&
            ( (doG == !processChannels[1]) || !(dstNComps >= 2) ) &&
            ( (doB == !processChannels[2]) || !(dstNComps >= 3) ) &&
            ( (doA == !processChannels[3]) || !(dstNComps == 1 || dstNComps == 4) ) );
    ReadAccess acc( originalImage.get() );
    int dstRowElements = dstNComps * _bounds.width();
    PIX* dst_pixels = (PIX*)pixelAt(roi.x1, roi.y1);
    assert(dst_pixels);
    assert(srcNComps == 1 || srcNComps == 4 || !originalPremult); // only A or RGBA can be premult
    assert(dstNComps == 1 || dstNComps == 4 || !premult); // only A or RGBA can be premult

    for ( int y = roi.y1; y < roi.y2; ++y, dst_pixels += (dstRowElements - (roi.x2 - roi.x1) * dstNComps) ) {
        for (int x = roi.x1; x < roi.x2; ++x, dst_pixels += dstNComps) {
            const PIX* src_pixels = originalImage ? (const PIX*)acc.pixelAt(x, y) : 0;
            PIX srcA = src_pixels ? maxValue : 0; /* be opaque for anything that doesn't contain alpha */
            if ( ( (srcNComps == 1) || (srcNComps == 4) ) && src_pixels ) {
#             ifdef DEBUG
                assert( !(boost::math::isnan)(src_pixels[srcNComps - 1]) ); // check for NaN
#             endif
                srcA = src_pixels[srcNComps - 1];
            }

#        ifdef NATRON_COPY_CHANNELS_UNPREMULT
            // Repremult R G and B if output is premult and alpha was modified.
            // We do not consider it a good thing, since the user explicitly deselected the channels, and expects
            // to get the values from input instead.
#           define DOCHANNEL(c)                                                    \
    if (srcNComps == 1 || !src_pixels || c >= srcNComps) {      \
        dst_pixels[c] = 0;                                      \
    } \
    else if (originalPremult) {                               \
        if (srcA == 0) {                                        \
            dst_pixels[c] = src_pixels[c];         /* don't try to unpremult, just copy */ \
        } \
        else if (premult) {                                   \
            if (doA) {                                          \
                dst_pixels[c] = src_pixels[c];         /* dst will have same alpha as src, just copy src */ \
            } \
            else {                                            \
                dst_pixels[c] = (src_pixels[c] / (float)srcA) * dstAorig;         /* dst keeps its alpha, unpremult src and repremult */ \
            }                                                   \
        } \
        else {                                                \
            dst_pixels[c] = (src_pixels[c] / (float)srcA) * maxValue;         /* dst is not premultiplied, unpremult src */ \
        }                                                       \
    } \
    else {                                                    \
        if (premult) {                                          \
            if (doA) {                                          \
                dst_pixels[c] = (src_pixels[c] * (1.f / maxValue)) * srcA;         /* dst will have same alpha as src, just premult src with its alpha */ \
            } \
            else {                                            \
                dst_pixels[c] = (src_pixels[c] * (1.f / maxValue)) * dstAorig;         /* dst keeps its alpha, premult src with dst's alpha */ \
            }                                                   \
        } \
        else {                                                \
            dst_pixels[c] = src_pixels[c];         /* neither src nor dst is not premultiplied */ \
        }                                                       \
    }

            PIX dstAorig = maxValue;
#         else // !NATRON_COPY_CHANNELS_UNPREMULT
               // Just copy the channels, after all if the user unchecked a channel,
               // we do not want to change the values behind his back.
               // Rather we display a warning in  the GUI.
#           define DOCHANNEL(c) dst_pixels[c] = (!src_pixels || c >= srcNComps) ? 0 : src_pixels[c];
#         endif // !NATRON_COPY_CHANNELS_UNPREMULT

            if ( (dstNComps == 1) || (dstNComps == 4) ) {
#             ifdef DEBUG
                assert(  !(boost::math::isnan)(dst_pixels[dstNComps - 1]) ); // check for NaN
#             endif
#             ifdef NATRON_COPY_CHANNELS_UNPREMULT
                dstAorig = dst_pixels[dstNComps - 1];
#             endif // NATRON_COPY_CHANNELS_UNPREMULT
            }
            if (doR) {
#             ifdef DEBUG
                assert(!src_pixels ||  !(boost::math::isnan)(src_pixels[0]) ); // check for NaN
                assert(  !(boost::math::isnan)(dst_pixels[0]) ); // check for NaN
#             endif
                DOCHANNEL(0);
#             ifdef DEBUG
                assert(  !(boost::math::isnan)(dst_pixels[0]) ); // check for NaN
#             endif
            }
            if (doG) {
#             ifdef DEBUG
                assert(!src_pixels ||  !(boost::math::isnan)(src_pixels[1]) ); // check for NaN
                assert( !(boost::math::isnan)(dst_pixels[1]) ); // check for NaN
#             endif
                DOCHANNEL(1);
#             ifdef DEBUG
                assert( !(boost::math::isnan)(dst_pixels[1]) ); // check for NaN
#             endif
            }
            if (doB) {
#             ifdef DEBUG
                assert(!src_pixels ||  !(boost::math::isnan)(src_pixels[2]) ); // check for NaN
                assert( !(boost::math::isnan)(dst_pixels[2]) ); // check for NaN
#             endif
                DOCHANNEL(2);
#             ifdef DEBUG
                assert( !(boost::math::isnan)(dst_pixels[2]) ); // check for NaN
#             endif
            }
            if (doA) {
#             ifdef NATRON_COPY_CHANNELS_UNPREMULT
                if (premult) {
                    if (dstAorig != 0) {
                        // unpremult, then premult
                        if ( (dstNComps >= 2) && !doR ) {
                            dst_pixels[0] = (dst_pixels[0] / (float)dstAorig) * srcA;
#                         ifdef DEBUG
                            assert( !(boost::math::isnan)(dst_pixels[0]) ); // check for NaN
#                         endif
                        }
                        if ( (dstNComps >= 2) && !doG ) {
                            dst_pixels[1] = (dst_pixels[1] / (float)dstAorig) * srcA;
#                         ifdef DEBUG
                            assert( !(boost::math::isnan)(dst_pixels[1]) ); // check for NaN
#                         endif
                        }
                        if ( (dstNComps >= 2) && !doB ) {
                            dst_pixels[2] = (dst_pixels[2] / (float)dstAorig) * srcA;
#                         ifdef DEBUG
                            assert( !(boost::math::isnan)(dst_pixels[2]) ); // check for NaN
#                         endif
                        }
                    }
                }
#             endif // NATRON_COPY_CHANNELS_UNPREMULT
                if ( (dstNComps == 1) || (dstNComps == 4) ) {
                    dst_pixels[dstNComps - 1] = srcA;
#                 ifdef DEBUG
                    assert( !(boost::math::isnan)(dst_pixels[dstNComps - 1]) ); // check for NaN
#                 endif
                }
            }
        }
    }
} // Image::copyUnProcessedChannelsForPremult

template <typename PIX, int maxValue, int srcNComps, int dstNComps, bool ignorePremult>
void
Image::copyUnProcessedChannelsForPremult(const bool premult,
                                         const bool originalPremult,
                                         const std::bitset<4> processChannels,
                                         const RectI& roi,
                                         const ImagePtr& originalImage)
{
    ReadAccess acc( originalImage.get() );
    int dstRowElements = dstNComps * _bounds.width();
    PIX* dst_pixels = (PIX*)pixelAt(roi.x1, roi.y1);

    assert(dst_pixels);
    const bool doR = !processChannels[0] && (dstNComps >= 2);
    const bool doG = !processChannels[1] && (dstNComps >= 2);
    const bool doB = !processChannels[2] && (dstNComps >= 3);
    const bool doA = !processChannels[3] && (dstNComps == 1 || dstNComps == 4);
    assert(srcNComps == 4 || !originalPremult); // only RGBA can be premult
    assert(dstNComps == 4 || !premult); // only RGBA can be premult
    Q_UNUSED(premult);
    Q_UNUSED(originalPremult);

    for ( int y = roi.y1; y < roi.y2; ++y, dst_pixels += (dstRowElements - (roi.x2 - roi.x1) * dstNComps) ) {
        for (int x = roi.x1; x < roi.x2; ++x, dst_pixels += dstNComps) {
            const PIX* src_pixels = originalImage ? (const PIX*)acc.pixelAt(x, y) : 0;
            PIX srcA = src_pixels ? maxValue : 0; /* be opaque for anything that doesn't contain alpha */
            if ( ( (srcNComps == 1) || (srcNComps == 4) ) && src_pixels ) {
#             ifdef DEBUG
                assert( !(boost::math::isnan)(src_pixels[srcNComps - 1]) ); // check for NaN
#             endif
                srcA = src_pixels[srcNComps - 1];
            }
#         ifdef NATRON_COPY_CHANNELS_UNPREMULT
            PIX dstAorig = maxValue;
#         endif
            if ( (dstNComps == 1) || (dstNComps == 4) ) {
#             ifdef DEBUG
                assert( !(boost::math::isnan)(dst_pixels[dstNComps - 1]) ); // check for NaN
#             endif
#             ifdef NATRON_COPY_CHANNELS_UNPREMULT
                dstAorig = dst_pixels[dstNComps - 1];
#             endif
            }
            if (doR) {
#             ifdef DEBUG
                assert(!src_pixels || !(boost::math::isnan)(src_pixels[0]) ); // check for NaN
                assert( !(boost::math::isnan)(dst_pixels[0]) ); // check for NaN
#             endif
                DOCHANNEL(0);
#             ifdef DEBUG
                assert( !(boost::math::isnan)(dst_pixels[0]) ); // check for NaN
#             endif
            }
            if (doG) {
#             ifdef DEBUG
                assert(!src_pixels || !(boost::math::isnan)(src_pixels[1]) ); // check for NaN
                assert( !(boost::math::isnan)(dst_pixels[1]) ); // check for NaN
#             endif
                DOCHANNEL(1);
#             ifdef DEBUG
                assert( !(boost::math::isnan)(dst_pixels[1]) ); // check for NaN
#             endif
            }
            if (doB) {
#             ifdef DEBUG
                assert(!src_pixels || !(boost::math::isnan)(src_pixels[2]) ); // check for NaN
                assert( !(boost::math::isnan)(dst_pixels[2]) ); // check for NaN
#             endif
                DOCHANNEL(2);
#             ifdef DEBUG
                assert( !(boost::math::isnan)(dst_pixels[2]) ); // check for NaN
#             endif
            }
            if (doA) {
#             ifdef NATRON_COPY_CHANNELS_UNPREMULT
                if (premult) {
                    if (dstAorig != 0) {
                        // unpremult, then premult
                        if ( (dstNComps >= 2) && !doR ) {
                            dst_pixels[0] = (dst_pixels[0] / (float)dstAorig) * srcA;
#                         ifdef DEBUG
                            assert( !(boost::math::isnan)(dst_pixels[0]) ); // check for NaN
#                         endif
                        }
                        if ( (dstNComps >= 2) && !doG ) {
                            dst_pixels[1] = (dst_pixels[1] / (float)dstAorig) * srcA;
#                         ifdef DEBUG
                            assert( !(boost::math::isnan)(dst_pixels[1]) ); // check for NaN
#                         endif
                        }
                        if ( (dstNComps >= 2) && !doB ) {
                            dst_pixels[2] = (dst_pixels[2] / (float)dstAorig) * srcA;
#                         ifdef DEBUG
                            assert( !(boost::math::isnan)(dst_pixels[2]) ); // check for NaN
#                         endif
                        }
                    }
                }
#              endif // NATRON_COPY_CHANNELS_UNPREMULT
                // coverity[dead_error_line]
                dst_pixels[dstNComps - 1] = srcA;
#              ifdef DEBUG
                assert( !(boost::math::isnan)(dst_pixels[dstNComps - 1]) ); // check for NaN
#              endif
            }
        }
    }
} // Image::copyUnProcessedChannelsForPremult

template <typename PIX, int maxValue, int srcNComps, int dstNComps, bool doR, bool doG, bool doB, bool doA>
void
Image::copyUnProcessedChannelsForChannels(const std::bitset<4> processChannels,
                                          const bool premult,
                                          const RectI& roi,
                                          const ImagePtr& originalImage,
                                          const bool originalPremult,
                                          const bool ignorePremult)
{
    assert( ( (doR == !processChannels[0]) || !(dstNComps >= 2) ) &&
            ( (doG == !processChannels[1]) || !(dstNComps >= 2) ) &&
            ( (doB == !processChannels[2]) || !(dstNComps >= 3) ) &&
            ( (doA == !processChannels[3]) || !(dstNComps == 1 || dstNComps == 4) ) );
    if (premult) {
        if (originalPremult) {
            if (ignorePremult) {
                copyUnProcessedChannelsForPremult<PIX, maxValue, srcNComps, dstNComps, doR, doG, doB, doA, true, true, true>(processChannels, roi, originalImage);
            } else {
                copyUnProcessedChannelsForPremult<PIX, maxValue, srcNComps, dstNComps, doR, doG, doB, doA, true, true, false>(processChannels, roi, originalImage);
            }
        } else {
            if (ignorePremult) {
                copyUnProcessedChannelsForPremult<PIX, maxValue, srcNComps, dstNComps, true>(true, false, processChannels, roi, originalImage);
            } else {
                copyUnProcessedChannelsForPremult<PIX, maxValue, srcNComps, dstNComps, false>(true, false, processChannels, roi, originalImage);
            }
        }
    } else {
        if (originalPremult) {
            if (ignorePremult) {
                copyUnProcessedChannelsForPremult<PIX, maxValue, srcNComps, dstNComps, true>(false, true, processChannels, roi, originalImage);
            } else {
                copyUnProcessedChannelsForPremult<PIX, maxValue, srcNComps, dstNComps, false>(false, true, processChannels, roi, originalImage);
            }
        } else {
            if (ignorePremult) {
                copyUnProcessedChannelsForPremult<PIX, maxValue, srcNComps, dstNComps, doR, doG, doB, doA, false, false, true>(processChannels, roi, originalImage);
            } else {
                copyUnProcessedChannelsForPremult<PIX, maxValue, srcNComps, dstNComps, doR, doG, doB, doA, false, false, false>(processChannels, roi, originalImage);
            }
        }
    }
}

template <typename PIX, int maxValue, int srcNComps, int dstNComps>
void
Image::copyUnProcessedChannelsForChannels(const std::bitset<4> processChannels,
                                          const bool premult,
                                          const RectI& roi,
                                          const ImagePtr& originalImage,
                                          const bool originalPremult,
                                          const bool ignorePremult)
{
    if (ignorePremult) {
        copyUnProcessedChannelsForPremult<PIX, maxValue, srcNComps, dstNComps, true>(premult, originalPremult, processChannels, roi, originalImage);
    } else {
        copyUnProcessedChannelsForPremult<PIX, maxValue, srcNComps, dstNComps, false>(premult, originalPremult, processChannels, roi, originalImage);
    }
}

template <typename PIX, int maxValue, int srcNComps, int dstNComps>
void
Image::copyUnProcessedChannelsForComponents(const bool premult,
                                            const RectI& roi,
                                            const std::bitset<4> processChannels,
                                            const ImagePtr& originalImage,
                                            const bool originalPremult,
                                            const bool ignorePremult)
{
    const bool doR = !processChannels[0] && (dstNComps >= 2);
    const bool doG = !processChannels[1] && (dstNComps >= 2);
    const bool doB = !processChannels[2] && (dstNComps >= 3);
    const bool doA = !processChannels[3] && (dstNComps == 1 || dstNComps == 4);

    if (dstNComps == 1) {
        if (doA) {
            copyUnProcessedChannelsForChannels<PIX, maxValue, srcNComps, dstNComps, false, false, false, true>(processChannels, premult, roi, originalImage, originalPremult, ignorePremult);     // RGB were processed, copy A
        } else {
            copyUnProcessedChannelsForChannels<PIX, maxValue, srcNComps, dstNComps, false, false, false, false>(processChannels, premult, roi, originalImage, originalPremult, ignorePremult);     // RGBA were processed, only do premult
        }
    } else {
        assert(2 <= dstNComps && dstNComps <= 4);
        if (doR) {
            if (doG) {
                if ( (dstNComps >= 3) && doB ) {
                    if ( (dstNComps >= 4) && doA ) {
                        copyUnProcessedChannelsForChannels<PIX, maxValue, srcNComps, dstNComps, true, true, true, true>(processChannels, premult, roi, originalImage, originalPremult, ignorePremult); // none were processed, only do premult
                    } else {
                        copyUnProcessedChannelsForChannels<PIX, maxValue, srcNComps, dstNComps, true, true, true, false>(processChannels, premult, roi, originalImage, originalPremult, ignorePremult); // A was processed
                    }
                } else {
                    if ( (dstNComps >= 4) && doA ) {
                        copyUnProcessedChannelsForChannels<PIX, maxValue, srcNComps, dstNComps, true, true, false, true>(processChannels, premult, roi, originalImage, originalPremult, ignorePremult); // B was processed
                    } else {
                        copyUnProcessedChannelsForChannels<PIX, maxValue, srcNComps, dstNComps>(/*true, true, false, false, */ processChannels, premult, roi, originalImage, originalPremult, ignorePremult); // BA were processed (rare)
                    }
                }
            } else {
                if ( (dstNComps >= 3) && doB ) {
                    if ( (dstNComps >= 4) && doA ) {
                        copyUnProcessedChannelsForChannels<PIX, maxValue, srcNComps, dstNComps, true, false, true, true>(processChannels, premult, roi, originalImage, originalPremult, ignorePremult); // G was processed
                    } else {
                        copyUnProcessedChannelsForChannels<PIX, maxValue, srcNComps, dstNComps>(/*true, false, true, false, */ processChannels, premult, roi, originalImage, originalPremult, ignorePremult); // GA were processed (rare)
                    }
                } else {
                    //if (dstNComps >= 4 && doA) {
                    copyUnProcessedChannelsForChannels<PIX, maxValue, srcNComps, dstNComps>(/*true, false, false, true, */ processChannels, premult, roi, originalImage, originalPremult, ignorePremult);    // GB were processed (rare)
                    //} else {
                    //    copyUnProcessedChannelsForChannels<PIX, maxValue, srcNComps, dstNComps>(/*true, false, false, false, */processChannels, premult, roi, originalImage, originalPremult, ignorePremult); // GBA were processed (rare)
                    //}
                }
            }
        } else {
            if (doG) {
                if ( (dstNComps >= 3) && doB ) {
                    if ( (dstNComps >= 4) && doA ) {
                        copyUnProcessedChannelsForChannels<PIX, maxValue, srcNComps, dstNComps, false, true, true, true>(processChannels, premult, roi, originalImage, originalPremult, ignorePremult); // R was processed
                    } else {
                        copyUnProcessedChannelsForChannels<PIX, maxValue, srcNComps, dstNComps>(/*false, true, true, false, */ processChannels, premult, roi, originalImage, originalPremult, ignorePremult); // RA were processed (rare)
                    }
                } else {
                    //if (dstNComps >= 4 && doA) {
                    copyUnProcessedChannelsForChannels<PIX, maxValue, srcNComps, dstNComps>(/*false, true, false, true, */ processChannels, premult, roi, originalImage, originalPremult, ignorePremult);    // RB were processed (rare)
                    //} else {
                    //    copyUnProcessedChannelsForChannels<PIX, maxValue, srcNComps, dstNComps>(/*false, true, false, false, */processChannels, premult, roi, originalImage, originalPremult, ignorePremult); // RBA were processed (rare)
                    //}
                }
            } else {
                if ( (dstNComps >= 3) && doB ) {
                    //if (dstNComps >= 4 && doA) {
                    copyUnProcessedChannelsForChannels<PIX, maxValue, srcNComps, dstNComps>(/*false, false, true, true, */ processChannels, premult, roi, originalImage, originalPremult, ignorePremult);    // RG were processed (rare)
                    //} else {
                    //    copyUnProcessedChannelsForChannels<PIX, maxValue, srcNComps, dstNComps>(/*false, false, true, false, */processChannels, premult, roi, originalImage, originalPremult, ignorePremult); // RGA were processed (rare)
                    //}
                } else {
                    if ( (dstNComps >= 4) && doA ) {
                        copyUnProcessedChannelsForChannels<PIX, maxValue, srcNComps, dstNComps, false, false, false, true>(processChannels, premult, roi, originalImage, originalPremult, ignorePremult); // RGB were processed
                    } else {
                        copyUnProcessedChannelsForChannels<PIX, maxValue, srcNComps, dstNComps, false, false, false, false>(processChannels, premult, roi, originalImage, originalPremult, ignorePremult); // RGBA were processed
                    }
                }
            }
        }
    }
} // Image::copyUnProcessedChannelsForComponents

template <typename PIX, int maxValue>
void
Image::copyUnProcessedChannelsForDepth(const bool premult,
                                       const RectI& roi,
                                       const std::bitset<4> processChannels,
                                       const ImagePtr& originalImage,
                                       const bool originalPremult,
                                       bool ignorePremult)
{
    int dstNComps = getComponents().getNumComponents();
    int srcNComps = originalImage ? originalImage->getComponents().getNumComponents() : 0;

    switch (dstNComps) {
    case 1:
        switch (srcNComps) {
        case 0:
            copyUnProcessedChannelsForComponents<PIX, maxValue, 0, 1>(premult, roi, processChannels, originalImage, originalPremult, ignorePremult);
            break;
        case 1:
            copyUnProcessedChannelsForComponents<PIX, maxValue, 1, 1>(premult, roi, processChannels, originalImage, originalPremult, ignorePremult);
            break;
        case 2:
            copyUnProcessedChannelsForComponents<PIX, maxValue, 2, 1>(premult, roi, processChannels, originalImage, originalPremult, ignorePremult);
            break;
        case 3:
            copyUnProcessedChannelsForComponents<PIX, maxValue, 3, 1>(premult, roi, processChannels, originalImage, originalPremult, ignorePremult);
            break;
        case 4:
            copyUnProcessedChannelsForComponents<PIX, maxValue, 4, 1>(premult, roi, processChannels, originalImage, originalPremult, ignorePremult);
            break;
        default:
            assert(false);
            break;
        }
        break;
    case 2:
        switch (srcNComps) {
        case 0:
            copyUnProcessedChannelsForComponents<PIX, maxValue, 0, 2>(premult, roi, processChannels, originalImage, originalPremult, ignorePremult);
            break;
        case 1:
            copyUnProcessedChannelsForComponents<PIX, maxValue, 1, 2>(premult, roi, processChannels, originalImage, originalPremult, ignorePremult);
            break;
        case 2:
            copyUnProcessedChannelsForComponents<PIX, maxValue, 2, 2>(premult, roi, processChannels, originalImage, originalPremult, ignorePremult);
            break;
        case 3:
            copyUnProcessedChannelsForComponents<PIX, maxValue, 3, 2>(premult, roi, processChannels, originalImage, originalPremult, ignorePremult);
            break;
        case 4:
            copyUnProcessedChannelsForComponents<PIX, maxValue, 4, 2>(premult, roi, processChannels, originalImage, originalPremult, ignorePremult);
            break;
        default:
            assert(false);
            break;
        }
        break;
    case 3:
        switch (srcNComps) {
        case 0:
            copyUnProcessedChannelsForComponents<PIX, maxValue, 0, 3>(premult, roi, processChannels, originalImage, originalPremult, ignorePremult);
            break;
        case 1:
            copyUnProcessedChannelsForComponents<PIX, maxValue, 1, 3>(premult, roi, processChannels, originalImage, originalPremult, ignorePremult);
            break;
        case 2:
            copyUnProcessedChannelsForComponents<PIX, maxValue, 2, 3>(premult, roi, processChannels, originalImage, originalPremult, ignorePremult);
            break;
        case 3:
            copyUnProcessedChannelsForComponents<PIX, maxValue, 3, 3>(premult, roi, processChannels, originalImage, originalPremult, ignorePremult);
            break;
        case 4:
            copyUnProcessedChannelsForComponents<PIX, maxValue, 4, 3>(premult, roi, processChannels, originalImage, originalPremult, ignorePremult);
            break;
        default:
            assert(false);
            break;
        }
        break;
    case 4:
        switch (srcNComps) {
        case 0:
            copyUnProcessedChannelsForComponents<PIX, maxValue, 0, 4>(premult, roi, processChannels, originalImage, originalPremult, ignorePremult);
            break;
        case 1:
            copyUnProcessedChannelsForComponents<PIX, maxValue, 1, 4>(premult, roi, processChannels, originalImage, originalPremult, ignorePremult);
            break;
        case 2:
            copyUnProcessedChannelsForComponents<PIX, maxValue, 2, 4>(premult, roi, processChannels, originalImage, originalPremult, ignorePremult);
            break;
        case 3:
            copyUnProcessedChannelsForComponents<PIX, maxValue, 3, 4>(premult, roi, processChannels, originalImage, originalPremult, ignorePremult);
            break;
        case 4:
            copyUnProcessedChannelsForComponents<PIX, maxValue, 4, 4>(premult, roi, processChannels, originalImage, originalPremult, ignorePremult);
            break;
        default:
            assert(false);
            break;
        }
        break;

    default:
        assert(false);
        break;
    } // switch
} // Image::copyUnProcessedChannelsForDepth

bool
Image::canCallCopyUnProcessedChannels(const std::bitset<4> processChannels) const
{
    int numComp = getComponents().getNumComponents();

    if (numComp == 0) {
        return false;
    }
    if ( (numComp == 1) && processChannels[3] ) { // 1 component is alpha
        return false;
    } else if ( (numComp == 2) && processChannels[0] && processChannels[1] ) {
        return false;
    } else if ( (numComp == 3) && processChannels[0] && processChannels[1] && processChannels[2] ) {
        return false;
    } else if ( (numComp == 4) && processChannels[0] && processChannels[1] && processChannels[2] && processChannels[3] ) {
        return false;
    }

    return true;
}

void
Image::copyUnProcessedChannels(const RectI& roi,
                               const ImagePremultiplicationEnum outputPremult,
                               const ImagePremultiplicationEnum originalImagePremult,
                               const std::bitset<4> processChannels,
                               const ImagePtr& originalImage,
                               bool ignorePremult,
                               const OSGLContextPtr& glContext)
{
    int numComp = getComponents().getNumComponents();

    if (numComp == 0) {
        return;
    }
    if ( (numComp == 1) && processChannels[3] ) { // 1 component is alpha
        return;
    } else if ( (numComp == 2) && processChannels[0] && processChannels[1] ) {
        return;
    } else if ( (numComp == 3) && processChannels[0] && processChannels[1] && processChannels[2] ) {
        return;
    } else if ( (numComp == 4) && processChannels[0] && processChannels[1] && processChannels[2] && processChannels[3] ) {
        return;
    }


    if ( originalImage && ( getMipMapLevel() != originalImage->getMipMapLevel() ) ) {
        qDebug() << "WARNING: attempting to call copyUnProcessedChannels on images with different mipMapLevel";

        return;
    }

    QWriteLocker k(&_entryLock);
    assert( !originalImage || getBitDepth() == originalImage->getBitDepth() );


    RectI srcRoi;
    roi.intersect(_bounds, &srcRoi);

    if (getStorageMode() == eStorageModeGLTex) {
        assert(glContext);
        assert(originalImage->getStorageMode() == eStorageModeGLTex);
        GLShaderPtr shader = glContext->getOrCreateCopyUnprocessedChannelsShader(processChannels[0], processChannels[1], processChannels[2], processChannels[3]);
        assert(shader);
        GLuint fboID = glContext->getFBOId();

        glBindFramebuffer(GL_FRAMEBUFFER, fboID);
        int target = getGLTextureTarget();
        glEnable(target);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture( target, getGLTextureID() );

        glTexParameteri (target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexParameteri (target, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri (target, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, getGLTextureID(), 0 /*LoD*/);
        glCheckFramebufferError();
        glCheckError();
        glActiveTexture(GL_TEXTURE1);
        glBindTexture( target, originalImage->getGLTextureID() );

        glTexParameteri (target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexParameteri (target, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri (target, GL_TEXTURE_WRAP_T, GL_REPEAT);


        shader->bind();
        shader->setUniform("originalImageTex", 1);
        shader->setUniform("outputImageTex", 0);
        OfxRGBAColourF procChannelsV = {
            processChannels[0] ? 1.f : 0.f,
            processChannels[1] ? 1.f : 0.f,
            processChannels[2] ? 1.f : 0.f,
            processChannels[3] ? 1.f : 0.f
        };
        shader->setUniform("processChannels", procChannelsV);
        applyTextureMapping(_bounds, srcRoi);
        shader->unbind();

        glCheckError();
        glBindTexture(target, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(target, 0);
        glCheckError();

        return;
    }


    bool premult = (outputPremult == eImagePremultiplicationPremultiplied);
    bool originalPremult = (originalImagePremult == eImagePremultiplicationPremultiplied);
    switch ( getBitDepth() ) {
    case eImageBitDepthByte:
        copyUnProcessedChannelsForDepth<unsigned char, 255>(premult, roi, processChannels, originalImage, originalPremult, ignorePremult);
        break;
    case eImageBitDepthShort:
        copyUnProcessedChannelsForDepth<unsigned short, 65535>(premult, roi, processChannels, originalImage, originalPremult, ignorePremult);
        break;
    case eImageBitDepthFloat:
        copyUnProcessedChannelsForDepth<float, 1>(premult, roi, processChannels, originalImage, originalPremult, ignorePremult);
        break;
    default:

        return;
    }
} // copyUnProcessedChannels

NATRON_NAMESPACE_EXIT
