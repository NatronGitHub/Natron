//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "AboutWindow.h"

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QTextBrowser>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QFile>
#include <QTextCodec>
CLANG_DIAG_ON(deprecated)

#include "Global/GlobalDefines.h"
#include "Global/GitVersion.h"
#include "Gui/Button.h"
#include "Gui/Gui.h"
#include "Gui/Label.h"

AboutWindow::AboutWindow(Gui* gui,
                         QWidget* parent)
    : QDialog(parent)
      , _gui(gui)
{
    setWindowTitle(QObject::tr("About ") + NATRON_APPLICATION_NAME);
    _mainLayout = new QVBoxLayout(this);
    setLayout(_mainLayout);

    _iconLabel = new Natron::Label(this);
    _iconLabel->setPixmap( QPixmap(NATRON_APPLICATION_ICON_PATH).scaled(128, 128) );
    _mainLayout->addWidget(_iconLabel);

    _tabWidget = new QTabWidget(this);
    _mainLayout->addWidget(_tabWidget);

    _buttonContainer = new QWidget(this);
    _buttonLayout = new QHBoxLayout(_buttonContainer);
    _buttonLayout->addStretch();

    _closeButton = new Button(QObject::tr("Close"),_buttonContainer);
    QObject::connect( _closeButton, SIGNAL( clicked() ), this, SLOT( accept() ) );
    _buttonLayout->addWidget(_closeButton);

    _mainLayout->addWidget(_buttonContainer);

    ///filling tabs now

    _aboutText = new QTextBrowser(_tabWidget);
    _aboutText->setOpenExternalLinks(true);
    QString aboutText;
    QString customBuild(NATRON_CUSTOM_BUILD_USER_NAME);
    if (!customBuild.isEmpty()) {
        aboutText = QString("<p>%1 custom build for %2 %3.</p>")
        .arg(NATRON_APPLICATION_NAME)
        .arg(customBuild)
#ifdef DEBUG
        .arg("(debug)");
#else
#ifdef NDEBUG
        // release with asserts disabled (should be the *real* release)
        .arg("");
#else
        // release with asserts enabled
        .arg("(opt)");
#endif
#endif
    } else {
        aboutText = QString("<p>%1 version %2 %3%4.</p>")
        .arg(NATRON_APPLICATION_NAME)
        .arg(NATRON_VERSION_STRING)
        .arg(NATRON_DEVELOPMENT_STATUS)
#ifdef DEBUG
        .arg(" (debug)");
#else
#ifdef NDEBUG
        // release with asserts disabled (should be the *real* release)
        .arg("");
#else
        // release with asserts enabled
        .arg(" (opt)");
#endif
#endif
    }
    QString endAbout =
    QString("<p>This version was generated from the source code branch %3"
            " at commit %4.</p>"
            "<p>Copyright (C) 2015 the %1 developers.</p>"
            "<p>This is free software. You may redistribute copies of it "
            "under the terms of the <a href=\"http://www.mozilla.org/MPL/2.0/\">"
            "<font color=\"orange\">MPL Mozilla Public License</font></a>. "
            "There is NO WARRANTY, to the extent permitted by law.</p>"
            "<p>See <a href=\"%2\"><font color=\"orange\">%1 's website </font></a>"
            "for more information on this software.</p>")
    .arg(NATRON_APPLICATION_NAME) // %1
    .arg("https://natron.inria.fr") // %2
    .arg(GIT_BRANCH) // %3
    .arg(GIT_COMMIT); // %4
    aboutText.append(endAbout);

    const QString status(NATRON_DEVELOPMENT_STATUS);
    if (status == NATRON_DEVELOPMENT_DEVEL) {
        QString toAppend = QString("<p>Note: This is a development version of Natron, which probably contains bugs. "
                                   "If you feel like reporting a bug, please do so "
                                   "on the <a href=\"%1\"><font color=\"orange\"> issue tracker.</font></a></p>")
        .arg("https://github.com/MrKepzie/Natron/issues"); // %1
        ;
    } else if (status == NATRON_DEVELOPMENT_SNAPSHOT) {
        QString toAppend = QString("<p>Note: This is an official snapshot version, compiled on the Natron build "
                                   "farm, and it may still contain bugs. If you feel like reporting a bug, please do so "
                                   "on the <a href=\"%1\"><font color=\"orange\"> issue tracker.</font></a></p>")
        .arg("https://github.com/MrKepzie/Natron/issues"); // %1
        ;
    } else if (status == NATRON_DEVELOPMENT_ALPHA) {
        QString toAppend = QString("<p>Note: This software is currently in alpha version, meaning there are missing features,"
                                   " bugs and untested stuffs. If you feel like reporting a bug, please do so "
                                   "on the <a href=\"%1\"><font color=\"orange\"> issue tracker.</font></a></p>")
        .arg("https://github.com/MrKepzie/Natron/issues"); // %1
        ;
    } else if (status == NATRON_DEVELOPMENT_BETA) {
        QString toAppend = QString("<p>Note: This software is currently under beta testing, meaning there are "
                                   " bugs and untested stuffs. If you feel like reporting a bug, please do so "
                                   "on the <a href=\"%1\"><font color=\"orange\"> issue tracker.</font></a></p>")
                           .arg("https://github.com/MrKepzie/Natron/issues"); // %1
        ;

    } else if (status == NATRON_DEVELOPMENT_RELEASE_CANDIDATE) {
        QString toAppend = QString("The version of this sofware is a release candidate, which means it has the potential of becoming "
                                   "the future stable release but might still have some bugs.");
        aboutText.append(toAppend);
    }
    
    _aboutText->setText(aboutText);
    _tabWidget->addTab( _aboutText, QObject::tr("About") );
    
    _changelogText =  new QTextBrowser(_tabWidget);
    _changelogText->setOpenExternalLinks(true);
    {
        QFile changelogFile(":CHANGELOG.md");
        changelogFile.open(QIODevice::ReadOnly | QIODevice::Text);
        _changelogText->setText(QTextCodec::codecForName("UTF-8")->toUnicode( changelogFile.readAll() ) );
    }
    _tabWidget->addTab(_changelogText, QObject::tr("Changelog"));

    _libsText = new QTextBrowser(_tabWidget);
    _libsText->setOpenExternalLinks(true);
    updateLibrariesVersions();
    
    _tabWidget->addTab( _libsText, QObject::tr("Libraries") );

    _teamText = new QTextBrowser(_tabWidget);
    _teamText->setOpenExternalLinks(false);
    {
        QFile team_file(":CONTRIBUTORS.txt");
        team_file.open(QIODevice::ReadOnly | QIODevice::Text);
        _teamText->setText( QTextCodec::codecForName("UTF-8")->toUnicode( team_file.readAll() ) );
    }
    _tabWidget->addTab( _teamText, QObject::tr("Contributors") );

    _licenseText = new QTextBrowser(_tabWidget);
    _licenseText->setOpenExternalLinks(false);
    {
        QFile license(":LICENSE.txt");
        license.open(QIODevice::ReadOnly | QIODevice::Text);
        _licenseText->setText( QTextCodec::codecForName("UTF-8")->toUnicode( license.readAll() ) );
    }
    _tabWidget->addTab( _licenseText, QObject::tr("License") );

}

void
AboutWindow::updateLibrariesVersions()
{
    QString libsText = QString("<p> Python %1 </p>"
                               "<p> Qt %2 </p>"
                               "<p> Boost %3 </p>"
                               "<p> Glew %4 </p>"
                               "<p> OpenGL %5 </p>"
                               "<p> Cairo %6 </p>")
    .arg(PY_VERSION)
    .arg( _gui->getQtVersion() )
    .arg( _gui->getBoostVersion() )
    .arg( _gui->getGlewVersion() )
    .arg( _gui->getOpenGLVersion() )
    .arg( _gui->getCairoVersion() );
    
    _libsText->setText(libsText);
}

