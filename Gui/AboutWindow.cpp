//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "AboutWindow.h"

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QTextBrowser>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QLabel>
#include <QFile>
#include <QTextCodec>
CLANG_DIAG_ON(deprecated)

#include "Global/GlobalDefines.h"
#include "Global/GitVersion.h"
#include "Gui/Button.h"
#include "Gui/Gui.h"

AboutWindow::AboutWindow(Gui* gui,QWidget* parent)
: QDialog(parent)
, _gui(gui)
{
    
    setWindowTitle(QObject::tr("About ") + NATRON_APPLICATION_NAME);
    _mainLayout = new QVBoxLayout(this);
    setLayout(_mainLayout);
    
    _iconLabel = new QLabel(this);
    _iconLabel->setPixmap(QPixmap(NATRON_APPLICATION_ICON_PATH).scaled(128, 128));
    _mainLayout->addWidget(_iconLabel);
    
    _tabWidget = new QTabWidget(this);
    _mainLayout->addWidget(_tabWidget);
    
    _buttonContainer = new QWidget(this);
    _buttonLayout = new QHBoxLayout(_buttonContainer);
    _buttonLayout->addStretch();
    
    _closeButton = new Button(QObject::tr("Close"),_buttonContainer);
    QObject::connect(_closeButton, SIGNAL(clicked()), this, SLOT(accept()));
    _buttonLayout->addWidget(_closeButton);
    
    _mainLayout->addWidget(_buttonContainer);
    
    ///filling tabs now
    
    _aboutText = new QTextBrowser(_tabWidget);
    _aboutText->setOpenExternalLinks(true);
    QString aboutText = QString("<p>%1 version %2%7.</p>"
                                "<p>This version was generated from the source code branch %5"
                                " at commit %6.</p>"
                                "<p>Copyright (C) 2013 the %1 developers.</p>"
                                "<p>This is free software. You may redistribute copies of it "
                                "under the terms of the <a href=\"http://www.mozilla.org/MPL/2.0/\">"
                                "<font color=\"orange\">MPL Mozilla Public License</font></a>. "
                                "There is NO WARRANTY, to the extent permitted by law.</p>"
                                "<p>See <a href=\"%3\"><font color=\"orange\">%1 's website </font></a>"
                                "for more information on this software.</p>"
                                "<p>Note: This software is currently under beta testing, meaning there are "
                                " bugs and untested stuffs. If you feel like reporting a bug, please do so "
                                "on the <a href=\"%4\"><font color=\"orange\"> issue tracker.</font></a></p>")
                                .arg(NATRON_APPLICATION_NAME) // %1
                                .arg(NATRON_VERSION_STRING) // %2
                                .arg("https://natron.inria.fr") // %3
                                .arg("https://groups.google.com/forum/?hl=en#!categories/natron-vfx/installation-troobleshooting-bugs") // %4
                                .arg(GIT_BRANCH) // %5
                                .arg(GIT_COMMIT) // %6
#ifdef DEBUG
                                .arg(" (debug)") //%7
#else
#ifdef NDEBUG
                                // release with asserts disabled (should be the *real* release)
                                .arg("") // %7
#else
                                // release with asserts enabled
                                .arg(" (opt)") // %7
#endif
#endif
                                ;
    _aboutText->setText(aboutText);
    _tabWidget->addTab(_aboutText, QObject::tr("About"));
    
    _libsText = new QTextBrowser(_tabWidget);
    _libsText->setOpenExternalLinks(true);
    QString libsText = QString("<p> Qt %1 </p>"
                               "<p> Boost %2 </p>"
                               "<p> GLEW %3 </p>"
                               "<p> OpenGL %4 </p>"
                               "<p> Cairo %5 </p>")
    .arg(gui->getQtVersion())
    .arg(gui->getBoostVersion())
    .arg(gui->getGlewVersion())
    .arg(gui->getOpenGLVersion())
    .arg(gui->getCairoVersion());
    _libsText->setText(libsText);
    _tabWidget->addTab(_libsText, QObject::tr("Libraries"));
    
    _teamText = new QTextBrowser(_tabWidget);
    _teamText->setOpenExternalLinks(false);
    {
        QFile team_file(":CONTRIBUTORS.txt");
        team_file.open(QIODevice::ReadOnly | QIODevice::Text);
        _teamText->setText(QTextCodec::codecForName("UTF-8")->toUnicode(team_file.readAll()));
        _tabWidget->addTab(_teamText, QObject::tr("Contributors"));

        _licenseText = new QTextBrowser(_tabWidget);
        _licenseText->setOpenExternalLinks(false);
    }
    {
        QFile license(":LICENSE.txt");
        license.open(QIODevice::ReadOnly | QIODevice::Text);
        _licenseText->setText(QTextCodec::codecForName("UTF-8")->toUnicode(license.readAll()));
        _tabWidget->addTab(_licenseText, QObject::tr("License"));
    }
    
}



void AboutWindow::updateLibrariesVersions() {
    QString libsText = QString("<p> Qt %1 </p>"
                               "<p> Boost %2 </p>"
                               "<p> Glew %3 </p>"
                               "<p> OpenGL %4 </p>"
                               "<p> Cairo %5 </p>")
    .arg(_gui->getQtVersion())
    .arg(_gui->getBoostVersion())
    .arg(_gui->getGlewVersion())
    .arg(_gui->getOpenGLVersion())
    .arg(_gui->getCairoVersion());
    _libsText->setText(libsText);

}
