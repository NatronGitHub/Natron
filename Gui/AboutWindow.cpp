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

#include "AboutWindow.h"

#include <stdexcept>

#include <boost/config.hpp> // BOOST_COMPILER
#include <boost/predef.h> // BOOST_PREDEF_ macros

CLANG_DIAG_OFF(deprecated)
#include <QSplitter>
#include <QTextBrowser>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QtCore/QFile>
#include <QtCore/QTextCodec>
#include <QItemSelectionModel>
#include <QHeaderView>
#include <QtCore/QDir>
CLANG_DIAG_ON(deprecated)
#include <qhttpserver.h>

#include "Global/GlobalDefines.h"
#include "Global/GitVersion.h"

#include "Engine/AppManager.h"
#include "Engine/OSGLContext.h"
#include "Engine/Utils.h" // convertFromPlainText

#include "Gui/Button.h"
#include "Gui/Gui.h"
#include "Gui/Label.h"
#include "Gui/TableModelView.h"

#define THIRD_PARTY_LICENSE_DIR_PATH ":"

NATRON_NAMESPACE_ENTER

AboutWindow::AboutWindow(QWidget* parent)
    : QDialog(parent)
{
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

    setWindowTitle( tr("About %1").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _mainLayout = new QVBoxLayout(this);
    setLayout(_mainLayout);

    _iconLabel = new Label(this);
    _iconLabel->setPixmap( QPixmap( QString::fromUtf8(NATRON_APPLICATION_ICON_PATH) ).scaled(128 * _scale, 128 * _scale) );
    _mainLayout->addWidget(_iconLabel);

    _tabWidget = new QTabWidget(this);
    _mainLayout->addWidget(_tabWidget);

    _buttonContainer = new QWidget(this);
    _buttonLayout = new QHBoxLayout(_buttonContainer);
    _buttonLayout->addStretch();

    _closeButton = new Button(tr("Close"), _buttonContainer);
    QObject::connect( _closeButton, SIGNAL(clicked()), this, SLOT(accept()) );
    _buttonLayout->addWidget(_closeButton);

    _mainLayout->addWidget(_buttonContainer);

    ///filling tabs now

    _aboutText = new QTextBrowser(_tabWidget);
    _aboutText->setOpenExternalLinks(true);
    QString aboutText;
    {
        QString customBuild( QString::fromUtf8(NATRON_CUSTOM_BUILD_USER_NAME) );
        if ( !customBuild.isEmpty() ) {
            aboutText = QString::fromUtf8("<p>%1 custom build for %2 %3.</p>")
                        .arg( QString::fromUtf8(NATRON_APPLICATION_NAME) )
                        .arg(customBuild)
#ifdef DEBUG
                        .arg( QString::fromUtf8("(debug)") );
#else
#ifdef NDEBUG
                        // release with asserts disabled (should be the *real* release)
                        .arg( QString() );
#else
                        // release with asserts enabled
                        .arg( QString::fromUtf8("(opt)") );
#endif
#endif
        } else {
            aboutText = QString::fromUtf8("<p>%1 version %2 %3%4.</p>")
                        .arg( QString::fromUtf8(NATRON_APPLICATION_NAME) )
                        .arg( QString::fromUtf8(NATRON_VERSION_STRING) )
                        .arg( QString::fromUtf8(NATRON_DEVELOPMENT_STATUS) )
#ifdef DEBUG
                        .arg( QString::fromUtf8(" (debug)") );
#else
#ifdef NDEBUG
                        // release with asserts disabled (should be the *real* release)
                        .arg( QString() );
#else
                        // release with asserts enabled
                        .arg( QString::fromUtf8(" (opt)") );
#endif
#endif
        }
    }
    {
        QString licenseStr;
        QFile license( QString::fromUtf8(":LICENSE_SHORT.txt") );
        license.open(QIODevice::ReadOnly | QIODevice::Text);
        licenseStr = NATRON_NAMESPACE::convertFromPlainText(QTextCodec::codecForName("UTF-8")->toUnicode( license.readAll() ), NATRON_NAMESPACE::WhiteSpaceNormal);
        aboutText.append(licenseStr);
    }
    {
        QString endAbout = QString::fromUtf8("<p>%1</p>").arg( tr("See the <a href=\"%2\">%1 website</a> "
                                                                  "for more information on this software.")
                             .arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) // %1
                             .arg( QString::fromUtf8(NATRON_WEBSITE_URL) ) ); // %2
        aboutText.append(endAbout);
    }
    {
        QString argStr = ( QString::fromUtf8("<a href=\"https://github.com/NatronGitHub/Natron/tree/" GIT_COMMIT "\">")
                           + QString::fromUtf8(GIT_COMMIT).mid(0, 7)
                           + QString::fromUtf8("</a>") );
#ifdef _OPENMP
#if _OPENMP == 200505
// since GCC 4.2
#define OPENMP_VERSION_STRING "2.5"
#elif _OPENMP == 200805
// since GCC 4.4
#define OPENMP_VERSION_STRING "3.0"
#elif _OPENMP == 201107
// since GCC 4.7
#define OPENMP_VERSION_STRING "3.1"
#elif _OPENMP == 201307
// since GCC 4.9
#define OPENMP_VERSION_STRING "4.0"
#elif _OPENMP == 201511
// since GCC 6.1
#define OPENMP_VERSION_STRING "4.5"
#elif _OPENMP == 201811
#define OPENMP_VERSION_STRING "5.0"
#elif _OPENMP == 202011
#define OPENMP_VERSION_STRING "5.1"
#else
#define OPENMP_VERSION_STRING STRINGIZE_CPP_NAME(_OPENMP)
#endif

#define OPENMP_STRING " with OpenMP " OPENMP_VERSION_STRING
#else
#define OPENMP_STRING ""
#endif

#if BOOST_ARCH_ALPHA
#define BOOST_ARCH_NAME BOOST_ARCH_ALPHA_NAME
#elif BOOST_ARCH_ARM
#define BOOST_ARCH_NAME BOOST_ARCH_ARM_NAME
#elif BOOST_ARCH_BLACKFIN
#define BOOST_ARCH_NAME BOOST_ARCH_BLACKFIN_NAME
#elif BOOST_ARCH_CONVEX
#define BOOST_ARCH_NAME BOOST_ARCH_CONVEX_NAME
#elif BOOST_ARCH_IA64
#define BOOST_ARCH_NAME BOOST_ARCH_IA64_NAME
#elif BOOST_ARCH_M68K
#define BOOST_ARCH_NAME BOOST_ARCH_M68K_NAME
#elif BOOST_ARCH_MIPS
#define BOOST_ARCH_NAME BOOST_ARCH_MIPS_NAME
#elif BOOST_ARCH_PARISK
#define BOOST_ARCH_NAME BOOST_ARCH_PARISK_NAME
#elif BOOST_ARCH_PPC
#define BOOST_ARCH_NAME BOOST_ARCH_PPC_NAME
#elif BOOST_ARCH_PYRAMID
#define BOOST_ARCH_NAME BOOST_ARCH_PYRAMID_NAME
#elif BOOST_ARCH_RS6000
#define BOOST_ARCH_NAME BOOST_ARCH_RS6000_NAME
#elif BOOST_ARCH_SPARC
#define BOOST_ARCH_NAME BOOST_ARCH_SPARC_NAME
#elif BOOST_ARCH_SH
#define BOOST_ARCH_NAME BOOST_ARCH_SH_NAME
#elif BOOST_ARCH_SYS370
#define BOOST_ARCH_NAME BOOST_ARCH_SYS370_NAME
#elif BOOST_ARCH_SYS390
#define BOOST_ARCH_NAME BOOST_ARCH_SYS390_NAME
#elif BOOST_ARCH_Z
#define BOOST_ARCH_NAME BOOST_ARCH_Z_NAME
#elif BOOST_ARCH_X86_32
#define BOOST_ARCH_NAME BOOST_ARCH_X86_32_NAME
#elif BOOST_ARCH_X86_64
#define BOOST_ARCH_NAME BOOST_ARCH_X86_64_NAME
#else
#define BOOST_ARCH_NAME "unknown arch"
#endif

#if BOOST_OS_AIX
#define BOOST_OS_NAME BOOST_OS_AIX_NAME
#define BOOST_OS BOOST_OS_AIX
#elif BOOST_OS_AMIGAOS
#define BOOST_OS_NAME BOOST_OS_AMIGAOS_NAME
#define BOOST_OS BOOST_OS_AMIGAOS
#elif BOOST_OS_ANDROID
#define BOOST_OS_NAME BOOST_OS_ANDROID_NAME
#define BOOST_OS BOOST_OS_ANDROID
#elif BOOST_OS_BEOS
#define BOOST_OS_NAME BOOST_OS_BEOS_NAME
#define BOOST_OS BOOST_OS_BEOS
#elif BOOST_OS_BSD
#define BOOST_OS_NAME BOOST_OS_BSD_NAME
#define BOOST_OS BOOST_OS_BSD
#elif BOOST_OS_CYGWIN
#define BOOST_OS_NAME BOOST_OS_CYGWIN_NAME
#define BOOST_OS BOOST_OS_CYGWIN
#elif BOOST_OS_HAIKU
#define BOOST_OS_NAME BOOST_OS_HAIKU_NAME
#define BOOST_OS BOOST_OS_HAIKU
#elif BOOST_OS_HPUX
#define BOOST_OS_NAME BOOST_OS_HPUX_NAME
#define BOOST_OS BOOST_OS_HPUX
#elif BOOST_OS_IOS
#define BOOST_OS_NAME BOOST_OS_IOS_NAME
#define BOOST_OS BOOST_OS_IOS
#elif BOOST_OS_IRIX
#define BOOST_OS_NAME BOOST_OS_IRIX_NAME
#define BOOST_OS BOOST_OS_IRIX
#elif BOOST_OS_LINUX
#define BOOST_OS_NAME BOOST_OS_LINUX_NAME
#define BOOST_OS BOOST_OS_LINUX
#elif BOOST_OS_MACOS
#define BOOST_OS_NAME BOOST_OS_MACOS_NAME
#define BOOST_OS BOOST_OS_MACOS
#elif BOOST_OS_OS400
#define BOOST_OS_NAME BOOST_OS_OS400_NAME
#define BOOST_OS BOOST_OS_OS400
#elif BOOST_OS_QNX
#define BOOST_OS_NAME BOOST_OS_QNX_NAME
#define BOOST_OS BOOST_OS_QNX
#elif BOOST_OS_SOLARIS
#define BOOST_OS_NAME BOOST_OS_SOLARIS_NAME
#define BOOST_OS BOOST_OS_SOLARIS
#elif BOOST_OS_UNIX
#define BOOST_OS_NAME BOOST_OS_UNIX_NAME
#define BOOST_OS BOOST_OS_UNIX
#elif BOOST_OS_SVR4
#define BOOST_OS_NAME BOOST_OS_SVR4_NAME
#define BOOST_OS BOOST_OS_SVR4
#elif BOOST_OS_VMS
#define BOOST_OS_NAME BOOST_OS_VMS_NAME
#define BOOST_OS BOOST_OS_VMS
#elif BOOST_OS_WINDOWS
#define BOOST_OS_NAME BOOST_OS_WINDOWS_NAME
#define BOOST_OS BOOST_OS_WINDOWS
#elif BOOST_OS_BSD_BSDI
#define BOOST_OS_NAME BOOST_OS_BSD_BSDI_NAME
#define BOOST_OS BOOST_OS_BSD_BSDI
#elif BOOST_OS_BSD_DRAGONFLY
#define BOOST_OS_NAME BOOST_OS_BSD_DRAGONFLY_NAME
#define BOOST_OS BOOST_OS_BSD_DRAGONFLY
#elif BOOST_OS_BSD_FREE
#define BOOST_OS_NAME BOOST_OS_BSD_FREE_NAME
#define BOOST_OS BOOST_OS_BSD_FREE
#elif BOOST_OS_BSD_NET
#define BOOST_OS_NAME BOOST_OS_BSD_NET_NAME
#define BOOST_OS BOOST_OS_BSD_NET
#elif BOOST_OS_BSD_OPEN
#define BOOST_OS_NAME BOOST_OS_BSD_OPEN_NAME
#define BOOST_OS BOOST_OS_BSD_OPEN
#else
#define BOOST_OS_NAME "unknown OS"
#define BOOST_OS BOOST_VERSION_NUMBER_NOT_AVAILABLE
#endif

        QString osVer = QString::fromUtf8(BOOST_OS_NAME);
#if BOOST_OS_MACOS >= BOOST_VERSION_NUMBER(10,0,0)
        // special case for OSX, detect min/max versions
        // see #include <AvailabilityMacros.h>
        //    MAC_OS_X_VERSION_MIN_REQUIRED
        //    MAC_OS_X_VERSION_MAX_ALLOWED
        //#define MAC_OS_X_VERSION_10_0         1000
        //#define MAC_OS_X_VERSION_10_1         1010
        //#define MAC_OS_X_VERSION_10_2         1020
        //#define MAC_OS_X_VERSION_10_3         1030
        //#define MAC_OS_X_VERSION_10_4         1040
        //#define MAC_OS_X_VERSION_10_5         1050
        //#define MAC_OS_X_VERSION_10_6         1060
        //#define MAC_OS_X_VERSION_10_7         1070
        //#define MAC_OS_X_VERSION_10_8         1080
        //#define MAC_OS_X_VERSION_10_9         1090
        //#define MAC_OS_X_VERSION_10_10      101000
        //#define MAC_OS_X_VERSION_10_10_2    101002
        //#define MAC_OS_X_VERSION_10_10_3    101003
        //#define MAC_OS_X_VERSION_10_11      101100
        //#define MAC_OS_X_VERSION_10_11_2    101102
        //#define MAC_OS_X_VERSION_10_11_3    101103
        //#define MAC_OS_X_VERSION_10_11_4    101104
        //#define MAC_OS_X_VERSION_10_12      101200
        //#define MAC_OS_X_VERSION_10_12_1    101201
        //#define MAC_OS_X_VERSION_10_12_2    101202
        //#define MAC_OS_X_VERSION_10_12_4    101204
        //#define MAC_OS_X_VERSION_10_13      101300
        //#define MAC_OS_X_VERSION_10_13_1    101301
        //#define MAC_OS_X_VERSION_10_13_2    101302
        //#define MAC_OS_X_VERSION_10_13_4    101304
        //#define MAC_OS_X_VERSION_10_14      101400
        //#define MAC_OS_X_VERSION_10_14_1    101401
        //#define MAC_OS_X_VERSION_10_14_4    101404
        //#define MAC_OS_X_VERSION_10_15      101500
        //#define MAC_OS_VERSION_11_0         110000
        //#define MAC_OS_VERSION_11_1         110100
        //#define MAC_OS_VERSION_11_3         110300
        const bool version_available = true;
#if MAC_OS_X_VERSION_MIN_REQUIRED < 100000
        const unsigned major = MAC_OS_X_VERSION_MIN_REQUIRED / 100;
        const unsigned minor = (MAC_OS_X_VERSION_MIN_REQUIRED % 100) / 10;
        const unsigned patch = MAC_OS_X_VERSION_MIN_REQUIRED % 10;
#else
        const unsigned major = MAC_OS_X_VERSION_MIN_REQUIRED / 10000;
        const unsigned minor = (MAC_OS_X_VERSION_MIN_REQUIRED % 10000) / 100;
        const unsigned patch = MAC_OS_X_VERSION_MIN_REQUIRED % 100;
#endif
#else
        //#define BOOST_VERSION_NUMBER(major,minor,patch) ( (((major)%100)*10000000) + (((minor)%100)*100000) + ((patch)%100000) )
        const bool version_available = BOOST_OS != BOOST_VERSION_NUMBER_NOT_AVAILABLE;
        const unsigned major = BOOST_OS/10000000;
        const unsigned minor = (BOOST_OS-10000000*major)/100000;
        const unsigned patch = BOOST_OS % 100000;
#endif
        // coverity[unsigned_compare]
        if (version_available && major > 0) {
            osVer += QString::fromUtf8(" %1").arg(major);
            // coverity[unsigned_compare]
            if (minor > 0 || patch > 0) {
                osVer += QString::fromUtf8(".%1").arg(minor);
                if (patch > 0) {
                    osVer += QString::fromUtf8(".%1").arg(patch);
                }
            }
        }
        QString gitStr = QString::fromUtf8("<p>%1</p>").arg(tr("This software was compiled from the source "
                                                               "code branch %1 at version %2 using %3 targeting %4 for %5.")
                                                            .arg( QString::fromUtf8("<a href=\"https://github.com/NatronGitHub/Natron/tree/" GIT_BRANCH "\">" GIT_BRANCH "</a>") ) // %1
                                                            .arg(argStr) // %2
                                                            .arg( QString::fromUtf8(BOOST_COMPILER OPENMP_STRING) ) // %3
                                                            .arg( QString::fromUtf8(BOOST_ARCH_NAME) ) // %4
                                                            .arg(osVer) );// %5

        aboutText.append(gitStr);
    }
    if ( !std::string(IO_GIT_COMMIT).empty() ) {
        QString argStr = ( QString::fromUtf8("<a href=\"https://github.com/NatronGitHub/openfx-io/tree/" IO_GIT_COMMIT "\">")
                           + QString::fromUtf8(IO_GIT_COMMIT).mid(0, 7)
                           + QString::fromUtf8("</a>") );
        QString gitStr = QString::fromUtf8("<p>%1</p>").arg(tr("The bundled %1 "
                                                               "plugins were compiled from version %2.")
                                                            .arg(QString::fromUtf8("<a href=\"https://github.com/NatronGitHub/openfx-io\">openfx-io</a>")) //%1
                                                            .arg(argStr));  // %2
        aboutText.append(gitStr);
    }
    if ( !std::string(MISC_GIT_COMMIT).empty() ) {
        QString argStr = ( QString::fromUtf8("<a href=\"https://github.com/NatronGitHub/openfx-misc/tree/" MISC_GIT_COMMIT "\">")
                           + QString::fromUtf8(MISC_GIT_COMMIT).mid(0, 7)
                           + QString::fromUtf8("</a>") );
        QString gitStr = QString::fromUtf8("<p>%1</p>").arg(tr("The bundled %1 "
                                                               "plugins were compiled from version %2.")
                                                            .arg(QString::fromUtf8("<a href=\"https://github.com/NatronGitHub/openfx-misc\">openfx-misc</a>")) //%1
                                                            .arg(argStr));  // %2
        aboutText.append(gitStr);
    }
    if ( !std::string(ARENA_GIT_COMMIT).empty() ) {
        QString argStr = ( QString::fromUtf8("<a href=\"https://github.com/NatronGitHub/openfx-arena/tree/" ARENA_GIT_COMMIT "\">")
                           + QString::fromUtf8(ARENA_GIT_COMMIT).mid(0, 7)
                          + QString::fromUtf8("</a>") );
        QString gitStr = QString::fromUtf8("<p>%1</p>").arg(tr("The bundled %1 "
                                                               "plugins were compiled from version %2.")
                                                            .arg(QString::fromUtf8("<a href=\"https://github.com/NatronGitHub/openfx-arena\">openfx-arena</a>")) //%1
                                                            .arg(argStr));  // %2
        aboutText.append(gitStr);
    }
    if ( !std::string(GMIC_GIT_COMMIT).empty() ) {
        QString argStr = ( QString::fromUtf8("<a href=\"https://github.com/NatronGitHub/openfx-gmic/tree/" GMIC_GIT_COMMIT "\">")
                           + QString::fromUtf8(GMIC_GIT_COMMIT).mid(0, 7)
                          + QString::fromUtf8("</a>") );
        QString gitStr = QString::fromUtf8("<p>%1</p>").arg(tr("The bundled %1 "
                                                               "plugins were compiled from version %2.")
                                                            .arg(QString::fromUtf8("<a href=\"https://github.com/NatronGitHub/openfx-gmic\">openfx-gmic</a>")) //%1
                                                            .arg(argStr));  // %2
        aboutText.append(gitStr);
    }
    const QString status( QString::fromUtf8(NATRON_DEVELOPMENT_STATUS) );
    if ( status == QString::fromUtf8(NATRON_DEVELOPMENT_DEVEL) ) {
        QString toAppend = QString::fromUtf8("<p>%1</p>").arg(tr("Note: This is a development version, which probably contains bugs. "
                                                                 "If you feel like reporting a bug, please do so "
                                                                 "on the %1.")
                                                              .arg( QString::fromUtf8("<a href=\"%1\">%2</a>").arg(QString::fromUtf8(NATRON_ISSUE_TRACKER_URL)).arg(tr("issue tracker")) )); // %1
        aboutText.append(toAppend);
    } else if ( status == QString::fromUtf8(NATRON_DEVELOPMENT_SNAPSHOT) ) {
        QString toAppend = QString::fromUtf8("<p>%1</p>").arg(tr("Note: This is an official snapshot version, compiled on the Natron build "
                                                                 "farm, and it may still contain bugs. If you feel like reporting a bug, please do so "
                                                                 "on the %1.")
                                                              .arg( QString::fromUtf8("<a href=\"%1\">%2</a>").arg(QString::fromUtf8(NATRON_ISSUE_TRACKER_URL)).arg(tr("issue tracker")) )); // %1
        aboutText.append(toAppend);
    } else if ( status == QString::fromUtf8(NATRON_DEVELOPMENT_ALPHA) ) {
        QString toAppend = QString::fromUtf8("<p>%1</p>").arg(tr("Note: This software is currently in alpha version, meaning there are missing features,"
                                                                 " bugs and untested stuffs. If you feel like reporting a bug, please do so "
                                                                 "on the %1.")
                                                              .arg( QString::fromUtf8("<a href=\"%1\">%2</a>").arg(QString::fromUtf8(NATRON_ISSUE_TRACKER_URL)).arg(tr("issue tracker")) )); // %1
        aboutText.append(toAppend);
    } else if ( status == QString::fromUtf8(NATRON_DEVELOPMENT_BETA) ) {
        QString toAppend = QString::fromUtf8("<p>%1</p>").arg(tr("Note: This software is currently under beta testing, meaning there are "
                                                                 " bugs and untested stuffs. If you feel like reporting a bug, please do so "
                                                                 "on the %1.")
                                                              .arg( QString::fromUtf8("<a href=\"%1\">%2</a>").arg(QString::fromUtf8(NATRON_ISSUE_TRACKER_URL)).arg(tr("issue tracker")) )); // %1
        aboutText.append(toAppend);
    } else if ( status == QString::fromUtf8(NATRON_DEVELOPMENT_RELEASE_CANDIDATE) ) {
        QString toAppend = tr("The version of this software is a release candidate, which means it has the potential of becoming "
                                             "the future stable release but might still have some bugs.");
        aboutText.append(toAppend);
    }

    _aboutText->setText(aboutText);
    _tabWidget->addTab( _aboutText, tr("About") );

    _changelogText =  new QTextBrowser(_tabWidget);
    _changelogText->setOpenExternalLinks(true);
    {
        QFile changelogFile( QString::fromUtf8(":CHANGELOG.md") );
        changelogFile.open(QIODevice::ReadOnly | QIODevice::Text);
        _changelogText->setText( QTextCodec::codecForName("UTF-8")->toUnicode( changelogFile.readAll() ) );
    }
    _tabWidget->addTab( _changelogText, tr("Changelog") );

    _libsText = new QTextBrowser(_tabWidget);
    _libsText->setOpenExternalLinks(true);
    updateLibrariesVersions();

    _tabWidget->addTab( _libsText, tr("Libraries") );

    _teamText = new QTextBrowser(_tabWidget);
    _teamText->setOpenExternalLinks(false);
    {
        QFile team_file( QString::fromUtf8(":CONTRIBUTORS.txt") );
        team_file.open(QIODevice::ReadOnly | QIODevice::Text);
        _teamText->setText( QTextCodec::codecForName("UTF-8")->toUnicode( team_file.readAll() ) );
    }
    _tabWidget->addTab( _teamText, tr("Contributors") );

    _licenseText = new QTextBrowser(_tabWidget);
    _licenseText->setOpenExternalLinks(false);
    {
        QFile license( QString::fromUtf8(":LICENSE.txt") );
        license.open(QIODevice::ReadOnly | QIODevice::Text);
        _licenseText->setText( QTextCodec::codecForName("UTF-8")->toUnicode( license.readAll() ) );
    }
    _tabWidget->addTab( _licenseText, tr("License") );

    QWidget* thirdPartyContainer = new QWidget(_tabWidget);
    QVBoxLayout* thidPartyLayout = new QVBoxLayout(thirdPartyContainer);
    thidPartyLayout->setContentsMargins(0, 0, 0, 0);
    QSplitter *splitter = new QSplitter();

    _view = new TableView(thirdPartyContainer);
    _model = new TableModel(0, 0, _view);
    _view->setTableModel(_model);

    QItemSelectionModel *selectionModel = _view->selectionModel();
    _view->setColumnCount(1);

    _view->setAttribute(Qt::WA_MacShowFocusRect, 0);
    _view->setUniformRowHeights(true);
    _view->setSelectionMode(QAbstractItemView::SingleSelection);
    _view->header()->close();
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    _view->header()->setResizeMode(QHeaderView::ResizeToContents);
#else
    _view->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
#endif
    _view->header()->setStretchLastSection(true);
    splitter->addWidget(_view);

    _thirdPartyBrowser = new QTextBrowser(thirdPartyContainer);
    _thirdPartyBrowser->setOpenExternalLinks(false);
    splitter->addWidget(_thirdPartyBrowser);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 4);
    thidPartyLayout->addWidget(splitter);

    QString thirdPartyLicenseDir = QString::fromUtf8(THIRD_PARTY_LICENSE_DIR_PATH);
    QDir thirdPartyDir(thirdPartyLicenseDir);
    QStringList rowsTmp;
    {
        QStringList rows = thirdPartyDir.entryList(QDir::NoDotAndDotDot | QDir::Files);
        for (int i = 0; i < rows.size(); ++i) {
            if ( !rows[i].startsWith( QString::fromUtf8("LICENSE-") ) ) {
                continue;
            }
            if ( rows[i] == QString::fromUtf8("LICENSE-README.md") ) {
                rowsTmp.prepend(rows[i]);
            } else {
                rowsTmp.push_back(rows[i]);
            }
        }
    }
    _view->setRowCount( rowsTmp.size() );

    TableItem* readmeIndex = 0;
    for (int i = 0; i < rowsTmp.size(); ++i) {
        if ( !rowsTmp[i].startsWith( QString::fromUtf8("LICENSE-") ) ) {
            continue;
        }
        TableItem* item = new TableItem;
        item->setText( rowsTmp[i].remove( QString::fromUtf8("LICENSE-") ).remove( QString::fromUtf8(".txt") ).remove( QString::fromUtf8(".md") ) );
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        _view->setItem(i, 0, item);
        if ( rowsTmp[i] == QString::fromUtf8("README") ) {
            readmeIndex = item;
        }
    }

    QObject::connect( selectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this,
                      SLOT(onSelectionChanged(QItemSelection,QItemSelection)) );
    if (readmeIndex) {
        readmeIndex->setSelected(true);
    }
    _tabWidget->addTab( thirdPartyContainer, QString::fromUtf8("Third-Party components") );
    resize(560 * _scale, 460 * _scale);
}

