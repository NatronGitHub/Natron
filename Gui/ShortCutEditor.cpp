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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "ShortCutEditor.h"

#include <list>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QVBoxLayout>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QTextDocument>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QApplication>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/Button.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/ActionShortcuts.h"
#include "Gui/Label.h"
#include "Gui/Utils.h"

NATRON_NAMESPACE_USING

struct GuiBoundAction
{
    QTreeWidgetItem* item;
    BoundAction* action;
};

struct GuiShortCutGroup
{
    std::list<GuiBoundAction> actions;
    QTreeWidgetItem* item;
};

static QString
keybindToString(const Qt::KeyboardModifiers & modifiers,
                Qt::Key key)
{
    return makeKeySequence(modifiers, key).toString(QKeySequence::NativeText);
}

static QString
mouseShortcutToString(const Qt::KeyboardModifiers & modifiers,
                      Qt::MouseButton button)
{
    QString ret = makeKeySequence(modifiers, (Qt::Key)0).toString(QKeySequence::NativeText);

    switch (button) {
    case Qt::LeftButton:
        ret.append( QObject::tr("LeftButton") );
        break;
    case Qt::MiddleButton:
        ret.append( QObject::tr("MiddleButton") );
        break;
    case Qt::RightButton:
        ret.append( QObject::tr("RightButton") );
        break;
    default:
        break;
    }

    return ret;
}

typedef std::list<GuiShortCutGroup> GuiAppShorcuts;

///A small hack to the QTreeWidget class to make 2 fuctions public so we can use them in the ShortcutDelegate class
class HackedTreeWidget
    : public QTreeWidget
{
public:

    HackedTreeWidget(QWidget* parent)
        : QTreeWidget(parent)
    {
    }

    QModelIndex indexFromItem_natron(QTreeWidgetItem * item,
                                     int column = 0) const
    {
        return indexFromItem(item,column);
    }

    QTreeWidgetItem* itemFromIndex_natron(const QModelIndex & index) const
    {
        return itemFromIndex(index);
    }
};

struct NATRON_NAMESPACE::ShortCutEditorPrivate
{
    QVBoxLayout* mainLayout;
    HackedTreeWidget* tree;
    QWidget* shortcutGroup;
    QHBoxLayout* shortcutGroupLayout;
    Natron::Label* shortcutLabel;
    KeybindRecorder* shortcutEditor;
    
    QWidget* altShortcutGroup;
    QHBoxLayout* altShortcutGroupLayout;
    Natron::Label* altShortcutLabel;
    KeybindRecorder* altShortcutEditor;
    
    Button* validateButton;
    Button* clearButton;
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
    , shortcutEditor(0)
    , altShortcutGroup(0)
    , altShortcutGroupLayout(0)
    , altShortcutLabel(0)
    , altShortcutEditor(0)
    , validateButton(0)
    , clearButton(0)
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
        for (GuiAppShorcuts::const_iterator it = appShortcuts.begin(); it != appShortcuts.end(); ++it) {
            if (it->item == item) {
                return NULL;
            }
            for (std::list<GuiBoundAction>::const_iterator it2 = it->actions.begin(); it2 != it->actions.end(); ++it2) {
                if (it2->item == item) {
                    return it2->action;
                }
            }
        }

        return (BoundAction*)NULL;
    }
    
    GuiAppShorcuts::iterator buildGroupHierarchy(QString grouping);
    
    void makeGuiActionForShortcut(GuiAppShorcuts::iterator guiGroupIterator,BoundAction* action);
};

