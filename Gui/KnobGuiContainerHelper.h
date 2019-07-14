/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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

#ifndef KNOBGUICONTAINERHELPER_H
#define KNOBGUICONTAINERHELPER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <map>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QObject>
#include <QMessageBox>
#include <QPixmap>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/DockablePanelI.h"
#include "Gui/GuiFwd.h"
#include "Gui/KnobGuiContainerI.h"


NATRON_NAMESPACE_ENTER


#define NATRON_SETTINGS_VERTICAL_SPACING_PIXELS 3
#define NATRON_FORM_LAYOUT_LINES_SPACING 0


typedef std::list<std::pair<KnobIWPtr, KnobGuiPtr> > KnobsGuiMapping;

class KnobPageGui
{
public:
    QWidget* tab;
    TabGroup* groupAsTab; //< to gather group knobs that are set as a tab
    KnobPageWPtr pageKnob;
    QGridLayout* gridLayout;

    KnobPageGui()
        : tab(0)
        , groupAsTab(0)
        , pageKnob()
        , gridLayout(0)
    {
    }
};

typedef boost::shared_ptr<KnobPageGui> KnobPageGuiPtr;
typedef std::map<KnobPageWPtr, KnobPageGuiPtr> PagesMap;

/**
 * @brief Helper class to handle signal/slots so we do not make KnobGuiContainerHelper inherit QObject.
 * We do this because derived class of KnobGuiContainerHelper might also want inherit QWidget in which case
 * there would be an ambiguous class derivation of QObject.
 **/
class KnobGuiContainerSignalsHandler
    : public QObject
{
    Q_OBJECT

public:


    KnobGuiContainerSignalsHandler(KnobGuiContainerHelper* container)
        : QObject()
        , _container(container)
    {
        QObject::connect(this, SIGNAL( deleteCurCmdLater() ), this, SLOT( onDeleteCurCmdLaterTriggered() ), Qt::QueuedConnection);
    }

    virtual ~KnobGuiContainerSignalsHandler()
    {
    }

    void s_deleteCurCmdLater()
    {
        Q_EMIT deleteCurCmdLater();
    }

Q_SIGNALS:

    void deleteCurCmdLater();

public Q_SLOTS:

    void onPageLabelChangedInternally();

    void onPageSecretnessChanged();

    void onDeleteCurCmdLaterTriggered();

private:

    KnobGuiContainerHelper* _container;
};

/**
 * @brief KnobGuiContainerHelper is a base class for displaying paged knobs in the interface. Derived implementations
 * must indicate how pages are selected and which one is active at a time. @see DockablePanel for an implementation.
 **/
struct KnobGuiContainerHelperPrivate;
class KnobGuiContainerHelper
    : public KnobGuiContainerI, public DockablePanelI

