#This Source Code Form is subject to the terms of the Mozilla Public
#License, v. 2.0. If a copy of the MPL was not distributed with this
#file, You can obtain one at http://mozilla.org/MPL/2.0/.


TARGET = openMVG
TEMPLATE = lib
CONFIG += staticlib
CONFIG -= qt

# Make openMVG use openmp
#DEFINES += OPENMVG_USE_OPENMP

# Do not use any serialization in openmvg (cereal, ply, stlplus ...)
DEFINES += OPENMVG_NO_SERIALIZATION

# Use this to use OsiMskSolverInterface.cpp
#DEFINES += OPENMVG_HAVE_MOSEK

include(../global.pri)
include(../config.pri)


INCLUDEPATH += $$PWD/../libs/ceres/config
INCLUDEPATH += $$PWD/../libs/ceres/include
INCLUDEPATH += $$PWD/../libs/ceres/internal
INCLUDEPATH += $$PWD/../libs/gflags
INCLUDEPATH += $$PWD/../libs/gflags/src
INCLUDEPATH += $$PWD/../libs/gflags/src/gflags
INCLUDEPATH += $$PWD/../libs/glog/src
INCLUDEPATH += $$PWD/../libs/openMVG/openMVG
INCLUDEPATH += $$PWD/../libs/openMVG
INCLUDEPATH += $$PWD/../libs/openMVG/dependencies/osi_clp/Clp/src
INCLUDEPATH += $$PWD/../libs/openMVG/dependencies/osi_clp/Clp/src/OsiClip
INCLUDEPATH += $$PWD/../libs/openMVG/dependencies/osi_clp/CoinUtils/src
INCLUDEPATH += $$PWD/../libs/openMVG/dependencies/osi_clp/Osi/src/Osi
INCLUDEPATH += $$PWD/../libs/Eigen3
INCLUDEPATH += $$PWD/../libs/flann/src/cpp
#INCLUDEPATH += $$PWD/../libs/lemon

win32-msvc* {
	CONFIG(64bit) {
		QMAKE_LFLAGS += /MACHINE:X64
	} else {
		QMAKE_LFLAGS += /MACHINE:X86
	}
}


SOURCES += \
        ../libs/openMVG/openMVG/features/akaze/AKAZE.cpp \
        ../libs/openMVG/openMVG/features/liop/liop_descriptor.cpp \
        ../libs/openMVG/openMVG/linearProgramming/linearProgrammingMOSEK.cpp \
#        ../libs/openMVG/openMVG/linearProgramming/lInfinityCV/resection_kernel.cpp \
        ../libs/openMVG/openMVG/matching/kvld/kvld.cpp \
        ../libs/openMVG/openMVG/matching/kvld/algorithm.cpp \
#        ../libs/openMVG/openMVG/matching/regions_matcher.cpp \
#        ../libs/openMVG/openMVG/matching_image_collection/Cascade_Hashing_Matcher_Regions_AllInMemory.cpp \
#        ../libs/openMVG/openMVG/matching_image_collection/Matcher_Regions_AllInMemory.cpp \
        ../libs/openMVG/openMVG/multiview/conditioning.cpp \
        ../libs/openMVG/openMVG/multiview/essential.cpp \
        ../libs/openMVG/openMVG/multiview/projection.cpp \
#        ../libs/openMVG/openMVG/multiview/rotation_averaging_l1.cpp \
#        ../libs/openMVG/openMVG/multiview/rotation_averaging_l2.cpp \
        ../libs/openMVG/openMVG/multiview/solver_affine.cpp \
        ../libs/openMVG/openMVG/multiview/solver_essential_five_point.cpp \
        ../libs/openMVG/openMVG/multiview/solver_essential_kernel.cpp \
        ../libs/openMVG/openMVG/multiview/solver_fundamental_kernel.cpp \
        ../libs/openMVG/openMVG/multiview/solver_homography_kernel.cpp \
        ../libs/openMVG/openMVG/multiview/solver_resection_kernel.cpp \
        ../libs/openMVG/openMVG/multiview/translation_averaging_solver_l2_chordal.cpp \
        ../libs/openMVG/openMVG/multiview/translation_averaging_solver_softl1.cpp \
        ../libs/openMVG/openMVG/multiview/triangulation_nview.cpp \
        ../libs/openMVG/openMVG/multiview/triangulation.cpp \
        ../libs/openMVG/openMVG/numeric/numeric.cpp \
