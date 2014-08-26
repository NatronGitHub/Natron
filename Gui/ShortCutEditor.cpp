//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "ShortCutEditor.h"

#include <list>
#include <QVBoxLayout>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>

#include "Gui/Button.h"
#include "Gui/LineEdit.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/ActionShortcuts.h"

struct GuiBoundAction {
    QTreeWidgetItem* item;
    BoundAction* action;
};

struct GuiShortCutGroup {
    std::list<GuiBoundAction> actions;
    QTreeWidgetItem* item;
};

static QString keybindToString(const Qt::KeyboardModifiers& modifiers,Qt::Key key)
{
    return makeKeySequence(modifiers, key).toString(QKeySequence::NativeText);
}

static QString mouseShortcutToString(const Qt::KeyboardModifiers& modifiers,Qt::MouseButton button)
{
    QString ret = makeKeySequence(modifiers, (Qt::Key)0).toString(QKeySequence::NativeText);
    ret.append('+');
    switch (button) {
        case Qt::LeftButton:
            ret.append(QObject::tr("LeftButton"));
            break;
        case Qt::MiddleButton:
            ret.append(QObject::tr("Wheel"));
            break;
        case Qt::RightButton:
            ret.append(QObject::tr("RightButton"));
            break;
        default:
            break;
    }
    return ret;
}

typedef std::list<GuiShortCutGroup> GuiAppShorcuts;

struct ShortCutEditorPrivate
{
    QVBoxLayout* mainLayout;
    QTreeWidget* tree;
    
    QGroupBox* shortcutGroup;
    QHBoxLayout* shortcutGroupLayout;
    QLabel* shortcutLabel;
    LineEdit* shortcutEditor;
    Button* resetButton;
    
    QWidget* buttonsContainer;
    QHBoxLayout* buttonsLayout;
    Button* restoreDefaultsButton;
    Button* applyButton;
    Button* cancelButton;
    Button* okButton;
    
    GuiAppShorcuts appShortcuts;
    
    ShortCutEditorPrivate()
    : mainLayout(0)
    , tree(0)
    , shortcutGroup(0)
    , shortcutGroupLayout(0)
    , shortcutLabel(0)
    , resetButton(0)
    , buttonsContainer(0)
    , buttonsLayout(0)
    , restoreDefaultsButton(0)
    , applyButton(0)
    , cancelButton(0)
    , okButton(0)
    , appShortcuts()
    {
        
    }
    
    BoundAction* getActionForTreeItem(QTreeWidgetItem* item) const
    {
        for (GuiAppShorcuts::const_iterator it = appShortcuts.begin();it!=appShortcuts.end();++it) {
            if (it->item == item) {
                return NULL;
            }
            for (std::list<GuiBoundAction>::const_iterator it2 = it->actions.begin();it2 != it->actions.end();++it2) {
                if (it2->item == item) {
                    return it2->action;
                }
            }
        }
        return (BoundAction*)NULL;
    }
    
};

static void setItemShortCutText(QTreeWidgetItem* item,const BoundAction* action,bool useDefault)
{
    const KeyBoundAction* ka = dynamic_cast<const KeyBoundAction*>(action);
    const MouseAction* ma = dynamic_cast<const MouseAction*>(action);
    QString shortcutStr;
    if (ka) {
        if (useDefault) {
            shortcutStr = keybindToString(ka->defaultModifiers, ka->defaultShortcut);
        } else {
            shortcutStr = keybindToString(ka->modifiers, ka->currentShortcut);
        }
    } else if (ma) {
        if (useDefault) {
            shortcutStr = mouseShortcutToString(ma->defaultModifiers, ma->button);
        } else {
            shortcutStr = mouseShortcutToString(ma->modifiers, ma->button);
        }
    } else {
        assert(false);
    }
    item->setText(1, shortcutStr);
}

