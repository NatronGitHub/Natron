#ifndef SETTINGSPANEL_H
#define SETTINGSPANEL_H


#include <QtWidgets/QFrame>
#include "Core/node.h"
#include "Gui/knob.h"

class Node_ui;
class Knob_Callback;
class QVBoxLayout;
class QTabWidget;
class QPushButton;
#define MINIMUM_WIDTH 340

class SettingsPanel:public QFrame
{
    Q_OBJECT

public:

    SettingsPanel(Node_ui* NodeUi, QWidget *parent=0);
    virtual ~SettingsPanel();
	void addKnobDynamically(Knob* knob);
public slots:
    void close();
    void minimizeOrMaximize(bool toggled);

private:
    Node_ui* NodeUi;
    QLineEdit* nodeName;
    QPushButton* minimize;
    QPushButton* cross;
    QTabWidget* tab;
    QWidget* settings;
    QVBoxLayout* layout_settings;
    QVBoxLayout* layout_labels;
    QWidget* labels;
    QVBoxLayout* layout;
    bool minimized;
    Knob_Callback *cb;

    void initialize_knobs();
	
};

#endif // SETTINGSPANEL_H
