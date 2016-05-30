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

#include "DefaultShaders.h"

NATRON_NAMESPACE_ENTER;

const char* fillConstant_FragmentShader =
"uniform vec4 fillColor;\n"
"\n"
"void main() {\n"
"	gl_FragColor = fillColor;\n"
"}";

const char* applyMaskMix_FragmentShader =
"uniform sampler2D originalImageTex;"
"uniform sampler2D outputImageTex;"
"uniform sampler2D maskImageTex;"
"uniform int maskEnabled;"
"uniform float mixValue;\n"
"\n"
"void main() {\n"
"   vec4 srcColor = texture2D(originalImageTex,gl_TexCoord[0].st);\n"
"   vec4 dstColor = texture2D(outputImageTex,gl_TexCoord[0].st);\n"
"   float alpha;\n"
"   if (maskEnabled != 0) {\n"
"       vec4 maskColor = texture2D(maskImageTex,gl_TexCoord[0].st);\n"
"       alpha = mixValue * maskColor.a;\n"
"   } else {\n"
"       alpha = mixValue;"
"   }"
"   gl_FragColor = dstColor * alpha + (1.0 - alpha) * srcColor;"
"}";

const char* copyUnprocessedChannels_FragmentShader =
"uniform sampler2D originalImageTex;"
"uniform sampler2D outputImageTex;"
"uniform vec4 processChannels;"
"\n"
"void main() {\n"
"   vec4 srcColor = texture2D(originalImageTex,gl_TexCoord[0].st);\n"
"   vec4 dstColor = texture2D(outputImageTex,gl_TexCoord[0].st);\n"
"   if (processChannels.r == 0.0) \n"
"       gl_FragColor.r = srcColor.r;\n"
"   else\n"
"       gl_FragColor.r = dstColor.r;\n"
"\n"
"   if (processChannels.g == 0.0) \n"
"       gl_FragColor.g = srcColor.g;\n"
"   else\n"
"       gl_FragColor.g = dstColor.g;\n"
"\n"
"   if (processChannels.b == 0.0) \n"
"       gl_FragColor.b = srcColor.b;\n"
"   else\n"
"       gl_FragColor.b = dstColor.b;\n"
"\n"
"   if (processChannels.a == 0.0) \n"
"       gl_FragColor.a = srcColor.a;\n"
"   else\n"
"       gl_FragColor.a = dstColor.a;\n"
"}";

NATRON_NAMESPACE_EXIT;
