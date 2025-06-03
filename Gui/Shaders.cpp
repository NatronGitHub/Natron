/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "Shaders.h"

#include <stdexcept>

NATRON_NAMESPACE_ENTER

const char* fragRGB = R"(
    uniform sampler2D Tex;
    uniform float gain;
    uniform float offset;
    uniform int lut;
    uniform float gamma;
    uniform int dither;

    #if __VERSION__ < 150
        float rnd(vec2 p)
        { 
            return 1.0 - 2.0*fract(sin(dot(p.xy ,vec2(12.9898,78.233))) * 43758.5453);
        }
    #else
        float rnd(vec2 p)
        { 
            int n = int(p.x * 40.0 + p.y * 6400.0); 
            n = (n << 13) ^ n; 
            return 1.0 - float( (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0; 
        }
    #endif
    
    float linear_to_srgb(float c) {
        return (c<=0.0031308) ? (12.92*c) : (((1.0+0.055)*pow(c,1.0/2.4))-0.055);
    }
    float linear_to_rec709(float c) {
        return (c<0.018) ? (4.500*c) : (1.099*pow(c,0.45) - 0.099);
    }
    float linear_to_bt1886(float c) {
        return pow(c,1.0/2.4);
    }
    void main() {
        vec4 color_tmp = texture2D(Tex,gl_TexCoord[0].st);
        color_tmp.rgb = (color_tmp.rgb * gain) + offset;
        if (lut == 0) { // srgb
            // << TO SRGB
           color_tmp.r = linear_to_srgb(color_tmp.r);
           color_tmp.g = linear_to_srgb(color_tmp.g);
           color_tmp.b = linear_to_srgb(color_tmp.b);
            // << END TO SRGB
       } else if (lut == 2) { // Rec 709
            // << TO REC 709
           color_tmp.r = linear_to_rec709(color_tmp.r);
           color_tmp.g = linear_to_rec709(color_tmp.g);
           color_tmp.b = linear_to_rec709(color_tmp.b);
            // << END TO REC 709
       } else if (lut == 3) { // BT1886
            // << TO BT1886
           color_tmp.r = linear_to_bt1886(color_tmp.r);
           color_tmp.g = linear_to_bt1886(color_tmp.g);
           color_tmp.b = linear_to_bt1886(color_tmp.b);
            // << END TO BT1886
       }
       if (gamma <= 0.) {
           color_tmp.r = (color_tmp.r >= 1.) ? 1. : 0.;
           color_tmp.g = (color_tmp.g >= 1.) ? 1. : 0.;
           color_tmp.b = (color_tmp.b >= 1.) ? 1. : 0.;
       } else {
           color_tmp.r = pow(color_tmp.r, 1./gamma);
           color_tmp.g = pow(color_tmp.g, 1./gamma);
           color_tmp.b = pow(color_tmp.b, 1./gamma);
       }

       if (dither){
        //dithering
        ivec2 texsize = textureSize2D(Tex, 0);
        vec2 coord = gl_TexCoord[0].st / texsize;

        vec3 c = color_tmp.rgb; 
        float a = color_tmp.a;
        float scale = 255.0;
        float seed = 32;

        vec2 pr = (0.9 + 0.1 * seed) * coord.xy * 1000.1; 
        vec2 pg = (0.9 + 0.1 * seed) * coord.xy * 1000.2; 
        vec2 pb = (0.9 + 0.1 * seed) * coord.xy * 1000.3;

        
        gl_FragColor = vec4(c.rgb + vec3(rnd(pr), rnd(pg), rnd(pb)) * vec3(0.5) / vec3(scale), a);
       } else {
           gl_FragColor = vec4(color_tmp.rgb, color_tmp.a);
       }
    }
)";
const char* vertRGB = R"(
    void main()
    {
        gl_TexCoord[0] = gl_MultiTexCoord0;
        gl_Position = ftransform();
    }
    )";

/*There's a black texture used for when the user disconnect the viewer
   It's not just a shader,because we still need coordinates feedback.
 */
const char* blackFrag = R"(
    uniform sampler2D Tex;
    void main()
    {
        gl_FragColor = texture2D(Tex,gl_TexCoord[0].st);
    };
)";

const char *histogramComputation_frag = R"(
    #extension GL_ARB_texture_rectangle : enable
    uniform sampler2DRect Tex;
    uniform int channel;
    void main()
    {
        gl_FragColor = vec4(1.0,0.0,0.0,1.0);
    }
)";

