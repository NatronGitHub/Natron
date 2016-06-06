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

#ifndef KNOBGUICONTAINERHELPER_H
#define KNOBGUICONTAINERHELPER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <map>

#include <QObject>

#include "Engine/DockablePanelI.h"
#include "Gui/GuiFwd.h"
#include "Gui/KnobGuiContainerI.h"

NATRON_NAMESPACE_ENTER;

typedef std::list<std::pair<boost::weak_ptr<KnobI>, KnobGuiPtr> > KnobsGuiMapping;

struct KnobPageGui 
{

    QWidget* tab;
    int currentRow;
    TabGroup* groupAsTab; //< to gather group knobs that are set as a tab
    boost::weak_ptr<KnobPage> pageKnob;
    QGridLayout* gridLayout;

    KnobPageGui()
    : tab(0)
    , currentRow(0)
    , groupAsTab(0)
    , pageKnob()
    , gridLayout(0)
    {
    }

};

typedef boost::shared_ptr<KnobPageGui> KnobPageGuiPtr;
typedef std::map<boost::weak_ptr<KnobPage>, KnobPageGuiPtr> PagesMap;

class KnobGuiContainerSignalsHandler : public QObject
{
    Q_OBJECT

public:


    KnobGuiContainerSignalsHandler(KnobGuiContainerHelper* container)
    : QObject()
    , _container(container)
    {
        QObject::connect(this, SIGNAL(deleteCurCmdLater()), this, SLOT(onDeleteCurCmdLaterTriggered()), Qt::QueuedConnection);

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

    void onDeleteCurCmdLaterTriggered();

private:

    KnobGuiContainerHelper* _container;

};

struct KnobGuiContainerHelperPrivate;
class KnobGuiContainerHelper : public KnobGuiContainerI, public DockablePanelI

{

public:

    KnobGuiContainerHelper(KnobHolder* holder, const boost::shared_ptr<QUndoStack>& stack);

    virtual ~KnobGuiContainerHelper();

    void initializeKnobs();

    void initializeKnobVector(const KnobsVec& knobs);

    const PagesMap& getPages() const;

    void getUserPages(std::list<KnobPage*>& userPages) const;

    void setUserPageActiveIndex();

    void setPageActiveIndex(const boost::shared_ptr<KnobPage>& page);

    boost::shared_ptr<KnobPage> getOrCreateUserPageKnob() const;

    boost::shared_ptr<KnobPage> getUserPageKnob() const;

    int getPagesCount() const;

    KnobPageGuiPtr getCurrentPage() const;

    const KnobsVec& getInternalKnobs() const;

    const KnobsGuiMapping& getKnobsMapping() const;

    boost::shared_ptr<QUndoStack> getUndoStack() const;

    virtual bool isPagingEnabled() const
    {
        return true;
    }

    virtual bool useScrollAreaForTabs() const
    {
        return false;
    }

    //// Overriden from DockablePanelI
    virtual void deleteKnobGui(const KnobPtr& knob) OVERRIDE FINAL;

    virtual void recreateUserKnobs(bool restorePageIndex) OVERRIDE FINAL;

    virtual void refreshGuiForKnobsChanges(bool restorePageIndex) OVERRIDE FINAL;
    ///// End override from DockablePanelI

    //// Overriden from KnobGuiContainerI
    virtual Gui* getGui() const OVERRIDE = 0;

    virtual const QUndoCommand* getLastUndoCommand() const OVERRIDE FINAL;

    virtual void pushUndoCommand(QUndoCommand* cmd) OVERRIDE FINAL;

    virtual KnobGuiPtr getKnobGui(const KnobPtr& knob) const OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual int getItemsSpacingOnSameLine() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    ///// End override from KnobGuiContainerI
protected:

    void setCurrentPage(const KnobPageGuiPtr& curPage);

    virtual void onKnobsInitialized() {}

    virtual void onPageActivated(const KnobPageGuiPtr& page) = 0;

    virtual void refreshCurrentPage() = 0;

    virtual QWidget* getPagesContainer() const = 0;

    virtual QWidget* createPageMainWidget(QWidget* parent) const = 0;

    virtual void addPageToPagesContainer(const KnobPageGuiPtr& page) = 0;

    virtual void removePageFromContainer(const KnobPageGuiPtr& page) = 0;

    virtual void refreshUndoRedoButtonsEnabledNess(bool canUndo, bool canRedo) = 0;

    virtual void setPagesOrder(const std::list<KnobPageGuiPtr>& order, const KnobPageGuiPtr& curPage, bool restorePageIndex) = 0;

    virtual void onKnobsRecreated() {}

    virtual void onPageLabelChanged(const KnobPageGuiPtr& page) = 0;


    KnobPageGuiPtr getOrCreatePage(const boost::shared_ptr<KnobPage>& page);

    KnobPageGuiPtr getOrCreateDefaultPage();


private:


    void refreshPagesOrder(const KnobPageGuiPtr& curTabName, bool restorePageIndex);

    void clearUndoRedoStack();

    KnobGuiPtr findKnobGuiOrCreate(const KnobPtr & knob,
                                   bool makeNewLine,
                                   QWidget* lastRowWidget,
                                   const KnobsVec& knobsOnSameLine);

    void initializeKnobVectorInternal(const KnobsVec& siblingsVec, KnobsVec* regularKnobsVec);

    void recreateKnobsInternal(const KnobPageGuiPtr& curPage, bool restorePageIndex);

    void onDeleteCurCmdLater();

    friend class KnobGuiContainerSignalsHandler;
    boost::scoped_ptr<KnobGuiContainerHelperPrivate> _imp;

};

NATRON_NAMESPACE_EXIT;

#endif // KNOBGUICONTAINERHELPER_H
