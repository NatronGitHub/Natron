//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
*contact: immarespond at gmail dot com
*
*/

#ifndef NODEINSTANCE_H
#define NODEINSTANCE_H

#include <map>
#include <vector>

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QPointF>

class Node;
class NodeGui;
class ChannelSet;
class AppInstance;
class QUndoCommand;
class KnobInstance;
/*This class manages the interaction between
a NodeGui object and a Node object.
*/
class NodeInstance : public QObject
{
    
    Q_OBJECT
    
    AppInstance *_app; //ptr to the application instance owning this node
    Node* _node;
    NodeGui* _gui;
    
    std::map<int,NodeInstance*> _outputs;
    std::map<int,NodeInstance*> _inputs;
    
    std::vector<KnobInstance*> _knobs;
    
public:
    
    typedef std::map<int,NodeInstance*> InputMap;
    typedef std::map<int,NodeInstance*> OutputMap;
    
    NodeInstance(Node* node,AppInstance* app);
    
    virtual ~NodeInstance();
    
    void setNodeGuiPTR(NodeGui* gui){_gui = gui;}
    
    Node* getNode() const {return _node;}
    
    NodeGui* getNodeGui() const {return _gui;}
    
    bool isOpenFXNode() const;
    
    bool isInputNode() const;
    
    bool isOutputNode() const;
    
    const std::string className() const;
    
    const std::string& getName() const;
    
    const InputMap& getInputs() const {return _inputs;}
    
    const OutputMap& getOutputs() const {return _outputs;}
    
    const std::vector<KnobInstance*>& getKnobs() const {return _knobs;}
    
    NodeInstance* input(int inputNb) const;
    
    /** @brief Adds the node parent to the input inputNumber of the
     * node. Returns true if it succeeded, false otherwise.
     * When returning false, this means an input was already
     * connected for this inputNumber. It should be removed
     * beforehand.
     */
    bool connectInput(NodeInstance* parent,int inputNumber);
    
    /** @brief Adds the node child to the output outputNumber of the
     * node.
     */
    void connectOutput(NodeInstance* child,int outputNumber = 0);
    
    /** @brief Removes the node connected to the input inputNumber of the
     * node. Returns true if it succeeded, false otherwise.
     * When returning false, this means no node was connected for this input.
     */
    bool disconnectInput(int inputNumber);
    
    /** @brief Removes the node input of the
     * node inputs. Returns true if it succeeded, false otherwise.
     * When returning false, this means the node input was not
     * connected.
     */
    bool disconnectInput(NodeInstance* input);
    
    /** @brief Removes the node connected to the output outputNumber of the
     * node. Returns true if it succeeded, false otherwise.
     * When returning false, this means no node was connected for this output.
     */
    bool disconnectOutput(int outputNumber);
    
    /** @brief Removes the node output of the
     * node outputs. Returns true if it succeeded, false otherwise.
     * When returning false, this means the node output was not
     * connected.
     */
    bool disconnectOutput(NodeInstance* output);
    
    
    void setPosGui(double x,double y);
    
    QPointF getPosGui() const;
    
    void updateNodeChannelsGui(const ChannelSet& channels);
    
    void refreshEdgesGui();
    
    /*Make this node inactive. It will appear
     as if it was removed from the graph editor
     but the object still lives to allow
     undo/redo operations.*/
    void deactivate();
    
    /*Make this node active. It will appear
     again on the node graph.
     WARNING: this function can only be called
     after a call to deactivate() has been made.
     Calling activate() on a node whose already
     been activated will not do anything.
     */
    void activate();
    
    void checkIfViewerConnectedAndRefresh() const;
    
    void initializeInputs();
    
    /*Called by AppInstance::createNode.
     You should never call this yourself.
     */
    void initializeKnobs();
    
    /*You can call this if you created a knob
     outside of Node::initKnobs and
     OfxParamInstance's derived classes constructor*/
    void createKnobGuiDynamically();

    /*Called by KnobFactory::createKnob. You
     should never call this yourself.*/
    void addKnobInstance(KnobInstance* knob){
        _knobs.push_back(knob);
    }
    
    /*Called by the desctructor of KnobInstance.
     You should never call this yourself.*/
    void removeKnobInstance(KnobInstance* knob);
    
    void pushUndoCommand(QUndoCommand* cmd);
    
    bool isInputConnected(int inputNb) const;
    
    bool hasOutputConnected() const;
    
public slots:
    
    void setName(const QString& name);
    
private:
    

};

#endif // NODEINSTANCE_H