#        ../libs/openMVG/openMVG/sfm/pipelines/global/GlobalSfM_rotation_averaging.cpp \
#        ../libs/openMVG/openMVG/sfm/pipelines/global/GlobalSfM_translation_averaging.cpp \
#        ../libs/openMVG/openMVG/sfm/pipelines/global/sfm_global_engine_relative_motions.cpp \
#        ../libs/openMVG/openMVG/sfm/pipelines/localization/SfM_Localizer_Single_3DTrackObservation_Database.cpp \
#        ../libs/openMVG/openMVG/sfm/pipelines/localization/SfM_Localizer.cpp \
#        ../libs/openMVG/openMVG/sfm/pipelines/sequential/sequential_SfM.cpp \
#        ../libs/openMVG/openMVG/sfm/pipelines/structure_from_known_poses/structure_estimator.cpp \
#        ../libs/openMVG/openMVG/sfm/pipelines/sfm_robust_model_estimation.cpp \
#        ../libs/openMVG/openMVG/sfm/sfm_data_BA_ceres.cpp \
#        ../libs/openMVG/openMVG/sfm/sfm_data_filters_frustum.cpp \
#        ../libs/openMVG/openMVG/sfm/sfm_data_io.cpp \
#        ../libs/openMVG/openMVG/sfm/sfm_data_triangulation.cpp \
#        ../libs/openMVG/openMVG/sfm/sfm_data_utils.cpp \
        ../libs/openMVG/openMVG/system/timer.cpp


HEADERS += \
        ../libs/openMVG/openMVG/cameras/Camera_Common.hpp \
        ../libs/openMVG/openMVG/cameras/Camera_Intrinsics.hpp \
        ../libs/openMVG/openMVG/cameras/Camera_Pinhole_Brown.hpp \
        ../libs/openMVG/openMVG/cameras/Camera_Pinhole_Radial.hpp \
        ../libs/openMVG/openMVG/cameras/Camera_Pinhole.hpp \
        ../libs/openMVG/openMVG/cameras/Camera_undistort_image.hpp \
        ../libs/openMVG/openMVG/cameras/cameras.hpp \
        ../libs/openMVG/openMVG/cameras/PinholeCamera.hpp \
        ../libs/openMVG/openMVG/cameras/Camera_IO.hpp \
        ../libs/openMVG/openMVG/color_harmonization/global_quantile_gain_offset_alignment.hpp \
        ../libs/openMVG/openMVG/color_harmonization/selection_fullFrame.hpp \
        ../libs/openMVG/openMVG/color_harmonization/selection_interface.hpp \
        ../libs/openMVG/openMVG/color_harmonization/selection_matchedPoints.hpp \
        ../libs/openMVG/openMVG/color_harmonization/selection_VLDSegment.hpp \
        ../libs/openMVG/openMVG/features/akaze/AKAZE.hpp \
        ../libs/openMVG/openMVG/features/akaze/mldb_descriptor.hpp \
        ../libs/openMVG/openMVG/features/akaze/msurf_descriptor.hpp \
        ../libs/openMVG/openMVG/features/descriptor.hpp \
        ../libs/openMVG/openMVG/features/feature.hpp \
        ../libs/openMVG/openMVG/features/features.hpp \
        ../libs/openMVG/openMVG/features/image_describer.hpp \
        ../libs/openMVG/openMVG/features/image_describer_akaze.hpp \
        ../libs/openMVG/openMVG/features/io_regions_type.hpp \
        ../libs/openMVG/openMVG/features/keypointSet.hpp \
        ../libs/openMVG/openMVG/features/regions_factory.hpp \
        ../libs/openMVG/openMVG/features/regions.hpp \
        ../libs/openMVG/openMVG/features/liop/liop_descriptor.hpp \
        ../libs/openMVG/openMVG/geometry/frustum.hpp \
        ../libs/openMVG/openMVG/geometry/half_space_intersection.hpp \
        ../libs/openMVG/openMVG/geometry/pose3.hpp \
        ../libs/openMVG/openMVG/geometry/rigid_transformation3D_srt.hpp \
        ../libs/openMVG/openMVG/geometry/Similarity3.hpp \
