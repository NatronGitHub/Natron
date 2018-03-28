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

#include "EffectInstancePrivate.h"

#include "Engine/AppInstance.h"
#include "Engine/Backdrop.h"
#include "Engine/Project.h"
#include "Engine/FileSystemModel.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/GroupInput.h"
#include "Engine/Node.h"
#include "Engine/NodeGuiI.h"
#include "Engine/KnobItemsTable.h"
#include "Engine/RotoPaint.h"
#include "Engine/ReadNode.h"
#include "Engine/RotoDrawableItem.h"
#include "Engine/RenderQueue.h"
#include "Engine/StubNode.h"
#include "Engine/WriteNode.h"
#include <SequenceParsing.h>

NATRON_NAMESPACE_ENTER


void
EffectInstance::createMaskSelectors(const std::vector<std::pair<bool, bool> >& hasMaskChannelSelector,
                                    const std::vector<std::string>& inputLabels,
                                    const KnobPagePtr& mainPage,
                                    bool addNewLineOnLastMask,
                                    KnobIPtr* lastKnobCreated)
{
    assert( hasMaskChannelSelector.size() == inputLabels.size() );

    for (std::size_t i = 0; i < hasMaskChannelSelector.size(); ++i) {
        if (!hasMaskChannelSelector[i].first) {
            continue;
        }
        MaskSelector sel;

        {
            std::string name;
            if (hasMaskChannelSelector[i].second) {
                name = (std::string(kEnableMaskKnobName) + "_" + inputLabels[i]);
            } else {
                name = (std::string(kEnableInputKnobName) + "_" + inputLabels[i]);
            }


            KnobBoolPtr param = createKnob<KnobBool>(name);
            param->setLabel(inputLabels[i]);
            QString hint;
            if (hasMaskChannelSelector[i].second) {
                hint =  tr("Enable the mask to come from the channel named by the choice parameter on the right. "
                           "Turning this off will act as though the mask was disconnected.");
            } else {
                hint = tr("Enable the image to come from the channel named by the choice parameter on the right. "
                          "Turning this off will act as though the input was disconnected.");
            }
            param->setHintToolTip(hint);
            param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
            param->setDefaultValue(false);
            param->setAddNewLine(false);
            param->setAnimationEnabled(false);
            if (mainPage) {
                mainPage->addKnob(param);
            }

            sel.enabled = param;
            if (!*lastKnobCreated) {
                *lastKnobCreated = param;
            }
        }


        {
            std::string name;
            if (hasMaskChannelSelector[i].second) {
                name = (std::string(kMaskChannelKnobName) + "_" + inputLabels[i]);
            } else {
                name = (std::string(kInputChannelKnobName) + "_" + inputLabels[i]);
            }

            KnobChoicePtr param = createKnob<KnobChoice>(name);
            // By default if connected it should be filled with None, Color.R, Color.G, Color.B, Color.A (@see refreshChannelSelectors)
            param->setLabel(QString());
            param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
            param->setDefaultValue(4);
            param->setAnimationEnabled(false);
            param->setIsMetadataSlave(true);
            param->setHintToolTip( tr("Use this channel from the original input to mix the output with the original input. "
                                      "Setting this to None is the same as disconnecting the input.") );

            sel.channel = param;
            if (mainPage) {
                mainPage->addKnob(param);
            }


            //Make sure the first default param in the vector is MaskInvert

            if (!addNewLineOnLastMask) {
                //If there is a MaskInvert parameter, make it on the same line as the Mask channel parameter
                param->setAddNewLine(false);
            }
        }
        
        
        _imp->defKnobs->maskSelectors[i] = sel;
    } // for (int i = 0; i < inputsCount; ++i) {
} // Node::createMaskSelectors


void
EffectInstance::createChannelSelector(int inputNb,
                                      const std::string & inputName,
                                      bool isOutput,
                                      const KnobPagePtr& page,
                                      KnobIPtr* lastKnobBeforeAdvancedOption)
{
    ChannelSelector sel;

    std::string name;
    if (!isOutput) {
        name = inputName + std::string("_") + std::string(kOutputChannelsKnobName);
    } else {
        name = kOutputChannelsKnobName;
    }

    {
        KnobChoicePtr param = createKnob<KnobChoice>(name);
        param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
        QString label,  hint;
        if (isOutput) {
            label =  tr("Output Plane");
            hint = tr("Select here the plane onto which the processing should occur.");
        } else {
            label =  tr("%1 Plane").arg( QString::fromUtf8( inputName.c_str() ) );
            hint = tr("Select here the plane that will be used in input %1.").arg( QString::fromUtf8( inputName.c_str() ));
        }

        if (isOutput) {
            param->setNewOptionCallback(&Node::choiceParamAddLayerCallback);
        }
        param->setLabel(label);
        param->setHintToolTip(hint);
        param->setAnimationEnabled(false);
        param->setSecret(!isOutput);
        param->setIsMetadataSlave(true);
        page->addKnob(param);
        if (isOutput) {
            param->setAddNewLine(false);
        }
        param->setDefaultValue(isOutput ? 0 : 1);


        sel.layer = param;


        if (!*lastKnobBeforeAdvancedOption) {
            *lastKnobBeforeAdvancedOption = param;
        }
    }



    if (isOutput) {
        KnobBoolPtr param = createKnob<KnobBool>(kNodeParamProcessAllLayers);
        param->setLabel(tr(kNodeParamProcessAllLayersLabel));
        param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
        param->setHintToolTip(tr(kNodeParamProcessAllLayersHint));
        param->setAnimationEnabled(false);
        param->setIsMetadataSlave(true);
        page->addKnob(param);

        // If the effect wants by default to render all planes set default value
        if ( isOutput && (getPlanePassThrough() == ePassThroughRenderAllRequestedPlanes) ) {
            param->setDefaultValue(true);
            //Hide all other input selectors if choice is All in output
            for (std::map<int, ChannelSelector>::iterator it = _imp->defKnobs->channelsSelectors.begin(); it != _imp->defKnobs->channelsSelectors.end(); ++it) {
                it->second.layer.lock()->setSecret(true);
            }
        }

        _imp->defKnobs->processAllLayersKnob = param;
    }

    _imp->defKnobs->channelsSelectors[inputNb] = sel;
} // createChannelSelector

void
EffectInstance::findPluginFormatKnobs()
{
    findPluginFormatKnobs(getKnobs(), true);
}

void
EffectInstance::findRightClickMenuKnob(const KnobsVec& knobs)
{
    NodePtr node = getNode();
    for (std::size_t i = 0; i < knobs.size(); ++i) {
        if (knobs[i]->getName() == kNatronOfxParamRightClickMenu) {
            KnobIPtr rightClickKnob = knobs[i];
            KnobChoicePtr isChoice = toKnobChoice(rightClickKnob);
            if (isChoice) {
                QObject::connect( isChoice.get(), SIGNAL(populated()), node.get(), SIGNAL(rightClickMenuKnobPopulated()) );
            }
            break;
        }
    }
}

void
EffectInstance::findPluginFormatKnobs(const KnobsVec & knobs, bool loadingSerialization)
{
    ///Try to find a format param and hijack it to handle it ourselves with the project's formats
    KnobIPtr formatKnob;

    for (std::size_t i = 0; i < knobs.size(); ++i) {
        if (knobs[i]->getName() == kNatronParamFormatChoice) {
            formatKnob = knobs[i];
            break;
        }
    }
    if (formatKnob) {
        KnobIPtr formatSize;
        for (std::size_t i = 0; i < knobs.size(); ++i) {
            if (knobs[i]->getName() == kNatronParamFormatSize) {
                formatSize = knobs[i];
                break;
            }
        }
        KnobIPtr formatPar;
        for (std::size_t i = 0; i < knobs.size(); ++i) {
            if (knobs[i]->getName() == kNatronParamFormatPar) {
                formatPar = knobs[i];
                break;
            }
        }
        if (formatSize && formatPar) {
            _imp->defKnobs->pluginFormatKnobs.formatChoice = toKnobChoice(formatKnob);
            formatSize->setEvaluateOnChange(false);
            formatPar->setEvaluateOnChange(false);
            formatSize->setSecret(true);
            formatPar->setSecret(true);
            _imp->defKnobs->pluginFormatKnobs.size = toKnobInt(formatSize);
            _imp->defKnobs->pluginFormatKnobs.par = toKnobDouble(formatPar);

            std::vector<ChoiceOption> formats;
            int defValue;
            getApp()->getProject()->getProjectFormatEntries(&formats, &defValue);

            refreshFormatParamChoice(formats, defValue, loadingSerialization);
        }
    }
}

