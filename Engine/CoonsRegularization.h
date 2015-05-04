//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef COONSREGULARIZATION_H
#define COONSREGULARIZATION_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <list>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif


class BezierCP;

namespace Natron {

    void regularize(const std::list<boost::shared_ptr<BezierCP> >& coonsPatch,
                    int time,
                    std::list<std::list<boost::shared_ptr<BezierCP> > >* fixedPatch);
    
}

#endif // COONSREGULARIZATION_H