static void
makeItemShortCutText(const BoundAction* action,
                     bool useDefault,
                     QString* shortcutStr,
                     QString* altShortcutStr)
{
    const KeyBoundAction* ka = dynamic_cast<const KeyBoundAction*>(action);
    const MouseAction* ma = dynamic_cast<const MouseAction*>(action);

    if (ka) {
        if (useDefault) {
            if (!ka->defaultModifiers.empty()) {
                assert(ka->defaultModifiers.size() == ka->defaultShortcut.size());
                std::list<Qt::KeyboardModifiers>::const_iterator mit = ka->defaultModifiers.begin();
                std::list<Qt::Key>::const_iterator sit = ka->defaultShortcut.begin();
                *shortcutStr = keybindToString(*mit, *sit);
                if (ka->defaultShortcut.size() > 1) {
                    ++mit;
                    ++sit;
                    *altShortcutStr = keybindToString(*mit, *sit);
                }
            }
        } else {
            if (!ka->modifiers.empty()) {
                assert(ka->modifiers.size() == ka->currentShortcut.size());
                std::list<Qt::KeyboardModifiers>::const_iterator mit = ka->modifiers.begin();
                std::list<Qt::Key>::const_iterator sit = ka->currentShortcut.begin();
                *shortcutStr = keybindToString(*mit, *sit);
                if (ka->currentShortcut.size() > 1) {
                    ++mit;
                    ++sit;
                    *altShortcutStr = keybindToString(*mit, *sit);
                }
            }
        }
    } else if (ma) {
        if (useDefault) {
            if (!ma->defaultModifiers.empty()) {
                std::list<Qt::KeyboardModifiers>::const_iterator mit = ma->defaultModifiers.begin();
                *shortcutStr = mouseShortcutToString(*mit, ma->button);
                if (ma->defaultModifiers.size() > 1) {
                    ++mit;
                    *altShortcutStr = mouseShortcutToString(*mit, ma->button);
                }
            }
        } else {
            if (!ma->modifiers.empty()) {
                std::list<Qt::KeyboardModifiers>::const_iterator mit = ma->modifiers.begin();
                *shortcutStr = mouseShortcutToString(*mit, ma->button);
                if (ma->modifiers.size() > 1) {
                    ++mit;
                    *altShortcutStr = mouseShortcutToString(*mit, ma->button);
                }
            }
        }
    } else {
        assert(false);
    }

}

static void
setItemShortCutText(QTreeWidgetItem* item,
                    const BoundAction* action,
                    bool useDefault)
{
    QString sc,altSc;
    makeItemShortCutText(action,useDefault,&sc,&altSc);
    item->setText( 1,  sc);
    item->setText( 2,  altSc);
}

class ShortcutDelegate
    : public QStyledItemDelegate
{
    HackedTreeWidget* tree;

public:

    ShortcutDelegate(HackedTreeWidget* parent)
        : QStyledItemDelegate(parent)
          , tree(parent)
    {
    }

private:

    virtual void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const OVERRIDE FINAL;
};