void
EffectInstance::createNodePage(const KnobPagePtr& settingsPage)
{


    {
        KnobBoolPtr param = createKnob<KnobBool>("hideInputs");
        param->setLabel(tr("Hide inputs"));
        param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
        param->setDefaultValue(false);
        param->setAnimationEnabled(false);
        param->setAddNewLine(false);
        param->setIsPersistent(true);
        param->setEvaluateOnChange(false);
        param->setHintToolTip( tr("When checked, the input arrows of the node in the nodegraph will be hidden") );
        settingsPage->addKnob(param);

        _imp->defKnobs->hideInputs = param;

    }


    {
        KnobBoolPtr param = createKnob<KnobBool>("forceCaching");
        param->setLabel(tr("Force caching"));
        param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
        param->setDefaultValue(false);
        param->setAnimationEnabled(false);
        param->setAddNewLine(false);
        param->setIsPersistent(true);
        param->setEvaluateOnChange(false);
        param->setHintToolTip( tr("When checked, the output of this node will always be kept in the RAM cache for fast access of already computed images.") );
        settingsPage->addKnob(param);

        _imp->defKnobs->forceCaching = param;
    }

    {
        KnobBoolPtr param = createKnob<KnobBool>(kEnablePreviewKnobName);
        param->setLabel(tr("Preview"));
        param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
        param->setDefaultValue( makePreviewByDefault() );
        param->setAnimationEnabled(false);
        param->setAddNewLine(false);
        param->setIsPersistent(false);
        param->setEvaluateOnChange(false);
        param->setHintToolTip( tr("Whether to show a preview on the node box in the node-graph.") );
        settingsPage->addKnob(param);


        _imp->defKnobs->previewEnabledKnob = param;
    }

    {
        KnobBoolPtr param = createKnob<KnobBool>(kDisableNodeKnobName);
        param->setLabel(tr("Disable"));
        param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
        param->setAnimationEnabled(false);
        param->setIsMetadataSlave(true);
        param->setAddNewLine(false);
        addOverlaySlaveParam(param);
        param->setHintToolTip( tr("When disabled, this node acts as a pass through.") );
        settingsPage->addKnob(param);

        _imp->renderKnobs.disableNodeKnob = param;
    }

    {
        KnobButtonPtr param = createKnob<KnobButton>(kNatronNodeKnobKeepInAnimationModuleButton);
        param->setLabel(tr(kNatronNodeKnobKeepInAnimationModuleButtonLabel));
        param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
        param->setHintToolTip(tr(kNatronNodeKnobKeepInAnimationModuleButtonHint));
        param->setCheckable(true);
        param->setIconLabel(NATRON_IMAGES_PATH "DS_CE_Icon.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "DS_CE_Icon.png", false);
        settingsPage->addKnob(param);

        _imp->defKnobs->keepInAnimationModuleKnob = param;
    }


    {
        KnobIntPtr param = createKnob<KnobInt>(kLifeTimeNodeKnobName, 2);
        param->setLabel(tr("Lifetime Range"));
        param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
        param->setAnimationEnabled(false);
        param->setIsMetadataSlave(true);
        param->setAddNewLine(false);
        param->setHintToolTip( tr("This is the frame range during which the node will be active if Enable Lifetime is checked") );
        settingsPage->addKnob(param);

        _imp->renderKnobs.lifeTimeKnob = param;
    }
    {
        KnobBoolPtr param = createKnob<KnobBool>(kEnableLifeTimeNodeKnobName);
        param->setLabel(tr("Enable Lifetime"));
        param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
        param->setAnimationEnabled(false);
        param->setDefaultValue(false);
        param->setIsMetadataSlave(true);
        param->setHintToolTip( tr("When checked, the node is only active during the specified frame range by the Lifetime Range parameter. "
                                  "Outside of this frame range, it behaves as if the Disable parameter is checked") );
        settingsPage->addKnob(param);

        _imp->renderKnobs.enableLifeTimeKnob = param;
    }

    NodeGroup* isGroup = dynamic_cast<NodeGroup*>(this);

    PluginOpenGLRenderSupport glSupport = ePluginOpenGLRenderSupportNone;

    PluginPtr plugin = getNode()->getPlugin();
    if (plugin && plugin->isOpenGLEnabled()) {
        glSupport = plugin->getEffectDescriptor()->getPropertyUnsafe<PluginOpenGLRenderSupport>(kEffectPropSupportsOpenGLRendering);
    }

    // For Groups, always create a GPU support knob, even if the node itself does not handle gpu support: this allows nodes of the
    // sub-graph to hook on the value of the Group node.
    if (isGroup || glSupport == ePluginOpenGLRenderSupportYes) {
        getOrCreateOpenGLEnabledKnob();
    }


    {
        KnobStringPtr param = createKnob<KnobString>("onParamChanged");
        param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
        param->setLabel(tr("After param changed callback"));
        param->setHintToolTip( tr("Set here the name of a function defined in Python which will be called for each  "
                                  "parameter change. Either define this function in the Script Editor "
                                  "or in the init.py script or even in the script of a Python group plug-in.\n"
                                  "The signature of the callback is: callback(thisParam, thisNode, thisGroup, app, userEdited) where:\n"
                                  "- thisParam: The parameter which just had its value changed\n"
                                  "- userEdited: A boolean informing whether the change was due to user interaction or "
                                  "because something internally triggered the change.\n"
                                  "- thisNode: The node holding the parameter\n"
                                  "- app: points to the current application instance\n"
                                  "- thisGroup: The group holding thisNode (only if thisNode belongs to a group)") );
        param->setAnimationEnabled(false);
        settingsPage->addKnob(param);

        _imp->defKnobs->knobChangedCallback = param;
    }
    {
        KnobStringPtr param = createKnob<KnobString>("onInputChanged");
        param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
        param->setLabel(tr("After input changed callback"));
        param->setHintToolTip( tr("Set here the name of a function defined in Python which will be called after "
                                  "each connection is changed for the inputs of the node. "
                                  "Either define this function in the Script Editor "
                                  "or in the init.py script or even in the script of a Python group plug-in.\n"
                                  "The signature of the callback is: callback(inputIndex, thisNode, thisGroup, app):\n"
                                  "- inputIndex: the index of the input which changed, you can query the node "
                                  "connected to the input by calling the getInput(...) function.\n"
                                  "- thisNode: The node holding the parameter\n"
                                  "- app: points to the current application instance\n"
                                  "- thisGroup: The group holding thisNode (only if thisNode belongs to a group)") );

        param->setAnimationEnabled(false);
        settingsPage->addKnob(param);

        _imp->defKnobs->inputChangedCallback = param;
    }
    if (isGroup) {
        {
            KnobStringPtr param = createKnob<KnobString>("afterNodeCreated");
            param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
            param->setLabel(tr("After Node Created"));

            param->setHintToolTip( tr("Add here the name of a Python-defined function that will be called each time a node "
                                      "is created in the group. This will be called in addition to the After Node Created "
                                      " callback of the project for the group node and all nodes within it (not recursively).\n"
                                      "The boolean variable userEdited will be set to True if the node was created "
                                      "by the user or False otherwise (such as when loading a project, or pasting a node).\n"
                                      "The signature of the callback is: callback(thisNode, app, userEdited) where:\n"
                                      "- thisNode: the node which has just been created\n"
                                      "- userEdited: a boolean indicating whether the node was created by user interaction or from "
                                      "a script/project load/copy-paste\n"
                                      "- app: points to the current application instance.") );
            param->setAnimationEnabled(false);
            settingsPage->addKnob(param);

            _imp->defKnobs->nodeCreatedCallback = param;

        }
        {
            KnobStringPtr param = createKnob<KnobString>("beforeNodeRemoval");
            param->setLabel(tr("Before Node Removal"));
            param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
            param->setHintToolTip( tr("Add here the name of a Python-defined function that will be called each time a node "
                                      "is about to be deleted. This will be called in addition to the Before Node Removal "
                                      " callback of the project for the group node and all nodes within it (not recursively).\n"
                                      "This function will not be called when the project is closing.\n"
                                      "The signature of the callback is: callback(thisNode, app) where:\n"
                                      "- thisNode: the node about to be deleted\n"
                                      "- app: points to the current application instance.") );
            param->setAnimationEnabled(false);

            settingsPage->addKnob(param);

            _imp->defKnobs->nodeRemovalCallback = param;
        }
    }

    std::list<KnobItemsTablePtr> tables = getAllItemsTables();
    if (!tables.empty()) {
        KnobStringPtr param = createKnob<KnobString>("afterItemsSelectionChanged");
        param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
        param->setLabel( tr("After Items Selection Changed"));
        param->setHintToolTip( tr("Add here the name of a Python-defined function that will be called each time the "
                                  "selection in the table changes. "
                                  "The variable \"reason\" will be set to a value of type NatronEngine.Natron.TableChangeReasonEnum "
                                  "depending on where the selection was made from. If reason is "
                                  "NatronEngine.Natron.TableChangeReasonEnum.eTableChangeReasonViewer then the selection was made "
                                  "from the viewer. If reason is NatronEngine.Natron.TableChangeReasonEnum.eTableChangeReasonPanel "
                                  "then the selection was made from the settings panel. Otherwise the selection was not changed "
                                  "by the user directly and results from an internal A.P.I call.\n"
                                  "The signature of the callback is: callback(thisNode, app, deselected, selected, reason) where:\n"
                                  "- thisNode: the node holding the items table\n"
                                  "- thisTable: the table holding the items"
                                  "- app: points to the current application instance\n"
                                  "- deselected: a sequence of items that were removed from the selection\n"
                                  "- selected: a sequence of items that were added to the selection\n"
                                  "- reason: a value of type NatronEngine.Natron.TableChangeReasonEnum") );
        param->setAnimationEnabled(false);

        settingsPage->addKnob(param);

        _imp->defKnobs->tableSelectionChangedCallback = param;
    }

    if (isOutput()) {
        {
            KnobStringPtr param =  createKnob<KnobString>("beforeFrameRender");
            param->setLabel(tr("Before frame render"));
            param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
            param->setAnimationEnabled(false);
            param->setHintToolTip( tr("Add here the name of a Python defined function that will be called before rendering "
                                      "any frame.\n "
                                      "The signature of the callback is: callback(frame, thisNode, app) where:\n"
                                      "- frame: the frame to be rendered\n"
                                      "- thisNode: points to the writer node\n"
                                      "- app: points to the current application instance") );
            settingsPage->addKnob(param);

            _imp->defKnobs->beforeFrameRender = param;

        }

        {
            KnobStringPtr param =  createKnob<KnobString>("beforeRender");
            param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
            param->setLabel(tr("Before render"));
            param->setAnimationEnabled(false);
            param->setHintToolTip( tr("Add here the name of a Python defined function that will be called once when "
                                      "starting rendering.\n "
                                      "The signature of the callback is: callback(thisNode, app) where:\n"
                                      "- thisNode: points to the writer node\n"
                                      "- app: points to the current application instance") );
            settingsPage->addKnob(param);
            _imp->defKnobs->beforeRender = param;

        }

        {
            KnobStringPtr param =  createKnob<KnobString>("afterFrameRender");
            param->setLabel(tr("After frame render"));
            param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
            param->setAnimationEnabled(false);
            param->setHintToolTip( tr("Add here the name of a Python defined function that will be called after rendering "
                                      "any frame.\n "
                                      "The signature of the callback is: callback(frame, thisNode, app) where:\n"
                                      "- frame: the frame that has been rendered\n"
                                      "- thisNode: points to the writer node\n"
                                      "- app: points to the current application instance") );
            settingsPage->addKnob(param);

            _imp->defKnobs->afterFrameRender = param;
        }

        {
            KnobStringPtr param =  createKnob<KnobString>("afterRender");
            param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
            param->setLabel(tr("After render"));
            param->setAnimationEnabled(false);
            param->setHintToolTip( tr("Add here the name of a Python defined function that will be called once when the rendering "
                                      "is finished.\n "
                                      "The signature of the callback is: callback(aborted, thisNode, app) where:\n"
                                      "- aborted: True if the render ended because it was aborted, False upon completion\n"
                                      "- thisNode: points to the writer node\n"
                                      "- app: points to the current application instance") );
            settingsPage->addKnob(param);
            _imp->defKnobs->afterRender = param;
            
        }
    } // isOutput
} // createNodePage

void
EffectInstance::createInfoPage()
{
    KnobPagePtr page = createKnob<KnobPage>(kInfoPageParamName);
    page->setLabel(tr(kInfoPageParamLabel));
    page->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
    _imp->defKnobs->infoPage = page;

    {
        KnobStringPtr param = createKnob<KnobString>("nodeInfos");
        param->setLabel(QString());
        param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
        param->setAnimationEnabled(false);
        param->setIsPersistent(false);
        param->setAsMultiLine();
        param->setAsCustomHTMLText(true);
        param->setEvaluateOnChange(false);
        param->setHintToolTip( tr("Input and output informations, press Refresh to update them with current values") );
        page->addKnob(param);
        _imp->defKnobs->nodeInfos = param;
    }

    {
        KnobButtonPtr param = createKnob<KnobButton>("refreshButton");
        param->setLabel(tr("Refresh Info"));
        param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
        param->setEvaluateOnChange(false);
        page->addKnob(param);
        _imp->defKnobs->refreshInfoButton = param;
    }
}

void
EffectInstance::createPyPlugPage()
{
    PluginPtr pyPlug = getNode()->getPyPlugPlugin();
    KnobPagePtr page = createKnob<KnobPage>(kPyPlugPageParamName);
    page->setLabel(tr(kPyPlugPageParamLabel));
    page->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
    page->setSecret(true);
    _imp->defKnobs->pyPlugPage = page;

    {
        KnobStringPtr param = createKnob<KnobString>(kNatronNodeKnobPyPlugPluginID);
        param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
        param->setLabel(tr(kNatronNodeKnobPyPlugPluginIDLabel));
        if (pyPlug) {
            param->setValue(pyPlug->getPluginID());
        }
        param->setEvaluateOnChange(false);
        param->setHintToolTip(tr(kNatronNodeKnobPyPlugPluginIDHint));
        page->addKnob(param);
        _imp->defKnobs->pyPlugIDKnob = param;
    }
    {
        KnobStringPtr param = createKnob<KnobString>(kNatronNodeKnobPyPlugPluginLabel);
        param->setLabel(tr(kNatronNodeKnobPyPlugPluginLabelLabel));
        param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
        if (pyPlug) {
            param->setValue(pyPlug->getPluginLabel());
        }
        param->setEvaluateOnChange(false);
        param->setHintToolTip( tr(kNatronNodeKnobPyPlugPluginLabelHint));
        page->addKnob(param);
        _imp->defKnobs->pyPlugLabelKnob = param;
    }
    {
        KnobStringPtr param = createKnob<KnobString>(kNatronNodeKnobPyPlugPluginGrouping);
        param->setLabel(tr(kNatronNodeKnobPyPlugPluginGroupingLabel));
        param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
        if (pyPlug) {
            param->setValue(pyPlug->getGroupingString());
        } else {
            param->setValue("PyPlugs");
        }
        param->setEvaluateOnChange(false);
        param->setHintToolTip( tr(kNatronNodeKnobPyPlugPluginGroupingHint));
        page->addKnob(param);
        _imp->defKnobs->pyPlugGroupingKnob = param;
    }
    {
        KnobStringPtr param = createKnob<KnobString>(kNatronNodeKnobPyPlugPluginDescription);
        param->setLabel(tr(kNatronNodeKnobPyPlugPluginDescriptionLabel));
        param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
        param->setEvaluateOnChange(false);
        param->setAsMultiLine();
        if (pyPlug) {
            param->setValue(pyPlug->getPropertyUnsafe<std::string>(kNatronPluginPropDescription));
        }
        param->setHintToolTip( tr(kNatronNodeKnobPyPlugPluginDescriptionHint));
        param->setAddNewLine(false);
        page->addKnob(param);
        _imp->defKnobs->pyPlugDescKnob = param;
    }
    {
        KnobBoolPtr param = createKnob<KnobBool>(kNatronNodeKnobPyPlugPluginDescriptionIsMarkdown);
        param->setLabel(tr(kNatronNodeKnobPyPlugPluginDescriptionIsMarkdownLabel));
        param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
        param->setEvaluateOnChange(false);
        if (pyPlug) {
            param->setValue(pyPlug->getPropertyUnsafe<bool>(kNatronPluginPropDescriptionIsMarkdown));
        }
        param->setHintToolTip( tr(kNatronNodeKnobPyPlugPluginDescriptionIsMarkdownHint));
        page->addKnob(param);
        _imp->defKnobs->pyPlugDescIsMarkdownKnob = param;
    }
    {
        KnobIntPtr param = createKnob<KnobInt>(kNatronNodeKnobPyPlugPluginVersion, 2);
        param->setLabel(tr(kNatronNodeKnobPyPlugPluginVersionLabel));
        param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
        param->setEvaluateOnChange(false);
        param->setDimensionName(DimIdx(0), "Major");
        param->setDimensionName(DimIdx(1), "Minor");
        if (pyPlug) {
            param->setValue((int)pyPlug->getPropertyUnsafe<unsigned int>(kNatronPluginPropVersion, 0));
            param->setValue((int)pyPlug->getPropertyUnsafe<unsigned int>(kNatronPluginPropVersion, 1), ViewSetSpec::all(), DimSpec(1));
        }
        param->setHintToolTip( tr(kNatronNodeKnobPyPlugPluginVersionHint));
        page->addKnob(param);
        _imp->defKnobs->pyPlugVersionKnob = param;
    }
    {
        KnobIntPtr param = createKnob<KnobInt>(kNatronNodeKnobPyPlugPluginShortcut, 2);
        param->setLabel(tr(kNatronNodeKnobPyPlugPluginShortcutLabel));
        param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
        param->setEvaluateOnChange(false);
        param->setAsShortcutKnob(true);
        if (pyPlug) {
            param->setValue(pyPlug->getPropertyUnsafe<int>(kNatronPluginPropShortcut, 0));
            param->setValue(pyPlug->getPropertyUnsafe<int>(kNatronPluginPropShortcut, 1), ViewSetSpec::all(), DimSpec(1));
        }
        param->setHintToolTip( tr(kNatronNodeKnobPyPlugPluginShortcutHint));
        page->addKnob(param);
        _imp->defKnobs->pyPlugShortcutKnob = param;
    }
    {
        KnobFilePtr param = createKnob<KnobFile>(kNatronNodeKnobPyPlugPluginCallbacksPythonScript);
        param->setLabel(tr(kNatronNodeKnobPyPlugPluginCallbacksPythonScriptLabel));
        param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
        param->setEvaluateOnChange(false);
        if (pyPlug) {
            param->setValue(pyPlug->getPropertyUnsafe<std::string>(kNatronPluginPropPyPlugExtScriptFile));
        }
        param->setHintToolTip( tr(kNatronNodeKnobPyPlugPluginCallbacksPythonScriptHint));
        page->addKnob(param);
        _imp->defKnobs->pyPlugExtPythonScript = param;
    }
    {
        KnobFilePtr param = createKnob<KnobFile>(kNatronNodeKnobPyPlugPluginIconFile);
        param->setLabel(tr(kNatronNodeKnobPyPlugPluginIconFileLabel));
        param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
        param->setEvaluateOnChange(false);
        if (pyPlug) {
            param->setValue(pyPlug->getPropertyUnsafe<std::string>(kNatronPluginPropIconFilePath));
        }
        param->setHintToolTip( tr(kNatronNodeKnobPyPlugPluginIconFileHint));
        page->addKnob(param);
        _imp->defKnobs->pyPlugIconKnob = param;
    }

    {
        KnobFilePtr param = createKnob<KnobFile>(kNatronNodeKnobExportDialogFilePath);
        param->setLabel(tr(kNatronNodeKnobExportDialogFilePathLabel));
        param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
        param->setDialogType(KnobFile::eKnobFileDialogTypeSaveFile);
        {
            std::vector<std::string> filters;
            filters.push_back(NATRON_PRESETS_FILE_EXT);
            param->setDialogFilters(filters);
        }
        param->setEvaluateOnChange(false);
        param->setIsPersistent(false);
        param->setHintToolTip(tr(kNatronNodeKnobExportDialogFilePathHint));
        page->addKnob(param);
        _imp->defKnobs->pyPlugExportDialogFile = param;
    }

    {
        KnobButtonPtr param = createKnob<KnobButton>(kNatronNodeKnobExportPyPlugButton);
        param->setLabel(tr(kNatronNodeKnobExportPyPlugButtonLabel));
        param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
        param->setEvaluateOnChange(false);
        param->setHintToolTip( tr("Click to export this group to a PyPlug file (.%1)").arg(QLatin1String(NATRON_PRESETS_FILE_EXT)));
        page->addKnob(param);
        _imp->defKnobs->pyPlugExportButtonKnob = param;
    }
} // createPyPlugPage


KnobDoublePtr
EffectInstance::getOrCreateHostMixKnob(const KnobPagePtr& mainPage)
{
    KnobDoublePtr param = _imp->defKnobs->mixWithSource.lock();
    if (!param) {
        param = createKnob<KnobDouble>(kHostMixingKnobName);
        param->setLabel(tr(kHostMixingKnobLabel));
        param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
        param->setHintToolTip( tr(kHostMixingKnobHint) );
        param->setRange(0., 1.);
        param->setDefaultValue(1.);
        if (mainPage) {
            mainPage->addKnob(param);
        }

        _imp->defKnobs->mixWithSource = param;
    }

    return param;
}


KnobPagePtr
EffectInstance::getOrCreateMainPage()
{
    const KnobsVec & knobs = getKnobs();
    KnobPagePtr mainPage;

    for (std::size_t i = 0; i < knobs.size(); ++i) {
        KnobPagePtr p = toKnobPage(knobs[i]);
        if ( p && (p->getName() != kInfoPageParamName) &&
            (p->getName() != kNodePageParamName) ) {
            mainPage = p;
            break;
        }
    }
    if (!mainPage) {
        mainPage = createKnob<KnobPage>("settingsPage");
        mainPage->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
        mainPage->setLabel(tr("Settings"));
    }

    return mainPage;
}

void
EffectInstance::createLabelKnob(const KnobPagePtr& settingsPage,
                                const std::string& label)
{
    KnobStringPtr param = createKnob<KnobString>(kUserLabelKnobName);
    param->setLabel(label);
    param->setAnimationEnabled(false);
    param->setEvaluateOnChange(false);
    param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
    param->setAsMultiLine();
    param->setUsesRichText(true);
    param->setHintToolTip( tr("This label gets appended to the node name on the node graph.") );
    settingsPage->addKnob(param);

    _imp->defKnobs->nodeLabelKnob = param;
}



int
EffectInstance::getFrameStepKnobValue() const
{
    KnobIPtr knob = getKnobByName(kNatronWriteParamFrameStep);
    if (!knob) {
        return 1;
    }
    KnobIntPtr k = toKnobInt(knob);

    if (!k) {
        return 1;
    } else {
        int v = k->getValue();

        return std::max(1, v);
    }
}



bool
EffectInstance::handleFormatKnob(const KnobIPtr& knob)
{
    KnobChoicePtr choice = _imp->defKnobs->pluginFormatKnobs.formatChoice.lock();

    if (!choice) {
        return false;
    }

    if (knob != choice) {
        return false;
    }
  
    int curIndex = choice->getValue();
    Format f;
    if ( !getApp()->getProject()->getProjectFormatAtIndex(curIndex, &f) ) {
        assert(false);

        return true;
    }

    KnobIntPtr size = _imp->defKnobs->pluginFormatKnobs.size.lock();
    KnobDoublePtr par = _imp->defKnobs->pluginFormatKnobs.par.lock();
    assert(size && par);

    ScopedChanges_RAII changes(this);
    size->blockValueChanges();
    std::vector<int> values(2);
    values[0] = f.width();
    values[1] = f.height();
    size->setValueAcrossDimensions(values);
    size->unblockValueChanges();
    par->blockValueChanges();
    par->setValue( f.getPixelAspectRatio() );
    par->unblockValueChanges();

    return true;
} // handleFormatKnob

void
EffectInstance::refreshFormatParamChoice(const std::vector<ChoiceOption>& entries,
                                         int defValue,
                                         bool loadingProject)
{
    KnobChoicePtr choice = _imp->defKnobs->pluginFormatKnobs.formatChoice.lock();
    
    if (!choice) {
        return;
    }
    int curIndex = choice->getValue();
    choice->populateChoices(entries);

    ScopedChanges_RAII changes(choice.get());


    choice->setDefaultValueWithoutApplying(defValue);
    // We don't want to serialize the default value even if it changed, because it will be restored by the project
    choice->setCurrentDefaultValueAsInitialValue();

    if (!loadingProject) {
        //changedKnob was not called because we are initializing knobs
        handleFormatKnob(choice);
    } else {
        if ( curIndex < (int)entries.size() ) {
            choice->setValue(curIndex);
        }
    }
}


double
EffectInstance::getHostMixingValue(TimeValue time,
                         ViewIdx view) const
{
    KnobDoublePtr knob;
    if (!getMainInstance()) {
        knob = _imp->defKnobs->mixWithSource.lock();
    } else {
        knob = _imp->renderKnobs.mixWithSource.lock();
    }
    if (!knob) {
        return 1.;
    }

    return knob->getValueAtTime(time, DimIdx(0), view);
}

bool
EffectInstance::isKeepInAnimationModuleButtonDown() const
{
    KnobButtonPtr b = _imp->defKnobs->keepInAnimationModuleKnob.lock();
    if (!b) {
        return false;
    }
    return b->getValue();
}

bool
EffectInstance::getHideInputsKnobValue() const
{
    KnobBoolPtr k = _imp->defKnobs->hideInputs.lock();

    if (!k) {
        return false;
    }

    return k->getValue();
}

void
EffectInstance::setHideInputsKnobValue(bool hidden)
{
    KnobBoolPtr k = _imp->defKnobs->hideInputs.lock();

    if (!k) {
        return;
    }
    k->setValue(hidden);
}


bool
EffectInstance::isForceCachingEnabled() const
{
    KnobBoolPtr b = _imp->defKnobs->forceCaching.lock();
    return b ? b->getValue() : false;
}

void
EffectInstance::setForceCachingEnabled(bool value)
{
    KnobBoolPtr b = _imp->defKnobs->forceCaching.lock();
    if (!b) {
        return;
    }
    b->setValue(value);
}

KnobStringPtr
EffectInstance::getExtraLabelKnob() const
{
    return _imp->defKnobs->nodeLabelKnob.lock();
}

KnobStringPtr
EffectInstance::getOFXSubLabelKnob() const
{
    return _imp->defKnobs->ofxSubLabelKnob.lock();
}


KnobBoolPtr
EffectInstance::getDisabledKnob() const
{
    return _imp->renderKnobs.disableNodeKnob.lock();
}

bool
EffectInstance::isLifetimeActivated(int *firstFrame,
                          int *lastFrame) const
{
    KnobBoolPtr enableLifetimeKnob = _imp->renderKnobs.enableLifeTimeKnob.lock();

    if (!enableLifetimeKnob) {
        return false;
    }
    if ( !enableLifetimeKnob->getValue() ) {
        return false;
    }
    KnobIntPtr lifetimeKnob = _imp->renderKnobs.lifeTimeKnob.lock();
    *firstFrame = lifetimeKnob->getValue(DimIdx(0));
    *lastFrame = lifetimeKnob->getValue(DimIdx(1));

    return true;
}


KnobPagePtr
EffectInstance::getPyPlugPage() const
{
    return _imp->defKnobs->pyPlugPage.lock();
}

KnobStringPtr
EffectInstance::getPyPlugIDKnob() const
{
    return _imp->defKnobs->pyPlugIDKnob.lock();
}

KnobStringPtr
EffectInstance::getPyPlugLabelKnob() const
{
    return _imp->defKnobs->pyPlugLabelKnob.lock();
}

KnobFilePtr
EffectInstance::getPyPlugIconKnob() const
{
    return _imp->defKnobs->pyPlugIconKnob.lock();
}

KnobFilePtr
EffectInstance::getPyPlugExtScriptKnob() const
{
    return _imp->defKnobs->pyPlugExtPythonScript.lock();
}

KnobBoolPtr
EffectInstance::getProcessAllLayersKnob() const
{
    return _imp->defKnobs->processAllLayersKnob.lock();
}


KnobBoolPtr
EffectInstance::getLifeTimeEnabledKnob() const
{
    return _imp->renderKnobs.enableLifeTimeKnob.lock();
}

KnobIntPtr
EffectInstance::getLifeTimeKnob() const
{
    return _imp->renderKnobs.lifeTimeKnob.lock();
}


std::string
EffectInstance::getNodeExtraLabel() const
{
    KnobStringPtr s = _imp->defKnobs->nodeLabelKnob.lock();

    if (s) {
        return s->getValue();
    } else {
        return std::string();
    }
}

bool
EffectInstance::isNodeDisabledForFrame(TimeValue time, ViewIdx view) const
{
    // Check disabled knob
    KnobBoolPtr b = _imp->renderKnobs.disableNodeKnob.lock();
    if (b && b->getValueAtTime(time, DimIdx(0), view)) {
        return true;
    }

    NodeGroupPtr isContainerGrp = toNodeGroup( getNode()->getGroup() );

    // If within a group, check the group's disabled knob
    if (isContainerGrp) {
        if (isContainerGrp->isNodeDisabledForFrame(time, view)) {
            return true;
        }
    }

    // If an internal IO node, check the main meta-node disabled knob
    NodePtr ioContainer = getNode()->getIOContainer();
    if (ioContainer) {
        return ioContainer->getEffectInstance()->isNodeDisabledForFrame(time, view);
    }

    RotoDrawableItemPtr attachedItem = getAttachedRotoItem();
    if (attachedItem && !attachedItem->isActivated(time, view)) {
        return true;
    }

    // Check lifetime
    int lifeTimeFirst, lifeTimeEnd;
    bool lifeTimeEnabled = isLifetimeActivated(&lifeTimeFirst, &lifeTimeEnd);
    bool enabled = ( !lifeTimeEnabled || (time >= lifeTimeFirst && time <= lifeTimeEnd) );
    return !enabled;
} // isNodeDisabledForFrame

bool
EffectInstance::getDisabledKnobValue() const
{
    // Check disabled knob
    KnobBoolPtr b = _imp->renderKnobs.disableNodeKnob.lock();
    if (b && b->getValue()) {
        return true;
    }

    NodeGroupPtr isContainerGrp = toNodeGroup( getNode()->getGroup() );

    // If within a group, check the group's disabled knob
    if (isContainerGrp) {
        if (isContainerGrp->getDisabledKnobValue()) {
            return true;
        }
    }

    // If an internal IO node, check the main meta-node disabled knob
    NodePtr ioContainer = getNode()->getIOContainer();
    if (ioContainer) {
        return ioContainer->getEffectInstance()->getDisabledKnobValue();
    }


    int lifeTimeFirst, lifeTimeEnd;
    bool lifeTimeEnabled = isLifetimeActivated(&lifeTimeFirst, &lifeTimeEnd);
    double curFrame = getCurrentRenderTime();
    bool enabled = ( !lifeTimeEnabled || (curFrame >= lifeTimeFirst && curFrame <= lifeTimeEnd) );

    return !enabled;
}


void
EffectInstance::setNodeDisabled(bool disabled)
{
    KnobBoolPtr b = _imp->renderKnobs.disableNodeKnob.lock();

    if (b) {
        b->setValue(disabled);
    }
}

void
EffectInstance::Implementation::fetchSubLabelKnob()
{

    KnobIPtr sublabelKnob = _publicInterface->getKnobByName(kNatronOfxParamStringSublabelName);
    if (!sublabelKnob) {
        return;
    }

    // Make sure the knob is not persistent
    sublabelKnob->setIsPersistent(false);

    KnobStringPtr sublabelKnobIsString = toKnobString(sublabelKnob);
    defKnobs->ofxSubLabelKnob = sublabelKnobIsString;

    NodePtr ioContainer = _publicInterface->getNode()->getIOContainer();
    if (ioContainer) {
        ioContainer->getEffectInstance()->_imp->defKnobs->ofxSubLabelKnob = sublabelKnobIsString;
        Q_EMIT ioContainer->s_nodeExtraLabelChanged();
    }
}

KnobChoicePtr
EffectInstance::getOrCreateOpenGLEnabledKnob()
{
    KnobChoicePtr openglRenderingKnob = _imp->defKnobs->openglRenderingEnabledKnob.lock();
    if (openglRenderingKnob) {
        return openglRenderingKnob;
    }
    openglRenderingKnob = getOrCreateKnob<KnobChoice>("enableGPURendering");
    openglRenderingKnob->setLabel(tr("GPU Rendering"));
    openglRenderingKnob->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
    assert(openglRenderingKnob);
    openglRenderingKnob->setAnimationEnabled(false);
    {
        std::vector<ChoiceOption> entries;
        entries.push_back(ChoiceOption("Enabled", "", tr("If a plug-in support GPU rendering, prefer rendering using the GPU if possible.").toStdString() ));
        entries.push_back(ChoiceOption("Disabled", "", tr("Disable GPU rendering for all plug-ins.").toStdString()));
        entries.push_back(ChoiceOption("Disabled if background", "", tr("Disable GPU rendering when rendering with NatronRenderer but not in GUI mode.").toStdString()));
        openglRenderingKnob->populateChoices(entries);
    }

    openglRenderingKnob->setHintToolTip( tr("Select when to activate GPU rendering for this node. Note that if the GPU Rendering parameter in the Project settings is set to disabled then GPU rendering will not be activated regardless of that value.") );

    KnobPagePtr settingsPageKnob = getOrCreateKnob<KnobPage>(kNodePageParamName);
    settingsPageKnob->setLabel(tr(kNodePageParamLabel));
    settingsPageKnob->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
    settingsPageKnob->addKnob(openglRenderingKnob);
    _imp->defKnobs->openglRenderingEnabledKnob = openglRenderingKnob;

    return openglRenderingKnob;
}

void
EffectInstance::initializeDefaultKnobs(bool loadingSerialization, bool /*hasGUI*/)
{
    //Readers and Writers don't have default knobs since these knobs are on the ReadNode/WriteNode itself
    NodePtr ioContainer = getNode()->getIOContainer();


    //Add the "Node" page
    KnobPagePtr settingsPage = getOrCreateKnob<KnobPage>(kNodePageParamName);
    settingsPage->setLabel(tr(kNodePageParamLabel));
    settingsPage->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);


    //Create the "Label" knob
    NodeGroup* isGrpNode = dynamic_cast<NodeGroup*>(this);
    Backdrop* isBackdropNode = dynamic_cast<Backdrop*>(this);
    QString labelKnobLabel = isBackdropNode ? tr("Name label") : tr("Label");
    createLabelKnob( settingsPage, labelKnobLabel.toStdString() );

    if (isBackdropNode || dynamic_cast<StubNode*>(this)) {
        //backdrops just have a label
        return;
    }


    ///find in all knobs a page param to set this param into
    const KnobsVec & knobs = getKnobs();

    findPluginFormatKnobs(knobs, loadingSerialization);
    findRightClickMenuKnob(knobs);

    KnobPagePtr mainPage;


    // Scan all inputs to find masks and get inputs labels
    // Pair hasMaskChannelSelector, isMask
    int inputsCount = getNInputs();
    std::vector<std::pair<bool, bool> > hasMaskChannelSelector(inputsCount);
    std::vector<std::string> inputLabels(inputsCount);
    for (int i = 0; i < inputsCount; ++i) {
        inputLabels[i] = getNode()->getInputLabel(i);

        std::bitset<4> inputSupportedComps = getNode()->getSupportedComponents(i);
        bool isMask = getNode()->isInputMask(i);
        bool supportsOnlyAlpha = inputSupportedComps[0] && !inputSupportedComps[1] && !inputSupportedComps[2] && !inputSupportedComps[3];

        hasMaskChannelSelector[i].first = false;
        hasMaskChannelSelector[i].second = isMask;

        if ( isMask || supportsOnlyAlpha ) {
            hasMaskChannelSelector[i].first = true;
            if (!mainPage) {
                mainPage = getOrCreateMainPage();
            }
        }
    }


    // Create the Output Layer choice if needed plus input layers selectors
    KnobIPtr lastKnobBeforeAdvancedOption;
    bool wantsPlaneSelector = isHostPlaneSelectorEnabled();
    if (wantsPlaneSelector) {
        if (!mainPage) {
            mainPage = getOrCreateMainPage();
        }
        createChannelSelectors(hasMaskChannelSelector, inputLabels, mainPage, &lastKnobBeforeAdvancedOption);
    }

    // If multi-planar, try to detect the "all" layers knob
    if (isMultiPlanar()) {
        _imp->defKnobs->processAllLayersKnob = toKnobBool(getKnobByName(kNodeParamProcessAllLayers));
    }

    if (!isGrpNode && mainPage) {
        // Add a knob so we can create layers for this effect
        getOrCreateUserPlanesKnob(mainPage);
    }

    findOrCreateChannelEnabled();

    ///Find in the plug-in the Mask/Mix related parameter to re-order them so it is consistent across nodes
    std::vector<std::pair<std::string, KnobIPtr > > foundPluginDefaultKnobsToReorder;
    foundPluginDefaultKnobsToReorder.push_back( std::make_pair( kOfxMaskInvertParamName, KnobIPtr() ) );
    foundPluginDefaultKnobsToReorder.push_back( std::make_pair( kOfxMixParamName, KnobIPtr() ) );
    ///Insert auto-added knobs before mask invert if found
    for (std::size_t i = 0; i < knobs.size(); ++i) {
        for (std::size_t j = 0; j < foundPluginDefaultKnobsToReorder.size(); ++j) {
            if (knobs[i]->getName() == foundPluginDefaultKnobsToReorder[j].first) {
                foundPluginDefaultKnobsToReorder[j].second = knobs[i];
            }

        }
    }



    assert(foundPluginDefaultKnobsToReorder.size() > 0 && foundPluginDefaultKnobsToReorder[0].first == kOfxMaskInvertParamName);

    createMaskSelectors(hasMaskChannelSelector, inputLabels, mainPage, !foundPluginDefaultKnobsToReorder[0].second.get(), &lastKnobBeforeAdvancedOption);


    //Create the host mix if needed
    if ( isHostMixEnabled() ) {
        if (!mainPage) {
            mainPage = getOrCreateMainPage();
        }
        getOrCreateHostMixKnob(mainPage);
    }


    /*
     * Reposition the MaskInvert and Mix parameters declared by the plug-in
     */

    for (std::size_t i = 0; i < foundPluginDefaultKnobsToReorder.size(); ++i) {
        if (foundPluginDefaultKnobsToReorder[i].second) {
            if (!mainPage) {
                mainPage = getOrCreateMainPage();
            }
            if (foundPluginDefaultKnobsToReorder[i].second->getParentKnob() == mainPage) {
                mainPage->removeKnob( foundPluginDefaultKnobsToReorder[i].second);
                mainPage->addKnob(foundPluginDefaultKnobsToReorder[i].second);
            }
        }
    }


    if (lastKnobBeforeAdvancedOption && mainPage) {

        KnobsVec mainPageChildren = mainPage->getChildren();
        int i = 0;
        for (KnobsVec::iterator it = mainPageChildren.begin(); it != mainPageChildren.end(); ++it, ++i) {
            if (*it == lastKnobBeforeAdvancedOption) {
                if (i > 0) {
                    int j = i - 1;
                    KnobsVec::iterator prev = it;
                    --prev;

                    while (j >= 0 && (*prev)->getIsSecret()) {
                        --j;
                        --prev;
                    }

                    if ( j >= 0 && !toKnobSeparator(*prev) ) {
                        KnobSeparatorPtr sep = createKnob<KnobSeparator>("advancedSep");
                        sep->setLabel(QString());
                        mainPage->insertKnob(i, sep);
                    }
                }

                break;
            }
        }
    }


    createNodePage(settingsPage);

    if (!isGrpNode && !isBackdropNode) {
        createInfoPage();
    } else if (isGrpNode) {
        if (isGrpNode->isSubGraphPersistent()) {
            createPyPlugPage();
            {
                KnobButtonPtr param = createKnob<KnobButton>(kNatronNodeKnobConvertToGroupButton);
                param->setLabel(tr(kNatronNodeKnobConvertToGroupButtonLabel));
                param->setEvaluateOnChange(false);
                param->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
                param->setHintToolTip( tr("Converts this node to a Group: the internal node-graph and the user parameters will become editable") );
                settingsPage->addKnob(param);

                _imp->defKnobs->pyPlugConvertToGroupButtonKnob = param;
            }
        }
    }

    _imp->fetchSubLabelKnob();


    if (isWriter()
        && !ioContainer) {
        //Create a frame step parameter for writers, and control it in OutputSchedulerThread.cpp

        if (!mainPage) {
            mainPage = getOrCreateMainPage();
        }

        KnobButtonPtr renderButton = createKnob<KnobButton>(kNatronWriteParamStartRender);
        renderButton->setKnobDeclarationType(KnobI::eKnobDeclarationTypeHost);
        renderButton->setLabel(tr("Render"));
        renderButton->setHintToolTip( tr("Starts rendering the specified frame range.") );
        renderButton->setAsRenderButton();
        renderButton->setName(kNatronWriteParamStartRender);
        renderButton->setEvaluateOnChange(false);
        _imp->defKnobs->renderButton = renderButton;
        mainPage->addKnob(renderButton);
    }
} // initializeDefaultKnobs

