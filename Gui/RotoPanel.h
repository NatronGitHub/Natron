//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef ROTOPANEL_H
#define ROTOPANEL_H

#include <boost/shared_ptr.hpp>

#include <QWidget>

class NodeGui;
class RotoPanel : public QWidget
{
public:
    RotoPanel(const boost::shared_ptr<NodeGui>& n,QWidget* parent = 0);
};

#endif // ROTOPANEL_H