#        ../libs/openMVG/openMVG/graph/connectedComponent.hpp \
#        ../libs/openMVG/openMVG/graph/graph_builder.hpp \
#        ../libs/openMVG/openMVG/graph/graph_graphviz_export.hpp \
#        ../libs/openMVG/openMVG/graph/graph.hpp \
#        ../libs/openMVG/openMVG/graph/triplet_finder.hpp \
        ../libs/openMVG/openMVG/image/image_concat.hpp \
        ../libs/openMVG/openMVG/image/image_container.hpp \
        ../libs/openMVG/openMVG/image/image_converter.hpp \
        ../libs/openMVG/openMVG/image/image_convolution_base.hpp \
        ../libs/openMVG/openMVG/image/image_convolution.hpp \
        ../libs/openMVG/openMVG/image/image_diffusion.hpp \
        ../libs/openMVG/openMVG/image/image_drawing.hpp \
        ../libs/openMVG/openMVG/image/image_filtering.hpp \
        ../libs/openMVG/openMVG/image/image_resampling.hpp \
        ../libs/openMVG/openMVG/image/image.hpp \
        ../libs/openMVG/openMVG/image/image_warping.hpp \
        ../libs/openMVG/openMVG/image/pixel_types.hpp \
        ../libs/openMVG/openMVG/image/sample.hpp \
        ../libs/openMVG/openMVG/linearProgramming/bisectionLP.hpp \
        ../libs/openMVG/openMVG/linearProgramming/linearProgramming.hpp \
        ../libs/openMVG/openMVG/linearProgramming/linearProgrammingInterface.hpp \
        ../libs/openMVG/openMVG/linearProgramming/linearProgrammingMOSEK.hpp \
        ../libs/openMVG/openMVG/linearProgramming/linearProgrammingOSI_X.hpp \
        ../libs/openMVG/openMVG/linearProgramming/lInfinityCV/global_translations_fromTij.hpp \
        ../libs/openMVG/openMVG/linearProgramming/lInfinityCV/global_translations_fromTriplets.hpp \
        ../libs/openMVG/openMVG/linearProgramming/lInfinityCV/lInfinityCV.hpp \
        ../libs/openMVG/openMVG/linearProgramming/lInfinityCV/resection_kernel.hpp \
        ../libs/openMVG/openMVG/linearProgramming/lInfinityCV/tijsAndXis_From_xi_Ri_noise.hpp \
        ../libs/openMVG/openMVG/linearProgramming/lInfinityCV/tijsAndXis_From_xi_Ri.hpp \
        ../libs/openMVG/openMVG/linearProgramming/lInfinityCV/triangulation.hpp \
        ../libs/openMVG/openMVG/linearProgramming/lInfinityCV/triplet_tijsAndXis_kernel.hpp \
        ../libs/openMVG/openMVG/matching/cascade_hasher.hpp \
        ../libs/openMVG/openMVG/matching/indMatch_utils.hpp \
        ../libs/openMVG/openMVG/matching/indMatch.hpp \
        ../libs/openMVG/openMVG/matching/indMatchDecoratorXY.hpp \
        ../libs/openMVG/openMVG/matching/kvld/algorithm.h \
        ../libs/openMVG/openMVG/matching/kvld/kvld_draw.h \
        ../libs/openMVG/openMVG/matching/kvld/kvld.h \
        ../libs/openMVG/openMVG/matching/matcher_brute_force.hpp \
        ../libs/openMVG/openMVG/matching/matcher_cascade_hashing.hpp \