ShortCutEditor::ShortCutEditor(QWidget* parent)
: QWidget(parent)
, _imp(new ShortCutEditorPrivate())
{
    _imp->mainLayout = new QVBoxLayout(this);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Window);
    setWindowTitle(tr("Shortcuts editor"));
    _imp->tree = new QTreeWidget(this);
    _imp->tree->setItemsExpandable(false);
    _imp->tree->setColumnCount(2);
    QStringList headers;
    headers << tr("Command") << tr("Shortcut");
    _imp->tree->setHeaderLabels(headers);
    _imp->tree->setSelectionMode(QAbstractItemView::SingleSelection);
    
    const AppShortcuts& appShortcuts = appPTR->getAllShortcuts();
    for (AppShortcuts::const_iterator it = appShortcuts.begin(); it!= appShortcuts.end(); ++it) {
        GuiShortCutGroup group;
        group.item = new QTreeWidgetItem(_imp->tree);
        for (GroupShortcuts::const_iterator it2 = it->second.begin(); it2!=it->second.end(); ++it2) {
            GuiBoundAction action;
            action.action = it2->second;
            action.item = new QTreeWidgetItem(group.item);
            action.item->setText(0, action.action->description);
            const KeyBoundAction* ka = dynamic_cast<const KeyBoundAction*>(it2->second);
            const MouseAction* ma = dynamic_cast<const MouseAction*>(it2->second);
            QString shortcutStr;
            if (ka) {
                shortcutStr = keybindToString(ka->modifiers, ka->currentShortcut);
            } else if (ma) {
                shortcutStr = mouseShortcutToString(ma->modifiers, ma->button);
            } else {
                assert(false);
            }
            if (!it2->second->editable) {
                action.item->setDisabled(true);
            }
            action.item->setExpanded(true);
            action.item->setText(1, shortcutStr);
            group.actions.push_back(action);
        }
        _imp->appShortcuts.push_back(group);
        group.item->setFlags(Qt::ItemIsEnabled);
        _imp->tree->addTopLevelItem(group.item);
    }
    QObject::connect(_imp->tree, SIGNAL(itemSelectionChanged()), this, SLOT(onSelectionChanged()));
    
    _imp->mainLayout->addWidget(_imp->tree);
    
    _imp->shortcutGroup = new QGroupBox(tr("Shortcut"),this);
    _imp->mainLayout->addWidget(_imp->shortcutGroup);
    
    _imp->shortcutGroupLayout = new QHBoxLayout(_imp->shortcutGroup);
    _imp->shortcutGroupLayout->setContentsMargins(0, 0, 0, 0);
    
    _imp->shortcutLabel = new QLabel(_imp->shortcutGroup);
    _imp->shortcutLabel->setText(tr("Sequence:"));
    _imp->shortcutGroupLayout->addWidget(_imp->shortcutLabel);
    
    _imp->shortcutEditor = new LineEdit(_imp->shortcutGroup);
    _imp->shortcutEditor->setPlaceholderText(tr("Type to set shortcut"));
    _imp->shortcutGroupLayout->addWidget(_imp->shortcutEditor);
    
    _imp->resetButton = new Button(tr("Reset"),_imp->shortcutGroup);
    QObject::connect(_imp->resetButton, SIGNAL(clicked(bool)), this, SLOT(onResetButtonClicked()));
    _imp->shortcutGroupLayout->addWidget(_imp->resetButton);
    _imp->shortcutGroupLayout->addStretch();
    
    _imp->buttonsContainer = new QWidget(this);
    _imp->buttonsLayout = new QHBoxLayout(_imp->buttonsContainer);
    
    _imp->mainLayout->QLayout::addWidget(_imp->buttonsContainer);
    
    _imp->restoreDefaultsButton = new Button(tr("Restore defaults"),_imp->buttonsContainer);
    QObject::connect(_imp->restoreDefaultsButton, SIGNAL(clicked(bool)), this, SLOT(onRestoreDefaultsButtonClicked()));
    _imp->buttonsLayout->addWidget(_imp->restoreDefaultsButton);
    
    _imp->applyButton = new Button(tr("Apply"),_imp->buttonsContainer);
    QObject::connect(_imp->applyButton, SIGNAL(clicked(bool)), this, SLOT(onApplyButtonClicked()));
    _imp->buttonsLayout->addWidget(_imp->applyButton);
    
    _imp->buttonsLayout->addStretch();
    
    _imp->cancelButton = new Button(tr("Cancel"),_imp->buttonsContainer);
    QObject::connect(_imp->cancelButton, SIGNAL(clicked(bool)), this, SLOT(onCancelButtonClicked()));
    _imp->buttonsLayout->addWidget(_imp->cancelButton);
    
    _imp->okButton = new Button(tr("Ok"),_imp->buttonsContainer);
    QObject::connect(_imp->okButton, SIGNAL(clicked(bool)), this, SLOT(onOkButtonClicked()));
    _imp->buttonsLayout->addWidget(_imp->okButton);
    
    _imp->buttonsLayout->addStretch();
}

ShortCutEditor::~ShortCutEditor()
{
    
}

void ShortCutEditor::onSelectionChanged()
{
    QList<QTreeWidgetItem*> items = _imp->tree->selectedItems();
    if (items.size() > 1) {
        //we do not support selection of more than 1 item
        return;
    }
    
    if (items.empty()) {
        _imp->shortcutEditor->setText("");
        _imp->shortcutEditor->setPlaceholderText(tr("Type to set shortcut"));
        return;
    }
    
    QTreeWidgetItem* selection = items.front();
    BoundAction* action = _imp->getActionForTreeItem(selection);
    assert(action);
}

void ShortCutEditor::onResetButtonClicked()
{
    QList<QTreeWidgetItem*> items = _imp->tree->selectedItems();
    if (items.size() > 1) {
        //we do not support selection of more than 1 item
        return;
    }
    
    if (items.empty()) {
        _imp->shortcutEditor->setText("");
        _imp->shortcutEditor->setPlaceholderText(tr("Type to set shortcut"));
        return;
    }
    
    QTreeWidgetItem* selection = items.front();
    BoundAction* action = _imp->getActionForTreeItem(selection);
    assert(action);
    setItemShortCutText(selection, action, true);

}

void ShortCutEditor::onRestoreDefaultsButtonClicked()
{
    Natron::StandardButton reply = Natron::questionDialog(tr("Restore defaults").toStdString(), tr("Restoring default shortcuts "
                                                                                                   "will wipe all the current configuration "
                                                                                                   "are you sure you want to do this?").toStdString());
    if (reply == Natron::Yes) {
        appPTR->restoreDefaultShortcuts();
        for (GuiAppShorcuts::const_iterator it = _imp->appShortcuts.begin();it!=_imp->appShortcuts.end();++it) {
            for (std::list<GuiBoundAction>::const_iterator it2 = it->actions.begin();it2 != it->actions.end();++it2) {
                setItemShortCutText(it2->item, it2->action, true);
            }
        }
        _imp->tree->clearSelection();
    }
}

void ShortCutEditor::onApplyButtonClicked()
{
    appPTR->saveShortcuts();
}

void ShortCutEditor::onCancelButtonClicked()
{
    close();
}

void ShortCutEditor::onOkButtonClicked()
{
    appPTR->saveShortcuts();
    close();
}
