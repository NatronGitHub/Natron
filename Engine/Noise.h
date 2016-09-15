/*
 Copyright Disney Enterprises, Inc.  All rights reserved.

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License
 and the following modification to it: Section 6 Trademarks.
 deleted and replaced with:

 6. Trademarks. This License does not grant permission to use the
 trade names, trademarks, service marks, or product names of the
 Licensor and its affiliates, except as required for reproducing
 the content of the NOTICE file.

 You may obtain a copy of the License at
 http://www.apache.org/licenses/LICENSE-2.0
*/
#ifndef NATRON_ENGINE_NOISE_H
#define NATRON_ENGINE_NOISE_H

#include "Global/Macros.h"

NATRON_NAMESPACE_ENTER

//! One octave of non-periodic Perlin noise
template <int d_in, int d_out, class T>
void Noise(const T* in, T* out);

//! One octave of periodic noise
//! period gives the integer period before tiles repease
template <int d_in, int d_out, class T>
void PNoise(const T* in, const int* period, T* out);

//! Fractional Brownian Motion. If turbulence is true then turbulence computed.
template <int d_in, int d_out, bool turbulence, class T>
void FBM(const T* in, T* out, int octaves, T lacunarity, T gain);

//! Cellular noise with input and output dimensionality
template <int d_in, int d_out, class T>
void CellNoise(const T* in, T* out);

NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_NOISE_H
