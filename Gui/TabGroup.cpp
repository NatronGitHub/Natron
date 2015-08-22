//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

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
TabGroup::addTab(const boost::shared_ptr<Group_Knob>& group,const QString& name)
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
        _tabWidget->addTab(tab,name);
        _tabs.push_back(group);
    }
    assert(tabLayout);
    return tabLayout;
}

void
TabGroup::removeTab(Group_Knob* group)
{
    int i = 0;
    for (std::vector<boost::weak_ptr<Group_Knob> >::iterator it = _tabs.begin(); it != _tabs.end(); ++it, ++i) {
        if (it->lock().get() == group) {
            _tabWidget->removeTab(i);
            _tabs.erase(it);
            break;
        }
    }
}