void
EffectInstance::initializeKnobs(bool loadingSerialization, bool hasGUI)
{
    ////Only called by the main-thread


    ScopedChanges_RAII changes(this);

    assert( QThread::currentThread() == qApp->thread() );

    InitializeKnobsFlag_RAII __isInitializingKnobsFlag__(shared_from_this());

    //Initialize plug-in knobs
    initializeKnobsPublic();

    if ( getMakeSettingsPanel() ) {
        //initialize default knobs added by Natron
        initializeDefaultKnobs(loadingSerialization, hasGUI);
    }
    
    getNode()->declarePythonKnobs();
    
    std::list<KnobItemsTablePtr> tables = getAllItemsTables();

    for (std::list<KnobItemsTablePtr>::const_iterator it = tables.begin(); it!=tables.end(); ++it) {
        (*it)->declareItemsToPython();
    }

    
} // initializeKnobs

void
EffectInstance::fetchRenderCloneKnobs()
{
    KnobHolder::fetchRenderCloneKnobs();
    EffectInstancePtr mainInstance = toEffectInstance(getMainInstance());
    assert(mainInstance);

    // For default knobs, since most of them are not used when rendering use the main instance ones instead.
    _imp->defKnobs = mainInstance->_imp->defKnobs;
    _imp->renderKnobs.mixWithSource = toKnobDouble(getKnobByName(kHostMixingKnobName));
    _imp->renderKnobs.disableNodeKnob = toKnobBool(getKnobByName(kDisableNodeKnobName));
    _imp->renderKnobs.lifeTimeKnob = toKnobInt(getKnobByName(kLifeTimeNodeKnobName));
    _imp->renderKnobs.enableLifeTimeKnob = toKnobBool(getKnobByName(kEnableLifeTimeNodeKnobName));
}

