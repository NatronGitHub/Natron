/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "Engine/ViewIdx.h"

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
    static NodeViewerContextPtr create(const NodeGuiPtr& node,
                                                       ViewerTab* viewer)
    {
        return NodeViewerContextPtr( new NodeViewerContext(node, viewer) );
    }

    void createGui();

    virtual ~NodeViewerContext();

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

    void setCurrentTool(const QString& toolID, bool notifyNode);

    void notifyGuiClosing();

    virtual bool isInViewerUIKnob() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    void onToolButtonShortcutPressed(const QString& roleID);

Q_SIGNALS:

    /**
     * @brief Emitted when the selected role changes
     **/
    void roleChanged(int previousRole, int newRole);

public Q_SLOTS:

    void onNodeColorChanged(const QColor& color);

    /**
     * @brief Received when the selection rectangle has changed on the viewer.
     * @param onRelease When true, this signal is emitted on the mouse release event
     * which means this is the last selection desired by the user.
     * Receivers can either update the selection always or just on mouse release.
     **/
    void updateSelectionFromViewerSelectionRectangle(bool onRelease);

    void onViewerSelectionCleared();

    void onToolActionTriggered();

    void onToolActionTriggered(QAction* act);

    void onToolGroupValueChanged(ViewSpec view,
                                 int dimension,
                                 int reason);

    void onToolActionValueChanged(ViewSpec view,
                                  int dimension,
                                  int reason);

    void onNodeSettingsPanelClosed(bool closed);

    void onNodeSettingsPanelMinimized();

    void onNodeSettingsPanelMaximized();

private:

    boost::scoped_ptr<NodeViewerContextPrivate> _imp;
};


NATRON_NAMESPACE_EXIT

#endif // NODEVIEWERCONTEXT_H
