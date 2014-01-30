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

#ifndef SPLASHSCREEN_H
#define SPLASHSCREEN_H

#include <QWidget>
#include <QPixmap>
#include <QString>

class SplashScreen : public QWidget
{
    
    QPixmap _pixmap;
    QString _text;
    QString _versionString;
    
public:
    
    SplashScreen(const QString& filePath);
    
    virtual ~SplashScreen() {}
        
    void updateText(const QString& text);
    
private:
    
    virtual void paintEvent(QPaintEvent* event);
};

#endif // SPLASHSCREEN_H
