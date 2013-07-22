//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#include "Gui/comboBox.h"

#include <algorithm>
#include <QLayout>
#include <QLabel>
#include <QMenu>
#include <QStyle>

#include "Superviser/powiterFn.h"

using namespace std;
ComboBox::ComboBox(QWidget* parent):QFrame(parent),_maximumTextSize(0),pressed(false){
    
    _mainLayout = new QHBoxLayout(this);
    _mainLayout->setSpacing(0);
    _mainLayout->setContentsMargins(0, 0, 0, 0);
    setLayout(_mainLayout);
    setFrameShape(QFrame::Box);
    
    _currentText = new QLabel(this);
    setCurrentIndex(-1);
    _currentText->setMinimumWidth(10);
    _mainLayout->addWidget(_currentText);
        
    _dropDownIcon = new QLabel(this);
    QImage imgC(IMAGES_PATH"combobox.png");
    QPixmap pixC=QPixmap::fromImage(imgC);
    pixC = pixC.scaled(10,10);
    _dropDownIcon->setPixmap(pixC);
    _mainLayout->addWidget(_dropDownIcon);
    
    _menu = new QMenu(this);
}



void ComboBox::paintEvent(QPaintEvent *e)
{
//    QStyleOption opt;
//    opt.init(this);
//    QPainter p(this);
//    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    QFrame::paintEvent(e);
}

void ComboBox::mousePressEvent(QMouseEvent* e){
    QImage imgC(IMAGES_PATH"pressed_combobox.png");
    QPixmap pixC=QPixmap::fromImage(imgC);
    pixC = pixC.scaled(10,10);
    _dropDownIcon->setPixmap(pixC);
    setPressed(true);
    style()->unpolish(this);
    style()->polish(this);
    createMenu();
    QFrame::mousePressEvent(e);
}

void ComboBox::mouseReleaseEvent(QMouseEvent* e){
    QImage imgC(IMAGES_PATH"combobox.png");
    QPixmap pixC=QPixmap::fromImage(imgC);
    pixC = pixC.scaled(10,10);
    _dropDownIcon->setPixmap(pixC);
    setPressed(false);
    style()->unpolish(this);
    style()->polish(this);
    QFrame::mouseReleaseEvent(e);
}

void ComboBox::createMenu(){
    _menu->clear();
    for (U32 i = 0 ; i < _actions.size(); i++) {
        for (U32 j = 0; j < _separators.size(); j++) {
            if (_separators[j] == (int)i) {
                _menu->addSeparator();
                break;
            }
        }
        _menu->addAction(_actions[i]);
    }
    QAction* triggered = _menu->exec(this->mapToGlobal(QPoint(0,height())));
    for (U32 i = 0; i < _actions.size(); i++) {
        if (triggered == _actions[i]) {
            setCurrentIndex(i);
            break;
        }
    }
    
    QImage imgC(IMAGES_PATH"combobox.png");
    QPixmap pixC=QPixmap::fromImage(imgC);
    pixC = pixC.scaled(10,10);
    _dropDownIcon->setPixmap(pixC);
    setPressed(false);
    style()->unpolish(this);
    style()->polish(this);
    
}

void ComboBox::insertItem(int index,const QString& item,QIcon icon,QKeySequence key){
    QAction* action =  new QAction(this);
    action->setText(item);
    if (!icon.isNull()) {
        action->setIcon(icon);
    }
    if (!key.isEmpty()) {
        action->setShortcut(key);
    }
    if (item.size() > _maximumTextSize) {
        _maximumTextSize = item.size();
    }
    _actions.insert(_actions.begin()+index, action);
    /*if this is the first action we add, make it current*/
    if(_actions.size() == 1){
        setCurrentIndex(0);
    }
 
}

void ComboBox::addItem(const QString& item,QIcon icon ,QKeySequence key){
    QAction* action =  new QAction(this);
    action->setText(item);
    if (!icon.isNull()) {
        action->setIcon(icon);
    }
    if (!key.isEmpty()) {
        action->setShortcut(key);
    }
    
    if (item.size() > _maximumTextSize) {
        _maximumTextSize = item.size();
    }
    
    _actions.push_back(action);
    
    /*if this is the first action we add, make it current*/
    if(_actions.size() == 1){
        setCurrentIndex(0);
    }
}

void ComboBox::setCurrentText(const QString& text){
    QString str(text);
    str.prepend("  ");
    str.append("  ");
    _currentText->setText(text);
}

void ComboBox::setCurrentIndex(int index){
    QString str;
    QString rawStr;
    if(index >= 0){
        str = _actions[index]->text();
        rawStr = str;
        /*before displaying,prepend and append the text by some spacing.
         This is a dirty way to do this but QLayout::addSpacing() doesn't preserve
         the same style for the label.*/
        int dsize = _maximumTextSize - str.size();
        dsize/=2;
        str.prepend("  ");
        for (int i = 0; i < dsize ; i++) {str.prepend(" ");}
        str.append("  ");
        for (int i = 0; i < dsize ; i++) {str.append(" ");}
    }else{
        str = "    ";
    }
    _currentText->setText(str);
    emit currentIndexChanged(index);
    emit currentIndexChanged(rawStr);
}
void ComboBox::addSeparator(){
    _separators.push_back(_actions.size()-1);
}
void ComboBox::insertSeparator(int index){
    _separators.push_back(index);
}

QString ComboBox::itemText(int index) const{
    return _actions[index]->text();
}

void ComboBox::removeItem(const QString& item){
    for (U32 i = 0; i < _actions.size(); i++) {
        if (_actions[i]->text() == item) {
            _actions.erase(_actions.begin()+i);
            if (_currentText->text() == item) {
                setCurrentIndex(i-1);
            }
            /*adjust separators that were placed after this item*/
            for (U32 j = 0; j < _separators.size(); j++) {
                if (_separators[j] >= (int)i) {
                    _separators[j]--;
                }
            }
        }
    }
    /*we also need to re-calculate the maximum text size*/
    _maximumTextSize = 0;
    for (U32 i = 0; i < _actions.size(); i++) {
        if (_actions[i]->text().size() > _maximumTextSize) {
            _maximumTextSize = _actions[i]->text().size();
        }
    }
}

void ComboBox::setItemText(int index,const QString& item){
    _actions[index]->setText(item);
    /*we also need to re-calculate the maximum text size*/
    _maximumTextSize = 0;
    for (U32 i = 0; i < _actions.size(); i++) {
        if (_actions[i]->text().size() > _maximumTextSize) {
            _maximumTextSize = _actions[i]->text().size();
        }
    }
}

void ComboBox::setItemShortcut(int index,const QKeySequence& sequence){
    _actions[index]->setShortcut(sequence);
}

void ComboBox::setItemIcon(int index,const QIcon& icon){
    _actions[index]->setIcon(icon);
}
void ComboBox::disableItem(int index){
    _actions[index]->setEnabled(false);
}

void ComboBox::enableItem(int index){
    _actions[index]->setEnabled(true);
}