ShortCutEditor::ShortCutEditor(QWidget* parent)
    : QWidget(parent)
      , _imp( new ShortCutEditorPrivate() )
{
    _imp->mainLayout = new QVBoxLayout(this);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Window);
    setWindowTitle( tr("Shortcuts Editor") );
    _imp->tree = new HackedTreeWidget(this);
    _imp->tree->setColumnCount(3);
    QStringList headers;
    headers << tr("Command") << tr("Shortcut") << tr("Alt. Shortcut");
    _imp->tree->setHeaderLabels(headers);
    _imp->tree->setSelectionMode(QAbstractItemView::SingleSelection);
    _imp->tree->setAttribute(Qt::WA_MacShowFocusRect,0);
    _imp->tree->setSortingEnabled(false);
    _imp->tree->setToolTip( Natron::convertFromPlainText(
                                tr("In this table is represented each action of the application that can have a possible keybind/mouse shortcut."
                                   " Note that this table also have some special assignments which also involve the mouse. "
                                   "You cannot assign a keybind to a shortcut involving the mouse and vice versa. "
                                   "Note that internally " NATRON_APPLICATION_NAME " does an emulation of a three-button mouse "
                                   "if your computer doesn't have one, that is: \n"
                                   "---> Middle mouse button is emulated by holding down Options (alt) coupled with a left click.\n "
                                   "---> Right mouse button is emulated by holding down Command (cmd) coupled with a left click."), Qt::WhiteSpaceNormal) );
    _imp->tree->setItemDelegate( new ShortcutDelegate(_imp->tree) );
    
    const AppShortcuts & appShortcuts = appPTR->getAllShortcuts();
    
    for (AppShortcuts::const_iterator it = appShortcuts.begin(); it != appShortcuts.end(); ++it) {
      
        GuiAppShorcuts::iterator foundGuiGroup = _imp->buildGroupHierarchy(it->first);
        assert(foundGuiGroup != _imp->appShortcuts.end());
        
        for (GroupShortcuts::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            _imp->makeGuiActionForShortcut(foundGuiGroup, it2->second);
        }
    }
    
    _imp->tree->resizeColumnToContents(0);
    QObject::connect( _imp->tree, SIGNAL( itemSelectionChanged() ), this, SLOT( onSelectionChanged() ) );

    _imp->mainLayout->addWidget(_imp->tree);

    _imp->shortcutGroup = new QWidget(this);
    _imp->mainLayout->addWidget(_imp->shortcutGroup);

    _imp->shortcutGroupLayout = new QHBoxLayout(_imp->shortcutGroup);
    _imp->shortcutGroupLayout->setContentsMargins(0, 0, 0, 0);

    _imp->shortcutLabel = new Natron::Label(_imp->shortcutGroup);
    _imp->shortcutLabel->setText( tr("Sequence:") );
    _imp->shortcutGroupLayout->addWidget(_imp->shortcutLabel);

    _imp->shortcutEditor = new KeybindRecorder(_imp->shortcutGroup);
    _imp->shortcutEditor->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    _imp->shortcutEditor->setPlaceholderText( tr("Type to set shortcut") );
    _imp->shortcutGroupLayout->addWidget(_imp->shortcutEditor);
    
    _imp->altShortcutGroup = new QWidget(this);
    _imp->mainLayout->addWidget(_imp->altShortcutGroup);
    
    _imp->altShortcutGroupLayout = new QHBoxLayout(_imp->altShortcutGroup);
    _imp->altShortcutGroupLayout->setContentsMargins(0, 0, 0, 0);
    
    _imp->altShortcutLabel = new Natron::Label(_imp->altShortcutGroup);
    _imp->altShortcutLabel->setText( tr("Alternative Sequence:") );
    _imp->altShortcutGroupLayout->addWidget(_imp->altShortcutLabel);
    
    _imp->altShortcutEditor = new KeybindRecorder(_imp->altShortcutGroup);
    _imp->altShortcutEditor->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    _imp->altShortcutEditor->setPlaceholderText( tr("Type to set an alternative shortcut") );
    _imp->altShortcutGroupLayout->addWidget(_imp->altShortcutEditor);
    

    _imp->validateButton = new Button(tr("Validate"),_imp->shortcutGroup);
    _imp->validateButton->setToolTip(Natron::convertFromPlainText(tr("Validates the shortcut on the field editor and set the selected shortcut."), Qt::WhiteSpaceNormal));
    _imp->shortcutGroupLayout->addWidget(_imp->validateButton);
    QObject::connect( _imp->validateButton, SIGNAL( clicked(bool) ), this, SLOT( onValidateButtonClicked() ) );

    _imp->clearButton = new Button(tr("Clear"),_imp->shortcutGroup);
    QObject::connect( _imp->clearButton, SIGNAL( clicked(bool) ), this, SLOT( onClearButtonClicked() ) );
    _imp->shortcutGroupLayout->addWidget(_imp->clearButton);

    _imp->resetButton = new Button(tr("Reset"),_imp->shortcutGroup);
    QObject::connect( _imp->resetButton, SIGNAL( clicked(bool) ), this, SLOT( onResetButtonClicked() ) );
    _imp->shortcutGroupLayout->addWidget(_imp->resetButton);

    _imp->buttonsContainer = new QWidget(this);
    _imp->buttonsLayout = new QHBoxLayout(_imp->buttonsContainer);

    _imp->mainLayout->QLayout::addWidget(_imp->buttonsContainer);

    _imp->restoreDefaultsButton = new Button(tr("Restore defaults"),_imp->buttonsContainer);
    QObject::connect( _imp->restoreDefaultsButton, SIGNAL( clicked(bool) ), this, SLOT( onRestoreDefaultsButtonClicked() ) );
    _imp->buttonsLayout->addWidget(_imp->restoreDefaultsButton);

    _imp->applyButton = new Button(tr("Apply"),_imp->buttonsContainer);
    QObject::connect( _imp->applyButton, SIGNAL( clicked(bool) ), this, SLOT( onApplyButtonClicked() ) );
    _imp->buttonsLayout->addWidget(_imp->applyButton);

    _imp->buttonsLayout->addStretch();

    _imp->cancelButton = new Button(tr("Cancel"),_imp->buttonsContainer);
    QObject::connect( _imp->cancelButton, SIGNAL( clicked(bool) ), this, SLOT( onCancelButtonClicked() ) );
    _imp->buttonsLayout->addWidget(_imp->cancelButton);

    _imp->okButton = new Button(tr("Ok"),_imp->buttonsContainer);
    QObject::connect( _imp->okButton, SIGNAL( clicked(bool) ), this, SLOT( onOkButtonClicked() ) );
    _imp->buttonsLayout->addWidget(_imp->okButton);

    _imp->buttonsLayout->addStretch();

    resize(700,400);
}

