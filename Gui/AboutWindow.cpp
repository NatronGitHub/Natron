//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "AboutWindow.h"

#include <QTextBrowser>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QLabel>

#include "Global/GlobalDefines.h"
#include "Gui/Button.h"

AboutWindow::AboutWindow(QWidget* parent)
: QDialog(parent)
{
    
    setWindowTitle("About " NATRON_APPLICATION_NAME);
    _mainLayout = new QVBoxLayout(this);
    setLayout(_mainLayout);
    
    _iconLabel = new QLabel(this);
    _iconLabel->setPixmap(QPixmap(NATRON_APPLICATION_ICON_PATH));
    _mainLayout->addWidget(_iconLabel);
    
    _tabWidget = new QTabWidget(this);
    _mainLayout->addWidget(_tabWidget);
    
    _buttonContainer = new QWidget(this);
    _buttonLayout = new QHBoxLayout(_buttonContainer);
    _buttonLayout->addStretch();
    
    _closeButton = new Button("Close",_buttonContainer);
    QObject::connect(_closeButton, SIGNAL(clicked()), this, SLOT(accept()));
    _buttonLayout->addWidget(_closeButton);
    
    _mainLayout->addWidget(_buttonContainer);
    
    ///filling tabs now
    
    _aboutText = new QTextBrowser(_tabWidget);
    _aboutText->setOpenExternalLinks(true);
    QString aboutText = QString("<p>%1 version %2</p>"
                                "<p>Copyright (C) 2013 the %1 developers.</p>"
                                "<p>This is free software. You may redistribute copies of it "
                                "under the terms of the <a href=\"http://www.mozilla.org/MPL/2.0/\">"
                                "<font color=\"orange\">MPL Mozilla Public License</font></a>."
                                "There is NO WARRANTY, to the extent permitted by law.</p>"
                                "<p>See <a href=\"%3\"><font color=\"orange\">%1 's website</font></a>"
                                "for more information on this software.</p>")
                                .arg(NATRON_APPLICATION_NAME)
                                .arg(NATRON_VERSION_STRING)
                                .arg("https://natron.inria.fr");
    _aboutText->setText(aboutText);
    _tabWidget->addTab(_aboutText, "About");
    
//    _libsText = new QTextBrowser(_tabWidget);
//    _libsText->setOpenExternalLinks(true);
//    QString libsText = QString("");
//    _libsText->setText(libsText);
//    _tabWidget->addTab(_libsText, "Libraries");
    
    _teamText = new QTextBrowser(_tabWidget);
    _teamText->setOpenExternalLinks(false);
    QFile team_file(":CONTRIBUTORS.txt");
    team_file.open(QIODevice::ReadOnly | QIODevice::Text);
    _teamText->setText(QTextCodec::codecForName("UTF-8")->toUnicode(team_file.readAll()));
    _tabWidget->addTab(_teamText, "Contributors");
    
    _licenseText = new QTextBrowser(_tabWidget);
    _licenseText->setOpenExternalLinks(false);
    QFile license(":LICENSE.txt");
    license.open(QIODevice::ReadOnly | QIODevice::Text);
    _licenseText->setText(QTextCodec::codecForName("UTF-8")->toUnicode(license.readAll()));
    _tabWidget->addTab(_licenseText, "License");
    
}
