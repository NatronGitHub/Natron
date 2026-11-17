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

//#define SEEXPR_USE_SSE

#include "Noise.h"

#include <iostream>
#ifdef SEEXPR_USE_SSE
#include <smmintrin.h>
#endif
#include <cmath>
#include <cstdint>

#include "Global/Macros.h"

namespace {
#include "NoiseTables.h"
}

NATRON_NAMESPACE_ENTER

inline double floorSSE(double val) {
#ifdef SEEXPR_USE_SSE
    return _mm_cvtsd_f64(_mm_floor_sd(_mm_set_sd(0.0), _mm_set_sd(val)));
#else
    return std::floor(val);
#endif
}

inline double roundSSE(double val) {
#ifdef  SEEXPR_USE_SSE
    return _mm_cvtsd_f64(_mm_round_sd(_mm_set_sd(0.0), _mm_set_sd(val), _MM_FROUND_TO_NEAREST_INT));
#else
    return std::round(val);
#endif
}

//! This is the Quintic interpolant from Perlin's Improved Noise Paper
double s_curve(double t) { return t * t * t * (t * (6 * t - 15) + 10); }

//! Does a hash reduce to a character
template <int d>
unsigned char hashReduceChar(int index[d]) {
    uint32_t seed = 0;
    // blend with seed (constants from Numerical Recipes, attrib. from Knuth)
    for (int k = 0; k < d; k++) {
        static const uint32_t M = 1664525, C = 1013904223;
        seed = seed * M + index[k] + C;
    }
    // tempering (from Matsumoto)
    seed ^= (seed >> 11);
    seed ^= (seed << 7) & 0x9d2c5680UL;
    seed ^= (seed << 15) & 0xefc60000UL;
    seed ^= (seed >> 18);
    // compute one byte by mixing third and first bytes
    return (((seed & 0xff0000) >> 4) + (seed & 0xff)) & 0xff;
}

//! Does a hash reduce to an integer
template <int d>
uint32_t hashReduce(uint32_t index[d]) {
    union {
        uint32_t i;
        unsigned char c[4];
    } u1, u2;
    // blend with seed (constants from Numerical Recipes, attrib. from Knuth)
    u1.i = 0;
    for (int k = 0; k < d; k++) {
        static const uint32_t M = 1664525, C = 1013904223;
        u1.i = u1.i * M + index[k] + C;
    }
    // tempering (from Matsumoto)
    u1.i ^= (u1.i >> 11);
    u1.i ^= (u1.i << 7) & 0x9d2c5680U;
    u1.i ^= (u1.i << 15) & 0xefc60000U;
    u1.i ^= (u1.i >> 18);
    // permute bytes (shares perlin noise permutation table)
    u2.c[3] = p[u1.c[0]];
    u2.c[2] = p[u1.c[1] + u2.c[3]];
    u2.c[1] = p[u1.c[2] + u2.c[2]];
    u2.c[0] = p[u1.c[3] + u2.c[1]];

    return u2.i;
}

//! Noise with d_in dimensional domain, 1 dimensional abcissa
template <int d, class T, bool periodic>
T noiseHelper(const T* X, const int* period = 0) {
    // find lattice index
    T weights[2][d];  // lower and upper weights
    int index[d];
    for (int k = 0; k < d; k++) {
        T f = floorSSE(X[k]);
        index[k] = (int)f;
        if (periodic) {
            index[k] %= period[k];
            if (index[k] < 0) index[k] += period[k];
        }
        weights[0][k] = X[k] - f;
        weights[1][k] = weights[0][k] - 1;  // dist to cell with index one above
    }
    // compute function values propagated from zero from each node
    int num = 1 << d;
    T vals[1 << d];
    for (int dummy = 0; dummy < num; dummy++) {
        int latticeIndex[d];
        int offset[d];
        for (int k = 0; k < d; k++) {
            offset[k] = ((dummy & (1 << k)) != 0);
            latticeIndex[k] = index[k] + offset[k];
        }
        // hash to get representative gradient vector
        int lookup = hashReduceChar<d>(latticeIndex);
        T val = 0;
        for (int k = 0; k < d; k++) {
            double grad = NOISE_TABLES<d>::g[lookup][k];
            double weight = weights[offset[k]][k];
            val += grad * weight;
        }
        vals[dummy] = val;
    }
    // compute linear interpolation coefficients
    T alphas[d];
    for (int k = 0; k < d; k++) alphas[k] = s_curve(weights[0][k]);
    // perform multilinear interpolation (i.e. linear, bilinear, trilinear, quadralinear)
    for (int newd = d - 1; newd >= 0; newd--) {
        int newnum = 1 << newd;
        int k = (d - newd - 1);
        T alpha = alphas[k];
        T beta = T(1) - alphas[k];
        for (int dummy = 0; dummy < newnum; dummy++) {
            int index = dummy * (1 << (d - newd));
            int otherIndex = index + (1 << k);
            // T alpha=s_curve(weights[0][k]);
            vals[index] = beta * vals[index] + alpha * vals[otherIndex];
        }
    }
    // return reduced version
    return vals[0];
}