const char *histogramComputationVertex_vert = R"(
    #extension GL_ARB_texture_rectangle : enable
    uniform sampler2DRect Tex;
    uniform int channel;
    attribute vec2 TexCoord;
    void main()
    {
    
        vec4 c = texture2DRect(Tex, TexCoord.xy  );
    
        float sel = 0.0;
        if(channel == 0){ // luminance
         sel = 0.299*c.r + 0.587*c.g +0.114*c.b;
        }else if(channel == 1){ // red
                sel = c.r;
        }else if(channel == 2){ //green
                sel = c.g;
        }else if(channel == 3){ // blue
                sel = c.b;
        }else if(channel == 4){ // alpha
                sel = c.a;
        }
        clamp(sel, 0.0, 1.0);
    // set new point position to the color intensity in [-1.0,1.0] interval
    // as this is homogeneous coord. clip space
        gl_Position.x =(2.0-4.0/257.0)*sel-1.0+2.0/257.0;
        gl_Position.y = 0.0;
        gl_Position.z = 0.0;
    }
)";

const char *histogramRendering_frag = R"(
    #extension GL_ARB_texture_rectangle : enable
    uniform sampler2DRect HistogramTex;
    uniform sampler2DRect MaximumRedTex;
    uniform sampler2DRect MaximumGreenTex;
    uniform sampler2DRect MaximumBlueTex;
    uniform int channel;
    void main()
    {
    	if(channel == 0){
    		gl_FragColor =vec4(0.8,0.8,0.8,0.8);
    	}else if(channel == 1){
    		gl_FragColor =vec4(1.0,0.0,0.0,0.8);
    	}else if(channel == 2){
    		gl_FragColor =vec4(0.0,1.0,0.0,0.8);
    	}else if(channel == 3){
    		gl_FragColor =vec4(0.0,0.0,1.0,0.8);
    	}
       
    }
)";

const char *histogramRenderingVertex_vert = R"(
    #extension GL_ARB_texture_rectangle : enable
    uniform sampler2DRect HistogramTex;
    uniform sampler2DRect MaximumRedTex;
    uniform sampler2DRect MaximumGreenTex;
    uniform sampler2DRect MaximumBlueTex;
    uniform int channel;
    attribute vec3 TexCoord;
    void main()
    {
    
        vec4 c = texture2DRect(HistogramTex, TexCoord.xy  );
    	float bottom = TexCoord.z;
    	float maximum = 0.0;
    	float maximumRed = texture2DRect(MaximumRedTex,vec2(0.0,0.0)).r;
    	float maximumGreen = texture2DRect(MaximumGreenTex,vec2(0.0,0.0)).r;
    	float maximumBlue = texture2DRect(MaximumBlueTex,vec2(0.0,0.0)).r;
    	maximum = max(max(maximumRed,maximumGreen),maximumBlue);
    	if(maximum == 0.0){
    		maximum = 1000000.0;
    	}
    // set new point position to the color intensity in [-1.0,1.0] interval
    // as this is homogeneous coord. clip space
        gl_Position.x =(2.0-4.0/257.0)*(TexCoord.x/255.0)-1.0+2.0/257.0;
    	if(bottom == 1.0){
               gl_Position.y = -1.0;
    	}else{
    		float y = c.r/maximum;
    		gl_Position.y = 2.0*y-1.0;
    	}
        gl_Position.z = 0.0;
    }
)";

const char* minimal_vert = R"(
    #extension GL_ARB_texture_rectangle : enable
    gl_TexCoord[0]=gl_MultiTexCoord0;
    gl_Position = ftransform();
;
const char *histogramMaximum_frag =
    #extension GL_ARB_texture_rectangle : enable
    uniform sampler2DRect Tex;
    void main()
    {
        vec4 a,b,c,d;
    	vec2 texCoord = gl_TexCoord[0].st;
    	texCoord.s = (texCoord.s-0.5) * 4.0 + 0.5;
        a = texture2DRect(Tex,texCoord.st);
    	vec2 texCoord1,texCoord2,texCoord3;
    	texCoord1 = texCoord.st + vec2(1.0,0.0);
    	texCoord2 = texCoord.st + vec2(2.0,0.0);
    	texCoord3 = texCoord.st + vec2(3.0,0.0);
    	if(texCoord1.s <= 256.0){
    		b = texture2DRect(Tex,texCoord1);
    	}else{
    		b = a;
    	}
    	if(texCoord2.s <= 256.0){
    		c = texture2DRect(Tex,texCoord2);
    	}else{
    		c = a;
    	}
    	if(texCoord3.s <= 256.0){
    		d = texture2DRect(Tex,texCoord3);
    	}else{
    		d = a;
    	}
        gl_FragColor = vec4(max(max(a.r,b.r),max(c.r,d.r)),0.0,0.0,1.0);
    }
)";


NATRON_NAMESPACE_EXIT
