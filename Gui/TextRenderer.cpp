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

#include "TextRenderer.h"

#include <boost/shared_ptr.hpp>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QString>
#include <QtGui/QFont>
#include <QtGui/QImage>
#include <QtCore/QHash>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/Macros.h"
#include "Global/GLIncludes.h" //!<must be included before QGlWidget because of gl.h and glew.h
CLANG_DIAG_OFF(deprecated)
#include <QtOpenGL/QGLWidget>
CLANG_DIAG_ON(deprecated)

// warning: 'gluErrorString' is deprecated: first deprecated in OS X 10.9 [-Wdeprecated-declarations]
CLANG_DIAG_OFF(deprecated-declarations)
GCC_DIAG_OFF(deprecated-declarations)

NATRON_NAMESPACE_ENTER;

#define TEXTURE_SIZE 256

#define NATRON_TEXT_RENDERER_USE_CACHE

namespace {
struct CharBitmap
{
    GLuint texID;
    uint w;
    uint h;
    GLfloat xTexCoords[2];
    GLfloat yTexCoords[2];
};

struct TextRendererPrivate
{
    TextRendererPrivate(const QFont &font);

    ~TextRendererPrivate();

    void newTransparentTexture();

    CharBitmap * createCharacter(QChar c);

    void clearUsedTextures();

    void clearBitmapCache();

    QFont _font;
    QFontMetrics _fontMetrics;

#ifdef NATRON_TEXT_RENDERER_USE_CACHE
    ///foreach unicode character, a character texture (multiple characters belong to the same real opengl texture)
    typedef QHash<ushort, CharBitmap* > BitmapCache;
    BitmapCache _bitmapsCache;
#endif

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
    clearUsedTextures();
    clearBitmapCache();
}

void
TextRendererPrivate::clearUsedTextures()
{
    for (std::list<GLuint>::iterator it = _usedTextures.begin(); it != _usedTextures.end(); ++it) {
        //https://www.opengl.org/sdk/docs/man2/xhtml/glIsTexture.xml
        //A name returned by glGenTextures, but not yet associated with a texture by calling glBindTexture, is not the name of a texture.
        //Not sure if we should leave this assert here since  textures are not bound any longer at this point.
        //        assert(glIsTexture(texture));
        glDeleteTextures( 1, &(*it) );
    }
}

void
TextRendererPrivate::clearBitmapCache()
{
#ifdef NATRON_TEXT_RENDERER_USE_CACHE
    for (BitmapCache::iterator it = _bitmapsCache.begin(); it != _bitmapsCache.end(); ++it) {
        delete it.value();
    }
    _bitmapsCache.clear();
#endif
}

void
TextRendererPrivate::newTransparentTexture()
{
    GLuint savedTexture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&savedTexture);
    GLuint texture;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    assert( glIsTexture(texture) );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    QImage image(TEXTURE_SIZE, TEXTURE_SIZE, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    image = QGLWidget::convertToGLFormat(image);
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, TEXTURE_SIZE, TEXTURE_SIZE,
                  0, GL_RGBA, GL_UNSIGNED_BYTE, image.bits() );

    glBindTexture(GL_TEXTURE_2D, savedTexture);
    _usedTextures.push_back(texture);
}

