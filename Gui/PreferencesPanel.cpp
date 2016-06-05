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

#include "PreferencesPanel.h"

#include <map>
#include <stdexcept>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QVBoxLayout>
#include <QDesktopWidget>
#include <QApplication>
#include <QDialogButtonBox>
#include <QKeyEvent>
#include <QDesktopServices>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/KnobTypes.h"
#include "Engine/Settings.h"

#include "Gui/DockablePanel.h"
#include "Gui/Button.h"
#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/Utils.h"
#include "Gui/Splitter.h"

NATRON_NAMESPACE_ENTER;

struct PreferenceTab
{
    QTreeWidgetItem* treeItem;
    QFrame* tab;
    boost::weak_ptr<KnobPageGui> page;
};

struct PreferencesPanelPrivate
{
    Gui* gui;
    QVBoxLayout* mainLayout;

    Splitter* splitter;
    QTreeWidget* tree;
    std::vector<PreferenceTab> tabs;
    int currentTabIndex;

    QDialogButtonBox* buttonBox;
    Button* restoreDefaultsB;
    Button* prefsHelp;
    Button* cancelB;
    Button* okB;
    std::vector<KnobI*> changedKnobs;
    bool closeIsOK;

    PreferencesPanelPrivate(Gui *parent)
    : gui(parent)
    , mainLayout(0)
    , splitter(0)
    , tree(0)
    , tabs()
    , currentTabIndex(-1)
    , buttonBox(0)
    , restoreDefaultsB(0)
    , prefsHelp(0)
    , cancelB(0)
    , okB(0)
    , changedKnobs(0)
    , closeIsOK(false)
    {
        
    }

    void createPreferenceTab(const KnobPageGuiPtr& page, PreferenceTab* tab);

    void setVisiblePage(int index);
};

PreferencesPanel::PreferencesPanel(Gui *parent)
    : QWidget(parent)
      , KnobGuiContainerHelper(appPTR->getCurrentSettings().get(), boost::shared_ptr<QUndoStack>())
      , _imp(new PreferencesPanelPrivate(parent))
{

}

PreferencesPanel::~PreferencesPanel()
{
    
}

Gui*
PreferencesPanel::getGui() const
{
    return _imp->gui;
}

void
PreferencesPanel::createGui()
{
    //setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    setWindowFlags(Qt::Window);
    setWindowTitle( tr("Preferences") );
    _imp->mainLayout = new QVBoxLayout(this);
    //_imp->mainLayout->setContentsMargins(0, 0, 0, 0);
    //_imp->mainLayout->setSpacing(0);

    _imp->splitter = new Splitter(Qt::Horizontal, this);
    //_imp->centerLayout->setContentsMargins(0, 0, 0, 0);
    //_imp->centerLayout->setSpacing(0);

    _imp->tree = new QTreeWidget(_imp->splitter);
    _imp->tree->setSelectionMode(QAbstractItemView::SingleSelection);
    _imp->tree->setColumnCount(1);
    _imp->tree->setAttribute(Qt::WA_MacShowFocusRect, 0);
    _imp->tree->header()->close();
    QObject::connect(_imp->tree, SIGNAL(itemSelectionChanged()), this, SLOT(onItemSelectionChanged()));

    initializeKnobs();

    _imp->splitter->addWidget(_imp->tree);

    _imp->tabs[0].treeItem->setSelected(true);
    onItemSelectionChanged();

    int maxLength = 0;
    QFontMetrics fm = fontMetrics();
    for (std::size_t i = 0; i < _imp->tabs.size(); ++i) {
        QString label = QString::fromUtf8(_imp->tabs[i].page.lock()->pageKnob.lock()->getLabel().c_str());
        int w = fm.width(label);
        maxLength = std::max(w, maxLength);
    }

    _imp->tree->setFixedWidth(maxLength + 40);

    _imp->buttonBox = new QDialogButtonBox(Qt::Horizontal);
    _imp->restoreDefaultsB = new Button( tr("Restore defaults") );
    _imp->restoreDefaultsB->setToolTip( GuiUtils::convertFromPlainText(tr("Restore default values for all preferences."), Qt::WhiteSpaceNormal) );

    _imp->prefsHelp = new Button( tr("Help") );
    _imp->prefsHelp->setToolTip( tr("Show help for preference") );

    _imp->cancelB = new Button( tr("Discard") );
    _imp->cancelB->setToolTip( GuiUtils::convertFromPlainText(tr("Cancel changes that were not saved and close the window."), Qt::WhiteSpaceNormal) );
    _imp->okB = new Button( tr("Save") );
    _imp->okB->setToolTip( GuiUtils::convertFromPlainText(tr("Save changes on disk and close the window."), Qt::WhiteSpaceNormal) );
    _imp->buttonBox->addButton(_imp->restoreDefaultsB, QDialogButtonBox::ResetRole);
    _imp->buttonBox->addButton(_imp->prefsHelp, QDialogButtonBox::ResetRole);
    _imp->buttonBox->addButton(_imp->cancelB, QDialogButtonBox::RejectRole);
    _imp->buttonBox->addButton(_imp->okB, QDialogButtonBox::AcceptRole);

    _imp->mainLayout->addWidget(_imp->splitter);
    _imp->mainLayout->addWidget(_imp->buttonBox);

    QObject::connect( _imp->restoreDefaultsB, SIGNAL(clicked()), this, SLOT(restoreDefaults()) );
    QObject::connect( _imp->prefsHelp, SIGNAL(clicked()), this, SLOT(openHelp()) );
    QObject::connect( _imp->buttonBox, SIGNAL(rejected()), this, SLOT(cancelChanges()) );
    QObject::connect( _imp->buttonBox, SIGNAL(accepted()), this, SLOT(saveChangesAndClose()) );
    QObject::connect( appPTR->getCurrentSettings().get(), SIGNAL(settingChanged(KnobI*)), this, SLOT(onSettingChanged(KnobI*)) );
    
    
    resize( TO_DPIX(900), TO_DPIY(600) );
}