//! Computes cellular noise (non-interpolated piecewise constant cell random values)
template <int d_in, int d_out, class T>
void CellNoise(const T* in, T* out) {
    uint32_t index[d_in];
    int dim = 0;
    for (int k = 0; k < d_in; k++) index[k] = uint32_t(floorSSE(in[k]));
    while (1) {
        out[dim] = hashReduce<d_in>(index) * (1.0 / 0xffffffffu);
        if (++dim >= d_out) break;
        // coverity[dead_error_begin]
        for (int k = 0; k < d_in; k++) index[k] += 1000;
    }
}

//! Noise with d_in dimensional domain, d_out dimensional abcissa
template <int d_in, int d_out, class T>
void Noise(const T* in, T* out) {
    T P[d_in];
    for (int i = 0; i < d_in; i++) P[i] = in[i];

    int i = 0;
    while (1) {
        out[i] = noiseHelper<d_in, T, false>(P);
        if (++i >= d_out) break;
        // coverity[dead_error_begin]
        for (int k = 0; k < d_out; k++) P[k] += (T)1000;
    }
}

//! Periodic Noise with d_in dimensional domain, d_out dimensional abcissa
template <int d_in, int d_out, class T>
void PNoise(const T* in, const int* period, T* out) {
    T P[d_in];
    for (int i = 0; i < d_in; i++) P[i] = in[i];

    int i = 0;
    while (1) {
        out[i] = noiseHelper<d_in, T, true>(P, period);
        if (++i >= d_out) break;
        // coverity[dead_error_begin]
        for (int k = 0; k < d_out; k++) P[k] += (T)1000;
    }
}

//! Noise with d_in dimensional domain, d_out dimensional abcissa
//! If turbulence is true then Perlin's turbulence is computed
template <int d_in, int d_out, bool turbulence, class T>
void FBM(const T* in, T* out, int octaves, T lacunarity, T gain) {
    T P[d_in];
    for (int i = 0; i < d_in; i++) P[i] = in[i];

    T scale = 1;
    for (int k = 0; k < d_out; k++) out[k] = 0;
    int octave = 0;
    while (1) {
        T localResult[d_out];
        Noise<d_in, d_out>(P, localResult);
        if (turbulence)
            for (int k = 0; k < d_out; k++) out[k] += fabs(localResult[k]) * scale;
        else
            for (int k = 0; k < d_out; k++) out[k] += localResult[k] * scale;
        if (++octave >= octaves) break;
        scale *= gain;
        for (int k = 0; k < d_in; k++) {
            P[k] *= lacunarity;
            P[k] += (T)1234;
        }
    }
}

// Explicit instantiations
template void CellNoise<3, 1, double>(const double*, double*);
template void CellNoise<3, 3, double>(const double*, double*);
template void Noise<1, 1, double>(const double*, double*);
template void Noise<2, 1, double>(const double*, double*);
template void Noise<3, 1, double>(const double*, double*);
template void PNoise<3, 1, double>(const double*, const int*, double*);
template void Noise<4, 1, double>(const double*, double*);
template void Noise<3, 3, double>(const double*, double*);
template void Noise<4, 3, double>(const double*, double*);
template void FBM<3, 1, false, double>(const double*, double*, int, double, double);
template void FBM<3, 1, true, double>(const double*, double*, int, double, double);
template void FBM<3, 3, false, double>(const double*, double*, int, double, double);
template void FBM<3, 3, true, double>(const double*, double*, int, double, double);
template void FBM<4, 1, false, double>(const double*, double*, int, double, double);
template void FBM<4, 3, false, double>(const double*, double*, int, double, double);
NATRON_NAMESPACE_EXIT

#ifdef MAINTEST
int main(int argc, char* argv[]) {
    typedef double T;
    T sum = 0;
    for (int i = 0; i < 10000000; i++) {
        T foo[3] = {.3, .3, .3};
        // for(int k=0;k<3;k++) foo[k]=(double)rand()/double(RAND_MAX)*100.;
        sum += SeExpr2::noiseHelper<3, T, false>(foo);
    }
}
#endif
