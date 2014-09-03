//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "SplashScreen.h"


#include <ctime>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QPainter>
#include <QStyleOption>
#include <QApplication>
#include <QDesktopWidget>
CLANG_DIAG_ON(deprecated)


SplashScreen::SplashScreen(const QString & filePath)
    : QWidget(0,Qt::ToolTip | Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint)
      , _pixmap()
      , _text()
      , _versionString()
{
    QString customBuildString(NATRON_CUSTOM_BUILD_USER_NAME);
    
    if (customBuildString.isEmpty()) {
        _versionString = QString("v" NATRON_VERSION_STRING " - " NATRON_DEVELOPMENT_STATUS + QObject::tr(" - built on ") + __DATE__ );
    } else {
        _versionString = QString (NATRON_APPLICATION_NAME +
                                  QObject::tr(" for ") +
                                  customBuildString +
                                  QObject::tr(" - built on ") +
                                  __DATE__ );
    }
    
    setAttribute( Qt::WA_TransparentForMouseEvents );
    setAttribute(Qt::WA_TranslucentBackground, true);

    _pixmap.load(filePath);
    _pixmap = _pixmap.scaled(768, 432);

    resize( _pixmap.width(), _pixmap.height() );
    show();

    QDesktopWidget* desktop = QApplication::desktop();
    QRect screen = desktop->screenGeometry();
    move(screen.width() / 2 - width() / 2, screen.height() / 2 - height() / 2);
}

void
SplashScreen::updateText(const QString & text)
{
    _text = text;
    repaint();
    QCoreApplication::processEvents();
}

void
SplashScreen::paintEvent(QPaintEvent*)
{
    QStyleOption opt;

    opt.init(this);
    QPainter p(this);

    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

    p.drawPixmap(0,0,_pixmap);
    p.setPen(Qt::white);
    p.drawText(QPointF(120,100), _text);
    p.drawText(QPointF(10, 420),_versionString);
}