#        ../libs/openMVG/openMVG/matching/matcher_kdtree_flann.hpp \
        ../libs/openMVG/openMVG/matching/matcher_type.hpp \
        ../libs/openMVG/openMVG/matching/matching_filters.hpp \
        ../libs/openMVG/openMVG/matching/matching_interface.hpp \
        ../libs/openMVG/openMVG/matching/metric_hamming.hpp \
        ../libs/openMVG/openMVG/matching/metric.hpp \
#        ../libs/openMVG/openMVG/matching/regions_matcher.hpp \
        ../libs/openMVG/openMVG/matching_image_collection/Cascade_Hashing_Matcher_Regions_AllInMemory.hpp \
        ../libs/openMVG/openMVG/matching_image_collection/F_ACRobust.hpp \
        ../libs/openMVG/openMVG/matching_image_collection/Geometric_Filter_utils.hpp \
        ../libs/openMVG/openMVG/matching_image_collection/GeometricFilter.hpp \
        ../libs/openMVG/openMVG/matching_image_collection/H_ACRobust.hpp \
#        ../libs/openMVG/openMVG/matching_image_collection/Matcher_Regions_AllInMemory.hpp \
        ../libs/openMVG/openMVG/matching_image_collection/Matcher.hpp \
        ../libs/openMVG/openMVG/matching_image_collection/Pair_Builder.hpp \
        ../libs/openMVG/openMVG/multiview/conditioning.hpp \
        ../libs/openMVG/openMVG/multiview/essential.hpp \
        ../libs/openMVG/openMVG/multiview/projection.hpp \
        ../libs/openMVG/openMVG/multiview/rotation_averaging_common.hpp \
#        ../libs/openMVG/openMVG/multiview/rotation_averaging_l1.hpp \
#        ../libs/openMVG/openMVG/multiview/rotation_averaging_l2.hpp \
#        ../libs/openMVG/openMVG/multiview/rotation_averaging.hpp \
        ../libs/openMVG/openMVG/multiview/solver_affine.hpp \
        ../libs/openMVG/openMVG/multiview/solver_essential_kernel.hpp \
        ../libs/openMVG/openMVG/multiview/solver_fundamental_kernel.hpp \
        ../libs/openMVG/openMVG/multiview/solver_homography_kernel.hpp \
        ../libs/openMVG/openMVG/multiview/solver_resection_kernel.hpp \
        ../libs/openMVG/openMVG/multiview/solver_resection_p3p.hpp \
        ../libs/openMVG/openMVG/multiview/solver_translation_knownRotation_kernel.hpp \
        ../libs/openMVG/openMVG/multiview/translation_averaging_common.hpp \
        ../libs/openMVG/openMVG/multiview/translation_averaging_solver.hpp \
        ../libs/openMVG/openMVG/multiview/triangulation_nview.hpp \
        ../libs/openMVG/openMVG/multiview/triangulation.hpp \
        ../libs/openMVG/openMVG/multiview/two_view_kernel.hpp \
        ../libs/openMVG/openMVG/numeric/accumulator_trait.hpp \
        ../libs/openMVG/openMVG/numeric/lm.hpp \
        ../libs/openMVG/openMVG/numeric/math_trait.hpp \
        ../libs/openMVG/openMVG/numeric/numeric.h \
        ../libs/openMVG/openMVG/numeric/poly.h \
        ../libs/openMVG/openMVG/robust_estimation/guided_matching.hpp \
        ../libs/openMVG/openMVG/robust_estimation/rand_sampling.hpp \
        ../libs/openMVG/openMVG/robust_estimation/robust_estimator_ACRansac.hpp \
        ../libs/openMVG/openMVG/robust_estimation/robust_estimator_ACRansacKernelAdaptator.hpp \
        ../libs/openMVG/openMVG/robust_estimation/robust_estimator_lineKernel_test.hpp \
        ../libs/openMVG/openMVG/robust_estimation/robust_estimator_LMeds.hpp \
        ../libs/openMVG/openMVG/robust_estimation/robust_estimator_MaxConsensus.hpp \
        ../libs/openMVG/openMVG/robust_estimation/robust_estimator_Ransac.hpp \
        ../libs/openMVG/openMVG/robust_estimation/robust_ransac_tools.hpp \
        ../libs/openMVG/openMVG/robust_estimation/score_evaluator.hpp \
