//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef GUIAPPINSTANCE_H
#define GUIAPPINSTANCE_H

#include "Engine/AppInstance.h"

#include "Global/Macros.h"

class NodeGui;

class Gui;

struct GuiAppInstancePrivate;
class GuiAppInstance : public AppInstance
{
    Q_OBJECT
    
public:
    
    GuiAppInstance(int appID);
    
    virtual ~GuiAppInstance();
    
    virtual void load(const QString& projectName = QString(),const QStringList& writers = QStringList()) OVERRIDE FINAL;
    
    Gui* getGui() const WARN_UNUSED_RETURN;
    
    //////////
    NodeGui* getNodeGui(Natron::Node* n) const WARN_UNUSED_RETURN;
    
    NodeGui* getNodeGui(const std::string& nodeName) const WARN_UNUSED_RETURN;
    
    Natron::Node* getNode(NodeGui* n) const WARN_UNUSED_RETURN;
    //////////
    
    virtual bool shouldRefreshPreview() const OVERRIDE FINAL;
    
    virtual void errorDialog(const std::string& title,const std::string& message) const OVERRIDE FINAL;
    
    virtual void warningDialog(const std::string& title,const std::string& message) const OVERRIDE FINAL;
    
    virtual void informationDialog(const std::string& title,const std::string& message) const OVERRIDE FINAL;
    
    virtual Natron::StandardButton questionDialog(const std::string& title,const std::string& message,Natron::StandardButtons buttons =
                                    Natron::StandardButtons(Natron::Yes | Natron::No),
                                    Natron::StandardButton defaultButton = Natron::NoButton) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual void loadProjectGui(boost::archive::xml_iarchive& archive) const OVERRIDE FINAL;
    
    virtual void saveProjectGui(boost::archive::xml_oarchive& archive) OVERRIDE FINAL;
    
    virtual void notifyRenderProcessHandlerStarted(const QString& sequenceName,
                                                   int firstFrame,int lastFrame,
                                                   ProcessHandler* process) OVERRIDE FINAL;
    
    virtual void setupViewersForViews(int viewsCount) OVERRIDE FINAL;
    
    void setViewersCurrentView(int view);

public slots:
    
    void onProjectNodesCleared();
private:
    
    virtual void createNodeGui(Natron::Node* node,bool loadRequest,bool openImageFileDialog) OVERRIDE FINAL;
    
    virtual void startRenderingFullSequence(Natron::OutputEffectInstance* writer) OVERRIDE FINAL;
    
    boost::scoped_ptr<GuiAppInstancePrivate> _imp;
    
};

#endif // GUIAPPINSTANCE_H
