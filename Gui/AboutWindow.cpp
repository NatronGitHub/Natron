/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QSplitter>
#include <QTextBrowser>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QFile>
#include <QTextCodec>
#include <QItemSelectionModel>
#include <QHeaderView>
#include <QDir>
CLANG_DIAG_ON(deprecated)

#include "Global/GlobalDefines.h"
#include "Global/GitVersion.h"
#include "Gui/Button.h"
#include "Gui/Gui.h"
#include "Gui/Label.h"
#include "Gui/Utils.h"
#include "Gui/TableModelView.h"

#define THIRD_PARTY_LICENSE_DIR_PATH ":"

NATRON_NAMESPACE_ENTER;

AboutWindow::AboutWindow(Gui* gui,
                         QWidget* parent)
    : QDialog(parent)
    , _gui(gui)
{
    setWindowTitle( QObject::tr("About ") + QString::fromUtf8(NATRON_APPLICATION_NAME) );
    _mainLayout = new QVBoxLayout(this);
    setLayout(_mainLayout);

    _iconLabel = new Label(this);
    _iconLabel->setPixmap( QPixmap( QString::fromUtf8(NATRON_APPLICATION_ICON_PATH) ).scaled(128, 128) );
    _mainLayout->addWidget(_iconLabel);

    _tabWidget = new QTabWidget(this);
    _mainLayout->addWidget(_tabWidget);

    _buttonContainer = new QWidget(this);
    _buttonLayout = new QHBoxLayout(_buttonContainer);
    _buttonLayout->addStretch();

    _closeButton = new Button(QObject::tr("Close"), _buttonContainer);
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
        licenseStr = GuiUtils::convertFromPlainText(QTextCodec::codecForName("UTF-8")->toUnicode( license.readAll() ), Qt::WhiteSpaceNormal);
        aboutText.append(licenseStr);
    }
    {
        QString endAbout = ( QString::fromUtf8("<p>See the <a href=\"%2\">%1 website</a> "
                                               "for more information on this software.</p>")
                             .arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) // %1
                             .arg( QString::fromUtf8(NATRON_WEBSITE_URL) ) ); // %2
        aboutText.append(endAbout);
    }
    {
        QString argStr = ( QString::fromUtf8("<a href=\"https://github.com/MrKepzie/Natron/tree/"GIT_COMMIT "\">")
                           + QString::fromUtf8(GIT_COMMIT).mid(0, 7)
                           + QString::fromUtf8("</a>") );
        QString gitStr = ( QString::fromUtf8("<p>This software was compiled on %3 from the source "
                                             "code branch %1 at version %2.</p>")
                           .arg( QString::fromUtf8("<a href=\"https://github.com/MrKepzie/Natron/tree/"GIT_BRANCH "\">"GIT_BRANCH "</a>") ) // %1
                           .arg(argStr) // %2
                           .arg( QString::fromUtf8(__DATE__) ) ); // %3
        aboutText.append(gitStr);
    }
    if ( !std::string(IO_GIT_COMMIT).empty() ) {
        QString argStr = ( QString::fromUtf8("<a href=\"https://github.com/MrKepzie/openfx-io/tree/"IO_GIT_COMMIT "\">")
                           + QString::fromUtf8(IO_GIT_COMMIT).mid(0, 7)
                           + QString::fromUtf8("</a>") );
        QString gitStr = QString::fromUtf8("<p>The bundled <a href=\"https://github.com/MrKepzie/openfx-io\">openfx-io</a> "
                                           "plugins were compiled from version %1.</p>")
                         .arg(argStr);  // %1
        aboutText.append(gitStr);
    }
    if ( !std::string(MISC_GIT_COMMIT).empty() ) {
        QString argStr = ( QString::fromUtf8("<a href=\"https://github.com/devernay/openfx-misc/tree/"MISC_GIT_COMMIT "\">")
                           + QString::fromUtf8(MISC_GIT_COMMIT).mid(0, 7)
                           + QString::fromUtf8("</a>") );
        QString gitStr = QString::fromUtf8("<p>The bundled <a href=\"https://github.com/devernay/openfx-misc\">openfx-misc</a> "
                                           "plugins were compiled from version %1.</p>")
                         .arg(argStr);  // %1
        aboutText.append(gitStr);
    }
    if ( !std::string(ARENA_GIT_COMMIT).empty() ) {
        QString argStr = ( QString::fromUtf8("<a href=\"https://github.com/olear/openfx-arena/tree/"ARENA_GIT_COMMIT "\">")
                           + QString::fromUtf8(ARENA_GIT_COMMIT).mid(0, 7)
                           + QString::fromUtf8("</a>") );
        QString gitStr = QString::fromUtf8("<p>The bundled <a href=\"https://github.com/olear/openfx-arena\">openfx-arena</a> "
                                           "plugins were compiled from version %1.</p>")
                         .arg(argStr);  // %1
        aboutText.append(gitStr);
    }
    const QString status( QString::fromUtf8(NATRON_DEVELOPMENT_STATUS) );
    if ( status == QString::fromUtf8(NATRON_DEVELOPMENT_DEVEL) ) {
        QString toAppend = QString::fromUtf8("<p>Note: This is a development version, which probably contains bugs. "
                                             "If you feel like reporting a bug, please do so "
                                             "on the <a href=\"%1\">issue tracker</a>.</p>")
                           .arg( QString::fromUtf8(NATRON_ISSUE_TRACKER_URL) ); // %1
        aboutText.append(toAppend);
    } else if ( status == QString::fromUtf8(NATRON_DEVELOPMENT_SNAPSHOT) ) {
        QString toAppend = QString::fromUtf8("<p>Note: This is an official snapshot version, compiled on the Natron build "
                                             "farm, and it may still contain bugs. If you feel like reporting a bug, please do so "
                                             "on the <a href=\"%1\">issue tracker</a>.</p>")
                           .arg( QString::fromUtf8(NATRON_ISSUE_TRACKER_URL) ); // %1
        aboutText.append(toAppend);
    } else if ( status == QString::fromUtf8(NATRON_DEVELOPMENT_ALPHA) ) {
        QString toAppend = QString::fromUtf8("<p>Note: This software is currently in alpha version, meaning there are missing features,"
                                             " bugs and untested stuffs. If you feel like reporting a bug, please do so "
                                             "on the <a href=\"%1\">issue tracker</a>.</p>")
                           .arg( QString::fromUtf8(NATRON_ISSUE_TRACKER_URL) ); // %1
        aboutText.append(toAppend);
    } else if ( status == QString::fromUtf8(NATRON_DEVELOPMENT_BETA) ) {
        QString toAppend = QString::fromUtf8("<p>Note: This software is currently under beta testing, meaning there are "
                                             " bugs and untested stuffs. If you feel like reporting a bug, please do so "
                                             "on the <a href=\"%1\">issue tracker</a>.</p>")
                           .arg( QString::fromUtf8(NATRON_ISSUE_TRACKER_URL) ); // %1
        aboutText.append(toAppend);
    } else if ( status == QString::fromUtf8(NATRON_DEVELOPMENT_RELEASE_CANDIDATE) ) {
        QString toAppend = QString::fromUtf8("The version of this sofware is a release candidate, which means it has the potential of becoming "
                                             "the future stable release but might still have some bugs.");
        aboutText.append(toAppend);
    }

    _aboutText->setText(aboutText);
    _tabWidget->addTab( _aboutText, QObject::tr("About") );

    _changelogText =  new QTextBrowser(_tabWidget);
    _changelogText->setOpenExternalLinks(true);
    {
        QFile changelogFile( QString::fromUtf8(":CHANGELOG.md") );
        changelogFile.open(QIODevice::ReadOnly | QIODevice::Text);
        _changelogText->setText( QTextCodec::codecForName("UTF-8")->toUnicode( changelogFile.readAll() ) );
    }
    _tabWidget->addTab( _changelogText, QObject::tr("Changelog") );

    _libsText = new QTextBrowser(_tabWidget);
    _libsText->setOpenExternalLinks(true);
    updateLibrariesVersions();

    _tabWidget->addTab( _libsText, QObject::tr("Libraries") );

    _teamText = new QTextBrowser(_tabWidget);
    _teamText->setOpenExternalLinks(false);
    {
        QFile team_file( QString::fromUtf8(":CONTRIBUTORS.txt") );
        team_file.open(QIODevice::ReadOnly | QIODevice::Text);
        _teamText->setText( QTextCodec::codecForName("UTF-8")->toUnicode( team_file.readAll() ) );
    }
    _tabWidget->addTab( _teamText, QObject::tr("Contributors") );

    _licenseText = new QTextBrowser(_tabWidget);
    _licenseText->setOpenExternalLinks(false);
    {
        QFile license( QString::fromUtf8(":LICENSE.txt") );
        license.open(QIODevice::ReadOnly | QIODevice::Text);
        _licenseText->setText( QTextCodec::codecForName("UTF-8")->toUnicode( license.readAll() ) );
    }
    _tabWidget->addTab( _licenseText, QObject::tr("License") );

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
#if QT_VERSION < 0x050000
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
    QString libsText = QString::fromUtf8("<p> Python %1 </p>"
                                         "<p> Qt %2 </p>"
                                         "<p> Boost %3 </p>"
                                         "<p> Glew %4 </p>"
                                         "<p> OpenGL %5 </p>"
                                         "<p> Cairo %6 </p>")
                       .arg( QString::fromUtf8(PY_VERSION) )
                       .arg( _gui->getQtVersion() )
                       .arg( _gui->getBoostVersion() )
                       .arg( _gui->getGlewVersion() )
                       .arg( _gui->getOpenGLVersion() )
                       .arg( _gui->getCairoVersion() );

    _libsText->setText(libsText);
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_AboutWindow.cpp"