ShortCutEditor::~ShortCutEditor()
{
}

GuiAppShorcuts::iterator
ShortCutEditorPrivate::buildGroupHierarchy(QString grouping)
{
    ///Do not allow empty grouping, make them under the Global shortcut
    if ( grouping.isEmpty() ) {
        grouping = kShortcutGroupGlobal;
    }
    
    ///Groups are separated by a '/'
    QStringList groupingSplit = grouping.split( QChar('/') );
    assert(groupingSplit.size() > 0);
    const QString & lastGroupName = groupingSplit.back();
    
    ///Find out whether the deepest parent group of the action already exist
    GuiAppShorcuts::iterator foundGuiGroup = appShortcuts.end();
    for (GuiAppShorcuts::iterator it2 = appShortcuts.begin(); it2 != appShortcuts.end(); ++it2) {
        if (it2->item->text(0) == lastGroupName) {
            foundGuiGroup = it2;
            break;
        }
    }
    
    QTreeWidgetItem* groupParent;
    if ( foundGuiGroup != appShortcuts.end() ) {
        groupParent = foundGuiGroup->item;
    } else {
        groupParent = 0;
        for (int i = 0; i < groupingSplit.size(); ++i) {
            QTreeWidgetItem* groupingItem;
            bool existAlready = false;
            if (groupParent) {
                for (int j = 0; j < groupParent->childCount(); ++j) {
                    QTreeWidgetItem* child = groupParent->child(j);
                    if (child->text(0) == groupingSplit[i]) {
                        groupingItem = child;
                        existAlready = true;
                        break;
                    }
                }
                if (!existAlready) {
                    groupingItem = new QTreeWidgetItem(groupParent);
                    groupParent->addChild(groupingItem);
                }
            } else {
                for (int j = 0; j < tree->topLevelItemCount(); ++j) {
                    QTreeWidgetItem* topLvlItem = tree->topLevelItem(j);
                    if (topLvlItem->text(0) == groupingSplit[i]) {
                        groupingItem = topLvlItem;
                        existAlready = true;
                        break;
                    }
                }
                if (!existAlready) {
                    groupingItem = new QTreeWidgetItem(tree);
                    tree->addTopLevelItem(groupingItem);
                }
            }
            if (!existAlready) {
                /* expand only top-level groups */
                groupingItem->setExpanded(i == 0);
                groupingItem->setFlags(Qt::ItemIsEnabled);
                groupingItem->setText(0, groupingSplit[i]);
            }
            groupParent = groupingItem;
        }
    }
    GuiShortCutGroup group;
    group.item = groupParent;
    foundGuiGroup = appShortcuts.insert(appShortcuts.end(), group);

    return foundGuiGroup;
}

