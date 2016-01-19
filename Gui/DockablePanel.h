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

#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER;

/**
 * @brief An abstract class that defines a dockable properties panel that can be found in the Property bin pane.
 **/
struct DockablePanelPrivate;
class DockablePanel
    : public QFrame
    , public DockablePanelI
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
                           KnobHolder* holder,
                           QVBoxLayout* container,
                           HeaderModeEnum headerMode,
                           bool useScrollAreasForTabs,
                           const boost::shared_ptr<QUndoStack>& stack,
                           const QString & initialName = QString(),
                           const QString & helpToolTip = QString(),
                           bool createDefaultPage = false,
                           const QString & defaultPageName = QString("Default"),
                           QWidget *parent = 0);

    virtual ~DockablePanel() OVERRIDE;

    bool isMinimized() const;

    const std::map<boost::weak_ptr<KnobI>,KnobGui*> & getKnobs() const;
    QVBoxLayout* getContainer() const;
    boost::shared_ptr<QUndoStack> getUndoStack() const;

    bool isClosed() const;
    bool isFloating() const;

    /*Creates a new button and inserts it in the header
       at position headerPosition. You can then take
       the pointer to the Button to customize it the
       way you want. The ownership of the Button remains
       to the DockablePanel.*/
    Button* insertHeaderButton(int headerPosition);


    void pushUndoCommand(QUndoCommand* cmd);


    const QUndoCommand* getLastUndoCommand() const;
    Gui* getGui() const;

    void insertHeaderWidget(int index,QWidget* widget);

    void appendHeaderWidget(QWidget* widget);

    QWidget* getHeaderWidget() const;
    KnobGui* getKnobGui(const boost::shared_ptr<KnobI> & knob) const;

    ///MT-safe
    virtual QColor getCurrentColor() const {
        return Qt::black;
    }

    ///MT-safe
    void setCurrentColor(const QColor & c);
    
    void setOverlayColor(const QColor& c);
    
    QColor getOverlayColor() const;
    
    bool hasOverlayColor() const;
    
    void resetHostOverlayColor();

    virtual boost::shared_ptr<MultiInstancePanel> getMultiInstancePanel() const
    {
        return boost::shared_ptr<MultiInstancePanel>();
    }

    KnobHolder* getHolder() const;

    void onGuiClosing();

    virtual void scanForNewKnobs(bool restorePageIndex = true) OVERRIDE FINAL;
    
    void setUserPageActiveIndex();
    
    void setPageActiveIndex(const boost::shared_ptr<KnobPage>& page);
    
    boost::shared_ptr<KnobPage> getOrCreateUserPageKnob() const;
    boost::shared_ptr<KnobPage> getUserPageKnob() const;
    
    void getUserPages(std::list<KnobPage*>& userPages) const;
    
    virtual void deleteKnobGui(const boost::shared_ptr<KnobI>& knob) OVERRIDE FINAL;
    
    int getPagesCount() const;
        
    /**
     * @brief When called, all knobs will go into the same page which will appear as a plain Widget and not as a tab
     **/
    void turnOffPages();
    
    void setPluginIcon(const QPixmap& pix) ;
    
    void setPluginDescription(const std::string& description) ;

    void setPluginIDAndVersion(const std::string& pluginLabel,const std::string& pluginID,unsigned int version);
    
    void refreshTabWidgetMaxHeight();
    
public Q_SLOTS:
    
    void onPageLabelChangedInternally();
    
    void onPageIndexChanged(int index);

    /*Internal slot, not meant to be called externally.*/
    void closePanel();

    /*Internal slot, not meant to be called externally.*/
    void minimizeOrMaximize(bool toggled);

    /*Internal slot, not meant to be called externally.*/
    void showHelp();

    ///Set the name on the line-edit/label header
    void setName(const QString & str);
    
    /*initializes the knobs GUI and also the roto context if any*/
    void initializeKnobs();

    /*Internal slot, not meant to be called externally.*/
    void onUndoClicked();

    /*Internal slot, not meant to be called externally.*/
    void onRedoPressed();

    void onRestoreDefaultsButtonClicked();

    void onLineEditNameEditingFinished();

    void floatPanel();

    void onColorButtonClicked();
    
    void onOverlayButtonClicked();

    void onColorDialogColorChanged(const QColor & color);
    
    void onOverlayColorDialogColorChanged(const QColor& color);

    void setClosed(bool closed);

    void onRightClickMenuRequested(const QPoint & pos);
    
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

    virtual RotoPanel* initializeRotoPanel()
    {
        return NULL;
    }

    virtual void initializeExtraGui(QVBoxLayout* /*layout*/)
    {
    }

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

NATRON_NAMESPACE_EXIT;

#endif // Gui_DockablePanel_h
