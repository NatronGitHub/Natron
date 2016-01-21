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

#ifndef Gui_NodeSettingsPanel_h
#define Gui_NodeSettingsPanel_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <map>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QFrame>
#include <QTabWidget>
#include <QDialog>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

#include "Engine/DockablePanelI.h"

#include "Gui/DockablePanel.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER;

class NodeSettingsPanel
    : public DockablePanel
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

    Q_PROPERTY( bool _selected READ isSelected WRITE setSelected)

    /*Pointer to the node GUI*/
    boost::weak_ptr<NodeGui> _nodeGUI;
    bool _selected;
    Button* _settingsButton;
    boost::shared_ptr<MultiInstancePanel> _multiPanel;

public:

    explicit NodeSettingsPanel(const boost::shared_ptr<MultiInstancePanel> & multiPanel,
                               Gui* gui,
                               const boost::shared_ptr<NodeGui> &NodeUi,
                               QVBoxLayout* container,
                               QWidget *parent = 0);

    virtual ~NodeSettingsPanel();

    void setSelected(bool s);

    bool isSelected() const
    {
        return _selected;
    }

    boost::shared_ptr<NodeGui> getNode() const
    {
        return _nodeGUI.lock();
    }

    virtual boost::shared_ptr<MultiInstancePanel> getMultiInstancePanel() const OVERRIDE
    {
        return _multiPanel;
    }
    
    virtual QColor getCurrentColor() const OVERRIDE FINAL;

private:

    
    virtual RotoPanel* initializeRotoPanel() OVERRIDE FINAL;
    virtual void initializeExtraGui(QVBoxLayout* layout) OVERRIDE FINAL;
    virtual void centerOnItem() OVERRIDE FINAL;

public Q_SLOTS:
    
    void onSettingsButtonClicked();
    
    void onImportPresetsActionTriggered();
    
    void onExportPresetsActionTriggered();
};

NATRON_NAMESPACE_EXIT;

#endif // Gui_NodeSettingsPanel_h