void
EffectInstance::computeFrameRangeForReader(const KnobIPtr& fileKnob, bool setFrameRange)
{
    /*
     We compute the original frame range of the sequence for the plug-in
     because the plug-in does not have access to the exact original pattern
     hence may not exactly end-up with the same file sequence as what the user
     selected from the file dialog.
     */
    ReadNode* isReadNode = dynamic_cast<ReadNode*>(this);
    std::string pluginID;

    if (isReadNode) {
        NodePtr embeddedPlugin = isReadNode->getEmbeddedReader();
        if (embeddedPlugin) {
            pluginID = embeddedPlugin->getPluginID();
        }
    } else {
        pluginID = getNode()->getPluginID();
    }

    int leftBound = INT_MIN;
    int rightBound = INT_MAX;
    ///Set the originalFrameRange parameter of the reader if it has one.
    KnobIntPtr originalFrameRangeKnob = toKnobInt(getKnobByName(kReaderParamNameOriginalFrameRange));
    KnobIntPtr firstFrameKnob = toKnobInt(getKnobByName(kReaderParamNameFirstFrame));
    KnobIntPtr lastFrameKnob = toKnobInt(getKnobByName(kReaderParamNameLastFrame));
    if (originalFrameRangeKnob) {
        if (originalFrameRangeKnob->getNDimensions() == 2){
            KnobFilePtr isFile = toKnobFile(fileKnob);
            assert(isFile);
            if (!isFile) {
                throw std::logic_error("Node::computeFrameRangeForReader");
            }

            if ( ReadNode::isVideoReader(pluginID) ) {
                ///If the plug-in is a video, only ffmpeg may know how many frames there are
                std::vector<int> frameRange(2);
                frameRange[0] = INT_MIN;
                frameRange[1] = INT_MAX;
                originalFrameRangeKnob->setValueAcrossDimensions(frameRange);
            } else {
                std::string pattern = isFile->getRawFileName();
                getApp()->getProject()->canonicalizePath(pattern);
                SequenceParsing::SequenceFromPattern seq;
                FileSystemModel::filesListFromPattern(pattern, &seq);
                if ( seq.empty() || (seq.size() == 1) ) {
                    leftBound = 1;
                    rightBound = 1;
                } else if (seq.size() > 1) {
                    leftBound = seq.begin()->first;
                    rightBound = seq.rbegin()->first;
                }
                std::vector<int> frameRange(2);
                frameRange[0] = leftBound;
                frameRange[1] = rightBound;
                originalFrameRangeKnob->setValueAcrossDimensions(frameRange);

                if (setFrameRange) {
                    if (firstFrameKnob) {
                        firstFrameKnob->setValue(leftBound);
                        firstFrameKnob->setRange(leftBound, rightBound);
                    }
                    if (lastFrameKnob) {
                        lastFrameKnob->setValue(rightBound);
                        lastFrameKnob->setRange(leftBound, rightBound);
                    }
                }
            }
        }
    }
    
} // computeFrameRangeForReader