void
PreferencesPanelPrivate::setVisiblePage(int index)
{
    if (currentTabIndex != -1) {
        QFrame* frame = tabs[currentTabIndex].tab;
        //frame->setParent(0);
        //splitter->removeWidget(frame);
        frame->hide();
    }
    if (index  != -1) {
        tabs[index].tab->show();
        splitter->addWidget(tabs[index].tab);
        currentTabIndex = index;
    }
    tree->resizeColumnToContents(0);
}

void
PreferencesPanel::onPageActivated(const KnobPageGuiPtr& page)
{
    if (!page) {
        return;
    }
    for (std::size_t i = 0; i < _imp->tabs.size(); ++i) {
        if (_imp->tabs[i].page.lock() == page) {
            _imp->setVisiblePage(i);
        }
    }
}

void
PreferencesPanel::onItemSelectionChanged()
{
    QList<QTreeWidgetItem*> selection = _imp->tree->selectedItems();
    assert(selection.size() <= 1);
    if (selection.empty()) {
        setCurrentPage(KnobPageGuiPtr());
        _imp->setVisiblePage(-1);
        return;
    }

    QTreeWidgetItem* selectedItem = selection.front();

    for (std::size_t i = 0; i < _imp->tabs.size(); ++i) {
        if (_imp->tabs[i].treeItem == selectedItem) {
            setCurrentPage(_imp->tabs[i].page.lock());
            _imp->setVisiblePage(i);
            return;
        }
    }

}

void
PreferencesPanel::refreshCurrentPage()
{
    QList<QTreeWidgetItem*> selection = _imp->tree->selectedItems();
    assert(selection.size() <= 1);
    if (selection.empty()) {
        setCurrentPage(KnobPageGuiPtr());
        return;
    }

    QTreeWidgetItem* selectedItem = selection.front();

    for (std::size_t i = 0; i < _imp->tabs.size(); ++i) {
        if (_imp->tabs[i].treeItem == selectedItem) {
            setCurrentPage(_imp->tabs[i].page.lock());
            _imp->setVisiblePage(i);
            return;
        }
    }
}

QWidget*
PreferencesPanel::getPagesContainer() const
{
    return const_cast<PreferencesPanel*>(this);
}

QWidget*
PreferencesPanel::createPageMainWidget(QWidget* parent) const
{
    QFrame* ret = new QFrame(parent);
    ret->setFrameShadow(QFrame::Sunken);
    ret->setFrameShape(QFrame::Box);
    return ret;
}

void
PreferencesPanel::addPageToPagesContainer(const KnobPageGuiPtr& page)
{
    PreferenceTab tab;
    _imp->createPreferenceTab(page, &tab);
    _imp->tabs.push_back(tab);
}

void
PreferencesPanel::removePageFromContainer(const KnobPageGuiPtr& page)
{
    for (std::size_t i = 0; i < _imp->tabs.size(); ++i) {
        if (_imp->tabs[i].tab == page->tab) {
            _imp->tabs.erase(_imp->tabs.begin() + i);
            return;
        }
    }
}

void
PreferencesPanel::refreshUndoRedoButtonsEnabledNess(bool /*canUndo*/, bool /*canRedo*/)
{

}

