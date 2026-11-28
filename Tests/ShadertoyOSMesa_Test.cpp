/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2026 The Natron developers
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

// CPU (OSMesa) rendering of openfx-misc's Shadertoy plug-in, compared with stored references
// (Tests/data/osmesa/*.pfm), so that an OSMesa, Mesa or LLVM upgrade of the packages is tested by
// renders, not only by loading the plug-in.
//
// Shadertoy renders on the CPU through a mangled OSMesa (llvmpipe, or softpipe) when GPU rendering
// is off ("enableGPU", which only a Shadertoy built with OSMesa has). Each test renders a fixed
// shader into a 64x48 image at a fixed frame, without any random or date input: the result only
// depends on OSMesa and on the shader compiler. The shaders, and not Shadertoy's presets, are in this
// file, so that a change to a preset in openfx-misc doesn't change the references.
//
// Without Shadertoy, or with a Shadertoy built without OSMesa (e.g. the natron_testing builds of
// ci.yml), the tests are skipped, unless NATRON_TESTS_REQUIRE_OSMESA=1, where they fail: the release
// builds (tools/jenkins, whose openfx-misc always has the OSMesa Shadertoy) set it, so that a
// missing OSMesa can't go unnoticed.
//
// With NATRON_TESTS_OSMESA_OUTPUT_DIR set, each render is also written to <that dir>/<name>.pfm, to
// look at the renders or make new references.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <ofxNatron.h>

#include <QtCore/QDir>
#include <QtCore/QString>

#include "Engine/Image.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"

#include "OfxTestPlugin.h"
#include "TestUtils.h"

#define PLUGINID_OFX_SHADERTOY "net.sf.openfx.Shadertoy"

NATRON_NAMESPACE_USING

