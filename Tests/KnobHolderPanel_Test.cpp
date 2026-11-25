/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2026 The Natron developers
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

#include "Global/Macros.h"

#include <new>

#include <gtest/gtest.h>

#include <QObject>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/DockablePanelI.h"
#include "Engine/Project.h"

NATRON_NAMESPACE_USING

namespace {
// Stands for the Gui's DockablePanel: a QObject that implements DockablePanelI
class CountingPanel
    : public QObject
    , public DockablePanelI
{
public:
    int calls = 0;

    virtual void recreateUserKnobs(bool) OVERRIDE FINAL { ++calls; }
    virtual void refreshGuiForKnobsChanges(bool) OVERRIDE FINAL { ++calls; }
    virtual void deleteKnobGui(const KnobIPtr&) OVERRIDE FINAL { ++calls; }
};
} // anonymous namespace

// The Gui may delete the settings panel of a holder that still exists (NodeGui::destroyGui()) without
// calling discardPanelPointer(): the holder must not call the deleted panel, nor another object
// created at the same address (#1618).
TEST(KnobHolderPanel, DeletedPanelIsNotUsed)
{
    ProjectPtr project = appPTR->getTopLevelInstance()->getProject();
    ASSERT_TRUE(project != nullptr);

    alignas(CountingPanel) unsigned char storage[sizeof(CountingPanel)];
    CountingPanel* panel = new (storage) CountingPanel;
    project->setPanelPointer(panel);
    project->recreateKnobs(false);
    project->recreateUserKnobs(false);
    project->deleteKnob(0, true);
    EXPECT_EQ(3, panel->calls);

    // Delete the panel, then create another one at the same address, which the holder doesn't know
    panel->~CountingPanel();
    CountingPanel* other = new (storage) CountingPanel;
    project->recreateKnobs(false);
    project->recreateUserKnobs(false);
    project->deleteKnob(0, true);
    EXPECT_EQ(0, other->calls) << "the holder used its deleted panel";

    project->discardPanelPointer();
    other->~CountingPanel();
}
