//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "Gui/TextRenderer.h"

#include <boost/shared_ptr.hpp>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QtCore/QString>
#include <QtGui/QFont>
#include <QtGui/QImage>
#include <QtCore/QHash>
CLANG_DIAG_ON(deprecated)

#include "Global/Macros.h"
#include "Global/GLIncludes.h" //!<must be included before QGlWidget because of gl.h and glew.h
CLANG_DIAG_OFF(deprecated)
#include <QtOpenGL/QGLWidget>
CLANG_DIAG_ON(deprecated)

// warning: 'gluErrorString' is deprecated: first deprecated in OS X 10.9 [-Wdeprecated-declarations]
CLANG_DIAG_OFF(deprecated-declarations)
GCC_DIAG_OFF(deprecated-declarations)

using namespace Natron;

#define TEXTURE_SIZE 256

namespace
{

struct CharBitmap {
    GLuint texID;
    uint w;
    uint h;
    GLfloat xTexCoords[2];
    GLfloat yTexCoords[2];
};

struct TextRendererPrivate {
    TextRendererPrivate(const QFont &font);

    ~TextRendererPrivate();

    void newTransparantTexture();

    CharBitmap *createCharacter(QChar c, const QColor &color);

    void clearCache();

    QFont _font;

    QFontMetrics _fontMetrics;
    
    struct QColor_less {
        bool operator() (const QColor& lhs, const QColor& rhs) const {
            if (lhs.redF() < rhs.redF()) {
                return true;
            } else if(lhs.redF() > rhs.redF()) {
                return false;
            } else {
                if(lhs.greenF() < rhs.greenF()) {
                    return true;
                } else if(lhs.greenF() > rhs.greenF()) {
                    return false;
                } else {
                    if(lhs.blueF() < rhs.blueF()) {
                        return true;
                    } else if(lhs.blueF() > rhs.blueF()) {
                        return false;
                    } else {
                        if(lhs.alphaF() < rhs.alphaF()) {
                            return true;
                        } else if(lhs.alphaF() > rhs.alphaF()) {
                            return false;
                        } else {
                            return false; //< r,g,b,a components are equals
                        }
                    }
                }
            }
        }
    };

    ///foreach unicode character, a map registering for each color the rendered character.
    typedef std::map<QColor,CharBitmap,QColor_less> TexturedCharMap;
    typedef QHash<ushort, TexturedCharMap > BitmapCache;
    BitmapCache _bitmapsCache;

    std::list<GLuint> _usedTextures;

    GLint _xOffset;

    GLint _yOffset;
};

    typedef std::map<QFont, boost::shared_ptr<TextRendererPrivate> > FontRenderers;

}

TextRendererPrivate::TextRendererPrivate(const QFont &font)
    : _font(font)
    , _fontMetrics(font)
    , _xOffset(0)
    , _yOffset(0)
{
}

TextRendererPrivate::~TextRendererPrivate()
{
    clearCache();
}

void TextRendererPrivate::clearCache()
{
    foreach(GLuint texture, _usedTextures) {
        assert(glIsTexture(texture));
        glDeleteTextures(1, &texture);
    }
}
void TextRendererPrivate::newTransparantTexture()
{
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    assert(glIsTexture(texture));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    QImage image(TEXTURE_SIZE, TEXTURE_SIZE, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    image = QGLWidget::convertToGLFormat(image);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, TEXTURE_SIZE, TEXTURE_SIZE,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, image.bits());

    glBindTexture(GL_TEXTURE_2D, 0);
    _usedTextures.push_back(texture);
}