void
ShortCutEditorPrivate::makeGuiActionForShortcut(GuiAppShorcuts::iterator guiGroupIterator,BoundAction* action)
{
    GuiBoundAction guiAction;
    guiAction.action = action;
    guiAction.item = new QTreeWidgetItem(guiGroupIterator->item);
    guiAction.item->setText(0, guiAction.action->description);
    const KeyBoundAction* ka = dynamic_cast<const KeyBoundAction*>(action);
    const MouseAction* ma = dynamic_cast<const MouseAction*>(action);
    QString shortcutStr,altShortcutStr;
    if (ka) {
        if (!ka->modifiers.empty()) {
            std::list<Qt::KeyboardModifiers>::const_iterator mit = ka->modifiers.begin();
            std::list<Qt::Key>::const_iterator sit = ka->currentShortcut.begin();
            shortcutStr = keybindToString(*mit, *sit);
            if (ka->modifiers.size() > 1) {
                ++mit;
                ++sit;
                altShortcutStr = keybindToString(*mit, *sit);
            }
        }
    } else if (ma) {
        if (!ma->modifiers.empty()) {
            std::list<Qt::KeyboardModifiers>::const_iterator mit = ma->modifiers.begin();
            shortcutStr = mouseShortcutToString(*mit, ma->button);
            if (ma->modifiers.size() > 1) {
                ++mit;
                altShortcutStr = mouseShortcutToString(*mit, ma->button);
            }
        }
    } else {
        assert(false);
    }
    if (!action->editable) {
        guiAction.item->setToolTip( 0, QObject::tr("This action is standard and its shortcut cannot be edited.") );
        guiAction.item->setToolTip( 1, QObject::tr("This action is standard and its shortcut cannot be edited.") );
        guiAction.item->setDisabled(true);
    }
    guiAction.item->setExpanded(true);
    guiAction.item->setText(1, shortcutStr);
    guiAction.item->setText(2, altShortcutStr);
    guiGroupIterator->actions.push_back(guiAction);
    guiGroupIterator->item->addChild(guiAction.item);

}

void
ShortCutEditor::addShortcut(BoundAction* action)
{
    GuiAppShorcuts::iterator foundGuiGroup = _imp->buildGroupHierarchy(action->grouping);
    assert(foundGuiGroup != _imp->appShortcuts.end());
    
    _imp->makeGuiActionForShortcut(foundGuiGroup, action);

}

void
ShortCutEditor::onSelectionChanged()
{
    QList<QTreeWidgetItem*> items = _imp->tree->selectedItems();
    if (items.size() > 1) {
        //we do not support selection of more than 1 item
        return;
    }

    if ( items.empty() ) {
        _imp->shortcutEditor->setText("");
        _imp->shortcutEditor->setPlaceholderText( tr("Type to set shortcut") );
        _imp->shortcutEditor->setReadOnly(true);

        return;
    }
    QTreeWidgetItem* selection = items.front();
    if ( !selection->isDisabled() ) {
        _imp->shortcutEditor->setReadOnly(false);
        _imp->clearButton->setEnabled(true);
        _imp->resetButton->setEnabled(true);
    } else {
        _imp->shortcutEditor->setReadOnly(true);
        _imp->clearButton->setEnabled(false);
        _imp->resetButton->setEnabled(false);
    }

    BoundAction* action = _imp->getActionForTreeItem(selection);
    assert(action);
    QString sc,altSc;
    makeItemShortCutText(action, false,&sc,&altSc);
    _imp->shortcutEditor->setText(sc);
    _imp->altShortcutEditor->setText(altSc);
}

