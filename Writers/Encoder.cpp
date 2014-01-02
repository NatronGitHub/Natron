//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#if 0 // deprecated

#include "Encoder.h"

#include "Engine/Row.h"
#include "Writers/Writer.h"

using namespace Natron;

/*Constructors should initialize variables, but shouldn't do any heavy computations, as these objects
 are oftenly re-created. To initialize the input color-space , you can do so by overloading
 initializeColorSpace. This function is called after the constructor and before any
 reading occurs.*/
Encoder::Encoder(Writer* writer):_premult(false),_lut(0),_writer(writer),_optionalKnobs(0),_filename()
{
}

Encoder::~Encoder()
{

}


void EncoderKnobs::initKnobs(const std::string&)
{
    _writer->createKnobDynamically();
}

#endif