CharBitmap *TextRendererPrivate::createCharacter(QChar c, const QColor &color)
{
    ushort unic = c.unicode();

    //c is already in the cache
    BitmapCache::iterator it = _bitmapsCache.find(unic);
    if (it != _bitmapsCache.end()) {
        TexturedCharMap::iterator it2 = it.value().find(color);
        if (it2 != it.value().end()) {
            return &(*it2).second;
        }
    }

    if (_usedTextures.empty()) {
        newTransparantTexture();
    }

    GLuint texture = _usedTextures.back();

    GLsizei width = _fontMetrics.width(c);
    GLsizei height = _fontMetrics.height();

    //render into a new transparant pixmap using QPainter
    QImage image(width, height, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter;
    painter.begin(&image);
    painter.setRenderHints(QPainter::HighQualityAntialiasing
                           | QPainter::TextAntialiasing);
    painter.setFont(_font);
    painter.setPen(color);

    painter.drawText(0, _fontMetrics.ascent(), c);
    painter.end();


    //fill the texture with the QImage
    image = QGLWidget::convertToGLFormat(image);
    glBindTexture(GL_TEXTURE_2D, texture);
    assert(glIsTexture(texture));
    glTexSubImage2D(GL_TEXTURE_2D, 0, _xOffset, _yOffset, width, height, GL_RGBA,
                    GL_UNSIGNED_BYTE, image.bits());
    glCheckError();


    if (it == _bitmapsCache.end()) {
        TexturedCharMap newHash;
        it = _bitmapsCache.insert(unic, newHash);
    }
    CharBitmap character;
    character.texID = texture;

    character.w = width;
    character.h = height;

    character.xTexCoords[0] = (GLfloat)_xOffset / TEXTURE_SIZE;
    character.xTexCoords[1] = (GLfloat)(_xOffset + width) / TEXTURE_SIZE;

    character.yTexCoords[0] = (GLfloat)_yOffset / TEXTURE_SIZE;
    character.yTexCoords[1] = (GLfloat)(_yOffset + height) / TEXTURE_SIZE;

    std::pair<TexturedCharMap::iterator,bool> retIt = it.value().insert(std::make_pair(color, character)); // insert a new charactr
    assert(retIt.second);
    
    _xOffset += width;
    if (_xOffset + _fontMetrics.maxWidth() >= TEXTURE_SIZE) {
        _xOffset = 1;
        _yOffset += height;
    }
    if (_yOffset + _fontMetrics.height() >= TEXTURE_SIZE) {
        newTransparantTexture();
        _yOffset = 1;
    }

    return &(retIt.first->second);
}



struct TextRenderer::Implementation {
    Implementation()
        : renderers() {}

    FontRenderers renderers;
};

TextRenderer::TextRenderer()
    : _imp(new Implementation)
{
}

TextRenderer::~TextRenderer()
{
}


void TextRenderer::renderText(float x, float y, const QString &text, const QColor &color, const QFont &font) const
{
    glCheckError();
    boost::shared_ptr<TextRendererPrivate> p;
    FontRenderers::iterator it = _imp->renderers.find(font);
    if (it != _imp->renderers.end()) {
        p  = (*it).second;
    } else {
        p = boost::shared_ptr<TextRendererPrivate>(new TextRendererPrivate(font));
        _imp->renderers[font] = p;
    }
    assert(p);
    glColor4f(1., 1., 1., 1.);
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
    glPushMatrix();
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLuint texture = 0;
    glTranslatef(x, y, 0);
    for (int i = 0; i < text.length(); ++i) {
        CharBitmap *c = p->createCharacter(text[i], color);
        if (!c) {
            continue;
        }
        if (texture != c->texID) {
            texture = c->texID;
            assert(glIsTexture(texture));
            glBindTexture(GL_TEXTURE_2D, texture);
        }
        glCheckError();
        glBegin(GL_QUADS);
        glTexCoord2f(c->xTexCoords[0], c->yTexCoords[0]);
        glVertex2f(0, 0);
        glTexCoord2f(c->xTexCoords[1], c->yTexCoords[0]);
        glVertex2f(c->w, 0);
        glTexCoord2f(c->xTexCoords[1], c->yTexCoords[1]);
        glVertex2f(c->w, c->h);
        glTexCoord2f(c->xTexCoords[0], c->yTexCoords[1]);
        glVertex2f(0, c->h);
        glEnd();
        glCheckErrorIgnoreOSXBug();
        glTranslatef(c->w, 0, 0);
        glCheckError();

    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glPopMatrix();
    glPopAttrib();
    glCheckError();
    glColor4f(1., 1., 1., 1.);
}