void
ShortCutEditor::onValidateButtonClicked()
{
    QString text = _imp->shortcutEditor->text();
    QString altText = _imp->altShortcutEditor->text();

    QList<QTreeWidgetItem*> items = _imp->tree->selectedItems();
    if ( (items.size() > 1) || items.empty() ) {
        return;
    }

    QTreeWidgetItem* selection = items.front();
    QKeySequence seq(text,QKeySequence::NativeText);
    QKeySequence altseq(altText,QKeySequence::NativeText);
    
    BoundAction* action = _imp->getActionForTreeItem(selection);

    QTreeWidgetItem* parent = selection->parent();
    while (parent) {
        QTreeWidgetItem* parentUp = parent->parent();
        if (!parentUp) {
            break;
        }
        parent = parentUp;
    }
    assert(parent); 
    
    //only keybinds can be edited...
    KeyBoundAction* ka = dynamic_cast<KeyBoundAction*>(action);
    assert(ka);

    Qt::KeyboardModifiers modifiers,altmodifiers;
    Qt::Key symbol,altsymbmol;
    extractKeySequence(seq, modifiers, symbol);
    extractKeySequence(altseq, altmodifiers, altsymbmol);
    
    for (GuiAppShorcuts::iterator it = _imp->appShortcuts.begin(); it != _imp->appShortcuts.end(); ++it) {
        for (std::list<GuiBoundAction>::iterator it2 = it->actions.begin(); it2 != it->actions.end(); ++it2) {
            if (it2->action != action && it->item->text(0) == parent->text(0)) {
                KeyBoundAction* keyAction = dynamic_cast<KeyBoundAction*>(it2->action);
                if (keyAction) {
                    assert(keyAction->modifiers.size() == keyAction->currentShortcut.size());
                    std::list<Qt::KeyboardModifiers>::const_iterator mit = keyAction->modifiers.begin();
                    for (std::list<Qt::Key>::const_iterator it3 = keyAction->currentShortcut.begin(); it3 != keyAction->currentShortcut.end(); ++it3,++mit) {
                        if (*mit == modifiers && *it3 == symbol) {
                            QString err = QString("Cannot bind this shortcut because the following action is already using it: %1")
                            .arg( it2->item->text(0) );
                            _imp->shortcutEditor->clear();
                            Natron::errorDialog( tr("Shortcuts Editor").toStdString(), tr( err.toStdString().c_str() ).toStdString() );
                            
                            return;
                        }
                    }
                    
                    
                }
            }
        }
    }

    selection->setText(1,text);
    action->modifiers.clear();
    if (!text.isEmpty()) {
        action->modifiers.push_back(modifiers);
        ka->currentShortcut.push_back(symbol);
    }
    if (!altText.isEmpty()) {
        action->modifiers.push_back(altmodifiers);
        ka->currentShortcut.push_back(altsymbmol);
    }
    
    appPTR->notifyShortcutChanged(ka);
}

void
ShortCutEditor::onClearButtonClicked()
{
    QList<QTreeWidgetItem*> items = _imp->tree->selectedItems();
    if (items.size() > 1) {
        //we do not support selection of more than 1 item
        return;
    }

    if ( items.empty() ) {
        _imp->shortcutEditor->setText("");
        _imp->shortcutEditor->setPlaceholderText( tr("Type to set shortcut") );
        _imp->altShortcutEditor->setText("");
        _imp->altShortcutEditor->setPlaceholderText( tr("Type to set an alternative shortcut") );
        return;
    }

    QTreeWidgetItem* selection = items.front();
    BoundAction* action = _imp->getActionForTreeItem(selection);
    assert(action);
    action->modifiers.clear();
    MouseAction* ma = dynamic_cast<MouseAction*>(action);
    KeyBoundAction* ka = dynamic_cast<KeyBoundAction*>(action);
    if (ma) {
        ma->button = Qt::NoButton;
    } else {
        assert(ka);
        ka->currentShortcut.clear();
    }

    selection->setText(1, "");
    _imp->shortcutEditor->setText("");
    _imp->shortcutEditor->setFocus();
}

void
ShortCutEditor::onResetButtonClicked()
{
    QList<QTreeWidgetItem*> items = _imp->tree->selectedItems();
    if (items.size() > 1) {
        //we do not support selection of more than 1 item
        return;
    }

    if ( items.empty() ) {
        _imp->shortcutEditor->setText("");
        _imp->shortcutEditor->setPlaceholderText( tr("Type to set shortcut") );

        return;
    }

    QTreeWidgetItem* selection = items.front();
    BoundAction* action = _imp->getActionForTreeItem(selection);
    assert(action);
    action->modifiers = action->defaultModifiers;
    KeyBoundAction* ka = dynamic_cast<KeyBoundAction*>(action);
    if (ka) {
        ka->currentShortcut = ka->defaultShortcut;
        appPTR->notifyShortcutChanged(ka);
    }
    setItemShortCutText(selection, action, true);
    
    QString sc,altsc;
    makeItemShortCutText(action, true,&sc,&altsc);
    _imp->shortcutEditor->setText(sc);
    _imp->altShortcutEditor->setText(altsc);
}