void
EffectInstance::onFileNameParameterChanged(const KnobIPtr& fileKnob)
{
    if ( isReader() ) {
        computeFrameRangeForReader(fileKnob, true);

        ///Refresh the preview automatically if the filename changed

        ///union the project frame range if not locked with the reader frame range
        bool isLocked = getApp()->getProject()->isFrameRangeLocked();
        if (!isLocked) {

            TimeValue left(INT_MIN), right(INT_MAX);
            {
                GetFrameRangeResultsPtr actionResults;
                ActionRetCodeEnum stat = getFrameRange_public(&actionResults);
                if (!isFailureRetCode(stat)) {
                    RangeD range;
                    actionResults->getFrameRangeResults(&range);
                    left = TimeValue(range.min);
                    right = TimeValue(range.max);
                }
            }

            if ( (left != INT_MIN) && (right != INT_MAX) ) {
                if ( getNode()->getGroup() || getNode()->getIOContainer() ) {
                    getApp()->getProject()->unionFrameRangeWith(left, right);
                }
            }
        }
    } else if ( isWriter() ) {
        KnobFilePtr isFile = toKnobFile(fileKnob);
        KnobStringPtr sublabel = _imp->defKnobs->ofxSubLabelKnob.lock();
        if (isFile && sublabel) {

            std::string pattern = isFile->getValue();
            if (sublabel) {
                std::size_t foundSlash = pattern.find_last_of("/");
                if (foundSlash != std::string::npos) {
                    pattern = pattern.substr(foundSlash + 1);
                }

                sublabel->setValue(pattern);
            }
        }

        /*
         Check if the filename param has a %V in it, in which case make sure to hide the Views parameter
         */
        if (isFile) {
            std::string pattern = isFile->getValue();
            std::size_t foundViewPattern = pattern.find_first_of("%v");
            if (foundViewPattern == std::string::npos) {
                foundViewPattern = pattern.find_first_of("%V");
            }
            if (foundViewPattern != std::string::npos) {
                //We found view pattern
                KnobIPtr viewsKnob = getKnobByName(kWriteOIIOParamViewsSelector);
                if (viewsKnob) {
                    KnobChoicePtr viewsSelector = toKnobChoice(viewsKnob);
                    if (viewsSelector) {
                        viewsSelector->setSecret(true);
                    }
                }
            }
        }
    }
} // onFileNameParameterChanged