void
PreferencesPanelPrivate::createPreferenceTab(const KnobPageGuiPtr& page, PreferenceTab* tab)
{
    tab->tab = dynamic_cast<QFrame*>(page->tab);
    tab->tab->hide();
    assert(tab->tab);
    tab->treeItem = new QTreeWidgetItem(tree);

    QString label = QString::fromUtf8(page->pageKnob.lock()->getLabel().c_str());
    tab->treeItem->setText(0, label);
    tab->page = page;
}

void
PreferencesPanel::setPagesOrder(const std::list<KnobPageGuiPtr>& order, const KnobPageGuiPtr& curPage, bool /*restorePageIndex*/)
{
    for (std::size_t i = 0; i < _imp->tabs.size(); ++i) {
        delete _imp->tabs[i].treeItem;
    }
    _imp->tabs.clear();
    for (std::list<KnobPageGuiPtr>::const_iterator it = order.begin(); it!=order.end(); ++it) {
        PreferenceTab tab;
        _imp->createPreferenceTab(*it, &tab);
        _imp->tabs.push_back(tab);

        if (*it != curPage) {
            tab.treeItem->setSelected(false);
        } else {
            tab.treeItem->setSelected(true);
        }
    }
}

void
PreferencesPanel::onPageLabelChanged(const KnobPageGuiPtr& page)
{
    for (std::size_t i = 0; i < _imp->tabs.size(); ++i) {
        if (_imp->tabs[i].tab == page->tab) {
            QString label = QString::fromUtf8(page->pageKnob.lock()->getLabel().c_str());
            _imp->tabs[i].treeItem->setText(0, label);
            return;
        }
    }
}

void
PreferencesPanel::onSettingChanged(KnobI* knob)
{
    for (U32 i = 0; i < _imp->changedKnobs.size(); ++i) {
        if (_imp->changedKnobs[i] == knob) {
            return;
        }
    }
    _imp->changedKnobs.push_back(knob);
}

void
PreferencesPanel::openHelp()
{
    int docSource = appPTR->getCurrentSettings()->getDocumentationSource();
    int serverPort = appPTR->getCurrentSettings()->getServerPort();
    QString localUrl = QString::fromUtf8("http://localhost:") + QString::number(serverPort) + QString::fromUtf8("/_prefs.html");
    QString remoteUrl = QString::fromUtf8(NATRON_DOCUMENTATION_ONLINE);

    switch (docSource) {
    case 0:
        QDesktopServices::openUrl( QUrl(localUrl) );
        break;
    case 1:
        QDesktopServices::openUrl( QUrl(remoteUrl) );
        break;
    case 2:
        Dialogs::informationDialog(tr("Missing documentation").toStdString(), tr("Missing documentation, please go to settings and select local or online documentation source.").toStdString(), true);
        break;
    }
}

void
PreferencesPanel::restoreDefaults()
{
    StandardButtonEnum reply = Dialogs::questionDialog( tr("Preferences").toStdString(),
                                                        tr("Restoring the settings will delete any custom configuration, are you sure you want to do this?").toStdString(), false );

    if (reply == eStandardButtonYes) {
        appPTR->getCurrentSettings()->restoreDefault();
    }
}

void
PreferencesPanel::cancelChanges()
{
    _imp->closeIsOK = false;
    close();
}

void
PreferencesPanel::saveChangesAndClose()
{
    ///Steal focus from other widgets so that we are sure all LineEdits and Spinboxes get the focusOut event and their editingFinished
    ///signal is emitted.
    _imp->okB->setFocus();
    appPTR->getCurrentSettings()->saveSettings(_imp->changedKnobs, true);
    _imp->closeIsOK = true;
    close();
}

void
PreferencesPanel::showEvent(QShowEvent* /*e*/)
{
    QDesktopWidget* desktop = QApplication::desktop();
    const QRect rect = desktop->screenGeometry();

    move( QPoint(rect.width() / 2 - width() / 2, rect.height() / 2 - height() / 2) );

    _imp->changedKnobs.clear();
}

void
PreferencesPanel::closeEvent(QCloseEvent*)
{
    if ( !_imp->closeIsOK && !_imp->changedKnobs.empty() ) {
        boost::shared_ptr<Settings> settings = appPTR->getCurrentSettings();
        settings->beginChanges();
        settings->restoreKnobsFromSettings(_imp->changedKnobs);
        settings->endChanges();
    }
}

void
PreferencesPanel::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape) {
        _imp->closeIsOK = false;
        close();
    } else {
        QWidget::keyPressEvent(e);
    }
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_PreferencesPanel.cpp"
