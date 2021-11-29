/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
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

#include "SplashScreen.h"

#include <ctime>
#include <stdexcept>

CLANG_DIAG_OFF(deprecated)
#include <QPainter>
#include <QStyleOption>
#include <QApplication>

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
#include <QScreen>
#else
#include <QDesktopWidget>
#endif

CLANG_DIAG_ON(deprecated)

#ifdef DEBUG
#include "Global/FloatingPointExceptions.h"
#endif

NATRON_NAMESPACE_ENTER

SplashScreen::SplashScreen(const QString & filePath)
    : QWidget(0, Qt::FramelessWindowHint)
    , _pixmap()
    , _text()
    , _versionString()
{
    QString customBuildString = QString::fromUtf8(NATRON_CUSTOM_BUILD_USER_NAME);

    if ( customBuildString.isEmpty() ) {
        QString buildNo;
        if ( QString::fromUtf8(NATRON_DEVELOPMENT_STATUS) == QString::fromUtf8(NATRON_DEVELOPMENT_RELEASE_CANDIDATE) ) {
            buildNo = QString::number(NATRON_BUILD_NUMBER);
        }
        _versionString = tr("v%1 - %2 %3")
                         .arg( QString::fromUtf8(NATRON_VERSION_STRING) )
                         .arg( QString::fromUtf8(NATRON_DEVELOPMENT_STATUS) )
                         .arg(buildNo);
#     if defined(NATRON_CONFIG_SNAPSHOT) || defined(DEBUG)
        _versionString += QString::fromUtf8(" - ") + tr("built on %1").arg( QString::fromUtf8(__DATE__) );
#     endif
    } else {
        _versionString = tr("%1 for %2")
                         .arg( QString::fromUtf8(NATRON_APPLICATION_NAME) )
                         .arg(customBuildString);
#     if defined(NATRON_CONFIG_SNAPSHOT) || defined(DEBUG)
        _versionString += QString::fromUtf8(" - ") + tr("built on %1").arg( QString::fromUtf8(__DATE__) );
#     endif
    }

    setAttribute( Qt::WA_TransparentForMouseEvents );
    setAttribute(Qt::WA_TranslucentBackground, true);

    _pixmap.load(filePath);
    _scale = 1.;
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#if defined(Q_WS_WIN32)
    // code from Gui::devicePixelRatio()
    HDC wscreen = GetDC(winId());
    FLOAT horizontalDPI = GetDeviceCaps(wscreen, LOGPIXELSX);
    ReleaseDC(0, wscreen);
    _scale = static_cast<qreal>(horizontalDPI) / 96.;
#endif
#endif
    if (_scale != 1.) {
        _pixmap = _pixmap.scaled( int(_pixmap.width() * _scale), int(_pixmap.height() * _scale),
                                 Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    resize( _pixmap.width(), _pixmap.height() );
    {
#ifdef DEBUG
        boost_adaptbx::floating_point::exception_trapping trap(0);
#endif
        show();
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    QScreen* desktop = QGuiApplication::primaryScreen();
    QRect screen = desktop->availableGeometry();
#else
    QDesktopWidget* desktop = QApplication::desktop();
    QRect screen = desktop->screenGeometry();
#endif
    move(screen.width() / 2 - width() / 2, screen.height() / 2 - height() / 2);
}

void
SplashScreen::updateText(const QString & text)
{
    _text = text;
    {
#ifdef DEBUG
        boost_adaptbx::floating_point::exception_trapping trap(0);
#endif
        update();
        QCoreApplication::processEvents();
    }
}

void
SplashScreen::paintEvent(QPaintEvent*)
{
    QStyleOption opt;

    opt.init(this);
    QPainter p(this);

    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

    p.drawPixmap(0, 0, _pixmap);
    p.setPen(Qt::white);
    p.drawText(QPointF(120 * _scale, 100 * _scale), _text);
    p.drawText(QPointF(20 * _scale, _pixmap.height() - 15 * _scale), _versionString);
}

LoadProjectSplashScreen::LoadProjectSplashScreen(const QString & filePath)
    : QWidget(0, Qt::SplashScreen)
    , _pixmap()
    , _text()
    , _projectName(filePath)
{
    setAttribute( Qt::WA_TransparentForMouseEvents );
    setAttribute(Qt::WA_TranslucentBackground, true);

    _pixmap.load( QString::fromUtf8(":Resources/Images/loadProjectSplashscreen.png") );

    _scale = 1.;
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#if defined(Q_WS_WIN32)
    // code from Gui::devicePixelRatio()
    HDC wscreen = GetDC(winId());
    FLOAT horizontalDPI = GetDeviceCaps(wscreen, LOGPIXELSX);
    ReleaseDC(0, wscreen);
    _scale = static_cast<qreal>(horizontalDPI) / 96.;
#endif
#endif
    if (_scale != 1.) {
        _pixmap = _pixmap.scaled( int(_pixmap.width() * _scale), int(_pixmap.height() * _scale),
                                 Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    resize( _pixmap.width(), _pixmap.height() );
    show();

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    QScreen* desktop = QGuiApplication::primaryScreen();
    QRect screen = desktop->availableGeometry();
#else
    QDesktopWidget* desktop = QApplication::desktop();
    QRect screen = desktop->screenGeometry();
#endif
    move(screen.width() / 2 - width() / 2, screen.height() / 2 - height() / 2);
}

void
LoadProjectSplashScreen::updateText(const QString & text)
{
    _text = text;
    update();
    {
#ifdef DEBUG
        boost_adaptbx::floating_point::exception_trapping trap(0);
#endif
        QCoreApplication::processEvents();
    }
}

void
LoadProjectSplashScreen::paintEvent(QPaintEvent* /*e*/)
{
    QStyleOption opt;

    opt.init(this);
    QPainter p(this);

    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

    p.drawPixmap(0, 0, _pixmap);
    p.setPen(Qt::white);
    p.drawText(QPointF(250 * _scale, _pixmap.height() - 51 * _scale), _text);

    QString loadString( tr("Loading ") );
    QFontMetrics fm = p.fontMetrics();
    QPointF loadStrPos(300 * _scale, _pixmap.height() / 2.);
    p.drawText(QPointF(loadStrPos.x() + (fm.width(loadString) + 5) * _scale, _pixmap.height() / 2.), _projectName);
    p.setPen( QColor(243, 137, 0) );
    p.drawText(loadStrPos, loadString);
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_SplashScreen.cpp"