namespace {
const int kWidth = 64;
const int kHeight = 48;
// the frame of every render: iTime is frame / fps (24 by default), 0.5 s
const double kFrame = 12.;

// Largest difference allowed between a render and its reference, on each channel. llvmpipe is
// deterministic for a given Mesa, LLVM and CPU: each pixel is computed independently of the others
// and of the threads that render the tiles, in 32-bit floats (OSMesa32 buffers, float textures).
// The references were checked against the same shaders evaluated in double precision (bilinear
// filtering and mirror wrapping included): every value is within 5e-7, with llvmpipe and softpipe.
// A reference is shared by every platform (x86_64 with SSE or AVX, aarch64) and by the Mesa and
// LLVM versions of the packages, whose renders may differ by the approximations of sin, exp and pow
// and by fused multiply-adds (at most about 1e-5). 1e-3 is far above that; a broken render (black,
// garbage, wrong filter or wrap mode, wrong iTime or iResolution, 8-bit textures) differs by more.
const float kTolerance = 1e-3f;

bool
envIsSet(const char* name)
{
    const char* v = std::getenv(name);

    return v && *v && ( std::string(v) != "0" );
}

// Skips the test, or fails it if OSMesa rendering is required (NATRON_TESTS_REQUIRE_OSMESA=1).
void
skipOrFail(const std::string& reason)
{
    if ( envIsSet("NATRON_TESTS_REQUIRE_OSMESA") ) {
        ADD_FAILURE() << reason << " (NATRON_TESTS_REQUIRE_OSMESA is set)";
    } else {
        NatronTest::skip(reason);
    }
}

// An RGBA float image of kWidth x kHeight, rows from bottom to top (as in a PFM file and in OpenGL).
typedef std::vector<float> Pixels;

// Reads a color PFM ("PF", 3 floats per pixel, rows from bottom to top), into RGBA with alpha 1.
bool
readPFM(const std::string& path,
        int* width,
        int* height,
        Pixels* pixels)
{
    std::ifstream f(path.c_str(), std::ios::binary);
    std::string magic;
    double scale = 0.;

    if ( !(f >> magic >> *width >> *height >> scale) || (magic != "PF") || (*width <= 0) || (*height <= 0) || (scale == 0.) ) {
        return false;
    }
    f.get(); // the single whitespace character after the scale
    const std::size_t n = static_cast<std::size_t>(*width) * *height;
    std::vector<float> rgb(n * 3);
    if ( !f.read( reinterpret_cast<char*>( rgb.data() ), rgb.size() * sizeof(float) ) ) {
        return false;
    }
    // a negative scale means little endian, the byte order of every platform that Natron runs on
    if (scale > 0.) {
        for (float& v : rgb) {
            unsigned char* b = reinterpret_cast<unsigned char*>(&v);
            std::swap(b[0], b[3]);
            std::swap(b[1], b[2]);
        }
    }
    pixels->resize(n * 4);
    for (std::size_t i = 0; i < n; ++i) {
        (*pixels)[i * 4 + 0] = rgb[i * 3 + 0];
        (*pixels)[i * 4 + 1] = rgb[i * 3 + 1];
        (*pixels)[i * 4 + 2] = rgb[i * 3 + 2];
        (*pixels)[i * 4 + 3] = 1.f;
    }

    return true;
}

// Writes the RGB channels of pixels as a little-endian color PFM.
bool
writePFM(const std::string& path,
         int width,
         int height,
         const Pixels& pixels)
{
    std::ofstream f(path.c_str(), std::ios::binary);

    f << "PF\n" << width << " " << height << "\n-1\n";
    for (std::size_t i = 0; i < static_cast<std::size_t>(width) * height; ++i) {
        f.write( reinterpret_cast<const char*>(&pixels[i * 4]), 3 * sizeof(float) );
    }

    return bool(f);
}

std::string
referencePath(const std::string& name)
{
    return std::string(NATRON_TESTS_DATA_DIR) + "/osmesa/" + name + ".pfm";
}

template <typename K>
std::shared_ptr<K>
knob(const NodePtr& node,
     const std::string& name)
{
    return std::dynamic_pointer_cast<K>( node->getKnobByName(name) );
}

// Sets a choice parameter to the option with that ID or label. False if there is no such option.
bool
setChoice(const NodePtr& node,
          const std::string& name,
          const std::string& option)
{
    KnobChoicePtr choice = knob<KnobChoice>(node, name);

    if (!choice) {
        return false;
    }
    const std::vector<ChoiceOption> options = choice->getEntries_mt_safe();
    for (std::size_t i = 0; i < options.size(); ++i) {
        if ( (options[i].id == option) || (options[i].label == option) ) {
            choice->setValue( static_cast<int>(i) );

            return choice->getValue() == static_cast<int>(i);
        }
    }

    return false;
}

class ShadertoyOSMesaTest
    : public testing::Test
{
protected:
    NatronTest::NodeScope _nodes;

    // A Shadertoy node that renders source on the CPU, with cpuDriver (if the driver can be selected),
    // into a kWidth x kHeight image; null (and the test is skipped or failed) if it can't.
    NodePtr createShadertoy(const std::string& source,
                            const std::string& cpuDriver = "llvmpipe")
    {
        if ( !NatronTest::isPluginAvailable(PLUGINID_OFX_SHADERTOY) ) {
            skipOrFail("plug-in " PLUGINID_OFX_SHADERTOY " not found (needs openfx-misc's Shadertoy.ofx.bundle in OFX_PLUGIN_PATH)");

            return NodePtr();
        }
        NodePtr node = _nodes.create(PLUGINID_OFX_SHADERTOY);
        if (!node) {
            ADD_FAILURE() << "cannot create a Shadertoy node";

            return NodePtr();
        }
        // only a Shadertoy built with OSMesa can switch GPU rendering off
        KnobBoolPtr enableGPU = knob<KnobBool>(node, "enableGPU");
        if (!enableGPU) {
            skipOrFail("Shadertoy was built without OSMesa (no \"enableGPU\" parameter): it can't render on the CPU");

            return NodePtr();
        }
        enableGPU->setValue(false);
        // OSMesaDriverSelectable(): with llvmpipe and softpipe; with only one of them, no choice
        if ( knob<KnobChoice>(node, "cpuDriver") && !setChoice(node, "cpuDriver", cpuDriver) ) {
            ADD_FAILURE() << "Shadertoy has no CPU driver " << cpuDriver;

            return NodePtr();
        }
        KnobIntPtr size = knob<KnobInt>(node, kNatronParamFormatSize);
        if ( !size || !setChoice(node, "bbox", "format") ) {
            ADD_FAILURE() << "cannot set the size of the Shadertoy output";

            return NodePtr();
        }
        size->setValue(kWidth, ViewSpec::all(), 0);
        size->setValue(kHeight, ViewSpec::all(), 1);
        KnobStringPtr imageShaderSource = knob<KnobString>(node, "imageShaderSource");
        KnobButtonPtr imageShaderCompile = knob<KnobButton>(node, "imageShaderCompile");
        if (!imageShaderSource || !imageShaderCompile) {
            ADD_FAILURE() << "Shadertoy has no imageShaderSource or imageShaderCompile parameter";

            return NodePtr();
        }
        imageShaderSource->setValue(source);
        // the shader is compiled by the next render
        imageShaderCompile->trigger();

        return node;
    }

    // Renders node at kFrame on the CPU, and compares the result with the reference called name.
    // outputName: the name of the render in NATRON_TESTS_OSMESA_OUTPUT_DIR (default: name).
    void renderAndCompare(const NodePtr& node,
                          const std::string& name,
                          const std::string& outputName = std::string())
    {
        ImagePtr img = NatronTestPlugin::renderFrame(node, kFrame, false /*allowGPURendering*/);

        ASSERT_TRUE( bool(img) ) << name << ": the render failed";
        ASSERT_EQ( eImageBitDepthFloat, img->getBitDepth() );
        ASSERT_EQ( 4u, img->getComponentsCount() );
        const RectI bounds = img->getBounds();
        ASSERT_TRUE( (bounds.x1 <= 0) && (bounds.y1 <= 0) && (bounds.x2 >= kWidth) && (bounds.y2 >= kHeight) )
            << name << ": the image is " << bounds.width() << "x" << bounds.height();

        Pixels pixels( static_cast<std::size_t>(kWidth) * kHeight * 4 );
        {
            Image::ReadAccess acc = img->getReadRights();
            for (int y = 0; y < kHeight; ++y) {
                const float* row = reinterpret_cast<const float*>( acc.pixelAt(0, y) );
                ASSERT_TRUE(row != nullptr);
                std::memcpy( &pixels[static_cast<std::size_t>(y) * kWidth * 4], row, kWidth * 4 * sizeof(float) );
            }
        }

        const char* outputDir = std::getenv("NATRON_TESTS_OSMESA_OUTPUT_DIR");
        if (outputDir && *outputDir) {
            QDir().mkpath( QString::fromUtf8(outputDir) );
            const std::string out = std::string(outputDir) + "/" + ( outputName.empty() ? name : outputName ) + ".pfm";
            EXPECT_TRUE( writePFM(out, kWidth, kHeight, pixels) ) << "cannot write " << out;
        }

        int refWidth = 0, refHeight = 0;
        Pixels reference;
        ASSERT_TRUE( readPFM(referencePath(name), &refWidth, &refHeight, &reference) ) << "cannot read " << referencePath(name);
        ASSERT_EQ(kWidth, refWidth);
        ASSERT_EQ(kHeight, refHeight);

        float maxError = 0.f;
        double sumError = 0.;
        int worst = 0;
        for (std::size_t i = 0; i < pixels.size(); ++i) {
            // a NaN or an infinity, in the render or in the reference, is the largest difference
            const float e = ( std::isfinite(pixels[i]) && std::isfinite(reference[i]) ) ? std::fabs(pixels[i] - reference[i]) : HUGE_VALF;
            sumError += e;
            if (e > maxError) {
                maxError = e;
                worst = static_cast<int>(i);
            }
        }
        const int c = worst % 4;
        const int x = (worst / 4) % kWidth;
        const int y = worst / 4 / kWidth;
        EXPECT_LE(maxError, kTolerance) << name << ": largest difference at (" << x << ", " << y << ") channel " << c
                                        << ": " << pixels[worst] << " instead of " << reference[worst]
                                        << "; mean difference " << sumError / pixels.size();
    }
};

// Arithmetic and the built-in functions (sin, exp, pow, sqrt, mix, smoothstep), iResolution and iTime.
const char* const kShaderFunctions =
    "void mainImage(out vec4 fragColor, in vec2 fragCoord)\n"
    "{\n"
    "    vec2 uv = fragCoord.xy / iResolution.xy;\n"
    "    float r = 0.5 + 0.5 * sin(6.2831853 * uv.x + 2.0 * iTime);\n"
    "    float g = smoothstep(0.1, 0.9, uv.y) * exp(-uv.x);\n"
    "    float b = mix(pow(uv.x + 0.05, 2.2), sqrt(uv.y), 0.5);\n"
    "    fragColor = vec4(r, g, b, 1.0);\n"
    "}\n";

// A loop, a function, a matrix, atan and length.
const char* const kShaderLoop =
    "vec2 rotate(vec2 p, float a)\n"
    "{\n"
    "    float c = cos(a);\n"
    "    float s = sin(a);\n"
    "    return mat2(c, s, -s, c) * p;\n"
    "}\n"
    "\n"
    "void mainImage(out vec4 fragColor, in vec2 fragCoord)\n"
    "{\n"
    "    vec2 p = (2.0 * fragCoord.xy - iResolution.xy) / iResolution.y;\n"
    "    vec3 col = vec3(0.0);\n"
    "    for (int i = 0; i < 8; ++i) {\n"
    "        p = rotate(p, 0.3) * 1.1;\n"
    "        float d = length(p - vec2(0.3, 0.1));\n"
    "        col += vec3(0.5 + 0.5 * cos(float(i) + atan(p.y, p.x)), exp(-3.0 * d), 1.0 / (1.0 + 4.0 * d * d)) / 8.0;\n"
    "    }\n"
    "    fragColor = vec4(col, 1.0);\n"
    "}\n";

// The image of an input (iChannel0, a render of kShaderFunctions), distorted: linear filtering, and
// mirror wrapping outside of [0,1].
const char* const kShaderTexture =
    "void mainImage(out vec4 fragColor, in vec2 fragCoord)\n"
    "{\n"
    "    vec2 c = fragCoord.xy / iResolution.xy - 0.5;\n"
    "    vec2 q = 0.5 + c * (1.0 + 0.8 * dot(c, c)) * 1.3;\n"
    "    vec3 t = texture2D(iChannel0, q).rgb;\n"
    "    fragColor = vec4(t.bgr * 0.9 + 0.05, 1.0);\n"
    "}\n";
} // anonymous namespace

