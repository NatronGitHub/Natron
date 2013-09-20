//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#ifndef POWITER_GUI_TEXTRENDERER_H
#define POWITER_GUI_TEXTRENDERER_H

#include <QtGui/QColor>

class ViewerGL;
class FTTextureFont;

class TextRenderer
{
    FTTextureFont* m_font;
    ViewerGL *m_glwidget;
protected:
     
    void do_begin();
    void do_end();
    
public:
    explicit TextRenderer(ViewerGL* glwidget);
    ~TextRenderer();
    
    void print( int x, int y, const QString&string, QColor color);
    
};


#endif /* defined(POWITER_GUI_TEXTRENDERER_H) */
