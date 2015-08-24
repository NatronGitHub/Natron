//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef _Gui_DockablePanel_h_
#define _Gui_DockablePanel_h_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <map>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QFrame>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif
#include <QTabWidget>
#include <QDialog>
#include "Global/GlobalDefines.h"

#include "Engine/DockablePanelI.h"

class KnobI;
class KnobGui;
class KnobHolder;
class NodeGui;
class Gui;
class Page_Knob;
class QVBoxLayout;
class Button;
class QUndoStack;
class QUndoCommand;
class QGridLayout;
class RotoPanel;
class MultiInstancePanel;
class QTabWidget;
class Group_Knob;

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
    
    void resetDefaultOverlayColor();

    virtual boost::shared_ptr<MultiInstancePanel> getMultiInstancePanel() const
    {
        return boost::shared_ptr<MultiInstancePanel>();
    }

    KnobHolder* getHolder() const;

    void onGuiClosing();

    virtual void scanForNewKnobs() OVERRIDE FINAL;
    
    void setUserPageActiveIndex();
    
    boost::shared_ptr<Page_Knob> getUserPageKnob() const;
    
    void getUserPages(std::list<Page_Knob*>& userPages) const;
    
    void deleteKnobGui(const boost::shared_ptr<KnobI>& knob);
    
    int getPagesCount() const;
        
    /**
     * @brief When called, all knobs will go into the same page which will appear as a plain Widget and not as a tab
     **/
    void turnOffPages();
    
    void setPluginIcon(const QPixmap& pix) ;
    
    void setPluginDescription(const std::string& description) ;

    void setPluginIDAndVersion(const std::string& pluginLabel,const std::string& pluginID,unsigned int version);
    
public Q_SLOTS:

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

#endif // _Gui_DockablePanel_h_