#        ../libs/openMVG/openMVG/sfm/pipelines/global/GlobalSfM_rotation_averaging.hpp \
#        ../libs/openMVG/openMVG/sfm/pipelines/global/GlobalSfM_rotation_averaging/score_evaluator.hpp \
#        ../libs/openMVG/openMVG/sfm/pipelines/global/GlobalSfM_translation_averaging.hpp \
#        ../libs/openMVG/openMVG/sfm/pipelines/global/sfm_global_engine_relative_motions.hpp \
#        ../libs/openMVG/openMVG/sfm/pipelines/global/sfm_global_reindex.hpp \
#        ../libs/openMVG/openMVG/sfm/pipelines/global/triplet_t_ACRansac_kernelAdaptator.hpp \
#        ../libs/openMVG/openMVG/sfm/pipelines/localization/SfM_Localizer_Single_3DTrackObservation_Database.hpp \
#        ../libs/openMVG/openMVG/sfm/pipelines/localization/SfM_Localizer.hpp \
#        ../libs/openMVG/openMVG/sfm/pipelines/sequential/sequential_SfM.hpp \
#        ../libs/openMVG/openMVG/sfm/pipelines/structure_from_known_poses/structure_estimator.hpp \
#        ../libs/openMVG/openMVG/sfm/pipelines/sfm_engine.hpp \
#        ../libs/openMVG/openMVG/sfm/pipelines/sfm_features_provider.hpp \
#        ../libs/openMVG/openMVG/sfm/pipelines/sfm_matches_provider.hpp \
#        ../libs/openMVG/openMVG/sfm/pipelines/sfm_regions_provider.hpp \
#        ../libs/openMVG/openMVG/sfm/pipelines/sfm_robust_model_estimation.hpp \
#        ../libs/openMVG/openMVG/sfm/sfm_data_BA_ceres_camera_functor.hpp \
#        ../libs/openMVG/openMVG/sfm/sfm_data_BA_ceres.hpp \
#        ../libs/openMVG/openMVG/sfm/sfm_data_BA.hpp \
#        ../libs/openMVG/openMVG/sfm/sfm_data_filters_frustum.hpp \
#        ../libs/openMVG/openMVG/sfm/sfm_data_filters.hpp \
#        ../libs/openMVG/openMVG/sfm/sfm_data_io_baf.hpp \
#        ../libs/openMVG/openMVG/sfm/sfm_data_io_cereal.hpp \
#        ../libs/openMVG/openMVG/sfm/sfm_data_io_ply.hpp \
#        ../libs/openMVG/openMVG/sfm/sfm_data_io.hpp \
#        ../libs/openMVG/openMVG/sfm/sfm_data_triangulation.hpp \
#        ../libs/openMVG/openMVG/sfm/sfm_data_utils.hpp \
#        ../libs/openMVG/openMVG/sfm/sfm_data.hpp \
#        ../libs/openMVG/openMVG/sfm/sfm_filters.hpp \
#        ../libs/openMVG/openMVG/sfm/sfm_landmark.hpp \
#        ../libs/openMVG/openMVG/sfm/sfm_view.hpp \
#        ../libs/openMVG/openMVG/sfm/sfm.hpp \
        ../libs/openMVG/openMVG/stl/dynamic_bitset.hpp \
        ../libs/openMVG/openMVG/stl/hash.hpp \
        ../libs/openMVG/openMVG/stl/indexed_sort.hpp \
        ../libs/openMVG/openMVG/stl/split.hpp \
        ../libs/openMVG/openMVG/stl/stl.hpp \
        ../libs/openMVG/openMVG/stl/stlMap.hpp \
        ../libs/openMVG/openMVG/system/timer.hpp \
        ../libs/openMVG/openMVG/tracks/tracks.hpp \
        ../libs/openMVG/openMVG/types.hpp \
        ../libs/openMVG/openMVG/version.hpp


