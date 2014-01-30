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

#include "SplashScreen.h"


#include <ctime>
#include <QPainter>
#include <QStyleOption>
#include <QApplication>
#include <QDesktopWidget>

#include "Global/Macros.h"


SplashScreen::SplashScreen(const QString& filePath)
: QWidget(0,Qt::ToolTip | Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint)
, _pixmap()
, _text()
, _versionString("v" NATRON_VERSION_STRING " built on " __DATE__ )
{
    setAttribute( Qt::WA_TransparentForMouseEvents );
    setAttribute(Qt::WA_TranslucentBackground, true);

    _pixmap.load(filePath);
    _pixmap = _pixmap.scaled(768, 432);
    
    resize(_pixmap.width(), _pixmap.height());
    show();

    QDesktopWidget* desktop = QApplication::desktop();
    QRect screen = desktop->screenGeometry();
    move(screen.width() / 2 - width() / 2, screen.height() / 2 - height() /2);

}

void SplashScreen::updateText(const QString& text) {
    _text = text;
    repaint();
    QCoreApplication::processEvents();
}

void SplashScreen::paintEvent(QPaintEvent*)
{
    
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    
    p.drawPixmap(0,0,_pixmap);
    p.setPen(Qt::white);
    p.drawText(QPointF(120,100), _text);
    p.drawText(QPointF(180, 250),_versionString);
}
