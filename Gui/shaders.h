//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com

#ifndef PowiterOsX_shaders_h
#define PowiterOsX_shaders_h

/*Contains the definitions of the display shaders*/


static const char* fragRGB ="uniform sampler2D Tex;\n"
"uniform float byteMode;\n"
"uniform float expMult;\n"
"uniform float lut;\n"
"\n"
"void main(){\n"
"    \n"
"    vec4 color_tmp = texture2D(Tex,gl_TexCoord[0].st);\n"
"if(byteMode == 0.0){\n"
"    \n"
"    color_tmp.rgb = color_tmp.rgb * color_tmp.a;\n"
"   if(lut == 1.0){ // srgb\n"
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
"   else if( lut ==2.0){ // Rec 709\n" // << TO REC 709
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
//"else{"
//"    float tmp = color_tmp.r;\n"
//"    color_tmp.r = color_tmp.b;\n"
//"    color_tmp.b = tmp;\n"
//"}\n"
"	gl_FragColor = color_tmp;\n"
"}\n"
;

static const char* vertRGB =

"void main()\n"
"{\n"
"\n"
"    gl_TexCoord[0] = gl_MultiTexCoord0;\n"
"	gl_Position = ftransform();\n"
"}\n"
"\n"
;

static const char* fragLC =
"uniform float lut;"
"uniform sampler2D Tex;\n"
"uniform float byteMode;\n"
"uniform vec3 yw;\n"
"uniform float expMult;\n"
"void main()\n"
"{\n"
"	vec4 yryby = texture2D(Tex,gl_TexCoord[0].st);\n"
"    \n"
"    float red = (yryby.r + 1.0) * yryby.r;\n"
"    float blue = red;\n"//(yryby.b + 1.0) * yryby.r;\n" // << TEMPORARY TO HANDLE LUMINANCE CHROMA EXR FILES AS GRAYSCALE
"    float green = red;\n"//(yryby.r - red * yw.x - blue * yw.z) / yw.y;\n" // << TEMPORARY TO HANDLE LUMINANCE CHROMA EXR FILES AS GRAYSCALE
"    \n"
"    vec4 color_tmp=vec4(red , green , blue ,yryby.a);\n"
"if(byteMode == 0.0){\n"
"   if(lut == 1.0){ // srgb\n"
"       \n" // << TO SRGB
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
"   else if( lut ==2.0){ // Rec 709\n" // << TO REC 709
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
"    color_tmp.rgb = color_tmp.rgb * expMult;\n"
"}\n"//endif bytemode
"	gl_FragColor = color_tmp;\n"
"}\n"
"\n"
;

static const char* vertLC =
"void main()\n"
"{\n"
"\n"
"    gl_TexCoord[0] = gl_MultiTexCoord0;\n"
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