#SOURCES += \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/CbcOrClpParam.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/Clp_C_Interface.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpCholeskyBase.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpCholeskyDense.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpCholeskyTaucs.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpConstraint.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpConstraintLinear.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpConstraintQuadratic.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpDualRowDantzig.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpDualRowPivot.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpDualRowSteepest.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpDummyMatrix.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpDynamicExampleMatrix.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpDynamicMatrix.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpEventHandler.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpFactorization.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpGubDynamicMatrix.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpGubMatrix.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpHelperFunctions.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpInterior.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpLinearObjective.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpLsqr.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpMain.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpMatrixBase.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpMessage.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpModel.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpNetworkBasis.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpNetworkMatrix.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpNode.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpNonLinearCost.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpObjective.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpPackedMatrix.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpPdco.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpPdcoBase.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpPlusMinusOneMatrix.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpPredictorCorrector.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpPresolve.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpPrimalColumnDantzig.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpPrimalColumnPivot.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpPrimalColumnSteepest.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpQuadraticObjective.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpSimplex.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpSimplexDual.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpSimplexNonlinear.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpSimplexOther.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpSimplexPrimal.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpSolve.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/Idiot.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/IdiSolve.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/MyEventHandler.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/MyMessageHandler.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/IdiSolve.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/OsiClp/OsiClpSolverInterface.cpp

#HEADERS += \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/CbcOrClpParam.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/Clp_C_Interface.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpCholeskyBase.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpCholeskyDense.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpCholeskyTaucs.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpConstraint.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpConfig.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpConstraintLinear.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpConstraintQuadratic.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpDualRowDantzig.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpDualRowPivot.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpDualRowSteepest.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpDummyMatrix.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpDynamicExampleMatrix.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpDynamicMatrix.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpEventHandler.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpFactorization.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpGubDynamicMatrix.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpGubMatrix.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpHelperFunctions.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpInterior.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpLinearObjective.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpLsqr.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpMain.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpMatrixBase.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpMessage.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpModel.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpNetworkBasis.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpNetworkMatrix.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpNode.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpNonLinearCost.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpObjective.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpPackedMatrix.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpParameters.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpPdco.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpPdcoBase.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpPlusMinusOneMatrix.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpPredictorCorrector.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpPresolve.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpPrimalColumnDantzig.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpPrimalColumnPivot.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpPrimalColumnSteepest.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpQuadraticObjective.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpSimplex.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpSimplexDual.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpSimplexNonlinear.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpSimplexOther.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpSimplexPrimal.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/ClpSolve.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/Idiot.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/IdiSolve.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/MyEventHandler.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/MyMessageHandler.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/IdiSolve.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/OsiClp/OsiClpSolverInterface.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/config_clp_default.h \
#        ../libs/openMVG/dependencies/osi_clp/Clp/src/config_default.h


#SOURCES += \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinAlloc.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinBuild.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinDenseFactorization.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinDenseVector.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinError.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinFactorization1.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinFactorization2.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinFactorization3.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinFactorization4.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinFileIO.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinFinite.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinIndexedVector.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinLpIO.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinMessage.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinMessageHandler.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinModel.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinModelUseful.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinModelUseful2.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinMpsIO.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinOslFactorization.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinOslFactorization2.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinOslFactorization3.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPackedMatrix.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPackedVector.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPackedVectorBase.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinParam.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinParamUtils.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPostsolveMatrix.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPrePostsolveMatrix.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPresolveDoubleton.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPresolveDual.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPresolveDupcol.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPresolveEmpty.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPresolveFixed.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPresolveForcing.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPresolveHelperFunctions.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPresolveImpliedFree.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPresolveIsolated.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPresolveMatrix.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPresolveMonitor.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPresolvePsdebug.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPresolveSingleton.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPresolveSubst.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPresolveTighten.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPresolveTripleton.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPresolveUseless.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPresolveZeros.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinSearchTree.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinShallowPackedVector.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinSimpFactorization.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinSnapshot.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinStructuredModel.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinWarmStartBasis.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinWarmStartDual.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinWarmStartPrimalDual.cpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinWarmStartVector.cpp



