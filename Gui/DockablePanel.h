/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
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

#ifndef Gui_DockablePanel_h
#define Gui_DockablePanel_h

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
#include "Engine/Markdown.h"

#include "Gui/GuiFwd.h"
#include "Gui/KnobGuiContainerHelper.h"

NATRON_NAMESPACE_ENTER

/**
 * @brief An abstract class that defines a dockable properties panel that can be found in the Property bin pane.
 **/
struct DockablePanelPrivate;
class DockablePanel
    : public QFrame
      , public KnobGuiContainerHelper
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    enum HeaderModeEnum
    {
        eHeaderModeFullyFeatured = 0,
        eHeaderModeReadOnlyName,
        eHeaderModeNoHeader
    };

    explicit DockablePanel(Gui* gui,
                           const KnobHolderPtr& holder,
                           QVBoxLayout* container,
                           HeaderModeEnum headerMode,
                           bool useScrollAreasForTabs,
                           const QUndoStackPtr& stack,
                           const QString & initialName = QString(),
                           const QString & helpToolTip = QString(),
                           QWidget *parent = 0);

    virtual ~DockablePanel() OVERRIDE;

    bool isMinimized() const;
    QVBoxLayout* getContainer() const;
    bool isClosed() const;
    bool isFloating() const;

    /*Creates a new button and inserts it in the header
       at position headerPosition. You can then take
       the pointer to the Button to customize it the
       way you want. The ownership of the Button remains
       to the DockablePanel.*/
    Button* insertHeaderButton(int headerPosition);

    void insertHeaderWidget(int index, QWidget* widget);

    void appendHeaderWidget(QWidget* widget);

    QWidget* getHeaderWidget() const;

    ///MT-safe
    virtual QColor getCurrentColor() const
    {
        return Qt::black;
    }

    ///MT-safe
    void setCurrentColor(const QColor & c);

    void setOverlayColor(const QColor& c);

    void resetHostOverlayColor();

    KnobHolderPtr getHolder() const;

    void onGuiClosing();


    virtual Gui* getGui() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    FloatingWidget* getFloatingWindow() const;

    void floatPanelInWindow(FloatingWidget* window);


    void setPluginIcon(const QPixmap& pix);

    void setPluginIDAndVersion(const std::string& pluginLabel,
                               const std::string& pluginID,
                               const std::string& pluginDesc,
                               unsigned int majorVersion,
                               unsigned int minorVersion);

    virtual NodeGuiPtr getNodeGui() const OVERRIDE ;
    virtual void refreshTabWidgetMaxHeight() OVERRIDE FINAL;
    virtual bool useScrollAreaForTabs() const OVERRIDE FINAL;
    virtual void onKnobsInitialized() OVERRIDE FINAL;
    virtual std::string getHolderFullyQualifiedScriptName() const OVERRIDE FINAL;
private:

    virtual void onPagingTurnedOff() OVERRIDE FINAL;
    virtual KnobItemsTableGuiPtr createKnobItemsTable(const KnobItemsTablePtr& table, QWidget* parent) OVERRIDE FINAL;
    virtual void refreshUndoRedoButtonsEnabledNess(bool canUndo, bool canRedo) OVERRIDE FINAL;
    virtual QWidget* createKnobHorizontalFieldContainer(QWidget* parent) const OVERRIDE FINAL;
    virtual QWidget* getPagesContainer() const OVERRIDE FINAL;
    virtual QWidget* getMainContainer() const OVERRIDE FINAL;
    virtual QLayout* getMainContainerLayout() const OVERRIDE FINAL;
    virtual QWidget* createPageMainWidget(QWidget* parent) const OVERRIDE FINAL;
    virtual void addPageToPagesContainer(const KnobPageGuiPtr& page) OVERRIDE FINAL;
    virtual void removePageFromContainer(const KnobPageGuiPtr& page) OVERRIDE FINAL;
    virtual void setPagesOrder(const std::list<KnobPageGuiPtr>& order, const KnobPageGuiPtr& curPage, bool restorePageIndex) OVERRIDE FINAL;
    virtual void onKnobsRecreated() OVERRIDE FINAL;
    virtual void onPageActivated(const KnobPageGuiPtr& page) OVERRIDE FINAL;
    virtual void refreshCurrentPage() OVERRIDE FINAL;
    virtual void onPageLabelChanged(const KnobPageGuiPtr& page) OVERRIDE FINAL;

public Q_SLOTS:

    void onPageIndexChanged(int index);

    /*Internal slot, not meant to be called externally.*/
    void closePanel();

    /*Internal slot, not meant to be called externally.*/
    void minimizeOrMaximize(bool toggled);

    /*Internal slot, not meant to be called externally.*/
    void showHelp();

    ///Set the name on the line-edit/label header
    void setName(const QString & str);

    /*Internal slot, not meant to be called externally.*/
    void onUndoClicked();

    /*Internal slot, not meant to be called externally.*/
    void onRedoPressed();

    void onRestoreDefaultsButtonClicked();

    void onLineEditNameEditingFinished();

    FloatingWidget* floatPanel();

    void onColorButtonClicked();

    void onOverlayButtonClicked();

    void onColorDialogColorChanged(const QColor & color);

    void onOverlayColorDialogColorChanged(const QColor& color);

    void setClosed(bool closed);

    void onRightClickMenuRequested(const QPoint & pos);

    void onPanelSelected(const QPoint& pos);

    void setKeyOnAllParameters();
    void removeAnimationOnAllParameters();

    void onCenterButtonClicked();

    void onHideUnmodifiedButtonClicked(bool checked);

    void onManageUserParametersActionTriggered();

    void onNodeScriptChanged(const QString& label);

    void onEnterInGroupClicked();

    void onSubGraphEditionChanged(bool editable);


Q_SIGNALS:

    /*emitted when the panel is clicked*/
    void selected();

    /*emitted when the name changed on the line edit*/
    void nameChanged(QString);

    /*emitted when the user pressed undo*/
    void undoneChange();

    /*emitted when the user pressed redo*/
    void redoneChange();

    /*emitted when the panel is minimized*/
    void minimized();

    /*emitted when the panel is maximized*/
    void maximized();

    void closeChanged(bool closed);

    void colorChanged(QColor);

protected:

    /**
     * @brief Called when the "center on..." button is clicked
     **/
    virtual void centerOnItem() {}

private:

    void setClosedInternal(bool c);

    void initializeKnobsInternal();
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL
    {
        Q_EMIT selected();
        QFrame::mousePressEvent(e);
    }

    virtual void focusInEvent(QFocusEvent* e) OVERRIDE FINAL;

    QString helpString() const;

    boost::scoped_ptr<DockablePanelPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // Gui_DockablePanel_h
