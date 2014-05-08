//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef ROTOPANEL_H
#define ROTOPANEL_H

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

#include <QWidget>

class NodeGui;

struct RotoPanelPrivate;
class RotoPanel : public QWidget
{
public:
    
    RotoPanel(NodeGui* n,QWidget* parent = 0);
    
    virtual ~RotoPanel();
    
private:
    
    boost::scoped_ptr<RotoPanelPrivate> _imp;
};

#endif // ROTOPANEL_H