#HEADERS += \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/Coin_C_defines.h \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinAlloc.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinBuild.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinDistance.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinDenseFactorization.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinDenseVector.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinError.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinFactorization.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinFileIO.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinFinite.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinFloatEqual.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinHelperFunctions.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinIndexedVector.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinLpIO.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinMessage.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinMessageHandler.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinModel.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinModelUseful.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinMpsIO.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinOslC.h \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinOslFactorization.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPackedMatrix.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPackedVector.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPackedVectorBase.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinParam.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPragma.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPostsolveMatrix.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPrePostsolveMatrix.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPresolveDoubleton.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPresolveDual.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPresolveDupcol.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPresolveEmpty.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPresolveFixed.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPresolveForcing.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPresolveImpliedFree.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPresolveIsolated.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPresolveMatrix.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPresolveMonitor.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPresolvePsdebug.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPresolveSingleton.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPresolveSubst.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPresolveTighten.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPresolveTripleton.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPresolveUseless.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinPresolveZeros.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinSearchTree.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinShallowPackedVector.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinSignal.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinSimpFactorization.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinSmartPtr.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinSnapshot.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinSort.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinStructuredModel.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinTime.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinTypes.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinUtility.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinUtilsConfig.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinWarmStart.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinWarmStartBasis.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinWarmStartDual.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinWarmStartPrimalDual.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/CoinWarmStartVector.hpp \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/config_coinutils_default.h \
#        ../libs/openMVG/dependencies/osi_clp/CoinUtils/src/config_default.h



#SOURCES += \
#        ../libs/openMVG/dependencies/osi_clp/Osi/src/Osi/OsiAuxInfo.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Osi/src/Osi/OsiBranchingObject.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Osi/src/Osi/OsiChooseVariable.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Osi/src/Osi/OsiColCut.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Osi/src/Osi/OsiCut.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Osi/src/Osi/OsiCuts.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Osi/src/Osi/OsiNames.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Osi/src/Osi/OsiPresolve.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Osi/src/Osi/OsiRowCut.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Osi/src/Osi/OsiRowCutDebugger.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Osi/src/Osi/OsiSolverBranch.cpp \
#        ../libs/openMVG/dependencies/osi_clp/Osi/src/Osi/OsiSolverInterface.cpp \
##        ../libs/openMVG/dependencies/osi_clp/Osi/src/OsiMsk/OsiMskSolverInterface.cpp


#HEADERS += \
#        ../libs/openMVG/dependencies/osi_clp/Osi/src/Osi/config_default.h \
#        ../libs/openMVG/dependencies/osi_clp/Osi/src/Osi/config_osi_default.h \
#        ../libs/openMVG/dependencies/osi_clp/Osi/src/Osi/OsiAuxInfo.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Osi/src/Osi/OsiBranchingObject.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Osi/src/Osi/OsiChooseVariable.hpp\
#        ../libs/openMVG/dependencies/osi_clp/Osi/src/Osi/OsiCollections.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Osi/src/Osi/OsiColCut.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Osi/src/Osi/OsiCut.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Osi/src/Osi/OsiCuts.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Osi/src/Osi/OsiPresolve.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Osi/src/Osi/OsiRowCut.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Osi/src/Osi/OsiRowCutDebugger.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Osi/src/Osi/OsiSolverBranch.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Osi/src/Osi/OsiSolverInterface.hpp \
#        ../libs/openMVG/dependencies/osi_clp/Osi/src/Osi/OsiSolverParameters.hpp \
##        ../libs/openMVG/dependencies/osi_clp/Osi/src/OsiMsk/OsiMskSolverInterface.hpp