void
AboutWindow::onSelectionChanged(const QItemSelection & newSelection,
                                const QItemSelection & /*oldSelection*/)
{
    QModelIndexList indexes = newSelection.indexes();

    assert(indexes.size() <= 1);
    if ( indexes.empty() ) {
        _thirdPartyBrowser->clear();
    } else {
        TableItem* item = _view->item(indexes.front().row(), 0);
        assert(item);
        if (!item) {
            return;
        }
        QString fileName = QString::fromUtf8(THIRD_PARTY_LICENSE_DIR_PATH);
        fileName += QChar::fromLatin1('/');
        fileName += QString::fromUtf8("LICENSE-");
        fileName += item->text();
        fileName += ( item->text() == QString::fromUtf8("README") ) ? QString::fromUtf8(".md") : QString::fromUtf8(".txt");
        QFile file(fileName);
        if ( file.open(QIODevice::ReadOnly | QIODevice::Text) ) {
            QString content = QTextCodec::codecForName("UTF-8")->toUnicode( file.readAll() );
            _thirdPartyBrowser->setText(content);
        }
    }
}

void
AboutWindow::updateLibrariesVersions()
{
    QString libsText = QString::fromUtf8("<p>Python %1</p>"
                                         "<p>Qt %2</p>"
                                         "<p>Boost %3</p>"
                                         "<p>Cairo %4</p>"
                                         "<p>Hoedown %5</p>"
                                         "<p>Ceres %6</p>"
                                         "<p>OpenMVG %7</p>"
                                         )
                       .arg( QString::fromUtf8(PY_VERSION) )
                       .arg( appPTR->getQtVersion() )
                       .arg( appPTR->getBoostVersion() )
                       .arg( appPTR->getCairoVersion() )
                       .arg( appPTR->getHoedownVersion() )
                       .arg( appPTR->getCeresVersion() )
                       .arg( appPTR->getOpenMVGVersion() );
    std::list<OpenGLRendererInfo> openGLRenderers;
    OSGLContext::getGPUInfos(openGLRenderers);
    if ( !openGLRenderers.empty() ) {
        libsText += (QLatin1String("<h3>") +
                     tr("OpenGL renderers:") +
                     QLatin1String("</h3>"));
        for (std::list<OpenGLRendererInfo>::iterator it = openGLRenderers.begin(); it != openGLRenderers.end(); ++it) {
            libsText += (QLatin1String("<p>") +
                         tr("%1 from %2").arg( QString::fromUtf8( it->rendererName.c_str() ) )
                                        .arg( QString::fromUtf8( it->vendorName.c_str() ) ) +
                         QLatin1String("<br />") +
                         tr("OpenGL version %1 with GLSL version %2")
                         .arg( QString::fromUtf8( it->glVersionString.c_str() ) )
                         .arg( QString::fromUtf8( it->glslVersionString.c_str() ) ) +
                         QLatin1String("</p>") );
        }
    }

    _libsText->setText(libsText);
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_AboutWindow.cpp"
