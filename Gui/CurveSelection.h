//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */
#ifndef CURVESELECTION_H
#define CURVESELECTION_H

#include <vector>
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif

class CurveGui;
class CurveSelection
{
public:

    CurveSelection() {}

    virtual void getSelectedCurves(std::vector<boost::shared_ptr<CurveGui> >* selection) = 0;
};

#endif // CURVESELECTION_H
