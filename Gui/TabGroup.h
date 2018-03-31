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

#ifndef Gui_TabGroup_h
#define Gui_TabGroup_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QFrame>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

/**
 * @brief Used when group are using the kFnOfxParamPropGroupIsTab extension
 **/
struct TabGroupPrivate;
class TabGroup
    : public QFrame
{
    Q_OBJECT

public:

    TabGroup(QWidget* parent);

    virtual ~TabGroup();

    QGridLayout* addTab(const KnobGroupPtr& group, const QString &label);

    void removeTab(KnobGroup* group);

    bool isEmpty() const;

    void refreshTabSecretNess(KnobGroup* group);

    void refreshTabSecretNess(KnobGroup* group, bool secret);

public Q_SLOTS:

    void onGroupKnobSecretChanged();

private:

    boost::scoped_ptr<TabGroupPrivate> _imp;
};


NATRON_NAMESPACE_EXIT

#endif // Gui_TabGroup_h
