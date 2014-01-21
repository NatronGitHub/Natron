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

 

 




#ifndef NatronOsX_shaders_h
#define NatronOsX_shaders_h

/*Contains the definitions of the display shaders*/

static const char* fragRGB =
"uniform sampler2D Tex;\n"
"uniform float expMult;\n"
"uniform int lut;\n"
"\n"
"float linear_to_srgb(float c) {\n"
"    return (c<=0.0031308) ? (12.92*c) : (((1.0+0.055)*pow(c,1.0/2.4))-0.055);\n"
"}\n"
"float linear_to_rec709(float c) {"
"    return (c<0.018) ? (4.500*c) : (1.099*pow(c,0.45) - 0.099);\n"
"}\n"
"void main(){\n"
"    \n"
"    vec4 color_tmp = texture2D(Tex,gl_TexCoord[0].st);\n"
"    \n"
"    color_tmp.rgb = color_tmp.rgb ;\n"
"   if(lut == 0){ // srgb\n"
// << TO SRGB
"       color_tmp.r = linear_to_srgb(color_tmp.r);"
"       color_tmp.g = linear_to_srgb(color_tmp.g);"
"       color_tmp.b = linear_to_srgb(color_tmp.b);"
// << END TO SRGB
"   }\n"
"   else if( lut == 2){ // Rec 709\n" // << TO REC 709
"       color_tmp.r = linear_to_rec709(color_tmp.r);"
"       color_tmp.g = linear_to_rec709(color_tmp.g);"
"       color_tmp.b = linear_to_rec709(color_tmp.b);"
"\n"
"   }\n"   // << END TO REC 709
"   color_tmp.rgb = color_tmp.rgb * expMult;\n"
"	gl_FragColor = color_tmp;\n"
"}\n"
;

static const char* vertRGB =

"void main()\n"
"{\n"
"\n"
"   gl_TexCoord[0] = gl_MultiTexCoord0;\n"
"	gl_Position = ftransform();\n"
"}\n"
"\n"
;

/*There's a black texture used for when the user disconnect the viewer
 It's not just a shader,because we still need coordinates feedback.
 */
static const char* blackFrag =
"uniform sampler2D Tex;\n"
"void main()\n"
"{\n"
"    gl_FragColor = texture2D(Tex,gl_TexCoord[0].st);\n"
"}\n";
#endif
