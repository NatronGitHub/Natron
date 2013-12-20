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
"uniform int bitDepth;\n"
"uniform float expMult;\n"
"uniform int lut;\n"
"\n"
"void main(){\n"
"    \n"
"    vec4 color_tmp = texture2D(Tex,gl_TexCoord[0].st);\n"
"if(bitDepth != 0){\n"
"    \n"
"    color_tmp.rgb = color_tmp.rgb ;\n"
"   if(lut == 0){ // srgb\n"
"           \n" // << TO SRGB
"       if (color_tmp.r<=0.0031308){\n"
"           color_tmp.r=(12.92*color_tmp.r);\n"
"       }else{\n"
"           color_tmp.r =(((1.0+0.055)*pow(color_tmp.r,1.0/2.4))-0.055);\n"
"       }\n"
"       if (color_tmp.g<=0.0031308){\n"
"           color_tmp.g=(12.92*color_tmp.g);\n"
"       }else{\n"
"           color_tmp.g =(((1.0+0.055)*pow(color_tmp.g,1.0/2.4))-0.055);\n"
"       }\n"
"       if (color_tmp.b<=0.0031308){\n"
"           color_tmp.b=(12.92*color_tmp.b);\n"
"       }else{\n"
"           color_tmp.b =(((1.0+0.055)*pow(color_tmp.b,1.0/2.4))-0.055);\n"
"       }\n" // << END TO SRGB
"   }\n"
"   else if( lut == 2){ // Rec 709\n" // << TO REC 709
"       if(color_tmp.r < 0.018){\n"
"           color_tmp.r=color_tmp.r*4.500;\n"
"       }else{\n"
"           color_tmp.r=(1.099*pow(color_tmp.r,0.45) - 0.099);\n"
"       }\n"
"       if(color_tmp.g < 0.018){\n"
"           color_tmp.g=color_tmp.g*4.500;\n"
"       }else{\n"
"           color_tmp.g=(1.099*pow(color_tmp.g,0.45) - 0.099);\n"
"       }\n"
""
"       if(color_tmp.b < 0.018){\n"
"           color_tmp.b=color_tmp.b*4.500;\n"
"       }else{\n"
"           color_tmp.b=(1.099*pow(color_tmp.b,0.45) - 0.099);\n"
"       }\n"
"\n"
"   }\n"   // << END TO REC 709
"   color_tmp.rgb = color_tmp.rgb * expMult;\n"
"}\n" // end if !bytemode
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
