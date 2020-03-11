/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#ifndef NODEVIEWERCONTEXT_H
#define NODEVIEWERCONTEXT_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/enable_shared_from_this.hpp>
#include <boost/scoped_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
#include <QtCore/QObject>
CLANG_DIAG_ON(deprecated)

#include "Engine/DimensionIdx.h"
#include "Engine/ViewIdx.h"
#include "Global/GlobalDefines.h"

#include "Gui/GuiFwd.h"
#include "Gui/KnobGuiContainerI.h"

NATRON_NAMESPACE_ENTER


struct NodeViewerContextPrivate;
class NodeViewerContext
    : public QObject
    , public KnobGuiContainerI
    , public boost::enable_shared_from_this<NodeViewerContext>
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

private:
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    NodeViewerContext(const NodeGuiPtr& node,
                      ViewerTab* viewer);

public:
    static boost::shared_ptr<NodeViewerContext> create(const NodeGuiPtr& node,
                                                       ViewerTab* viewer) WARN_UNUSED_RETURN
    {
        return NodeViewerContextPtr( new NodeViewerContext(node, viewer) );
    }

    void createGui();

    virtual ~NodeViewerContext();

    /**
    * @brief If this as a player toolbar (only the viewer) this will return it
    **/
    QWidget* getPlayerToolbar() const;

    /**
     * @brief If this node's viewer context has a toolbar, this will return it
     **/
    QToolBar* getToolBar() const;

    /**
     * @brief The selected role ID
     **/
    const QString& getCurrentRole() const;

    /**
     * @brief The selected tool ID
     **/
    const QString& getCurrentTool() const;
    virtual Gui* getGui() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual const QUndoCommand* getLastUndoCommand() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void pushUndoCommand(QUndoCommand* cmd) OVERRIDE FINAL;
    virtual KnobGuiPtr getKnobGui(const KnobIPtr& knob) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual int getItemsSpacingOnSameLine() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual QWidget* createKnobHorizontalFieldContainer(QWidget* parent) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual NodeGuiPtr getNodeGui() const OVERRIDE FINAL;

    void setCurrentTool(const QString& toolID, bool notifyNode);

    void notifyGuiClosing();

    void onToolButtonShortcutPressed(const QString& roleID);

Q_SIGNALS:

    /**
     * @brief Emitted when the selected role changes
     **/
    void roleChanged(int previousRole, int newRole);

public Q_SLOTS:

    void onNodeLabelChanged(const QString& oldLabel, const QString& newLabel);

    void onNodeColorChanged(const QColor& color);

    void onToolActionTriggered();

    void onToolActionTriggered(QAction* act);

    void onToolGroupValueChanged(ViewSetSpec,DimSpec,ValueChangedReasonEnum);

    void onToolActionValueChanged(ViewSetSpec,DimSpec,ValueChangedReasonEnum);

    void onNodeSettingsPanelClosed(bool closed);

private:

    boost::scoped_ptr<NodeViewerContextPrivate> _imp;
};


NATRON_NAMESPACE_EXIT

#endif // NODEVIEWERCONTEXT_H
