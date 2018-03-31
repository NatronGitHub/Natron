/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "TabGroup.h"
#include <map>
#include <stdexcept>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QGridLayout>

#include "Engine/KnobTypes.h"


#define NATRON_FORM_LAYOUT_LINES_SPACING 0

NATRON_NAMESPACE_ENTER

struct TabGroupTab
{
    KnobGroupWPtr groupKnob;
    QWidget* tab;
    QGridLayout* layout;
    bool visible;

    TabGroupTab()
        : groupKnob()
        , tab(0)
        , layout(0)
        , visible(false)
    {
    }
};

struct TabGroupPrivate
{
    QTabWidget* tabWidget;
    std::vector<TabGroupTab> tabs;

    TabGroupPrivate()
        : tabWidget(0)
        , tabs()
    {
    }
};

TabGroup::TabGroup(QWidget* parent)
    : QFrame(parent)
    , _imp( new TabGroupPrivate() )
{
    setFrameShadow(QFrame::Raised);
    setFrameShape(QFrame::NoFrame);
    QHBoxLayout* frameLayout = new QHBoxLayout(this);
    _imp->tabWidget = new QTabWidget(this);
    frameLayout->addWidget(_imp->tabWidget);
}

TabGroup::~TabGroup()
{
}

bool
TabGroup::isEmpty() const
{
    return _imp->tabs.empty();
}

QGridLayout*
TabGroup::addTab(const boost::shared_ptr<KnobGroup>& group,
                 const QString& label)
{
    TabGroupTab* tabGroup = 0;

    for (std::size_t i = 0; i < _imp->tabs.size(); ++i) {
        if (_imp->tabs[i].groupKnob.lock() == group) {
            tabGroup = &_imp->tabs[i];
        }
    }

    if (!tabGroup) {
        _imp->tabs.resize(_imp->tabs.size() + 1);
        tabGroup = &_imp->tabs.back();
        tabGroup->groupKnob = group;
        bool visible = !group->getIsSecret();
        tabGroup->visible = visible;
        tabGroup->tab = new QWidget(_imp->tabWidget);
        tabGroup->tab->setObjectName(label);
        tabGroup->layout = new QGridLayout(tabGroup->tab);
        tabGroup->layout->setColumnStretch(1, 1);
        tabGroup->layout->setSpacing(NATRON_FORM_LAYOUT_LINES_SPACING); // unfortunately, this leaves extra space when parameters are hidden

        if (visible) {
            _imp->tabWidget->addTab(tabGroup->tab, label);
        }
        //tabGroup->tab->setVisible(visible);

        boost::shared_ptr<KnobSignalSlotHandler> handler = group->getSignalSlotHandler();
        QObject::connect( handler.get(), SIGNAL(secretChanged()), this, SLOT(onGroupKnobSecretChanged()) );
    }
    assert(tabGroup->layout);

    return tabGroup->layout;
}

void
TabGroup::removeTab(KnobGroup* group)
{
    for (std::size_t i = 0; i < _imp->tabs.size(); ++i) {
        if (_imp->tabs[i].groupKnob.lock().get() == group) {
            _imp->tabWidget->removeTab(i);
            _imp->tabs.erase(_imp->tabs.begin() + i);
        }
    }
}

void
TabGroup::refreshTabSecretNess(KnobGroup* groupKnob,
                               bool secret)
{
    bool isVisible = !secret;
    bool oneVisible = false;

    for (std::size_t i = 0; i < _imp->tabs.size(); ++i) {
        if (_imp->tabs[i].groupKnob.lock().get() == groupKnob) {
            _imp->tabs[i].visible = isVisible;
            if (!isVisible) {
                _imp->tabs[i].tab->hide();
                _imp->tabWidget->removeTab(i);
            } else {
                _imp->tabs[i].tab->show();
                _imp->tabWidget->insertTab( i, _imp->tabs[i].tab, _imp->tabs[i].tab->objectName() );
            }
        }
        if (_imp->tabs[i].visible) {
            oneVisible = true;
        }
    }
    if (!oneVisible) {
        hide();
    } else {
        show();
    }
}

void
TabGroup::refreshTabSecretNess(KnobGroup* groupKnob)
{
    refreshTabSecretNess( groupKnob, groupKnob->getIsSecret() );
}

void
TabGroup::onGroupKnobSecretChanged()
{
    KnobSignalSlotHandler* handler = qobject_cast<KnobSignalSlotHandler*>( sender() );

    if (!handler) {
        return;
    }

    KnobIPtr knob = handler->getKnob();
    if (!knob) {
        return;
    }

    KnobGroup* groupKnob = dynamic_cast<KnobGroup*>( knob.get() );
    if (!groupKnob) {
        return;
    }
    refreshTabSecretNess(groupKnob);
}

NATRON_NAMESPACE_EXIT
NATRON_NAMESPACE_USING

#include "moc_TabGroup.cpp"