{
public:

    /**
     * @brief Creates a new container that will be used to display the knobs inside the given holder.
     * The option QUndoStack stack can be given in parameter so that we do not make our own undo/redo stack but
     * use this one instead.
     **/
    KnobGuiContainerHelper(const KnobHolderPtr& holder, const boost::shared_ptr<QUndoStack>& stack);

    virtual ~KnobGuiContainerHelper();

    /**
     * @brief Call once to create all the gui. This will properly create all the knobs by recursing over them.
     * Once created once, if knob changes (deletion, creation...) happen you should instead call refreshGuiForKnobsChanges() to refresh knob changes.
     **/
    void initializeKnobs();

    /**
     * @brief Returns all the pages inside this container
     **/
    const PagesMap& getPages() const;

    /**
     * @brief Return a list of all the internal pages which were created by the user
     **/
    void getUserPages(std::list<KnobPagePtr>& userPages) const;

    /**
     * @brief Make the given page current
     **/
    void setPageActiveIndex(const KnobPagePtr& page);

    /**
     * @brief Same as getPages().size()
     **/
    int getPagesCount() const;

    /**
     * @brief Returns the currently active page
     **/
    KnobPageGuiPtr getCurrentPage() const;

    /**
     * @brief Returns the internal vector of knobs
     **/
    const KnobsVec& getInternalKnobs() const;

    /**
     * @brief Returns a mapping between the internal knobs and the gui counter part
     **/
    const KnobsGuiMapping& getKnobsMapping() const;

    /**
     * @brief Returns a list of tables held by this container
     **/
    std::list<KnobItemsTableGuiPtr> getAllKnobItemsTables() const;

    /**
     * @brief Returns a table by its name
     **/
    KnobItemsTableGuiPtr getKnobItemsTable(const std::string& tableName) const;

    /**
     * @brief Returns the undo/redo stack used for commands applied on knobs
     **/
    QUndoStackPtr getUndoStack() const;

    /**
     * @brief When called, all knobs will go into the same page which will appear as a plain Widget and not as a tab. This must be called before
     * calling initializeKnobs()
     **/
    void turnOffPages();

    /**
     * @brief Configures the container so it only displays the knobs within the given group knob.
     * The knob should have its getIsDialog() function returning true.
     **/
    void setAsDialogForGroup(const KnobGroupPtr& groupKnobDialog);
    KnobGroupPtr isDialogForGroup() const;

    /**
     * @brief Returns whether paging is enabled or not. If paging is disabled, there will be only a single plain widget without header.
     **/
    bool isPagingEnabled() const;

    /**
     * @brief Returns whether the container for knobs should use a scroll-area or a plain widget
     **/
    virtual bool useScrollAreaForTabs() const
    {
        return false;
    }

    //// Overridden from DockablePanelI

    /**
     * @brief Removes a knob from the GUI, this should not be called directly, instead one should call KnobHolder::deleteKnob
     **/
    virtual void deleteKnobGui(const KnobIPtr& knob) OVERRIDE FINAL;

    /**
     * @brief Scan for changes among user created knobs and re-create them
     **/
    virtual void recreateUserKnobs(bool restorePageIndex) OVERRIDE FINAL;

    /**
     * @brief Scan for any change on all knobs and recreate them
     **/
    virtual void refreshGuiForKnobsChanges(bool restorePageIndex) OVERRIDE FINAL;

    /**
     * @brief Removes a specific knob from the gui
     **/
    virtual void recreateViewerUIKnobs() OVERRIDE FINAL;
    ///// End override from DockablePanelI

    //// Overridden from KnobGuiContainerI

    /**
     * @brief Get a pointer to the main gui
     **/
    virtual Gui* getGui() const OVERRIDE = 0;

    /**
     * @brief Get a pointer to the last pushed undo command
     **/
    virtual const QUndoCommand* getLastUndoCommand() const OVERRIDE FINAL;

    /**
     * @brief Push a new command to the undo/redo stack
     **/
    virtual void pushUndoCommand(QUndoCommand* cmd) OVERRIDE FINAL;


    /**
     * @brief Push a new undo command to the undo/redo stack associated to this node.
     * The stack takes ownership of the shared pointer, so you should not hold a strong reference to the passed pointer.
     * If no undo/redo stack is present, the command will just be redone once then destroyed.
     **/
    virtual void pushUndoCommand(const UndoCommandPtr& command) OVERRIDE FINAL;

    // Takes ownership, command is deleted when returning call
    void pushUndoCommand(UndoCommand* command);

    /**
     * @brief Returns a pointe to the KnobGui representing the given internal knob.
     **/
    virtual KnobGuiPtr getKnobGui(const KnobIPtr& knob) const OVERRIDE FINAL WARN_UNUSED_RETURN;

    /**
     * @brief Returns the horizontal spacing that should be used by default between knobs on a same layout line.
     * This will only be used if the function Knob::getSpacingBetweenitems() returns 0 for a given knob.
     **/
    virtual int getItemsSpacingOnSameLine() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    ///// End override from KnobGuiContainerI

    static bool setLabelFromTextAndIcon(KnobClickableLabel* widget, const QString& labelText, const QString& labelIconFilePath, bool setBold);

    static QPixmap getStandardIcon(QMessageBox::Icon icon, int size, QWidget* widget);

    /**
     * @brief Refresh whether a page should be made visible or not. A page is considered to be visible
     * when at least one of its children (recursively) is not secret.
     **/
    virtual void refreshPageVisibility(const KnobPagePtr& page) OVERRIDE;


    /**
     * @brief Creates the widget that will contain the GUI for a knob
     **/
    virtual QWidget* createKnobHorizontalFieldContainer(QWidget* parent) const OVERRIDE WARN_UNUSED_RETURN;
    
protected:

    /**
     * @brief To be implemented when turnOffPages() is called, to destroy or hide the main pages widget container (the TabWidget)
     **/
    virtual void onPagingTurnedOff() {}

    /**
     * @brief Set the pointer to the current page, this should be called by the derived implementation when the current page has changed,
     * i.e: when the current tab of a tabwidget has changed
     **/
    void setCurrentPage(const KnobPageGuiPtr& curPage);

    /**
     * @brief This is called once initializeKnobs() has finished, to initialize any remaining GUI
     **/
    virtual void onKnobsInitialized() {}

    /**
     * @brief This is called when a page is made current externally, e.g: not by the user changing it from the tab-widget
     **/
    virtual void onPageActivated(const KnobPageGuiPtr& page) = 0;

    /**
     * @brief This is call after knobs are initialized to set the current page pointer
     **/
    virtual void refreshCurrentPage() = 0;

    /**
     * @brief Returns the pages container
     **/
    virtual QWidget* getPagesContainer() const = 0;

    /**
     * @brief Returns the root widget
     **/
    virtual QWidget* getMainContainer() const = 0;
    virtual QLayout* getMainContainerLayout() const = 0;

    /**
     * @brief Creates the container for knobs within a page
     **/
    virtual QWidget* createPageMainWidget(QWidget* parent) const = 0;

    /**
     * @brief Called once a page is created to add it to the container (e.g: a tabwidget)
     **/
    virtual void addPageToPagesContainer(const KnobPageGuiPtr& page) = 0;

    /**
     * @brief Called once a page is removed to remove it from the container (e.g: from a tabwidget)
     **/
    virtual void removePageFromContainer(const KnobPageGuiPtr& page) = 0;

    /**
     * @brief Called when the undo/redo stack changes state to refresh any undo/redo button associated to it.
     **/
    virtual void refreshUndoRedoButtonsEnabledNess(bool canUndo, bool canRedo) = 0;

    /**
     * @brief Should set the pages in the given order with curPage being used as the current page if restorePageIndex is true.
     **/
    virtual void setPagesOrder(const std::list<KnobPageGuiPtr>& order, const KnobPageGuiPtr& curPage, bool restorePageIndex) = 0;

    /**
     * @brief Called once refreshGuiForKnobsChanges() has been called, typically to update any other GUI that display knobs information
     **/
    virtual void onKnobsRecreated() {}

    /**
     * @brief Called when a page knob label has changed to refresh the title associated to a page if any
     **/
    virtual void onPageLabelChanged(const KnobPageGuiPtr& page) = 0;

    /**
     * @brief Returns the given page or creates it
     **/
    KnobPageGuiPtr getOrCreatePage(const KnobPagePtr& page);

    /**
     * @brief Returns the page that should be used by default for knobs without a page.
     **/
    KnobPageGuiPtr getOrCreateDefaultPage();

    /**
     * @brief If the knobs container can display a knob items table GUI, implement it to create it
     **/
    virtual KnobItemsTableGuiPtr createKnobItemsTable(const KnobItemsTablePtr& /*table*/, QWidget* /*parent*/) { return KnobItemsTableGuiPtr(); };

private:

    void initializeKnobVector(const KnobsVec& knobs);

    void refreshPagesOrder(const KnobPageGuiPtr& curTabName, bool restorePageIndex);

    void clearUndoRedoStack();

    KnobGuiPtr findKnobGuiOrCreate(const KnobIPtr &knob);

    void createTabedGroupGui(const KnobGroupPtr& knob);

    void initializeKnobVectorInternal(const KnobsVec& siblingsVec, KnobsVec* regularKnobsVec);

    void recreateKnobsInternal(const KnobPageGuiPtr& curPage, bool restorePageIndex);

    void onDeleteCurCmdLater();

    friend class KnobGuiContainerSignalsHandler;
    boost::scoped_ptr<KnobGuiContainerHelperPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // KNOBGUICONTAINERHELPER_H
