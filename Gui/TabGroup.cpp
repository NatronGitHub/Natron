/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>

#define NATRON_FORM_LAYOUT_LINES_SPACING 0


using std::make_pair;
using namespace Natron;


TabGroup::TabGroup(QWidget* parent)
: QFrame(parent)
{
    setFrameShadow(QFrame::Raised);
    setFrameShape(QFrame::Box);
    QHBoxLayout* frameLayout = new QHBoxLayout(this);
    _tabWidget = new QTabWidget(this);
    frameLayout->addWidget(_tabWidget);
}

QGridLayout*
TabGroup::addTab(const boost::shared_ptr<KnobGroup>& group, const QString& label)
{
    
    QWidget* tab = 0;
    QGridLayout* tabLayout = 0;
    for (U32 i = 0 ; i < _tabs.size(); ++i) {
        if (_tabs[i].lock() == group) {
            tab = _tabWidget->widget(i);
            assert(tab);
            tabLayout = qobject_cast<QGridLayout*>( tab->layout() );
            assert(tabLayout);
        }
    }
   
    if (!tab) {
        tab = new QWidget(_tabWidget);
        tabLayout = new QGridLayout(tab);
        tabLayout->setColumnStretch(1, 1);
        //tabLayout->setContentsMargins(0, 0, 0, 0);
        tabLayout->setSpacing(NATRON_FORM_LAYOUT_LINES_SPACING); // unfortunately, this leaves extra space when parameters are hidden
        _tabWidget->addTab(tab, label);
        _tabs.push_back(group);
    }
    assert(tabLayout);
    return tabLayout;
}

void
TabGroup::removeTab(KnobGroup* group)
{
    int i = 0;
    for (std::vector<boost::weak_ptr<KnobGroup> >::iterator it = _tabs.begin(); it != _tabs.end(); ++it, ++i) {
        if (it->lock().get() == group) {
            _tabWidget->removeTab(i);
            _tabs.erase(it);
            break;
        }
    }
}