void
ShortCutEditor::onRestoreDefaultsButtonClicked()
{
    StandardButtonEnum reply = Natron::questionDialog( tr("Restore defaults").toStdString(), tr("Restoring default shortcuts "
                                                                                                    "will wipe all the current configuration "
                                                                                                    "are you sure you want to do this?").toStdString(), false );

    if (reply == Natron::eStandardButtonYes) {
        appPTR->restoreDefaultShortcuts();
        for (GuiAppShorcuts::const_iterator it = _imp->appShortcuts.begin(); it != _imp->appShortcuts.end(); ++it) {
            for (std::list<GuiBoundAction>::const_iterator it2 = it->actions.begin(); it2 != it->actions.end(); ++it2) {
                setItemShortCutText(it2->item, it2->action, true);
            }
        }
        _imp->tree->clearSelection();
    }
}

void
ShortCutEditor::onApplyButtonClicked()
{
    appPTR->saveShortcuts();
}

void
ShortCutEditor::onCancelButtonClicked()
{
    close();
}

void
ShortCutEditor::onOkButtonClicked()
{
    appPTR->saveShortcuts();
    close();
}

void
ShortCutEditor::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape) {
        close();
    } else {
        QWidget::keyPressEvent(e);
    }
}

void
ShortcutDelegate::paint(QPainter * painter,
                        const QStyleOptionViewItem & option,
                        const QModelIndex & index) const
{
    QTreeWidgetItem* item = tree->itemFromIndex_natron(index);

    if (!item) {
        QStyledItemDelegate::paint(painter, option, index);

        return;
    }

    ///Determine whether the item is top level or not
    bool isTopLevel = false;
    int topLvlCount = tree->topLevelItemCount();
    for (int i = 0; i < topLvlCount; ++i) {
        if (tree->topLevelItem(i) == item) {
            isTopLevel = true;
            break;
        }
    }

    QFont font = painter->font();
    QPen pen;

    if (isTopLevel) {
        font.setBold(true);
        font.setPixelSize(13);
        pen.setColor(Qt::black);
    } else {
        font.setBold(false);
        font.setPixelSize(11);
        if ( item->isDisabled() ) {
            pen.setColor(Qt::black);
        } else {
            pen.setColor( QColor(200,200,200) );
        }
    }
    painter->setFont(font);
    painter->setPen(pen);

    // get the proper subrect from the style
    QStyle *style = QApplication::style();
    QRect geom = style->subElementRect(QStyle::SE_ItemViewItemText, &option);

    ///Draw the item name column
    if (option.state & QStyle::State_Selected) {
        painter->fillRect( geom, option.palette.highlight() );
    }
    QRect r;
    painter->drawText(geom,Qt::TextSingleLine,item->data(index.column(), Qt::DisplayRole).toString(),&r);
} // paint

KeybindRecorder::KeybindRecorder(QWidget* parent)
    : LineEdit(parent)
{
}

KeybindRecorder::~KeybindRecorder()
{
}

void
KeybindRecorder::keyPressEvent(QKeyEvent* e)
{
    if ( !isEnabled() || isReadOnly() ) {
        return;
    }

    QKeySequence *seq;
    if (e->key() == Qt::Key_Control) {
        seq = new QKeySequence(Qt::CTRL);
    } else if (e->key() == Qt::Key_Shift) {
        seq = new QKeySequence(Qt::SHIFT);
    } else if (e->key() == Qt::Key_Meta) {
        seq = new QKeySequence(Qt::META);
    } else if (e->key() == Qt::Key_Alt) {
        seq = new QKeySequence(Qt::ALT);
    } else {
        seq = new QKeySequence( e->key() );
    }

    QString seqStr = seq->toString(QKeySequence::NativeText);
    delete seq;
    QString txt = text()  + seqStr;
    setText(txt);
}


#include "moc_ShortCutEditor.cpp"
