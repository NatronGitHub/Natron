//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "RenderStatsDialog.h"

#include "Gui/Gui.h"

struct RenderStatsDialogPrivate
{
    RenderStatsDialogPrivate()
    {
        
    }
};

RenderStatsDialog::RenderStatsDialog(Gui* gui)
: QWidget(gui)
, _imp(new RenderStatsDialogPrivate())
{
    setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
    setWindowTitle(tr("Render statistics"));
}

RenderStatsDialog::~RenderStatsDialog()
{
    
}