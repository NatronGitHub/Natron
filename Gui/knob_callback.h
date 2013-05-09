#ifndef KNOB_CALLBACK_H
#define KNOB_CALLBACK_H
#include <vector>

class Knob;
class SettingsPanel;
class Node;

class Knob_Callback
{
    SettingsPanel* getSettingsPanel(){return panel;}
public:
    void addKnob(Knob* knob){
       
        knobs.push_back(knob);
    }
    void initNodeKnobsVector();
    Knob_Callback(SettingsPanel* panel,Node* node);
	Node* getNode(){return node;}
	void createKnobDynamically();

private:
    Node* node;
    std::vector<Knob*> knobs;
    SettingsPanel* panel;
};

#endif // KNOB_CALLBACK_H
