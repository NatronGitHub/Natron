//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
/***************************************************************************
 ***************************************************************************
 copyright            : (C) 2006 by Benoit Jacob
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
//Bitstream Vera Fonts Copyright
//------------------------------
//
//Copyright (c) 2003 by Bitstream, Inc. All Rights Reserved. Bitstream Vera is
//a trademark of Bitstream, Inc.

#include <QtGui/QPainter>
#include "Gui/GLViewer.h"
#include "Gui/textRenderer.h"
#include <iostream>
using namespace std;
TextRenderer::TextRenderer(ViewerGL *glwidget)
{
	m_glwidget = glwidget;
    m_font = new FTTextureFont(ROOT"/libs/fonts/DejaVuSans.ttf");
    if(m_font->Error())
        cout << "error loading font " << endl;
    m_font->FaceSize(14);

}

TextRenderer::~TextRenderer()
{
	delete m_font;
}


void TextRenderer::print( int x, int y, const QString &string,QColor color )
{
	if( ! m_glwidget ) return;
	if( string.isEmpty() ) return;
	glPushMatrix();
    checkGLErrors();
    float zoomFactor = m_glwidget->getZoomFactor();
    glTranslatef( x, y , 0 );
    glScalef(1.f/zoomFactor,1.f/zoomFactor, 1);
    double pa= m_glwidget->displayWindow().pixel_aspect();
    if(pa >1){
        glScalef(1/pa, 1, 1);
    }else if(pa < 1){
        glScalef(1, pa, 1);
    }
    QByteArray ba = string.toLocal8Bit();
    glColor4f(color.redF(),color.greenF(),color.blueF(),color.alphaF());
    m_font->Render(ba.data());
    
    checkGLErrors();
	glPopMatrix();
	
}