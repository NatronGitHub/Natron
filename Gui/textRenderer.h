//
//  textRenderer.h
//  PowiterOsX
//
//  Created by Alexandre on 2/24/13.
//  Copyright (c) 2013 Alexandre. All rights reserved.
//

#ifndef __PowiterOsX__textRenderer__
#define __PowiterOsX__textRenderer__
#include "Superviser/gl_OsDependent.h"
#include <QtCore/QChar>
#include <QtGui/QFont>
#include <QtCore/QHash>
#include <QtGui/QColor>
#include <iostream>
#include <FTGL/ftgl.h>


class ViewerGL;



class TextRenderer
{
    FTTextureFont* m_font;
    ViewerGL *m_glwidget;
protected:
    
    
    GLboolean m_isBetweenBeginAndEnd;
   
    void do_begin();
    void do_end();
    
public:
    TextRenderer(ViewerGL *glwidget);
    ~TextRenderer();
    
    
    void print( int x, int y, const QString &string,QColor color);
    
    /**
     * Call this before doing multiple calls to print(). This is
     * not necessary, but will improve performance. Don't forget,
     * then, to call end() after.
     */
    void begin();
    
    /**
     * Call this after having called begin() and print().
     */
    void end();
};


#endif /* defined(__PowiterOsX__textRenderer__) */
