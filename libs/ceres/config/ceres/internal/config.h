// Ceres Solver - A fast non-linear least squares minimizer
// Copyright 2023 Google Inc. All rights reserved.
// http://ceres-solver.org/
//
// See ../../../LICENSE for the license (New BSD).
//
// Natron: hand-written equivalent of the config.h that Ceres' CMake build
// generates from cmake/config.h.in, for the configuration in which Natron
// builds the bundled Ceres (libs/ceres/CMakeLists.txt and ceres.pro):
// no LAPACK, no sparse linear algebra (SuiteSparse, Accelerate, Eigen sparse),
// no CUDA, no METIS, and only the dynamic Schur eliminator
// (SCHUR_SPECIALIZATIONS=OFF). That is the configuration of the Ceres 1.11
// that Natron bundled before.

#ifndef CERES_PUBLIC_INTERNAL_CONFIG_H_
#define CERES_PUBLIC_INTERNAL_CONFIG_H_

#define CERES_NO_LAPACK
#define CERES_NO_SUITESPARSE
#define CERES_NO_CUDA
#define CERES_NO_ACCELERATE_SPARSE
#define CERES_NO_SPARSE
#define CERES_RESTRICT_SCHUR_SPECIALIZATION
#define CERES_NO_CHOLMOD_PARTITION
#define CERES_NO_EIGEN_METIS

#endif  // CERES_PUBLIC_INTERNAL_CONFIG_H_
