#include <QtWidgets/QtWidgets>
#include "Gui/dockableSettings.h"
#include "Gui/node_ui.h"
#include "Gui/knob_callback.h"
#include "Superviser/powiterFn.h"

SettingsPanel::SettingsPanel(Node_ui* NodeUi ,QWidget *parent):QFrame(parent)
{

    this->NodeUi=NodeUi;
    //setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);

    this->setMinimumWidth(MINIMUM_WIDTH);
    setFrameShape(QFrame::StyledPanel);
    minimized=false;
    QGroupBox* box=new QGroupBox();
    box->setObjectName("box");
    box->setStyleSheet(QString("QGroupBox#box{padding :-15 px;background-color:rgb(50,50,50);}"));
    

    QWidget* buttonFont=new QWidget(box);
    QHBoxLayout* buttonFontLayout=new QHBoxLayout(buttonFont);
    QImage imgM(IMAGES_PATH"minimize.png");

    QPixmap pixM=QPixmap::fromImage(imgM);
    pixM.scaled(10,10);
    QImage imgC(IMAGES_PATH"close.png");

    QPixmap pixC=QPixmap::fromImage(imgC);
    pixC.scaled(10,10);
    minimize=new QPushButton(QIcon(pixM),"");
    minimize->setFixedSize(20,20);
    minimize->setCheckable(true);
    QObject::connect(minimize,SIGNAL(toggled(bool)),this,SLOT(minimizeOrMaximize(bool)));


    cross=new QPushButton(QIcon(pixC),"");
    cross->setFixedSize(20,20);
    QObject::connect(cross,SIGNAL(clicked()),this,SLOT(close()));

    QSpacerItem* spacerRight = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    buttonFontLayout->addItem(spacerRight);
    buttonFontLayout->addWidget(minimize);
    buttonFontLayout->addWidget(cross);
    buttonFontLayout->setSpacing(10);
    buttonFont->setLayout(buttonFontLayout);
    
    QHBoxLayout* layoutBox=new QHBoxLayout(box);
    nodeName = new QLineEdit(box);
    nodeName->setStyleSheet("color:rgb(200,200,200);");
    nodeName->setText(NodeUi->getNode()->getName());
   // QLabel* nodeName=new QLabel((NodeUi->getNode()->getName()));
    QObject::connect(nodeName,SIGNAL(textChanged(QString)),NodeUi,SLOT(setName(QString)));

    layoutBox->addWidget(nodeName);
    layoutBox->addWidget(buttonFont);
    layoutBox->setSpacing(0);
    box->setLayout(layoutBox);


    layout=new QVBoxLayout(this);
    layout_labels=new QVBoxLayout();
    layout_settings=new QVBoxLayout();
    tab=new QTabWidget(this);

    settings=new QWidget(tab);
    settings->setLayout(layout_settings);
    labels=new QWidget(tab);
    labels->setLayout(layout_labels);
    tab->addTab(settings,"Settings");
    tab->addTab(labels,"Label");
    this->setObjectName((NodeUi->getNode()->getName()));
    initialize_knobs();
    layout->addWidget(box);
    layout->addWidget(tab);
    layout->setSpacing(1);
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setVisible(true);

}
SettingsPanel::~SettingsPanel(){delete cb;}

void SettingsPanel::initialize_knobs(){

    cb=new Knob_Callback(this,NodeUi->getNode());
    NodeUi->getNode()->initKnobs(cb);
    std::vector<Knob*> knobs=NodeUi->getNode()->getKnobs();
    foreach(Knob* k,knobs){
        layout_settings->addWidget(k);
    }

}
void SettingsPanel::addKnobDynamically(Knob* knob){
	layout_settings->addWidget(knob);
}

void SettingsPanel::close(){

    NodeUi->setSettingsPanelEnabled(false);
    QWidget* pr=NodeUi->getSettingsLayout()->parentWidget();
    pr->setMinimumSize(NodeUi->getSettingsLayout()->sizeHint());
  //  delete this;
    setVisible(false);

}
void SettingsPanel::minimizeOrMaximize(bool toggled){
    minimized=toggled;
    if(minimized){

        QImage imgM(IMAGES_PATH"maximize.png");
        QPixmap pixM=QPixmap::fromImage(imgM);
        pixM.scaled(10,10);
        minimize->setIcon(QIcon(pixM));
        tab->setVisible(false);


        QWidget* pr=NodeUi->getSettingsLayout()->parentWidget();
        pr->setMinimumSize(NodeUi->getSettingsLayout()->sizeHint());


    }else{
        QImage imgM(IMAGES_PATH"minimize.png");
        QPixmap pixM=QPixmap::fromImage(imgM);
        pixM.scaled(10,10);
        minimize->setIcon(QIcon(pixM));
        tab->setVisible(true);

        QWidget* pr=NodeUi->getSettingsLayout()->parentWidget();
        pr->setMinimumSize(NodeUi->getSettingsLayout()->sizeHint());


    }
}