TEST_F(ShadertoyOSMesaTest, Functions)
{
    NodePtr node = createShadertoy(kShaderFunctions);

    if (node) {
        renderAndCompare(node, "Shadertoy-functions");
    }
}

TEST_F(ShadertoyOSMesaTest, Loop)
{
    NodePtr node = createShadertoy(kShaderLoop);

    if (node) {
        renderAndCompare(node, "Shadertoy-loop");
    }
}

TEST_F(ShadertoyOSMesaTest, Texture)
{
    NodePtr source = createShadertoy(kShaderFunctions);
    NodePtr node = source ? createShadertoy(kShaderTexture) : NodePtr();

    if (!node) {
        return;
    }
    ASSERT_TRUE( setChoice(node, "mipmap0", "linear") );
    ASSERT_TRUE( setChoice(node, "wrap0", "mirror") );
    ASSERT_TRUE( node->connectInput(source, 0) );
    renderAndCompare(node, "Shadertoy-texture");
}

// softpipe, Mesa's reference driver, gives the same image as llvmpipe (the same reference)
TEST_F(ShadertoyOSMesaTest, SoftpipeFunctions)
{
    NodePtr node = createShadertoy(kShaderFunctions, "softpipe");

    if (!node) {
        return;
    }
    if ( !knob<KnobChoice>(node, "cpuDriver") ) {
        // every OSMesa of the packages has llvmpipe and softpipe (osmesa-install)
        skipOrFail("this OSMesa has only one CPU driver: softpipe can't be selected");

        return;
    }
    renderAndCompare(node, "Shadertoy-functions", "Shadertoy-functions-softpipe");
}