CharBitmap *
TextRendererPrivate::createCharacter(QChar c)
{
#ifdef NATRON_TEXT_RENDERER_USE_CACHE

    ushort unic = c.unicode();
    //c is already in the cache
    BitmapCache::iterator it = _bitmapsCache.find(unic);
    if ( it != _bitmapsCache.end() ) {
        return it.value();
    }
#endif

    if ( _usedTextures.empty() ) {
        newTransparentTexture();
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
    painter.setPen(Qt::white);

    painter.drawText(0, _fontMetrics.ascent(), c);
    painter.end();


    //fill the texture with the QImage
    image = QGLWidget::convertToGLFormat(image);
    glCheckError();

    GLuint savedTexture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&savedTexture);
    glBindTexture(GL_TEXTURE_2D, texture);
    assert( glIsTexture(texture) );
    glTexSubImage2D( GL_TEXTURE_2D, 0, _xOffset, _yOffset, width, height, GL_RGBA,
                     GL_UNSIGNED_BYTE, image.bits() );
    glCheckError();
    glBindTexture(GL_TEXTURE_2D, savedTexture);


    CharBitmap *character = new CharBitmap;
    character->texID = texture;

    character->w = width;
    character->h = height;

    character->xTexCoords[0] = (GLfloat)_xOffset / TEXTURE_SIZE;
    character->xTexCoords[1] = (GLfloat)(_xOffset + width) / TEXTURE_SIZE;

    character->yTexCoords[0] = (GLfloat)_yOffset / TEXTURE_SIZE;
    character->yTexCoords[1] = (GLfloat)(_yOffset + height) / TEXTURE_SIZE;

#ifdef NATRON_TEXT_RENDERER_USE_CACHE
    BitmapCache::iterator retIt = _bitmapsCache.insert(unic, character); // insert a new charactr
    assert( retIt != _bitmapsCache.end() );
#endif

    _xOffset += width;
    if (_xOffset + width >= TEXTURE_SIZE) {
        _xOffset = 1;
        _yOffset += height;
    }
    if (_yOffset + height >= TEXTURE_SIZE) {
        newTransparentTexture();
        _yOffset = 1;
    }

#ifdef NATRON_TEXT_RENDERER_USE_CACHE

    return retIt.value();
#else

    return character;
#endif
} // createCharacter

struct TextRenderer::Implementation
{
    Implementation()
        : renderers()
    {
    }

    FontRenderers renderers;
};

TextRenderer::TextRenderer()
    : _imp(new Implementation)
{
}

TextRenderer::~TextRenderer()
{
}

void
TextRenderer::renderText(float x,
                         float y,
                         float scalex,
                         float scaley,
                         const QString &text,
                         const QColor &color,
                         const QFont &font) const
{
    glCheckError();
    boost::shared_ptr<TextRendererPrivate> p;
    FontRenderers::iterator it = _imp->renderers.find(font);
    if ( it != _imp->renderers.end() ) {
        p  = (*it).second;
    } else {
        p = boost::shared_ptr<TextRendererPrivate>( new TextRendererPrivate(font) );
        _imp->renderers[font] = p;
    }
    assert(p);

    GLuint savedTexture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&savedTexture);
    {
        GLProtectAttrib a(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_TRANSFORM_BIT);
        GLProtectMatrix pr(GL_MODELVIEW);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_TEXTURE_2D);
        GLuint texture = 0;

        glTranslatef(x, y, 0);
        glColor4f( color.redF(), color.greenF(), color.blueF(), color.alphaF() );
        for (int i = 0; i < text.length(); ++i) {
            CharBitmap *c = p->createCharacter(text[i]);
            if (!c) {
                continue;
            }
            if (texture != c->texID) {
                texture = c->texID;
                glBindTexture(GL_TEXTURE_2D, texture);
                assert( glIsTexture(texture) );
            }
            glCheckError();
            glBegin(GL_QUADS);
            glTexCoord2f(c->xTexCoords[0], c->yTexCoords[0]);
            glVertex2f(0, 0);
            glTexCoord2f(c->xTexCoords[1], c->yTexCoords[0]);
            glVertex2f(c->w * scalex, 0);
            glTexCoord2f(c->xTexCoords[1], c->yTexCoords[1]);
            glVertex2f(c->w * scalex, c->h * scaley);
            glTexCoord2f(c->xTexCoords[0], c->yTexCoords[1]);
            glVertex2f(0, c->h * scaley);
            glEnd();
            glCheckErrorIgnoreOSXBug();
            glTranslatef(c->w * scalex, 0, 0);
            glCheckError();
        }
    } // GLProtectAttrib a(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_TRANSFORM_BIT);
    glBindTexture(GL_TEXTURE_2D, savedTexture);

    glCheckError();
} // renderText

NATRON_NAMESPACE_EXIT;

