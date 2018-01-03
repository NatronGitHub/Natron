/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Hash64.h"

#include <algorithm>  // for std::for_each
#include <cassert>
#include <stdexcept>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/crc.hpp>
#endif
#include <QtCore/QString>

#include "Engine/Node.h"
#include "Engine/Curve.h"

NATRON_NAMESPACE_ENTER

void
Hash64::computeHash()
{
    if (hashValid) {
        return;
    }
    if ( node_values.empty() ) {
        return;
    }

    const unsigned char* data = reinterpret_cast<const unsigned char*>( &node_values.front() );
    boost::crc_optimal<64, 0x42F0E1EBA9EA3693ULL, 0, 0, false, false> crc_64;
    crc_64.process_bytes(data, node_values.size() * sizeof(node_values[0]));
    hash = crc_64.checksum();
    hashValid = true;
}

void
Hash64::reset()
{
    node_values.clear();
    hash = 0;
    hashValid = false;
}

void
Hash64::appendQString(const QString & str, Hash64* hash)
{
    std::size_t curSize = hash->node_values.size();
    hash->node_values.resize(curSize + str.size());

    int c = (int)curSize;
    for (QString::const_iterator it = str.begin(); it != str.end(); ++it, ++c) {
        hash->node_values[c] = toU64<unsigned short>(it->unicode());
    }
}

void
Hash64::appendCurve(const CurvePtr& curve, Hash64* hash)
{
    KeyFrameSet keys = curve->getKeyFrames_mt_safe();

    std::size_t curSize = hash->node_values.size();
    hash->node_values.resize(curSize + 4 * keys.size());


    int c = curSize;
    for (KeyFrameSet::const_iterator it = keys.begin(); it!=keys.end(); ++it, c += 4) {
        hash->node_values[c] = toU64((double)it->getTime());
        if (it->hasProperty(kKeyFramePropString)) {
            std::string value;
            it->getPropertySafe(kKeyFramePropString, 0, &value);
            appendQString(QString::fromUtf8(value.c_str()), hash);
        } else {
            hash->node_values[c + 1] = toU64(it->getValue());
            hash->node_values[c + 2] = toU64(it->getLeftDerivative());
            hash->node_values[c + 3] = toU64(it->getRightDerivative());
        }

    }
}


NATRON_NAMESPACE_EXIT