/*
 This is called AFTER the instanceChanged action has been called on the plug-in
 */
bool
EffectInstance::handleDefaultKnobChanged(const KnobIPtr& what, ValueChangedReasonEnum reason)
{
    if (!what) {
        return false;
    }
    for (std::map<int, MaskSelector >::iterator it = _imp->defKnobs->maskSelectors.begin(); it != _imp->defKnobs->maskSelectors.end(); ++it) {
        if (it->second.channel.lock() == what) {
            _imp->onMaskSelectorChanged(it->first, it->second);
            break;
        }
    }

    bool ret = true;
    if ( what == _imp->defKnobs->previewEnabledKnob.lock() ) {
        if (reason == eValueChangedReasonUserEdited ) {
            getNode()->s_previewKnobToggled();
        }
    } else if ( what == _imp->defKnobs->renderButton.lock() ) {
        if ( isWriter() ) {
            RenderQueue::RenderWork w;
            w.treeRoot = getNode();
            w.useRenderStats = getApp()->isRenderStatsActionChecked();
            std::list<RenderQueue::RenderWork> works;
            works.push_back(w);
            getApp()->getRenderQueue()->renderNonBlocking(works);
        }
    } else if (what == _imp->renderKnobs.disableNodeKnob.lock()) {
        getNode()->s_disabledKnobToggled( _imp->renderKnobs.disableNodeKnob.lock()->getValue() );
    } else if ( what == _imp->defKnobs->nodeLabelKnob.lock() || what == _imp->defKnobs->ofxSubLabelKnob.lock() ) {
        getNode()->s_nodeExtraLabelChanged();
    } else if ( what == _imp->defKnobs->hideInputs.lock() ) {
        getNode()->s_hideInputsKnobChanged( _imp->defKnobs->hideInputs.lock()->getValue() );
    } else if ( isReader() && (what->getName() == kReadOIIOAvailableViewsKnobName) ) {
        getNode()->refreshCreatedViews(what, false /*silent*/);
    } else if ( what == _imp->defKnobs->refreshInfoButton.lock() ||
               (what == _imp->defKnobs->infoPage.lock() && reason == eValueChangedReasonUserEdited) ) {
        refreshInfos();
    } else if (what == _imp->defKnobs->processAllLayersKnob.lock()) {

        std::map<int, ChannelSelector>::iterator foundOutput = _imp->defKnobs->channelsSelectors.find(-1);
        if (foundOutput != _imp->defKnobs->channelsSelectors.end()) {
            _imp->onLayerChanged(true);
        }
    } else if (what == _imp->defKnobs->pyPlugConvertToGroupButtonKnob.lock() && reason != eValueChangedReasonRestoreDefault) {
        NodeGroup* isGroup = dynamic_cast<NodeGroup*>(this);
        if (isGroup) {
            isGroup->setSubGraphEditedByUser(true);
        }
    } else if (what == _imp->defKnobs->pyPlugExportButtonKnob.lock() && reason == eValueChangedReasonUserEdited) {
        try {
            getNode()->exportNodeToPyPlug(_imp->defKnobs->pyPlugExportDialogFile.lock()->getValue());
        } catch (const std::exception& e) {
            Dialogs::errorDialog(tr("Export").toStdString(), e.what());
        }
    } else if (what == _imp->defKnobs->keepInAnimationModuleKnob.lock()) {
        getNode()->s_keepInAnimationModuleKnobChanged();
    } else if (what == _imp->defKnobs->openglRenderingEnabledKnob.lock()) {

        PluginOpenGLRenderSupport effectGLSupport;
        PluginOpenGLRenderSupport pluginGLSupport = getNode()->getPlugin()->getEffectDescriptor()->getPropertyUnsafe<PluginOpenGLRenderSupport>(kEffectPropSupportsOpenGLRendering);
        if (pluginGLSupport != ePluginOpenGLRenderSupportYes) {
            effectGLSupport = pluginGLSupport;
        } else {
            int knobValue = _imp->defKnobs->openglRenderingEnabledKnob.lock()->getValue();
            if (knobValue == 0 || (knobValue == 2 && !appPTR->isBackground())) {
                effectGLSupport = ePluginOpenGLRenderSupportYes;
            } else {
                effectGLSupport = ePluginOpenGLRenderSupportNone;
            }

            if (effectGLSupport == ePluginOpenGLRenderSupportYes) {
                if ((!getApp()->getProject()->isOpenGLRenderActivated() || !getNode()->getPlugin()->isOpenGLEnabled())) {
                    effectGLSupport = ePluginOpenGLRenderSupportNone;
                }
            }
        }
        setOpenGLRenderSupport(effectGLSupport);
    } else {
        ret = false;
    }

    if (!ret) {
        KnobGroupPtr isGroup = toKnobGroup(what);
        if ( isGroup && isGroup->getIsDialog() && reason == eValueChangedReasonUserEdited) {
            NodeGuiIPtr gui = getNode()->getNodeGui();
            if (gui) {
                gui->showGroupKnobAsDialog(isGroup);
                ret = true;
            }
        }
    }

    if (!ret) {
        for (std::map<int, ChannelSelector>::iterator it = _imp->defKnobs->channelsSelectors.begin(); it != _imp->defKnobs->channelsSelectors.end(); ++it) {
            if (it->second.layer.lock() == what) {
                _imp->onLayerChanged(it->first == -1);
                ret = true;
                break;
            }
        }
    }

    if (!ret) {
        for (int i = 0; i < 4; ++i) {
            KnobBoolPtr enabled = _imp->defKnobs->enabledChan[i].lock();
            if (!enabled) {
                break;
            }
            if (enabled == what) {
                checkForPremultWarningAndCheckboxes();
                ret = true;
                getNode()->s_enabledChannelCheckboxChanged();
                break;
            }
        }
    }

    if (!ret) {
        GroupInput* isInput = dynamic_cast<GroupInput*>(this);
        if (isInput) {
            if ( (what->getName() == kNatronGroupInputIsOptionalParamName)
                || ( what->getName() == kNatronGroupInputIsMaskParamName) ) {
                NodeCollectionPtr col = isInput->getNode()->getGroup();
                assert(col);
                NodeGroupPtr isGrp = toNodeGroup(col);
                assert(isGrp);
                if (isGrp) {
                    ///Refresh input arrows of the node to reflect the state
                    isGrp->refreshInputs();
                    ret = true;
                }
            }
        }
    }
    
    return ret;
} // handleDefaultKnobChanged


NATRON_NAMESPACE_EXIT
