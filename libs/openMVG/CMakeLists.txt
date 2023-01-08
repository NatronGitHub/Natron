# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <https://natrongithub.github.io/>,
# (C) 2018-2023 The Natron developers
# (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
#
# Natron is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Natron is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
# ***** END LICENSE BLOCK *****

set(openMVG_SOURCES
    openMVG/features/akaze/AKAZE.cpp
    openMVG/features/liop/liop_descriptor.cpp
    openMVG/linearProgramming/linearProgrammingMOSEK.cpp
    openMVG/matching/kvld/kvld.cpp
    openMVG/matching/kvld/algorithm.cpp
    openMVG/multiview/conditioning.cpp
    openMVG/multiview/essential.cpp
    openMVG/multiview/projection.cpp
    openMVG/multiview/solver_affine.cpp
    openMVG/multiview/solver_essential_five_point.cpp
    openMVG/multiview/solver_essential_kernel.cpp
    openMVG/multiview/solver_fundamental_kernel.cpp
    openMVG/multiview/solver_homography_kernel.cpp
    openMVG/multiview/solver_resection_kernel.cpp
    openMVG/multiview/translation_averaging_solver_l2_chordal.cpp
    openMVG/multiview/translation_averaging_solver_softl1.cpp
    openMVG/multiview/triangulation_nview.cpp
    openMVG/multiview/triangulation.cpp
    openMVG/numeric/numeric.cpp
    openMVG/system/timer.cpp
)
set(openMVG_HEADERS
    openMVG/cameras/Camera_Common.hpp
    openMVG/cameras/Camera_Intrinsics.hpp
    openMVG/cameras/Camera_Pinhole_Brown.hpp
    openMVG/cameras/Camera_Pinhole_Radial.hpp
    openMVG/cameras/Camera_Pinhole.hpp
    openMVG/cameras/Camera_undistort_image.hpp
    openMVG/cameras/cameras.hpp
    openMVG/cameras/PinholeCamera.hpp
    openMVG/cameras/Camera_IO.hpp
    openMVG/color_harmonization/global_quantile_gain_offset_alignment.hpp
    openMVG/color_harmonization/selection_fullFrame.hpp
    openMVG/color_harmonization/selection_interface.hpp
    openMVG/color_harmonization/selection_matchedPoints.hpp
    openMVG/color_harmonization/selection_VLDSegment.hpp
    openMVG/features/akaze/AKAZE.hpp
    openMVG/features/akaze/mldb_descriptor.hpp
    openMVG/features/akaze/msurf_descriptor.hpp
    openMVG/features/descriptor.hpp
    openMVG/features/feature.hpp
    openMVG/features/features.hpp
    openMVG/features/image_describer.hpp
    openMVG/features/image_describer_akaze.hpp
    openMVG/features/io_regions_type.hpp
    openMVG/features/keypointSet.hpp
    openMVG/features/regions_factory.hpp
    openMVG/features/regions.hpp
    openMVG/features/liop/liop_descriptor.hpp
    openMVG/geometry/frustum.hpp
    openMVG/geometry/half_space_intersection.hpp
    openMVG/geometry/pose3.hpp
    openMVG/geometry/rigid_transformation3D_srt.hpp
    openMVG/geometry/Similarity3.hpp
    openMVG/image/image_concat.hpp
    openMVG/image/image_container.hpp
    openMVG/image/image_converter.hpp
    openMVG/image/image_convolution_base.hpp
    openMVG/image/image_convolution.hpp
    openMVG/image/image_diffusion.hpp
    openMVG/image/image_drawing.hpp
    openMVG/image/image_filtering.hpp
    openMVG/image/image_resampling.hpp
    openMVG/image/image.hpp
    openMVG/image/image_warping.hpp
    openMVG/image/pixel_types.hpp
    openMVG/image/sample.hpp
    openMVG/linearProgramming/bisectionLP.hpp
    openMVG/linearProgramming/linearProgramming.hpp
    openMVG/linearProgramming/linearProgrammingInterface.hpp
    openMVG/linearProgramming/linearProgrammingMOSEK.hpp
    openMVG/linearProgramming/linearProgrammingOSI_X.hpp
    openMVG/linearProgramming/lInfinityCV/global_translations_fromTij.hpp
    openMVG/linearProgramming/lInfinityCV/global_translations_fromTriplets.hpp
    openMVG/linearProgramming/lInfinityCV/lInfinityCV.hpp
    openMVG/linearProgramming/lInfinityCV/resection_kernel.hpp
    openMVG/linearProgramming/lInfinityCV/tijsAndXis_From_xi_Ri_noise.hpp
    openMVG/linearProgramming/lInfinityCV/tijsAndXis_From_xi_Ri.hpp
    openMVG/linearProgramming/lInfinityCV/triangulation.hpp
    openMVG/linearProgramming/lInfinityCV/triplet_tijsAndXis_kernel.hpp
    openMVG/matching/cascade_hasher.hpp
    openMVG/matching/indMatch_utils.hpp
    openMVG/matching/indMatch.hpp
    openMVG/matching/indMatchDecoratorXY.hpp
    openMVG/matching/kvld/algorithm.h
    openMVG/matching/kvld/kvld_draw.h
    openMVG/matching/kvld/kvld.h
    openMVG/matching/matcher_brute_force.hpp
    openMVG/matching/matcher_cascade_hashing.hpp
    openMVG/matching/matcher_type.hpp
    openMVG/matching/matching_filters.hpp
    openMVG/matching/matching_interface.hpp
    openMVG/matching/metric_hamming.hpp
    openMVG/matching/metric.hpp
    openMVG/matching_image_collection/Cascade_Hashing_Matcher_Regions_AllInMemory.hpp
    openMVG/matching_image_collection/F_ACRobust.hpp
    openMVG/matching_image_collection/Geometric_Filter_utils.hpp
    openMVG/matching_image_collection/GeometricFilter.hpp
    openMVG/matching_image_collection/H_ACRobust.hpp
    openMVG/matching_image_collection/Matcher.hpp
    openMVG/matching_image_collection/Pair_Builder.hpp
    openMVG/multiview/conditioning.hpp
    openMVG/multiview/essential.hpp
    openMVG/multiview/projection.hpp
    openMVG/multiview/rotation_averaging_common.hpp
    openMVG/multiview/solver_affine.hpp
    openMVG/multiview/solver_essential_kernel.hpp
    openMVG/multiview/solver_fundamental_kernel.hpp
    openMVG/multiview/solver_homography_kernel.hpp
    openMVG/multiview/solver_resection_kernel.hpp
    openMVG/multiview/solver_resection_p3p.hpp
    openMVG/multiview/solver_translation_knownRotation_kernel.hpp
    openMVG/multiview/translation_averaging_common.hpp
    openMVG/multiview/translation_averaging_solver.hpp
    openMVG/multiview/triangulation_nview.hpp
    openMVG/multiview/triangulation.hpp
    openMVG/multiview/two_view_kernel.hpp
    openMVG/numeric/accumulator_trait.hpp
    openMVG/numeric/lm.hpp
    openMVG/numeric/math_trait.hpp
    openMVG/numeric/numeric.h
    openMVG/numeric/poly.h
    openMVG/robust_estimation/guided_matching.hpp
    openMVG/robust_estimation/rand_sampling.hpp
    openMVG/robust_estimation/robust_estimator_ACRansac.hpp
    openMVG/robust_estimation/robust_estimator_ACRansacKernelAdaptator.hpp
    openMVG/robust_estimation/robust_estimator_lineKernel_test.hpp
    openMVG/robust_estimation/robust_estimator_LMeds.hpp
    openMVG/robust_estimation/robust_estimator_MaxConsensus.hpp
    openMVG/robust_estimation/robust_estimator_Ransac.hpp
    openMVG/robust_estimation/robust_ransac_tools.hpp
    openMVG/robust_estimation/score_evaluator.hpp
    openMVG/stl/dynamic_bitset.hpp
    openMVG/stl/hash.hpp
    openMVG/stl/indexed_sort.hpp
    openMVG/stl/split.hpp
    openMVG/stl/stl.hpp
    openMVG/stl/stlMap.hpp
    openMVG/system/timer.hpp
    openMVG/tracks/tracks.hpp
    openMVG/types.hpp
    openMVG/version.hpp
    openMVG/robust_estimation/robust_estimator_Prosac.hpp
    openMVG/robust_estimation/robust_estimator_ProsacKernelAdaptator.hpp
)
add_library(openMVG STATIC ${openMVG_HEADERS} ${openMVG_SOURCES})
target_include_directories(openMVG PUBLIC openMVG .)
target_compile_definitions(openMVG
    PUBLIC
        OPENMVG_HAVE_BOOST
        OPENMVG_NO_SERIALIZATION
        OPENMVG_STD_UNORDERED_MAP
        OPENMVG_STD_SHARED_PTR
)
target_link_libraries(openMVG PUBLIC Ceres::ceres)
set_target_properties(openMVG PROPERTIES POSITION_INDEPENDENT_CODE ON)
